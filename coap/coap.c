#include <stdlib.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/threadapi.h"

#include "azure_umqtt_c/mqtt_client.h"
#include "azure_umqtt_c/mqtt_codec.h"
#include <inttypes.h>
#include "coap.h"

Coap::Coap() {
	this->_udp = new UDP();
}

//int main(int argc, char** argv)
//{
//	int result;
//
//	(void)argc, (void)argv;
//
//	if (platform_init() != 0)
//	{
//		(void)printf("Cannot initialize platform.");
//		result = __LINE__;
//	}
//	else
//	{
//	}
//}

