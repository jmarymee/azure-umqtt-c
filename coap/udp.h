
#ifdef WIN32
#include <WinSock2.h>
#include <inttypes.h>
#endif

typedef struct IPAddr {
	const char* ip; //IP4 dotted notation string '0.0.0.0'
	int			port;
} IPAddress;

typedef enum {
	BLOCKING = 0,
	NONBLOCKING = 1
} SOCKET_BLOCK_NOBLOCK;

typedef void(*ON_DATAGRAM_RECEIVED)(const unsigned char* buffer, size_t size); //Used to tell the UDP Object where to send the datagram

class UDP {
public:
	UDP();
	~UDP();
	uint32_t read(uint8_t *buffer, uint32_t packetlen);
	//sendto(s, (char*)buffer, packetSize, 0, (struct sockaddr *) &si_other, slen)
	uint16_t sendResponseDatagram(IPAddress addr, int port, char *buffer, int len);
	uint16_t sendClientDatagram(IPAddress addr, int port, char *buffer, int len);
	//uint16_t sendPacket(CoapPacket &packet, IPAddress ip, int port);
	uint32_t parsePacket(int maxDatagramSize); //Used to receive packets sent to server
	uint32_t parseClientPacket(int maxDatagramSize); //Used to read responses from client sends
	IPAddress remoteIP();
	uint32_t remotePort();
	uint8_t SetIPAddress(IPAddress ipaddr);
	ON_DATAGRAM_RECEIVED fnPtrDatagramReceived;
private:
	WSADATA _wsa;
	struct sockaddr_in si_other;
	struct sockaddr_in server; //For local listens
	struct sockaddr_in client;
	struct sockaddr_in cli_other;
	SOCKET serverSock;
	SOCKET clientSock;
	int slen, _recvlen;
	int _port;
	char* udpDataBuffer;
};