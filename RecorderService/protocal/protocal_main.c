#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include "utils.h"

#include "jt808_session.h"
#include "jt808_database.h"
#include "jt808_controller.h"

#include "system_param.h"

#define PROTOCAL_PLATFORM_MAX		2

int protocal_main()
{
	printf("ProtocalService start.\n");
	
#ifdef ONLY_3rdCH

	Mqtt_Ftp_Start();

#else

	JT808Controller_Init();
	
	AppPlatformParams_t appPlatform = { 0 };

	for (int i=0; i<PROTOCAL_PLATFORM_MAX; i++)
	{
		memset(&appPlatform, 0, sizeof(appPlatform));
		getAppPlatformParams(&appPlatform, i);
		
		if (0 != strlen(appPlatform.ip) && 0 != appPlatform.port)
		{
			if (PROTOCAL_808_2013 == appPlatform.protocalType)
			{
				JT808Controller_AddSession(appPlatform.ip, appPlatform.port, appPlatform.phoneNum, SESSION_TYPE_JT808);
			}
			else if (PROTOCAL_808_2019 == appPlatform.protocalType)
			{
				JT808Controller_AddSession(appPlatform.ip, appPlatform.port, appPlatform.phoneNum, SESSION_TYPE_JT808_2019);
			}
			else if (PROTOCAL_905_2014 == appPlatform.protocalType)
			{
			}
		}
	}

	JT808Controller_Start();

#endif

	return 0;
}

void protocal_exit()
{
	printf("ProtocalService exit.\n");

	JT808Controller_Stop();
}

