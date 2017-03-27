#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "udp.h"
#include <iostream>
#include <map>
#include <cstring>

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
	//server.sin_port = htons(PORT); //COAP Port

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
	//setup address structure
	memset((char *)&cli_other, 0, sizeof(cli_other));
	cli_other.sin_family = AF_INET;
	cli_other.sin_port = htons(port);
	cli_other.sin_addr.S_un.S_addr = inet_addr(addr.ip);

	int sockErr = sendto(clientSock, buffer, len, 0, (struct sockaddr *) &cli_other, sizeof(cli_other));
	if (sockErr == SOCKET_ERROR)
	{
		printf("sendto failed with error: %d\n", WSAGetLastError());
	}
	return sockErr;
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
		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		printf("Data: %s\n", udpDataBuffer);

		this->fnPtrDatagramReceived((const unsigned char*)udpDataBuffer, _recvlen); //Calls the function prt friend function
	}

	return _recvlen;
}

//Currently attempt to read a UDP packet as a blocking call
uint32_t UDP::parseClientPacket(int maxDatagramSize)
{
	int receiveLen = 0;
	char *rbuf = (char*)malloc(maxDatagramSize);
	memset(rbuf, '\0', maxDatagramSize); //Ensure buffer is empty
	memset((char *)&cli_other, 0, sizeof(cli_other));
	int cliLen = sizeof(cli_other);

	receiveLen = recvfrom(clientSock, rbuf, maxDatagramSize, 0, (struct sockaddr *) &cli_other, &cliLen);
	//receiveLen = recv(clientSock, rbuf, COAP_MAX_DATAGRAM_SIZE, 0);
	//try to receive some data, this is a blocking call
	if (receiveLen == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			printf("recvfrom() failed with error code : %d", WSAGetLastError());
			//exit(EXIT_FAILURE);
		}
		else if (WSAGetLastError() == WSAEWOULDBLOCK)
		{
			printf("waiting for client receieve : %d", WSAGetLastError());
		}
	}
	else
	{
		//print details of the client/peer and the data received
		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		printf("Data: %s\n", udpDataBuffer);

		this->fnPtrDatagramReceived((const unsigned char*)udpDataBuffer, _recvlen); //Calls the function prt friend function
	}

	return receiveLen;
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