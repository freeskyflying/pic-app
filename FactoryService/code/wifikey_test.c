#include "common_test.h"
#include "key.h"

static int wifiKeyStatus = 0;

int wifiKeyHandler(struct input_event* dev)
{
	if (1 == dev->type && 103 == dev->code)
	{
		if (0 == dev->value)
		{
			wifiKeyStatus = 1;
		}
		else if (1 == wifiKeyStatus)
		{
			gTestResults[ITEM_WIFIKEY].result = 1;
			gTestResults[ITEM_WIFIKEY].strInfo[0] = 0;
		}
	}
	return 0;
}

extern BoardSysInfo_t gBoardInfo;
void* WifiKeyTestThread(void* pArg)
{
	pthread_detach(pthread_self());

	//KeyHandle_Register(wifiKeyHandler, 103);

	gTestResults[ITEM_WIFIKEY].result = 0;
	strcpy(gTestResults[ITEM_WIFIKEY].strInfo, "等待按键按下再松开");

	while (gTestRunningFlag)
	{
		usleep(20*1000);

		int tmp = 0;
		
		tmp = GPIOKeyReadGpioState(83); // wifi key gpiof12 83 V1.2

		if (0 == tmp)
		{
			if (wifiKeyStatus)
			{
				gTestResults[ITEM_WIFIKEY].result = 1;
				gTestResults[ITEM_WIFIKEY].strInfo[0] = 0;
			}
		}
		else
		{
			wifiKeyStatus = tmp;
		}
		
		if (gTestResults[ITEM_WIFIKEY].result)
		{
			break;
		}
	}

	return NULL;
}

void WifiKeyTest_Start(void)
{
	pthread_t pid;

	pthread_create(&pid, 0, WifiKeyTestThread, NULL);
}

