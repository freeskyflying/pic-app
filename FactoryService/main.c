#include "http_handle.h"
#include "controller.h"
#include "common_test.h"
#include "key.h"
#include "system_param.h"
#include "algo_common.h"

volatile int gTestRunningFlag = 1;

BoardSysInfo_t gBoardInfo = { 0 };
AlCommonParam_t gAlComParam;

int main(int argc, char **argv)
{
	system("aplay /mnt/card/P-DC-1_Factory/factory_start.wav &");

    memset(&gBoardInfo, 0, sizeof(gBoardInfo));
    getBoardSysInfo(&gBoardInfo);

	GpioKey_Init();

	Global_SystemParam_Init();

	WifiTest_Start();
	TFCardTest_Start();
	GPSTest_Start();
	SOSTest_Start();
	WifiKeyTest_Start();
	RTCTest_Start();
	Net4GTest_Start();
	GSensorTest_Start();
	ACCTest_Start();
	CameraTest_Start();
	 
	HttpContorller_Start();

	extern int RtspServerInit(void);
	RtspServerInit();

	if (0 != EncryptCheck())
	{
		gTestResults[ITEM_ENCRYPT].result = 0;
		strcpy(gTestResults[ITEM_ENCRYPT].strInfo, "授权失败");
	}
	else
	{
		gTestResults[ITEM_ENCRYPT].result = 1;
		gTestResults[ITEM_ENCRYPT].strInfo[0] = '\0';
	}
	
	while (access("/data/deviceInfo.json", F_OK)) sleep(1);
	
	cJSON *para_root = parseCJSONFile("/data/deviceInfo.json");
	if (NULL != para_root)
	{
		cJSON* item = cJSON_GetObjectItem(para_root, "sw_ver");
		if (NULL != item)
		{
			strncpy(gBoardInfo.mSwVersion, item->valuestring, sizeof(gBoardInfo.mSwVersion));
		}
		
		item = cJSON_GetObjectItem(para_root, "hw_ver");
		if (NULL != item)
		{
			strncpy(gBoardInfo.mHwVersion, item->valuestring, sizeof(gBoardInfo.mHwVersion));
		}
		cJSON_Delete(para_root);
	}
	
	gTestResults[ITEM_VERSION].result = 1;
	sprintf(gTestResults[ITEM_VERSION].strInfo, "hw:%s,sw:%s", gBoardInfo.mHwVersion, gBoardInfo.mSwVersion);

	while (1)
	{
#if 0
		
		static int flag = 0;
		if (strlen(gGlobalSysParam->serialNum) > 0)
		{
			if (0 == flag)
			{
				AppPlatformParams_t param = { 0 };
				strcpy(param.ip, "192.168.2.107");
				param.port = 6608;
				param.protocalType = PROTOCAL_808_2013;
				strcpy(param.phoneNum, "1000");
				strncpy(param.phoneNum+4, gGlobalSysParam->serialNum+7, (strlen(gGlobalSysParam->serialNum)-7)>8?8:(strlen(gGlobalSysParam->serialNum)-7));
				setAppPlatformParams(&param, 0);
				playAudio("wifi_start.pcm");
			}

			flag = 1;
		}
#endif
		usleep(100*1000);
	}

	return 0;
}
