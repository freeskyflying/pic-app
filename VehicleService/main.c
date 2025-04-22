#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <signal.h>
#include <sys/statfs.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include "pnt_ipc_gps.h"
#include "pnt_ipc.h"

#include "utils.h"
#include "upgrade.h"
#include "adc.h"
#include "pnt_log.h"
#include "key.h"
#include "system_param.h"

BoardSysInfo_t gBoardInfo = { 0 };
volatile unsigned char gACCState = 1;
static int gMainProcessOn = 0;
static int shutdownProcess = 0;

void DeviceShutdown(void)
{
//	system("echo 120 1500 3000 > /sys/class/lombo_pmc/lombo_power_key_time");
//	system("echo 113 > /sys/class/gpio/export && echo out > /sys/class/gpio/gpio113/direction && echo 0 > /sys/class/gpio/gpio113/value");
	if (gGlobalSysParam->wakealarm_enable && gGlobalSysParam->wakealarm_interval > 0)
	{
		char cmd[128] = { 0 };
		PNT_LOGI("device will wakeup after %d minute.", gGlobalSysParam->wakealarm_interval);
		sprintf(cmd, "echo +%d > /sys/class/rtc/rtc0/wakealarm", gGlobalSysParam->wakealarm_interval*60);
		system(cmd);
	}
	system("poweroff -f");
	while (1) sleep(1);
}

void* ShutdownTask(void* pArg)
{
	pthread_detach(pthread_self());

	unsigned long shutdownStart = getUptime();

	PNT_LOGE("start shutdown device in 60s...");

	int recPid = -1, monitor = 0;

	while (getUptime() - shutdownStart < (60))
	{
		usleep(1000*1000);
		if (gACCState)
		{
			PNT_LOGE("stop shutdown");
			return NULL;
		}

		if ((gGlobalSysParam->parking_monitor) && 0 == gACCState)
		{
			if (!(gAdcResult->powerVolt > 3000 && gAdcResult->powerVolt<8600))
			{
				recPid = getProcessPID("RecorderService", "RecorderService");
				if (recPid < 0 && gMainProcessOn && 0 == shutdownProcess)
				{
					PNT_LOGE("RecorderService killed, restart it.");
					sleep(1);
					system("reboot");
				}
			}
			shutdownStart = getUptime();
		}
	}

	PNT_LOGE("shutdown device ...");

	system("touch /tmp/Shutdown");

	sleep(5);
	
	recPid = getProcessPID("RecorderService", "RecorderService");
	if (recPid > 0)
	{
		PNT_LOGE( "Stop Recorde!\n");
		char cmd[128] = { 0 };
		sprintf(cmd, "kill -2 %d", recPid);
		system(cmd);
		sleep(5);
	}

	DeviceShutdown();

	return NULL;
}

