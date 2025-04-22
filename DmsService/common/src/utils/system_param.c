#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <wait.h>

#include "system_param.h"
#include "media_utils.h"
#include "pnt_log.h"

GlobalSysParam_t* gGlobalSysParam;

void Global_SystemParam_Init(void)
{
	GlobalSysParam_t param;

	memset(&param, 0, sizeof(GlobalSysParam_t));
	
	strcpy(param.serialNum, "P-DC-100000001");
	strcpy(param.wifiPswd, "12345678");
	
	param.dsmEnable = 1;
	param.adasEnable = 1;
	
	param.recDuration = 1;
	param.recResolution = RES_720p;
	
	param.mVolume = 2;

	param.timezone = 8;

	param.protocalType = PROTOCAL_808_2013;

	param.micEnable = 1;
	param.osdEnable = 1;

	int fd = open(GLOBAL_SYS_PARAMS_FILE, O_RDWR | O_CREAT);
	if (fd < 0)
	{
		PNT_LOGE("create %s failed.\n", GLOBAL_SYS_PARAMS_FILE);
		return;
	}
	
	int size = lseek(fd, 0, SEEK_END);

	if (size != sizeof(GlobalSysParam_t))
	{
		PNT_LOGE("%s resize %d", GLOBAL_SYS_PARAMS_FILE, size);
		
		if (0 < size)
		{
			lseek(fd, 0, SEEK_SET);
			read(fd, &param, size);
		}
		ftruncate(fd, sizeof(GlobalSysParam_t)-size);
		lseek(fd, 0, SEEK_SET);
		write(fd, &param, sizeof(GlobalSysParam_t));
	}

	gGlobalSysParam = mmap(NULL, sizeof(GlobalSysParam_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (MAP_FAILED == gGlobalSysParam)
	{
		PNT_LOGE("mmap file %s failed.\n", GLOBAL_SYS_PARAMS_FILE);
		close(fd);
		return;
	}

	PNT_LOGI("GlobalParam serilNum: %s.\n", gGlobalSysParam->serialNum);
	PNT_LOGI("GlobalParam wifiSSID: %s.\n", gGlobalSysParam->wifiSSID);
	PNT_LOGI("GlobalParam wifiPswd: %s.\n", gGlobalSysParam->wifiPswd);
	PNT_LOGI("GlobalParam dsmEnable: %d.\n", gGlobalSysParam->dsmEnable);
	PNT_LOGI("GlobalParam adasEnable: %d.\n", gGlobalSysParam->adasEnable);
	PNT_LOGI("GlobalParam recDuration: %d.\n", gGlobalSysParam->recDuration);
	PNT_LOGI("GlobalParam recResolution: %d.\n", gGlobalSysParam->recResolution);
	PNT_LOGI("GlobalParam mVolume: %d.\n", gGlobalSysParam->mVolume);
	PNT_LOGI("GlobalParam timezone: %d.\n", gGlobalSysParam->timezone);

	if (strlen(gGlobalSysParam->wifiSSID) == 0)
	{
		strcpy(gGlobalSysParam->wifiSSID, gGlobalSysParam->serialNum);
	}
	
	setSpeakerVolume(gGlobalSysParam->mVolume);
	
	close(fd);
}

void Global_SystemParam_DeInit(void)
{
	if (gGlobalSysParam)
	{
		munmap(gGlobalSysParam, sizeof(GlobalSysParam_t));
	}
}


