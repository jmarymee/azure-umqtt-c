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
	address.ip = "10.0.0.66";
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
	//Serial.println("[Light] ON/OFF");

	//// send response
	//char p[packet.payloadlen + 1];
	//memcpy(p, packet.payload, packet.payloadlen);
	//p[packet.payloadlen] = NULL;

	//String message(p);

	//if (message.equals("0"))
	//	LEDSTATE = false;
	//else if (message.equals("1"))
	//	LEDSTATE = true;

	//if (LEDSTATE) {
	//	coap.sendResponse(ip, port, packet.messageid, "1");
	//	RGB.color(255, 255, 255);
	//}
	//else {
	//	coap.sendResponse(ip, port, packet.messageid, "0");
	//	RGB.color(0, 0, 0);
	//}
}