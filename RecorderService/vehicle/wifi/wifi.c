#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include "pnt_log.h"
#include "utils.h"
#include "key.h"
#include "wifi.h"
#include "controller.h"

volatile unsigned int gWifiEnable = 1;
volatile unsigned int gWifiMode = 2; // 0-station 1-ap 2-none

int checkWifiInterface(void)
{
    int exist = 0;
    FILE* fp=NULL;
    int BUFSZ=100;
    char buf[BUFSZ];
    char command[150];

    sprintf(command, "ifconfig -a | grep wlan0");

    if((fp = popen(command,"r")) == NULL)
    {
        return 0;
    }

    if( (fgets(buf,BUFSZ,fp))!= NULL )
    {
        exist = 1;
    }

    pclose(fp);

    return exist;
}

void* wifiTask(void* arg)
{
	int lastMode = gWifiMode;
	
    while (1)
    {
        if (checkWifiInterface())
        {
            break;
        }
		sleep(1);
    }

#if (defined WIFI_STATION)
	gWifiMode = 0; // 默认station模式
	HttpContorller_Start();
#endif

    while (gWifiEnable)
    {
    	if (lastMode != gWifiMode)
    	{
			if (0 == gWifiMode)
			{
				WifiApReboot(0);
				WifiStationReboot(1);
			}
			else if (1 == gWifiMode)
			{
				WifiStationReboot(0);
				WifiApReboot(1);
			}
			lastMode = gWifiMode;
    	}
        sleep(1);
    }
	
	if (0 == gWifiMode)
	{
		WifiStationReboot(0);
	}
	else if (1 == gWifiMode)
	{
		WifiApReboot(0);
	}

    return NULL;
}

int wifiKeyOffHandler(struct input_event* dev)
{
	if (1 == dev->type && 103 == dev->code)
	{
#if (defined WIFI_STATION)
		static uint32_t lastKeyTime = 0;

		if (0 == dev->value)
		{
			lastKeyTime = getUptime();
		}
		else
		{
			if (getUptime() - lastKeyTime >= 3 && 0 != lastKeyTime)
			{
				if (1 == gWifiMode)
				{
					playAudio("station_didi.pcm");
					gWifiMode = 0;
				}
				else
				{
					playAudio("hostap_dididi.pcm");
					gWifiMode = 1;
				}
			}
		}
#else
		if (0 == dev->value)
		{
			if (2 == gWifiMode)
			{
				playAudio("wifi_start.pcm");

				HttpContorller_Start();
			}
			gWifiMode = 1;
		}
#endif
	}
	return 0;
}

void wifiStart(void)
{
    pthread_t pid;
	
	system("insmod /lib/modules/4.19.73/8189fs.ko");

	KeyHandle_Register(wifiKeyOffHandler, 103);

    int ret = pthread_create(&pid, 0, wifiTask, NULL);
    if (ret)
    {
        PNT_LOGE("errno=%d, reason(%s)\n", errno, strerror(errno));
    }
}

void wifiStop(void)
{
	if (0 == gWifiMode)
	{
		WifiStationReboot(0);
	}
	else if (1 == gWifiMode)
	{
		WifiApReboot(0);
	}
}

