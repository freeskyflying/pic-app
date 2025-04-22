#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <termio.h>
#include <pthread.h>

#include "pnt_epoll.h"
#include "pnt_ipc.h"
#include "utils.h"
#include "gpio_led.h"

#include "pnt_ipc_4g.h"
#include "rild.h"
#include "pnt_log.h"
#include "gpsReader.h"
#include "system_param.h"

#define RILD_DBG        //printf

void net4G_YugaCML920_atCommandParser(void* arg, char* atCmd)
{
	net4GRild_t* rild = (net4GRild_t*)arg;
	
	rild->lastATAckTime = currentTimeMillis()/1000;

	PNT_LOGI("rild: %s.", atCmd);

    if(!strncmp("+CPIN: ", atCmd, 7))
    {
    	int state = 0;
		if (!strncmp(&atCmd[7], "READY", 5))
		{
			state = 1;
		}
		
		if (state != rild->simState)
		{
			if (0 == state)
			{
				playAudio("simcard_out.pcm");
			}
        	rild->simState = state;
		}
    }
    else if (!strncmp("+CSQ: ", atCmd, 6))
    {
        const char* str = strtok((char*)atCmd+6, ",");
        if(str == NULL)
        {
            return;
        }
        rild->signalStrength = atoi(str);
    }
	else if (!strncmp("+QNETDEVSTATUS: ", atCmd, 16))
	{
    	int temp = atCmd[16] - '0';
		if (temp != rild->netState)
		{
        	rild->netState = temp;
		}
	}
	else if (!strncmp("+QCCID: ", atCmd, 8))
	{
    	const char* ccid = &atCmd[8];
		PNT_LOGI("get ccid: %s", ccid);
		for (int i=0; i<sizeof(rild->CCID); i++)
		{
			rild->CCID[i] = (((ccid[i*2+0]-'0')<<4)&0xF0) + ((ccid[i*2+1]-'0')&0x0F);
		}
	}
	else if (!strncmp("+CCLK: ", atCmd, 7))
	{
		//+CCLK: "19/03/15,11:04:25+32"
		const char* str = &atCmd[8];
		PNT_LOGI("4g-now time: %s", str);
		
		if (strlen(str) >= 18)
		{
			unsigned char bcdTime[6] = { 0 };
			bcdTime[0] = (str[0]-'0')*16 + (str[1]-'0');
			bcdTime[1] = (str[3]-'0')*16 + (str[4]-'0');
			bcdTime[2] = (str[6]-'0')*16 + (str[7]-'0');
			bcdTime[3] = (str[9]-'0')*16 + (str[10]-'0');
			bcdTime[4] = (str[12]-'0')*16 + (str[13]-'0');
			bcdTime[5] = (str[15]-'0')*16 + (str[16]-'0');

			if (bcdTime[0] == 0x70 && bcdTime[1] == 0x01)
			{
			}
			else
			{
				time_t timenow = convertBCDToSysTime(bcdTime);
				
				struct timeval tv;
				tv.tv_sec = timenow + gTimezoneOffset;
				tv.tv_usec = 0;
				settimeofday(&tv, NULL);
				system("hwclock -w");

				rild->timeCalibFlag = 1;
			}
		}
	}
	else if (atCmd[0] >= '0' && atCmd[0] <= '9')//(!strncmp("+GSN: ", atCmd, 6))
	{
		PNT_LOGI("get imei: %s", atCmd);
		strncpy(rild->IMEI, atCmd, sizeof(rild->IMEI)-1);
	}
}