void* MainPowerMonitor(void* pArg)
{
	pthread_detach(pthread_self());

	while (NULL == gAdcResult)
	{
		sleep(1);
	}
	
	int recPid = -1;

	while (1)
	{
		if ((gAdcResult->powerVolt > 3000 && gAdcResult->powerVolt<8600) && !gACCState)
		{
			PNT_LOGE("Main power lower: %d.%dV\nWill shutdown device.", gAdcResult->powerVolt/1000, gAdcResult->powerVolt%1000);

			shutdownProcess = 1;
			
			system("touch /tmp/Shutdown");

			sleep(5);
			
			recPid = getProcessPID("RecorderService", "RecorderService");

			if (recPid > 0)
			{
				PNT_LOGE( "Stop Recorde!\n");
				char cmd[128] = { 0 };
				sprintf(cmd, "kill -2 %d", recPid);
				system(cmd);
				sleep(5);
			}

			DeviceShutdown();
		}
		
		PNT_LOGE("Main power: %d.%dV.", gAdcResult->powerVolt/1000, gAdcResult->powerVolt%1000);

		sleep(2);
	}

	return NULL;
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
				pthread_t pid;
				if (pthread_create(&pid, NULL, ShutdownTask, NULL))
				{
					PNT_LOGE("create ShutdownTask failed.\n");
				}
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

static int gWatchdogFd = 0;
void StartWatchdog(void)
{
	int flags = 0, ret = 0;
	
	gWatchdogFd = open("/dev/watchdog", O_WRONLY);

	if (gWatchdogFd < 0)
	{
		PNT_LOGE("watchdog open failed.");
		return;
	}

	flags = WDIOS_DISABLECARD;
	ret = ioctl(gWatchdogFd, WDIOC_SETOPTIONS, &flags);
	if (ret)
	{
		PNT_LOGE("WDIOS_DISABLECARD errno %s", strerror(errno));
		return;
	}

	flags = 15000;
	ret = ioctl(gWatchdogFd, WDIOC_SETTIMEOUT, &flags);
	if (ret)
	{
		PNT_LOGE("WDIOC_SETTIMEOUT errno %s", strerror(errno));
	}

	flags = WDIOS_ENABLECARD;
	ret = ioctl(gWatchdogFd, WDIOC_SETOPTIONS, &flags);
	if (ret)
	{
		PNT_LOGE("WDIOC_SETOPTIONS errno %s", strerror(errno));
	}
}

void StopWatchdog(void)
{
	int flags = 0, ret = 0;

	if (gWatchdogFd < 0)
	{
		PNT_LOGE("watchdog open failed.");
		return;
	}

	flags = WDIOS_DISABLECARD;
	ret = ioctl(gWatchdogFd, WDIOC_SETOPTIONS, &flags);
	if (ret)
	{
		PNT_LOGE("WDIOS_DISABLECARD errno %s", strerror(errno));
		return;
	}
	
	close(gWatchdogFd);
}

void KeepAlive(void)
{
	int dummy, ret;

	ret = ioctl(gWatchdogFd, WDIOC_KEEPALIVE, &dummy);

	if (ret)
	{
		PNT_LOGE("keep alive failed.");
	}
}

void SigForceStopRecorder(int signo)
{
	PNT_LOGE("force stop RecorderService.");

	char cmd[1024] = { 0 };
	int recPid = -1;
	
	recPid = getProcessPID("RecorderService", "RecorderService");
	
	PNT_LOGE("find PID:%d force to killed.", recPid);
	if (recPid>0)
	{
		sprintf(cmd, "kill -9 %d", recPid);
		system(cmd);
		sleep(1);
		KeepAlive();
		sleep(1);
		KeepAlive();
		sleep(1);
		KeepAlive();
		sleep(1);
		KeepAlive();
		sleep(1);
	}

	if ( 0 == access("/tmp/formatSDCard", F_OK))
	{
		StopWatchdog();
	
		PNT_LOGE("start to format tfcard");
		
		formatAndMountTFCard("/mnt/card", "/dev/mmcblk0p1");

		system("reboot");
	}
}

void RunMainService(void)
{
	if (!access("/data/RecorderService", R_OK))
	{
		system("/data/RecorderService &");
	}
	else
	{
		system("RecorderService &");
	}
}

static PNTIPC_t gLocationIPC = { 0 };
volatile unsigned int gLastMonitorTime = -1;

static int LocationIPCIRcvDataProcess(char* buff, int len, int fd, void* arg)
{
    GpsLocation_t location = { 0 };

	gLastMonitorTime = 0;

	gMainProcessOn = 1;

    if (len >= sizeof(GpsLocation_t))
    {
        return len;
    }
    else
    {
        return 0;
    }

}

void MainMonitorStart(void)
{
    LibPnt_IPCConnect(&gLocationIPC, PNT_IPC_GPS_NAME, LocationIPCIRcvDataProcess, NULL, 256, NULL);
}

int main(int argc, char* argv[])
{
	system("hwclock --hctosys");

    signal(SIGUSR1, SigForceStopRecorder);

	Global_SystemParam_Init();
	
    memset(&gBoardInfo, 0, sizeof(gBoardInfo));
    getBoardSysInfo(&gBoardInfo);
	
	if (0 == strcmp(gBoardInfo.mProductModel, "P-DC-1"))
	{
		gGlobalSysParam->parking_monitor = 0;
	}
	
	pthread_t pid;
	if (pthread_create(&pid, NULL, ShutdownTask, NULL))
	{
		PNT_LOGE("create ShutdownTask failed.\n");
	}

	GpioKey_Init();
	KeyHandle_Register(aacKeyOffHandler, 105);

    upgradeProcessStart();

	adcDetectStart();

	if (0 == strcmp(gBoardInfo.mProductModel, "P-DC-2") || 0 == strcmp(gBoardInfo.mProductModel, "P-DC-3"))
	{
		if (pthread_create(&pid, NULL, MainPowerMonitor, NULL))
		{
			PNT_LOGE("create MainPowerMonitor failed.\n");
		}
	}

	StartWatchdog();

    while (1)
    {
		int recPid = -1;
		
		recPid = getProcessPID("RecorderService", "RecorderService");

		if (recPid > 0)
		{
			break;
		}

		KeepAlive();

		sleep(1);
    }

	MainMonitorStart();
	
    while (1)
    {
		int recPid = -1;

		if (-1 == access("/tmp/formatSDCard", F_OK) && gACCState)
		{
			recPid = getProcessPID("RecorderService", "RecorderService");
			if (recPid < 0)
			{
				PNT_LOGE("RecorderService killed, restart it.");
				sleep(1);
				system("reboot");
			}

			if (-1 != gLastMonitorTime)
			{
				gLastMonitorTime ++;
				if (5 <= gLastMonitorTime)
				{
					int simSpeed = getFlagStatus("simSpeed");
					if (simSpeed > 0)
					{
						int fd = open("/data/simSpeed", O_WRONLY | O_CREAT);
						if (0 < fd)
						{
							write(fd, &simSpeed, sizeof(simSpeed));
							close(fd);
						}
					}
					system("reboot");
				}
			}
		}

		KeepAlive();

		sleep(2);
    }

	return 0;
}
