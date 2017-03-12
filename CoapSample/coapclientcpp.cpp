#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <coap.h>

int main(void)
{
	Coap* coap = new Coap();
	IPAddress address;
	address.ip = "127.0.0.1";
	coap->put(address, 5683, "light", "0");

}