static void net4G_SettingAPN(int fd)
{
	char cmd[512] = { 0 };

	if (strlen((char*)gGlobalSysParam->APN) > 0)
	{
		sprintf(cmd, "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",1\r\n", gGlobalSysParam->APN, gGlobalSysParam->apnUser, gGlobalSysParam->apnPswd);
		PNT_LOGI("set APN command: %s", cmd);
	    write(fd, cmd, strlen(cmd));

		sprintf(cmd, "AT*CGDFLT=1,\"IPV4V6\",\"%s\"\r\n", gGlobalSysParam->APN);
		PNT_LOGI("set APN command: %s", cmd);
	    write(fd, cmd, strlen(cmd));

		if (strlen((char*)gGlobalSysParam->apnUser) > 0)
		{
			sprintf(cmd, "AT*CGDFAUTH=1,2,\"%s\",\"%s\"\r\n", gGlobalSysParam->apnUser, gGlobalSysParam->apnPswd);
			PNT_LOGI("set APN command: %s", cmd);
	   	 	write(fd, cmd, strlen(cmd));
		}
		
	    //write(fd, cmd, strlen(cmd));
		usleep(60*1000);
	}
}

void* net4G_YugaCML920_RildThread(void* arg)
{
    int fd = -1, initFist = 0;
    int retry = 0, usbNode = 0;
	net4GRild_t* rild = (net4GRild_t*)arg;
	char netNode[32] = { 0 };

    pthread_mutex_init(&rild->mutex, NULL);

	rild->fd = -1;

restart:

	if (rild->fd >= 0)
	{
		LibPnt_EpollDelEvent(&rild->epoll, rild->fd, EPOLLIN | EPOLLET);
		LibPnt_EpollClose(&rild->epoll);
		close(rild->fd);
		rild->fd = -1;

		setLedStatus("/sys/class/leds/4gpwr", 0);
		sleep(1);
		setLedStatus("/sys/class/leds/4gpwr", 1);
		sleep(1);
	}
	net4GReboot();
    setLedStatus(NET4G_LED, 0);

open_uart:

	usbNode = 0;
	for (int i=0; i<10; i++)
	{
		sprintf(netNode, "/dev/ttyUSB%d", i);
		if (0 == access(netNode, F_OK))
		{
			usbNode ++;
			if (2 == usbNode)
			{
				break;
			}
		}
	}
	if (2 != usbNode)
	{
   	 	sleep(1);
        retry ++;
        if (retry == 10)
        {
        	if (1 != getFlagStatus("restart"))
        	{
				playAudio("4g_abnormal.pcm");
        	}
			
			setLedStatus("/sys/class/leds/4gpwr", 0);
			sleep(1);
			setLedStatus("/sys/class/leds/4gpwr", 1);
			sleep(1);
			
            PNT_LOGE("fatal error find 4Gmodule failed.\n");
        }
        goto open_uart;
	}
	
	retry = 0;

	fd = net4GuartOpen(netNode, 115200);
		
	if (!initFist)
	{
	    const char* simDetCfg = "AT+QSIMDET=1,0;\r\n";
	    write(fd, simDetCfg, strlen(simDetCfg));
	    usleep(60*1000);
	    const char* moduleReset = "AT+CFUN=1,1\r\n";
	    write(fd, moduleReset, strlen(moduleReset));
	    usleep(60*1000);
		close(fd);
		initFist = 1;
		goto open_uart;
	}

    rild->fd = fd;
    rild->size = 0;

    LibPnt_EpollCreate(&rild->epoll, 10, rild);
    LibPnt_EpollSetTimeout(&rild->epoll, 100);
    LibPnt_EpollAddEvent(&rild->epoll, rild->fd, EPOLLIN | EPOLLET);
    LibPnt_EpollSetOnEvent(&rild->epoll, net4GUartOnEpollEvent);
    LibPnt_EpollSetState(&rild->epoll, TRUE);
    
    system("ifconfig eth0 up");

    const char* disableEcho = "ATE0\r\n";
    write(fd, disableEcho, strlen(disableEcho));
	usleep(60*1000);

	net4G_SettingAPN(fd);

    const char* qRndisConnCfg= "AT+RNDISCALL=1\r\n";
    write(fd, qRndisConnCfg, strlen(qRndisConnCfg));
	usleep(60*1000);

    const char* qNetConnCfg= "AT+QNETDEVCTL=3,1,1;\r\n";
    write(fd, qNetConnCfg, strlen(qNetConnCfg));
	usleep(60*1000);

    const char* qSimState= "AT+CPIN?\r\n";
    write(fd, qSimState, strlen(qSimState));
	usleep(60*1000);

    const char* imeiQuery = "AT+GSN\r\n";
    write(fd, imeiQuery, strlen(imeiQuery));
	usleep(60*1000);

	int tips = 0, cnt = 0;
	
    const char* signalQuery = "AT+CSQ\r\n";
    while (!rild->simState)
    {
        write(fd, signalQuery, strlen(signalQuery));
        sleep(1);
		if (!rild->simState && 0 == tips && cnt >= 10)
		{
			tips = 1;
        	if (1 != getFlagStatus("restart"))
        	{
				playAudio("no_simcard.pcm");
        	}
		}
		cnt ++;
		if (currentTimeMillis()/1000 - rild->lastATAckTime > 4)
		{
			PNT_LOGE("4G module error.");
			goto restart;
		}
    }

    const char* simQueryCCID = "AT+QCCID\r\n";
    write(fd, simQueryCCID, strlen(simQueryCCID));
	usleep(60*1000);
	
    system("/sbin/udhcpc -i eth0 &");

	rild->netState = 1;

    while (1)
    {
        write(fd, signalQuery, strlen(signalQuery));
        sleep(3);
		if (currentTimeMillis()/1000 - rild->lastATAckTime > 4)
		{
			PNT_LOGE("4G module error.");
			goto restart;
		}

		if (!gGpsReader.timeCaliFlag && !rild->timeCalibFlag)
		{
		    const char* qeuryTime = "AT+CCLK?\r\n";
		    write(fd, qeuryTime, strlen(qeuryTime));
        	sleep(1);
		}
    }

    return NULL;
}


