#include "common_test.h"

void* TFCardTestThread(void* pArg)
{
	pthread_detach(pthread_self());

	while (gTestRunningFlag)
	{
		if (access("/mnt/card/P-DC-1_Factory/FactoryService", R_OK))
		{
			gTestResults[ITEM_TF].result = 0;
			strcpy(gTestResults[ITEM_TF].strInfo, "未检测到TF卡");
		}
		else
		{
			if (RET_SUCCESS == checkTFCard(NULL))
			{
				gTestResults[ITEM_TF].result = 1;
				gTestResults[ITEM_TF].strInfo[0] = 0;
			}
			else
			{
				gTestResults[ITEM_TF].result = 0;
				strcpy(gTestResults[ITEM_TF].strInfo, "TF卡写失败");
			}
		}
	
		sleep(1);
	}

	return NULL;
}

void TFCardTest_Start(void)
{
	pthread_t pid;

	pthread_create(&pid, 0, TFCardTestThread, NULL);
}

