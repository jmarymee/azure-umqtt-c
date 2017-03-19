#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdlib.h>
//#include "azure_c_shared_utility/gballoc.h"
//#include "azure_c_shared_utility/platform.h"
//#include "azure_c_shared_utility/tickcounter.h"
//#include "azure_c_shared_utility/crt_abstractions.h"
//#include "azure_c_shared_utility/xlogging.h"
//#include "azure_c_shared_utility/strings.h"
//#include "azure_c_shared_utility/agenttime.h"
//#include "azure_c_shared_utility/threadapi.h"

#include <iostream>
#include <map>
#include <cstring>

#include <inttypes.h>
#include "coap.h"

#define SERVER "127.0.0.1"  //ip address of udp server
//#define BUFLEN 512  //Max length of buffer
#define PORT 5683   //The port on which to listen for incoming data

Coap::Coap()
{
	_udp = new UDP();
	_udp->fnPtrCoapDatagramReceived = CoapDatagramReceived;
}

uint16_t Coap::sendPacket(CoapPacket &packet, IPAddress ip, int port) 
{
	uint8_t buffer[COAP_MAX_DATAGRAM_SIZE];
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

		if (packetSize + 5 + packet.options[i].length >= COAP_MAX_DATAGRAM_SIZE) {
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
		if ((packetSize + 1 + packet.payloadlen) >= COAP_MAX_DATAGRAM_SIZE) {
			return 0;
		}
		*p++ = 0xFF;
		memcpy(p, packet.payload, packet.payloadlen);
		packetSize += 1 + packet.payloadlen;
	}

	//Send message via the UDP class
	if (packet.type == COAP_ACK) //Is this a server ACK or a new conversation that was client-initiated?
	{
		if (_udp->sendResponseDatagram(ip, port, (char*)buffer, packetSize) == SOCKET_ERROR)
		{
			printf("sendResponseDatagram() failed with error code : %d", WSAGetLastError());
		}
	}
	else
	{
		if (_udp->sendClientDatagram(ip, port, (char*)buffer, packetSize) == SOCKET_ERROR)
		{
			printf("sendResponseDatagram() failed with error code : %d", WSAGetLastError());
		}
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

void CoapDatagramReceived(const unsigned char * buffer, size_t size)
{
	//throw (std::exception("Not implemented"));
	printf("In Friend function. Data: %s\n", buffer);
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
	uint8_t buffer[COAP_MAX_DATAGRAM_SIZE];
	int32_t packetlen = _udp->parsePacket();

	while (packetlen > 0) {
		packetlen = _udp->read(buffer, packetlen >= COAP_MAX_DATAGRAM_SIZE ? COAP_MAX_DATAGRAM_SIZE : packetlen);

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
			char* buf;
			resp(packet, _udp->remoteIP(), _udp->remotePort(), buf);

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
#if defined(WIN32)
			//If we can't find the URL then we return a NOT_FOUND ack
			if (uri.find(url) == uri.end()) {
#endif
				sendResponse(_udp->remoteIP(), _udp->remotePort(), packet.messageid, NULL, 0, COAP_NOT_FOUNT, COAP_NONE, NULL, 0);
			}
			else {
#if defined(WIN32)
				char *buf = nullptr;
				uri[url](packet, _udp->remoteIP(), _udp->remotePort(), buf); //Calls the callback which is part of the URI key/value map

				//This is a 'behind the scenes' ack response to the confirmable message. No payload just a 2.04 and returned messageID and token.
				//https://tools.ietf.org/html/rfc7252#section-4.2
				if (buf != nullptr)
				{
					sendResponse(_udp->remoteIP(), _udp->remotePort(), packet.messageid, buf, strlen(buf), COAP_CONTENT, COAP_APPLICATION_JSON, packet.token, packet.tokenlen);
					free(buf);
					buf = nullptr;
				}
				else
				{
					sendResponse(_udp->remoteIP(), _udp->remotePort(), packet.messageid, NULL, 0, COAP_CHANGED, COAP_NONE, packet.token, packet.tokenlen);
				}
#endif
			}
		}
		else if (packet.type == COAP_NONCON) {
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
#if defined(WIN32)
			//if (!uri.find(url)) {
			if (uri.find(url) == uri.end()) {
#endif
				//We are sending a response since we don't know this endpoint (2.04) per the RFC even though this isn't a CONFIRM message
				sendResponse(_udp->remoteIP(), _udp->remotePort(), packet.messageid, NULL, 0, COAP_NOT_FOUNT, COAP_NONE, NULL, 0);
			}
			else {
#if defined(WIN32)
				char *buf;
				uri[url](packet, _udp->remoteIP(), _udp->remotePort(), buf); //Calls the callback which is part of the URI key/value map
				
				//This is a response to a non-confirming request (such as a GET). If the buffer is not null then it means we have a payload to send
				// In that case per the spec we SHOULD respond with another non-confirming message. We are assuming that the client endpoint
				// Will be listening on the same url as the one is used to request data from us. 
				//https://tools.ietf.org/html/rfc7252#section-4.2
				char *uriEnpoint = (char*)url.c_str();
				if (buf != nullptr)
				{
					//sendResponse(_udp->remoteIP(), _udp->remotePort(), packet.messageid, buf, strlen(buf), COAP_CONTENT, COAP_APPLICATION_JSON, packet.token, packet.tokenlen);
					send(_udp->remoteIP(), _udp->remotePort(), uriEnpoint, COAP_NONCON, COAP_PUT, packet.token, packet.tokenlen, (uint8_t*)buf, strlen(buf));
					free(buf);
					buf = nullptr;
				}
				else
				{
					//No response since this was a non-confirming message
					//sendResponse(_udp->remoteIP(), _udp->remotePort(), packet.messageid, NULL, 0, COAP_CHANGED, COAP_NONE, packet.token, packet.tokenlen);
				}
#endif
			}
		}
		// next packet
		packetlen = _udp->parsePacket();
	}

	return true;
}


//These are the routines to read and send UDP Datagrams
UDP::UDP()
{
	WSAData wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	this->_wsa = wsa;
	slen = sizeof(si_other);

	int ccode; //Err codes
	u_long iMode; //Used for block/nonblock modes

	//This is used to listen on the COAP port (5683) and respond to requests such as GETs
	if ((serverSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		printf("Error opening client socket : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	//setup address structure for data/info on the other end. This gives us info about the other end (IP Address/Port)
	memset((char *)&si_other, 0, sizeof(si_other)); //Set to zeros for safety

	//Setup our local info
	memset((char*)&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(PORT); //COAP Port

	//Now bind the socket for listens
	if (bind(serverSock, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Server bind failed with error code : %d", WSAGetLastError());
		//exit(EXIT_FAILURE);
	}

	//Now put into non-blocking mode
	iMode = NONBLOCKING; //nonzero for non-blocking, zero (default) for blocking
	//u_long iMode = BLOCKING; //nonzero for non-blocking, zero (default) for blocking
	ccode = ioctlsocket(serverSock, FIONBIO, &iMode);

	//This is used to initiate requests to other COAP peers
	if ((clientSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		printf("Error opening client socket : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	//setup address structure for data/info on the other end. This gives us info about the other end (IP Address/Port)
	memset((char *)&cli_other, 0, sizeof(cli_other)); //Set to zeros for safety

	//Setup our local info
	memset((char*)&client, 0, sizeof(client));
	client.sin_family = AF_INET;
	client.sin_addr.s_addr = INADDR_ANY;
	client.sin_port = htons(0); //We will take ANY open port

	//Now bind the socket for listens
	if (bind(clientSock, (struct sockaddr *)&client, sizeof(client)) == SOCKET_ERROR)
	{
		printf("Client bind failed with error code : %d", WSAGetLastError());
		//exit(EXIT_FAILURE);
	}

	//Now put into non-blocking mode
	iMode = NONBLOCKING; //nonzero for non-blocking, zero (default) for blocking
	//u_long iMode = BLOCKING; //nonzero for non-blocking, zero (default) for blocking
	ccode = ioctlsocket(clientSock, FIONBIO, &iMode);
}

UDP::~UDP()
{
	closesocket(serverSock);
	closesocket(clientSock);
	WSACleanup(); //Cleans up the Winsock2 stuff
}

uint8_t UDP::SetIPAddress(IPAddress ipaddr)
{
	//si_other.sin_addr.S_un.S_addr = inet_addr(ipaddr.ip);
	return 0;
}

uint16_t UDP::sendResponseDatagram(IPAddress addr, int port, char *buffer, int len)
{

	//setup address structure
	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(port);
	si_other.sin_addr.S_un.S_addr = inet_addr(addr.ip);

	if (sendto(serverSock, (char*)buffer, len, 0, (struct sockaddr *) &si_other, sizeof(si_other)) == SOCKET_ERROR)
	{
		printf("sendto failed with error: %d\n", WSAGetLastError());
		return -1;
	}
	return 0;
}

//Test routine ONLY!
uint16_t UDP::sendClientDatagram(IPAddress addr, int port, char *buffer, int len)
{
	//uint16_t cSock = INVALID_SOCKET;
	//struct sockaddr_in cli_other;
	//int tolen = sizeof(cli_other);
	//cSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	//setup address structure
	memset((char *)&cli_other, 0, sizeof(cli_other));
	cli_other.sin_family = AF_INET;
	cli_other.sin_port = htons(port);
	cli_other.sin_addr.S_un.S_addr = inet_addr(addr.ip);

	//setup address structure
	//memset((char *)&si_other, 0, sizeof(si_other)); //Reuse the target structure
	//si_other.sin_family = AF_INET;
	//si_other.sin_port = htons(5683);
	//si_other.sin_addr.S_un.S_addr = inet_addr("10.0.0.66");

	int sockErr = sendto(clientSock, buffer, len, 0, (struct sockaddr *) &cli_other, sizeof(cli_other));
	if (sockErr == SOCKET_ERROR)
	{
		printf("sendto failed with error: %d\n", WSAGetLastError());
	}
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
	udpDataBuffer = (char*)malloc(COAP_MAX_DATAGRAM_SIZE);
	//char buf[BUFLEN];
	memset(udpDataBuffer, '\0', COAP_MAX_DATAGRAM_SIZE); //Ensure buffer is empty

	_recvlen = recvfrom(serverSock, udpDataBuffer, COAP_MAX_DATAGRAM_SIZE, 0, (struct sockaddr *) &si_other, &slen);
	//try to receive some data, this is a blocking call
	if (_recvlen == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			printf("recvfrom() failed with error code : %d", WSAGetLastError());
			//exit(EXIT_FAILURE);
		}
	}
	else
	{
		//print details of the client/peer and the data received
		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		printf("Data: %s\n", udpDataBuffer);

		//this->fnPtrCoapDatagramReceived((const unsigned char*)udpDataBuffer, _recvlen); //Calls the function prt friend function
	}

	return _recvlen;
}

//inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port)
IPAddress UDP::remoteIP() {
	IPAddress ipa;
	ipa.ip = inet_ntoa(si_other.sin_addr);
	return ipa;
}

uint32_t UDP::remotePort() {
	return ntohs(si_other.sin_port);
}




