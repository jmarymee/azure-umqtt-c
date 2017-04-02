#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <coap.h>
#include <conio.h>

//These are both server functions. We will wait a COAP client to ping us and we will respond
void callback_get_gateway_status(CoapPacket &packet, IPAddress ip, int port, char* &buffer);
void callback_put_light(CoapPacket &packet, IPAddress ip, int port, char* &buffer);

//This is an example of us making a GET (client->Server) and awaiting a response
void callback_for_client_reqs(CoapPacket &packet, IPAddress ip, int port, char* &buffer);

int main(void)
{
	Coap* coap = new Coap();
	coap->response(callback_for_client_reqs);
	coap->server(callback_put_light, "light");
	coap->server(callback_get_gateway_status, "temp");


	bool isLight = false;

	uint16_t messageID = 0;
	
	IPAddress address;
	address.ip = "10.0.0.40";
	//messageID = coap->get(address, 5683, "temp");
	//printf("Outbound MessageID: %d\n", messageID);
	//coap->put(address, 5683, "light", "0");
	while (!_kbhit())
	{

		coap->loop();
		messageID = coap->get(address, 5683, "temp");
		printf("Outbound MessageID: %d\n", messageID);
		if (isLight) 
		{
			messageID = coap->put(address, 5683, "light", "0");
			printf("Outbound MessageID: %d\n", &messageID);
			isLight = false;
		}
		else
		{
			messageID = coap->put(address, 5683, "light", "1");
			printf("Outbound MessageID: %d\n", &messageID);
			isLight = true;
		}
		printf("Sleeping...\n");
		Sleep(2000);
	}
	delete coap; //Cleans up resources like WinSock
	coap = nullptr;
}

void callback_for_client_reqs(CoapPacket &packet, IPAddress ip, int port, char* &buffer)
{
	char *data =  (char*)calloc(packet.payloadlen+1, sizeof(char));
	memcpy(data, packet.payload, packet.payloadlen);
	printf("InBound MessageID:  %d | Data: %s\n\n", packet.messageid, data);
	//printf("MessageID: %d\n", packet.messageid);
	free(data);
}

//This is an example of a GET requested to us as a CONFIRMABLE message. Note we will return the payload as a pointer to a buffer. 
// Currently out of band (delayed responses) to a GET are not supported
// This means if the code below takes a long time to get the requested data, the client may timeout it's request
void callback_get_gateway_status(CoapPacket &packet, IPAddress ip, int port, char * &buffer)
{

	//First we can decided if we want to handle this packet since we are expecting a PUT and got something else
	if (packet.code != COAP_GET) { return; }

	//Do our code to get the Gateway status
	char* wxJSON = "{\"coord\":{\"lon\":139,\"lat\":35},\"sys\":{\"country\":\"JP\",\"sunrise\":1369769524,\"sunset\":1369821049},\"weather\":[{\"id\":804,\"main\":\"clouds\",\"description\":\"overcast clouds\",\"icon\":\"04n\"}],\"main\":{\"temp\":289.5,\"humidity\":89,\"pressure\":1013,\"temp_min\":287.04,\"temp_max\":292.04},\"wind\":{\"speed\":7.31,\"deg\":187.002},\"rain\":{\"3h\":0},\"clouds\":{\"all\":92},\"dt\":1369824698,\"id\":1851632,\"name\":\"Maui Research Center\",\"cod\":200}";
	uint16_t len = strlen(wxJSON);

	//We are given a ptr to a buffer where the payload will be stored
	//We allocate it and copy to it before return (pass by reference)
	//The underlying code will use it as the payload and then free the memory. 
	buffer = (char*)malloc(len + 1);
	memcpy(buffer, wxJSON, len);

	printf("Outbound MessageID: %d\n", packet.messageid);

	return;
}

void callback_put_light(CoapPacket &packet, IPAddress ip, int port, char * &buffer)
{
	
	//First we can decided if we want to handle this packet since we are expecting a PUT and got something else
	if (packet.code != COAP_PUT) { return; }

	//Get payload - we are expecting an int from the payload
	char *payload = (char*)calloc((packet.payloadlen +1), '\0');
	if (payload == nullptr) { return; } //out of memory
	memcpy((char*)payload, packet.payload, packet.payloadlen);
	int isOn = atoi((char*)payload);

	//Do our code to set the light switch!

	printf("Outbound MessageID: %d\n", packet.messageid);

	return;
}