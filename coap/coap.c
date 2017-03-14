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

#include <iostream>
#include <map>
#include <cstring>

#include <inttypes.h>
#include "coap.h"

#define SERVER "127.0.0.1"  //ip address of udp server
#define BUFLEN 512  //Max length of buffer
#define PORT 5683   //The port on which to listen for incoming data

Coap::Coap() {
	//WSAData wsa;
	//WSAStartup(MAKEWORD(2, 2), &wsa);
	//this->_wsa = wsa;

	_udp = new UDP();

	//s, slen = sizeof(si_other);

	//create socket
	//if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR) //Parms for a UDP Datagram
	//{
	//	printf("socket() failed with error code : %d", WSAGetLastError());
	//	exit(EXIT_FAILURE);
	//}

	//setup address structure
	//memset((char *)&si_other, 0, sizeof(si_other));
	//si_other.sin_family = AF_INET;
	//si_other.sin_port = htons(PORT);

	//si_other.sin_addr.S_un.S_addr = inet_addr(SERVER); //Done in Send call now
}

uint16_t Coap::sendPacket(CoapPacket &packet, IPAddress ip, int port) {

	//si_other.sin_addr.S_un.S_addr = inet_addr(ip.ip);
	_udp->SetIPAddress(ip); //Tells the UDP client where we want to connect

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

	//TODO: This is the area that actually sends the packet. We are using a WinSock-specific send right below  this ins the SendTo function/method
	//_udp->beginPacket(ip, port);
	//_udp->write(buffer, packetSize);
	//_udp->endPacket();

	//send the message
	//if (sendto(s, (char*)buffer, packetSize, 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
	//{
	//	printf("sendto() failed with error code : %d", WSAGetLastError());
	//	exit(EXIT_FAILURE);
	//}

	//Send message via the UDP class now
	if (_udp->sendDatagram((char*)buffer, packetSize, 0, slen) == SOCKET_ERROR)
	{
		printf("sendto() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	return packet.messageid;
}

uint16_t Coap::sendPacket(CoapPacket &packet, IPAddress ip) {
	return this->sendPacket(packet, ip, COAP_DEFAULT_PORT);
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

//uint16_t Coap::loop()
//{
//	return uint16_t();
//}

uint16_t Coap::sendResponse(IPAddress ip, int port, uint16_t messageid, char *payload, int payloadlen,
	COAP_RESPONSE_CODE code, COAP_CONTENT_TYPE type, uint8_t *token, int tokenlen) {
	// make packet
	CoapPacket packet;

	packet.type = COAP_ACK;
	packet.code = code;
	packet.token = token;
	packet.tokenlen = tokenlen;
	packet.payload = (uint8_t *)payload;
	packet.payloadlen = payloadlen;
	packet.optionnum = 0;
	packet.messageid = messageid;

	// if more options?
	char optionBuffer[2];
	optionBuffer[0] = ((uint16_t)type & 0xFF00) >> 8;
	optionBuffer[1] = ((uint16_t)type & 0x00FF);
	packet.options[packet.optionnum].buffer = (uint8_t *)optionBuffer;
	packet.options[packet.optionnum].length = 2;
	packet.options[packet.optionnum].number = COAP_CONTENT_FORMAT;
	packet.optionnum++;

	return this->sendPacket(packet, ip, port);
}

uint16_t Coap::sendResponse(IPAddress ip, int port, uint16_t messageid) {
	return this->sendResponse(ip, port, messageid, NULL, 0, COAP_CONTENT, COAP_TEXT_PLAIN, NULL, 0);
}

uint16_t Coap::sendResponse(IPAddress ip, int port, uint16_t messageid, char *payload) {
	return this->sendResponse(ip, port, messageid, payload, strlen(payload), COAP_CONTENT, COAP_TEXT_PLAIN, NULL, 0);
}

uint16_t Coap::sendResponse(IPAddress ip, int port, uint16_t messageid, char *payload, int payloadlen) {
	return this->sendResponse(ip, port, messageid, payload, payloadlen, COAP_CONTENT, COAP_TEXT_PLAIN, NULL, 0);
}

int Coap::parseOption(CoapOption *option, uint16_t *running_delta, uint8_t **buf, size_t buflen) {
	uint8_t *p = *buf;
	uint8_t headlen = 1;
	uint8_t len, delta;

	if (buflen < headlen) return -1;

	delta = (p[0] & 0xF0) >> 4;
	len = p[0] & 0x0F;

	if (delta == 13) {
		headlen++;
		if (buflen < headlen) return -1;
		delta = p[1] + 13;
		p++;
	}
	else if (delta == 14) {
		headlen += 2;
		if (buflen < headlen) return -1;
		delta = ((p[1] << 8) | p[2]) + 269;
		p += 2;
	}
	else if (delta == 15) return -1;

	if (len == 13) {
		headlen++;
		if (buflen < headlen) return -1;
		len = p[1] + 13;
		p++;
	}
	else if (len == 14) {
		headlen += 2;
		if (buflen < headlen) return -1;
		len = ((p[1] << 8) | p[2]) + 269;
		p += 2;
	}
	else if (len == 15)
		return -1;

	if ((p + 1 + len) > (*buf + buflen))  return -1;
	option->number = delta + *running_delta;
	option->buffer = p + 1;
	option->length = len;
	*buf = p + 1 + len;
	*running_delta += delta;

	return 0;
}

uint16_t Coap::put(IPAddress ip, int port, char *url, char *payload) {
	return this->send(ip, port, url, COAP_CON, COAP_PUT, NULL, 0, (uint8_t *)payload, strlen(payload));
}

uint16_t Coap::put(IPAddress ip, int port, char *url, char *payload, int payloadlen) {
	return this->send(ip, port, url, COAP_CON, COAP_PUT, NULL, 0, (uint8_t *)payload, payloadlen);
}

uint16_t Coap::get(IPAddress ip, int port, char *url) {
	return this->send(ip, port, url, COAP_CON, COAP_GET, NULL, 0, NULL, 0);
}

bool Coap::loop()
{

	uint8_t buffer[BUF_MAX_SIZE];
	int32_t packetlen = _udp->parsePacket();

	while (packetlen > 0) {
		packetlen = _udp->read(buffer, packetlen >= BUF_MAX_SIZE ? BUF_MAX_SIZE : packetlen);

		CoapPacket packet;

		// parse coap packet header
		if (packetlen < COAP_HEADER_SIZE || (((buffer[0] & 0xC0) >> 6) != 1)) {
			packetlen = _udp->parsePacket();
			continue;
		}

		packet.type = (buffer[0] & 0x30) >> 4;
		packet.tokenlen = buffer[0] & 0x0F;
		packet.code = buffer[1];
		packet.messageid = 0xFF00 & (buffer[2] << 8);
		packet.messageid |= 0x00FF & buffer[3];

		if (packet.tokenlen == 0)  packet.token = NULL;
		else if (packet.tokenlen <= 8)  packet.token = buffer + 4;
		else {
			packetlen = _udp->parsePacket();
			continue;
		}

		// parse packet options/payload
		if (COAP_HEADER_SIZE + packet.tokenlen < packetlen) {
			int optionIndex = 0;
			uint16_t delta = 0;
			uint8_t *end = buffer + packetlen;
			uint8_t *p = buffer + COAP_HEADER_SIZE + packet.tokenlen;
			while (optionIndex < MAX_OPTION_NUM && *p != 0xFF && p < end) {
				packet.options[optionIndex];
				if (0 != parseOption(&packet.options[optionIndex], &delta, &p, end - p))
					return false;
				optionIndex++;
			}
			packet.optionnum = optionIndex;

			if (p + 1 < end && *p == 0xFF) {
				packet.payload = p + 1;
				packet.payloadlen = end - (p + 1);
			}
			else {
				packet.payload = NULL;
				packet.payloadlen = 0;
			}
		}

		if (packet.type == COAP_ACK) {
			// call response function
			resp(packet, _udp->remoteIP(), _udp->remotePort());

		}
		else if (packet.type == COAP_CON) {
			// call endpoint url function
			std::string url = "";
			for (int i = 0; i < packet.optionnum; i++) {
				if (packet.options[i].number == COAP_URI_PATH && packet.options[i].length > 0) {
					int pilen = packet.options[i].length + 1;
					std::string urlname((const char*)packet.options[i].buffer, (size_t)packet.options[i].length);
					//urlname = (char*)malloc(pilen);
					//memcpy(urlname, packet.options[i].buffer, packet.options[i].length);
					urlname[packet.options[i].length] = NULL;
					if (url.length() > 0)
						url += "/";
					url += urlname;
				}
			}
#if defined(ARDUINO)
			if (!uri.find(url)) {
#elif defined(SPARK)
			if (uri.find(url) == uri.end()) {
#elif defined(WIN32)
			//if (!uri.find(url)) {
			if (uri.find(url) == uri.end()) {
#endif
				sendResponse(_udp->remoteIP(), _udp->remotePort(), packet.messageid, NULL, 0,
					COAP_NOT_FOUNT, COAP_NONE, NULL, 0);
			}
			else {
#if defined(ARDUINO)
				uri.find(url)(packet, _udp->remoteIP(), _udp->remotePort());
#elif defined(SPARK)
				uri[url](packet, _udp->remoteIP(), _udp->remotePort());
#elif defined(WIN32)
				uri[url](packet, _udp->remoteIP(), _udp->remotePort());
#endif
			}
		}

		// next packet
		packetlen = _udp->parsePacket();
	}

	return true;
}


//These are the routines to read an incoming UDP Packet
UDP::UDP()
{
	WSAData wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	this->_wsa = wsa;
	slen = sizeof(si_other);
	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	//setup address structure for data/info on the other end
	memset((char *)&si_other, 0, sizeof(si_other));
	//si_other.sin_family = AF_INET;
	//si_other.sin_port = htons(PORT);

	//Setup the remote end info
	memset((char*)&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(PORT); //COAP Port

	//Now bind the socket
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d", WSAGetLastError());
		//exit(EXIT_FAILURE);
	}
}

UDP::~UDP()
{
	closesocket(this->s);
	WSACleanup(); //Cleans up the Winsock2 stuff
}

uint8_t UDP::SetIPAddress(IPAddress ipaddr)
{
	//si_other.sin_addr.S_un.S_addr = inet_addr(ipaddr.ip);
	return 0;
}

uint16_t UDP::sendDatagram(char *buffer, uint16_t bufferLen, uint16_t flags, uint16_t toLen)
{
	uint16_t sockErr = sendto(this->s, (char*)buffer, bufferLen, 0, (struct sockaddr *) &si_other, this->slen);
	return sockErr;
}
uint32_t UDP::read(uint8_t *buffer, uint32_t packetlen) {
	if (udpDataBuffer == nullptr) { return -1; } //No data waiting. ParsePacket hasn't been called yet
	memcpy((char*)buffer, udpDataBuffer, _recvlen);
	free(udpDataBuffer);
	udpDataBuffer = nullptr;
	return _recvlen;
}

//Currently attempt to read a UDP packet as a blocking call
uint32_t UDP::parsePacket() 
{
	_recvlen = 0;
	udpDataBuffer = (char*) malloc(BUFLEN);
	//char buf[BUFLEN];
	memset(udpDataBuffer, '\0', BUFLEN); //Ensure buffer is empty

	_recvlen = recvfrom(s, udpDataBuffer, BUFLEN, 0, (struct sockaddr *) &si_other, &slen);
	//try to receive some data, this is a blocking call
	if (_recvlen == SOCKET_ERROR)
	{
		printf("recvfrom() failed with error code : %d", WSAGetLastError());
		//exit(EXIT_FAILURE);
	}
	else
	{

		//print details of the client/peer and the data received
		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		printf("Data: %s\n", udpDataBuffer);
	}

	return _recvlen;
}

IPAddress UDP::remoteIP() {
	IPAddress ipa;
	ipa.ip = "127.0.0.1";
	return ipa;
}

uint32_t UDP::remotePort() {
	return 0;
}




