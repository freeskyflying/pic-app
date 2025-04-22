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

	if (access("/data/etc", F_OK))
	{
		mkdir("/data/etc", 660);
	}

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

#ifdef ENGLISH
	param.language = LANGUAGE_en_US;
#else
#ifdef SPANISH
	param.language = LANGUAGE_Spanish;
#else
	param.language = LANGUAGE_zh_CN;
#endif
#endif 
	param.limitSpeed = 100;

	param.mirror[0] = 0;
	param.mirror[1] = 0;
	param.mirror[2] = 0;
	param.mirror[3] = 0;
	param.mirror[4] = 0;
	param.mirror[5] = 0;
	param.mirror[6] = 0;
	param.mirror[7] = 0;
	
#if (defined CH1MIRRO)
	param.mirror[0] = 1;
#endif

#if (defined CH2MIRRO)
	param.mirror[1] = 1;
#endif

#if (defined CH3MIRRO)
	param.mirror[2] = 1;
#endif

#if (defined INDIA)
	param.parking_monitor = 0;
	param.timelapse_rate = 0;
	param.overspeed_report_intv = 30;
#else
	param.parking_monitor = 0;
	param.timelapse_rate = 0;
	param.overspeed_report_intv = 0;
#endif

#if (defined MILES)
	param.mSpdOsdType = 1;
#else
	param.mSpdOsdType = 0;
#endif

#if (defined SOSOFF)
	param.mSOSDisable = 1;
#else
	param.mSOSDisable = 0;
#endif

	param.wakealarm_enable = 0;
	param.wakealarm_interval = 15;

	strcpy(param.wifiSSIDEx, "PARRT");
	strcpy(param.wifiPswdEx, "PARRT2023");

	int fd = open(GLOBAL_SYS_PARAMS_FILE, O_RDWR | O_CREAT);
	if (fd < 0)
	{
		PNT_LOGE("create %s failed.\n", GLOBAL_SYS_PARAMS_FILE);
		return;
	}
	
	int size = lseek(fd, 0, SEEK_END);

	if (size < sizeof(GlobalSysParam_t))
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

	getStringByFile("/data/sn", gGlobalSysParam->serialNum, sizeof(gGlobalSysParam->serialNum));
	
	PNT_LOGI("GlobalParam serilNum: %s.\n", gGlobalSysParam->serialNum);
	PNT_LOGI("GlobalParam wifiSSID: %s.\n", gGlobalSysParam->wifiSSID);
	PNT_LOGI("GlobalParam wifiPswd: %s.\n", gGlobalSysParam->wifiPswd);
	PNT_LOGI("GlobalParam dsmEnable: %d.\n", gGlobalSysParam->dsmEnable);
	PNT_LOGI("GlobalParam adasEnable: %d.\n", gGlobalSysParam->adasEnable);
	PNT_LOGI("GlobalParam recDuration: %d.\n", gGlobalSysParam->recDuration);
	PNT_LOGI("GlobalParam recResolution: %d.\n", gGlobalSysParam->recResolution);
	PNT_LOGI("GlobalParam mVolume: %d.\n", gGlobalSysParam->mVolume);
	PNT_LOGI("GlobalParam timezone: %d.\n", gGlobalSysParam->timezone);
	PNT_LOGI("GlobalParam mirror[0]: %d.\n", gGlobalSysParam->mirror[0]);
	PNT_LOGI("GlobalParam mirror[1]: %d.\n", gGlobalSysParam->mirror[1]);
	PNT_LOGI("GlobalParam mirror[2]: %d.\n", gGlobalSysParam->mirror[2]);

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


