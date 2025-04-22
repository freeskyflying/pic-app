#include <stdio.h>
#include <signal.h>
#include <execinfo.h>
#include "sample_comm.h"
#include "typedef.h"
#include "pnt_log.h"
#include "media_osd.h"
#include "media_video.h"
#include "utils.h"
#include "queue_list.h"
#include "media_utils.h"
#include "media_storage.h"
#include "controller.h"
#include "system_param.h"
#include "dms/dms_config.h"
#include "adas/adas_config.h"
#include "key.h"
#include "gpio_led.h"
#include "kwatch.h"
#include "encrypt.h"
#include "gpsReader.h"

BoardSysInfo_t gBoardInfo = { 0 };
volatile unsigned char gACCState = 0;
volatile unsigned char gStopFlag = 0;

extern int protocal_main();

#define LOG_SAVE		0

void ShowStack()
{
    int i;
    void *buffer[1024];
    int n = backtrace(buffer, 1024);
    char **symbols = backtrace_symbols(buffer, n);
    for (i = 0; i < n; i++)
	{
        PNT_LOGE("%s\n", symbols[i]);
    }
}

extern void protocal_exit(void);
extern void wifiStop(void);
extern void net4gRildStop(void);
void trigForceStop(void)
{
	wifiStop();
	net4gRildStop();
	
	int recPid = getProcessPID("VehicleService", "VehicleService");
	if (recPid > 0)
	{
		PNT_LOGI( "trig force stop!\n");
		char cmd[128] = { 0 };
		sprintf(cmd, "kill -10 %d", recPid);
		system(cmd);
	}
}
void SigSegvProc(int signo)
{
	if (gStopFlag)
	{
		return;
	}
    PNT_LOGE("......catch you, SIGINT, signo = %d\n", signo);

	if (signo == SIGSEGV || signo == SIGABRT || signo == SIGKILL || signo == SIGILL)
	{
	    PNT_LOGE("-----call stack-----\n");
	    ShowStack();
	}

	gStopFlag = 1;
	protocal_exit();
	MediaStorage_Exit();

	gpsReaderStop();
	MediaVideo_Stop();

	if (0 == signo)
		trigForceStop();
#if LOG_SAVE
	LogSaveToFile("", 1);
#endif
	sleep(4);
	
    exit(1);
}
void RegSig()
{
    signal(SIGSEGV, SigSegvProc);
    signal(SIGTERM, SigSegvProc);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, SigSegvProc);
	signal(SIGABRT, SigSegvProc);
	signal(SIGKILL, SigSegvProc);
	signal(SIGILL, SigSegvProc);
}

int aacKeyOffHandler(struct input_event* dev)
{
	if (1 == dev->type && 105 == dev->code)
	{
		if (!dev->value) // Acc OFF
		{
			if (gACCState)
			{
				gACCState = 0;
				PNT_LOGE("acc turn off\n");
			}
		}
		else // ACC ON
		{
			if (!gACCState)
			{
				gACCState = 1;
				PNT_LOGE("acc turn on\n");
			}
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	// DNS解析，支持URL
	system("echo nameserver 8.8.8.8 >> /tmp/resolv.conf");

	if (!access("/tmp/mainPid", F_OK))
	{
		PNT_LOGE("******************fatal error ,re-start a main process.");
		exit(1);
		return -1;
	}

	setFlagStatus("mainPid", getpid());
	
	loadSysLastTime();
#if LOG_SAVE
	LogSaveInit();
#endif

	getBoardSysInfo(&gBoardInfo);
	
	GpioKey_Init();
	KeyHandle_Register(aacKeyOffHandler, 105);

	PNT_LOGI("RecorderService start.\n");

	RegSig();

	Global_SystemParam_Init();
	
	if (0 == strcmp(gBoardInfo.mProductModel, "P-DC-1"))
	{
		gGlobalSysParam->parking_monitor = 0;
	}

	dms_al_para_init();
	adas_al_para_init();

	extern int vehicle_main();
	vehicle_main();

	while (1)
	{
		if (EncryptCheck())
		{
			setLedStatus(NET4G_LED, 1);
			setLedStatus(GPS_LED, 1);
			setLedStatus(REC_LED1, 1);
			setLedStatus(REC_LED2, 1);
			sleep(1);
			setLedStatus(NET4G_LED, 0);
			setLedStatus(GPS_LED, 0);
			setLedStatus(REC_LED1, 0);
			setLedStatus(REC_LED2, 0);
			sleep(1);
		}
		else
		{
			break;
		}
		if (access("/tmp/noLicenseTest", F_OK) == 0)
		{
			break;
		}
	}

	protocal_main();

	extern void RtspServerInit(void);
	RtspServerInit();

	sleep(1);

	int ret = MediaVideo_RecorderInit();
	if (RET_FAILED == ret)
	{
		if (0 == access("/data/gc2083", F_OK))
		{
			remove("/data/gc2083");
			system("echo 1 > /data/gc2053");
		}
		else// if (0 == access("/data/gc2053", F_OK))
		{
			remove("/data/gc2053");
			system("echo 1 > /data/gc2083");
		}
		PNT_LOGE("fatal error !!! init failed, exit.");
		trigForceStop();
#if LOG_SAVE
		LogSaveToFile("", 1);
#endif
		sleep(1);
	}
	else
	{
	    EI_MI_MBASE_SetUsrLogLevel(EI_ID_VISS, 3);
	    EI_MI_MBASE_SetKerLogLevel(EI_ID_VISS, 3);
	    EI_MI_MBASE_SetUsrLogLevel(EI_ID_VPU, 3);
	    EI_MI_MBASE_SetKerLogLevel(EI_ID_VPU, 3);
	    EI_MI_MBASE_SetUsrLogLevel(EI_ID_VBUF, 3);
	    EI_MI_MBASE_SetKerLogLevel(EI_ID_VBUF, 3);
	    EI_MI_MBASE_SetUsrLogLevel(EI_ID_JVC, 3);
	    EI_MI_MBASE_SetKerLogLevel(EI_ID_JVC, 3);

		MediaVideo_Start();

		MediaStorage_Start();

		if (0 == strcmp(gBoardInfo.mProductModel, "P-DC-2") || 0 == strcmp(gBoardInfo.mProductModel, "P-DC-3"))
		{
			KWatchInit();
		}
		
		Algo_Start();
	}

	while (1)
	{
		sleep(1);
		updateSysLastTime();

		gCPUTemprature = getCpuTemprature();
		PNT_LOGI("CPU Temp: %d C  (> 95, stop adas and dms).", gCPUTemprature);

		if (0 == access("/tmp/Shutdown", F_OK))
		{
			PNT_LOGE("get /tmp/Shutdown flag, start to stop.");
			gStopFlag = 1;
			MediaStorage_Exit();

			gpsReaderStop();
			MediaVideo_Stop();
			protocal_exit();
#if LOG_SAVE
			LogSaveToFile("", 1);
#endif			
		    exit(1);
		}
	}

	return 0;
}
