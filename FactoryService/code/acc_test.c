#include "common_test.h"
#include "key.h"

int gTestShutdown = 0;
static int AccStatus = 0;

int AccHandler(struct input_event* dev)
{
	if (1 == dev->type && 105 == dev->code)
	{
		if (0 == dev->value)
		{
			if (gTestShutdown)
			{
				system("poweroff");
				while (1) sleep(1);
			}
			AccStatus = 1;
		}
		else if (1 == AccStatus)
		{
			gTestResults[ITEM_ACC].result = 1;
			gTestResults[ITEM_ACC].strInfo[0] = 0;
		}
	}
	return 0;
}

void* ACCTestThread(void* pArg)
{
	pthread_detach(pthread_self());

//	KeyHandle_Register(AccHandler, 105);
	strcpy(gTestResults[ITEM_ACC].strInfo, "等待ACC断开再上电");

	while (gTestRunningFlag)
	{
		sleep(5);
		
		if (1 == GPIOKeyReadGpioState(116))//(gTestResults[ITEM_ACC].result)
		{
			if (AccStatus)
			{
				gTestResults[ITEM_ACC].result = 1;
				gTestResults[ITEM_ACC].strInfo[0] = 0;
			}
		}
		else
		{
			if (gTestShutdown)
			{
				system("poweroff");
				while (1) sleep(1);
			}
			AccStatus = 1;
		}
	}

	return NULL;
}

void ACCTest_Start(void)
{
	pthread_t pid;

	pthread_create(&pid, 0, ACCTestThread, NULL);
}

