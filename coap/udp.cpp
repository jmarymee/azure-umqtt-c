#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "udp.h"
#include <iostream>
#include <map>
#include <cstring>

//These are the routines to read and send UDP Datagrams
UDP::UDP(uint16_t port)
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
	server.sin_port = htons(port); //COAP Port if passed in to constructor

	//Now bind the socket for listens
	if (bind(serverSock, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Server bind failed with error code : %d", WSAGetLastError());
		//exit(EXIT_FAILURE);
	}

	//Now put into non-blocking mode
	//nonzero for non-blocking, zero (default) for blocking
	//u_long iMode = BLOCKING; //nonzero for non-blocking, zero (default) for blocking
	iMode = NONBLOCKING;
	ccode = ioctlsocket(serverSock, FIONBIO, &iMode);

}

UDP::~UDP()
{
	closesocket(serverSock);
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

uint32_t UDP::read(uint8_t *buffer, uint32_t packetlen) {
	if (udpDataBuffer == nullptr) { return -1; } //No data waiting. ParsePacket hasn't been called yet
	memcpy((char*)buffer, udpDataBuffer, packetlen);
	free(udpDataBuffer);
	udpDataBuffer = nullptr;
	return packetlen;
}

uint32_t UDP::parsePacket(int maxDatagramSize)
{
	_recvlen = 0;
	udpDataBuffer = (char*)malloc(maxDatagramSize);
	//char buf[BUFLEN];
	memset(udpDataBuffer, '\0', maxDatagramSize); //Ensure buffer is empty

	_recvlen = recvfrom(serverSock, udpDataBuffer, maxDatagramSize, 0, (struct sockaddr *) &si_other, &slen);
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
		//printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		//printf("Data: %s\n", udpDataBuffer);

		//this->fnPtrDatagramReceived((const unsigned char*)udpDataBuffer, _recvlen); //Calls the function print friend function
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