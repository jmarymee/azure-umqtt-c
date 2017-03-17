#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <coap.h>
#include <conio.h>

void callback_get_gateway_status(CoapPacket &packet, IPAddress ip, int port);
void callback_put_light(CoapPacket &packet, IPAddress ip, int port);

Coap *gCoap; //This is used so that GETS can send a response to the requesting client without requiring the instantiation of another Coap Object

int main(void)
{
	Coap* coap = new Coap();
	gCoap = coap;
	coap->server(callback_put_light, "light");
	coap->server(callback_get_gateway_status, "temp");
	
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
	coap = nullptr;
	gCoap = nullptr;
}

void callback_get_gateway_status(CoapPacket &packet, IPAddress ip, int port) {

	//First we can decided if we want to handle this packet since we are expecting a PUT and got something else
	if (packet.code != COAP_GET) { return; }

	//Do our code to get the Gateway status
	char* wxJSON = "{\"coord\":{\"lon\":139,\"lat\":35},\"sys\":{\"country\":\"JP\",\"sunrise\":1369769524,\"sunset\":1369821049},\"weather\":[{\"id\":804,\"main\":\"clouds\",\"description\":\"overcast clouds\",\"icon\":\"04n\"}],\"main\":{\"temp\":289.5,\"humidity\":89,\"pressure\":1013,\"temp_min\":287.04,\"temp_max\":292.04},\"wind\":{\"speed\":7.31,\"deg\":187.002},\"rain\":{\"3h\":0},\"clouds\":{\"all\":92},\"dt\":1369824698,\"id\":1851632,\"name\":\"Maui Research Center\",\"cod\":200}";
	uint16_t len = strlen(wxJSON);
	//Call response code
	gCoap->sendResponse(ip, port, packet.messageid, wxJSON, len, COAP_CONTENT, COAP_APPLICATION_JSON, packet.token, packet.tokenlen);
	//sendResponse(_udp->remoteIP(), _udp->remotePort(), packet.messageid, NULL, 0, COAP_CHANGED, COAP_NONE, packet.token, packet.tokenlen);
	return;
}

void callback_put_light(CoapPacket &packet, IPAddress ip, int port) {
	
	//First we can decided if we want to handle this packet since we are expecting a PUT and got something else
	if (packet.type != COAP_PUT) { return; }

	//Do our code to set the light switch!

	return;
}