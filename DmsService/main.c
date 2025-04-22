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

BoardSysInfo_t gBoardInfo = { 0 };

extern int protocal_main();

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

void SigSegvProc(int signo)
{
    PNT_LOGE("......catch you, SIGINT, signo = %d\n", signo);
	
	MediaStorage_Exit();
	MediaVideo_Stop();
	JT808Controller_Stop();

	sleep(2);
	LogSaveToFile("", 1);

//	if (SIGSEGV == signo)
	{
	    //printf("Receive SIGSEGV signal\n");
	    PNT_LOGE("-----call stack-----\n");
	    ShowStack();
	}
	
    exit(-1);
}

void sig_int(int paraSigno)
{
    PNT_LOGE("......catch you, SIGINT, signo = %d\n", paraSigno);
	MediaStorage_Exit();
	MediaVideo_Stop();
	sleep(2);
	LogSaveToFile("", 1);
	exit(1);
}

void RegSig()
{
    signal(SIGSEGV, SigSegvProc);
    signal(SIGTERM, sig_int);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, sig_int);
	signal(SIGABRT, SigSegvProc);
	signal(SIGKILL, SigSegvProc);
	signal(SIGILL, SigSegvProc);
}

int main(int argc, char **argv)
{
	LogSaveInit();
	
	PNT_LOGI("RecorderService start.\n");

	RegSig();

	Global_SystemParam_Init();

	getBoardSysInfo(&gBoardInfo);

	extern int vehicle_main();
	vehicle_main();

	protocal_main();

	extern void RtspServerInit(void);
	RtspServerInit();

	sleep(1);

	int ret = MediaVideo_RecorderInit();
	if (RET_FAILED == ret)
	{
		PNT_LOGE("fatal error !!! init failed, exit.");
		return 0;
	}

	sleep(1);
	
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
	
	HttpContorller_Start();

	while (1)
	{
		SleepMs(100);
	}

	return 0;
}
