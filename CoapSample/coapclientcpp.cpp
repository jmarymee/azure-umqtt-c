#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <coap.h>
#include <conio.h>

int main(void)
{
	Coap* coap = new Coap();
	IPAddress address;
	//address.ip = "10.121.209.99";
	address.ip = "10.0.0.66";
	//coap->get(address, 5683, "temp");
	//coap->put(address, 5683, "light", "1");
	while (!_kbhit())
	{
		try
		{
			coap->loop();
			printf("Sleeping...\n");
			Sleep(2000);
		}
		catch(int e)
		{

		}
	}
	delete coap; //Cleans up resources like WinSock
}