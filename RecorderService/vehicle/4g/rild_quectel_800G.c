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

#define RILD_DBG        //printf

void net4G_Quectel800G_atCommandParser(void* arg, char* atCmd)
{
	net4GRild_t* rild = (net4GRild_t*)arg;
	
	rild->lastATAckTime = currentTimeMillis()/1000;

    if(!strncmp("+QSIMSTAT: ", atCmd, 11))
    {
    	int temp = atCmd[13] - '0';
		if (temp != rild->simState)
		{
			if (0 == temp)
			{
				playAudio("simcard_out.pcm");
			}
        	rild->simState = temp;
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
	else if (atCmd[0] >= '0' && atCmd[0] <= '9')//(!strncmp("+GSN: ", atCmd, 6))
	{
		PNT_LOGI("get imei: %s", atCmd);
		strncpy(rild->IMEI, atCmd, sizeof(rild->IMEI));
	}
}

void* net4G_Quectel800G_RildThread(void* arg)
{
    int fd = -1;
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

open_uart:

	usbNode = 0;
	for (int i=0; i<10; i++)
	{
		sprintf(netNode, "/dev/ttyUSB%d", i);
		if (0 == access(netNode, F_OK))
		{
			usbNode ++;
			if (1 == usbNode)
			{
				break;
			}
		}
	}
	if (1 != usbNode)
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

	if (fd < 0)
	{
		goto open_uart;
	}

    rild->fd = fd;
    rild->size = 0;

    LibPnt_EpollCreate(&rild->epoll, 10, rild);
    LibPnt_EpollSetTimeout(&rild->epoll, 100);
    LibPnt_EpollAddEvent(&rild->epoll, rild->fd, EPOLLIN | EPOLLET);
    LibPnt_EpollSetOnEvent(&rild->epoll, net4GUartOnEpollEvent);
    LibPnt_EpollSetState(&rild->epoll, TRUE);
    
    system("ifconfig usb0 up");

    const char* disableEcho = "ATE0\r\n";
    write(fd, disableEcho, strlen(disableEcho));
	usleep(10*1000);

    const char* simDetCfg = "AT+QSIMDET=1,0;\r\n";
    write(fd, simDetCfg, strlen(simDetCfg));
	usleep(10*1000);

    const char* qNetConnCfg= "AT+QNETDEVCTL=3,1,1;\r\n";
    write(fd, qNetConnCfg, strlen(qNetConnCfg));
	usleep(10*1000);

    const char* simDetNotify = "AT+QSIMSTAT=1;\r\n";
    write(fd, simDetNotify, strlen(simDetNotify));
	usleep(10*1000);

    const char* imeiQuery = "AT+GSN\r\n";
    write(fd, imeiQuery, strlen(imeiQuery));
	usleep(10*1000);

    // query simCard status
    const char* qSimState= "AT+QSIMSTAT?\r\n";
    write(fd, qSimState, strlen(qSimState));
	usleep(10*1000);

	int tips = 0;
	
    const char* signalQuery = "AT+CSQ\r\n";
    while (!rild->simState)
    {
        write(fd, signalQuery, strlen(signalQuery));
        SleepMs(2000);
		if (!rild->simState && 0 == tips)
		{
			tips = 1;
        	if (1 != getFlagStatus("restart"))
        	{
				playAudio("no_simcard.pcm");
        	}
		}
		if (currentTimeMillis()/1000 - rild->lastATAckTime > 4)
		{
			PNT_LOGE("4G module error.");
			goto restart;
		}
    }

    const char* simQueryCCID = "AT+QCCID\r\n";
    write(fd, simQueryCCID, strlen(simQueryCCID));
	usleep(10*1000);
	
    system("/sbin/udhcpc -i usb0");

	rild->netState = 1;

    while (1)
    {
        write(fd, signalQuery, strlen(signalQuery));
		sleep(1);
		if (currentTimeMillis()/1000 - rild->lastATAckTime > 4)
		{
			PNT_LOGE("4G module error.");
			goto restart;
		}
    }

    return NULL;
}

