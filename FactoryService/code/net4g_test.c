#include "common_test.h"
#include "rild.h"

extern net4GRild_t gNet4GRild;

void* Net4GTestThread(void* pArg)
{
	pthread_detach(pthread_self());
	
	sleep(3);

	strcpy(gTestResults[ITEM_NET4G].strInfo, "未检测到4G模块");

	while (-1 == gNet4GRild.fd)
	{
		sleep(1);
	}

	strcpy(gTestResults[ITEM_NET4G].strInfo, "未检测到SIM卡");
	while (!gNet4GRild.simState)
	{
		sleep(1);
	}
	
	sprintf(gTestResults[ITEM_NET4G].strInfo, "等待联网");
	while (!gNet4GRild.netState)
	{
		sleep(1);
	}

	gTestResults[ITEM_NET4G].result = 1;

	while (gTestRunningFlag)
	{
		sprintf(gTestResults[ITEM_NET4G].strInfo, "信号值 %d", gNet4GRild.signalStrength);
	
		sleep(1);
		
		if (strlen(gNet4GRild.IMEI) > 0)
		{
			gTestResults[ITEM_IMEI].result = 1;
			strcpy(gTestResults[ITEM_IMEI].strInfo, gNet4GRild.IMEI);
		}
		else
		{
			gTestResults[ITEM_IMEI].result = 0;
		}
	}

	return NULL;
}

void Net4GTest_Start(void)
{
	pthread_t pid;

	net4gRildStart();

	pthread_create(&pid, 0, Net4GTestThread, NULL);
}

