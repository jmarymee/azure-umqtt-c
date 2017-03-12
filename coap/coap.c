#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdlib.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/threadapi.h"

#include <inttypes.h>
#include "coap.h"

#define SERVER "127.0.0.1"  //ip address of udp server
#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to listen for incoming data

Coap::Coap() {
	WSAData wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	this->_wsa = wsa;

	s, slen = sizeof(si_other);

	//create socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR) //Parms for a UDP Datagram
	{
		printf("socket() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	//setup address structure
	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	//si_other.sin_addr.S_un.S_addr = inet_addr(SERVER); //Done in Send call now
}

uint16_t Coap::sendPacket(CoapPacket &packet, IPAddress ip, int port) {

	si_other.sin_addr.S_un.S_addr = inet_addr(ip.ip);

	uint8_t buffer[BUF_MAX_SIZE];
	uint8_t *p = buffer;
	uint16_t running_delta = 0;
	uint16_t packetSize = 0;

	// make coap packet base header
	*p = 0x01 << 6;
	*p |= (packet.type & 0x03) << 4;
	*p++ |= (packet.tokenlen & 0x0F);
	*p++ = packet.code;
	*p++ = (packet.messageid >> 8);
	*p++ = (packet.messageid & 0xFF);
	p = buffer + COAP_HEADER_SIZE;
	packetSize += 4;

	// make token
	if (packet.token != NULL && packet.tokenlen <= 0x0F) {
		memcpy(p, packet.token, packet.tokenlen);
		p += packet.tokenlen;
		packetSize += packet.tokenlen;
	}

	// make option header
	for (int i = 0; i < packet.optionnum; i++) {
		uint32_t optdelta;
		uint8_t len, delta;

		if (packetSize + 5 + packet.options[i].length >= BUF_MAX_SIZE) {
			return 0;
		}
		optdelta = packet.options[i].number - running_delta;
		COAP_OPTION_DELTA(optdelta, &delta);
		COAP_OPTION_DELTA((uint32_t)packet.options[i].length, &len);

		*p++ = (0xFF & (delta << 4 | len));
		if (delta == 13) {
			*p++ = (optdelta - 13);
			packetSize++;
		}
		else if (delta == 14) {
			*p++ = ((optdelta - 269) >> 8);
			*p++ = (0xFF & (optdelta - 269));
			packetSize += 2;
		} if (len == 13) {
			*p++ = (packet.options[i].length - 13);
			packetSize++;
		}
		else if (len == 14) {
			*p++ = (packet.options[i].length >> 8);
			*p++ = (0xFF & (packet.options[i].length - 269));
			packetSize += 2;
		}

		memcpy(p, packet.options[i].buffer, packet.options[i].length);
		p += packet.options[i].length;
		packetSize += packet.options[i].length + 1;
		running_delta = packet.options[i].number;
	}

	// make payload
	if (packet.payloadlen > 0) {
		if ((packetSize + 1 + packet.payloadlen) >= BUF_MAX_SIZE) {
			return 0;
		}
		*p++ = 0xFF;
		memcpy(p, packet.payload, packet.payloadlen);
		packetSize += 1 + packet.payloadlen;
	}

	//TODO: This is the area that actually sends the packet
	//_udp->beginPacket(ip, port);
	//_udp->write(buffer, packetSize);
	//_udp->endPacket();

	//send the message
	if (sendto(s, (char*)buffer, packetSize, 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
	{
		printf("sendto() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	return packet.messageid;
}

uint16_t Coap::send(IPAddress ip, int port, char *url, COAP_TYPE type, COAP_METHOD method, uint8_t *token, uint8_t tokenlen, uint8_t *payload, uint32_t payloadlen) {

	// make packet
	CoapPacket packet;

	packet.type = type;
	packet.code = method;
	packet.token = token;
	packet.tokenlen = tokenlen;
	packet.payload = payload;
	packet.payloadlen = payloadlen;
	packet.optionnum = 0;
	packet.messageid = rand();

	// if more options?
	packet.options[packet.optionnum].buffer = (uint8_t *)url;
	packet.options[packet.optionnum].length = strlen(url);
	packet.options[packet.optionnum].number = COAP_URI_PATH;
	packet.optionnum++;

	// send packet
	return this->sendPacket(packet, ip, port);
}

uint16_t Coap::put(IPAddress ip, int port, char *url, char *payload) {
	return this->send(ip, port, url, COAP_CON, COAP_PUT, NULL, 0, (uint8_t *)payload, strlen(payload));
}




