#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <coap.h>
#include <conio.h>

void callback_light(CoapPacket &packet, IPAddress ip, int port);

int main(void)
{
	Coap* coap = new Coap();
	coap->server(callback_light, "light");
	IPAddress address;
	//address.ip = "10.121.209.99";
	//address.ip = "10.0.0.66";
	//coap->get(address, 5683, "temp");
	//coap->put(address, 5683, "light", "1");
	while (!_kbhit())
	{

		coap->loop();
		printf("Sleeping...\n");
		Sleep(2000);
	}
	delete coap; //Cleans up resources like WinSock
}

void callback_light(CoapPacket &packet, IPAddress ip, int port) {
	printf("In Callback\n");

	switch (packet.code)
	{
	case COAP_GET:
		printf("Get\n");
		break;
	case COAP_POST:
		printf("Post\n");
		break;
	default:
		printf("Unknown\n");
		break;
	}
}