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
#include "nmea.h"
#include "utils.h"
#include "gpio_led.h"

#include "pnt_ipc_4g.h"
#include "rild.h"
#include "pnt_log.h"

#define RILD_DBG        //printf

net4GRild_t gNet4GRild = { 0 };

static int uartOpen(char* name, int bdr)
{
    int fd = -1;
    
    fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd < 0)
    {
        RILD_DBG("Open %s uart fail:%s ", name, strerror(errno));
        return -1;
    }

    struct termios uart_cfg_opt;

	if (tcgetattr(fd, &uart_cfg_opt) == -1)
    {
		RILD_DBG("error cannot get input description\n");
		return -1;
	}

    uart_cfg_opt.c_cc[VTIME] = 30;
    uart_cfg_opt.c_cc[VMIN] = 0;

    uart_cfg_opt.c_cflag|=(CLOCAL|CREAD ); /*CREAD 开启串行数据接收，CLOCAL并打开本地连接模式*/

	uart_cfg_opt.c_cflag = B115200;

    uart_cfg_opt.c_cflag &= ~CSIZE; /* 8bit数据 */
    uart_cfg_opt.c_cflag |= CS8;

    uart_cfg_opt.c_cflag &= ~PARENB; /* 无校验 */

    uart_cfg_opt.c_cflag &= ~CSTOPB; /* 1停止位 */

    tcflush(fd ,TCIFLUSH); /*tcflush清空终端未完成的输入/输出请求及数据；TCIFLUSH表示清空正收到的数据，且不读取出来 */

	int err = tcsetattr(fd, TCSANOW, &uart_cfg_opt);
	if(err == -1 || err == EINTR)
    {
		RILD_DBG("fialed to set attr to fd\n");
		return -1;
	}

    return fd;
}

void atCommandParser(char* atCmd)
{
    if(!strncmp("+QSIMSTAT: ", atCmd, 11))
    {
    	int temp = atCmd[13] - '0';
		if (temp != gNet4GRild.simState)
		{
			if (0 == temp)
			{
				playAudio("simcard_out.pcm");
			}
        	gNet4GRild.simState = temp;
		}
    }
    else if (!strncmp("+CSQ: ", atCmd, 6))
    {
        const char* str = strtok((char*)atCmd+6, ",");
        if(str == NULL)
        {
            return;
        }
        gNet4GRild.signalStrength = atoi(str);
        //N4GInformation_t info = { 0 };
        //info.type = 0;
        //info.signalStrength = gNet4GRild.signalStrength;
        //LibPnt_IPCSend(&gNet4GRild.ipcServer, (char*)&info, sizeof(info), 0);
		gNet4GRild.lastATAckTime = currentTimeMillis()/1000;
    }
	else if (!strncmp("+QNETDEVSTATUS: ", atCmd, 16))
	{
    	int temp = atCmd[16] - '0';
		if (temp != gNet4GRild.netState)
		{
        	gNet4GRild.netState = temp;
		}
	}
}

void net4GUartBufferParser(char* buffer, int* bufferLen)
{
    int i = 0, start = 0, cmdLen = 0, passed = 0;
    int len = *bufferLen;
    char atCmd[128] = { 0 };

    while (i<len)
    {
        if ('\r' == buffer[i] && '\n' == buffer[i+1])
        {
            if (cmdLen >= 2)
            {
                memset(atCmd, 0, sizeof(atCmd));
                memcpy(atCmd, buffer+start, cmdLen);
                //printf("cmd: %s ***\r\n", atCmd);
                atCommandParser(atCmd);
            }

            passed += (cmdLen+2);
            cmdLen = 0;
            i += 2;
            start = i;
            continue;
        }
        else
        {
            cmdLen ++;
        }

        i++;
    }

    if (passed < len)
    {
        memcpy(buffer, buffer + passed, len-passed);
        *bufferLen = len-passed;
    }
    else
    {
        memset(buffer, 0, len);
        *bufferLen = 0;
    }
}

int net4GUartOnEpollEvent(int fd, unsigned int event, void* arg)
{
    PNTEpoll_t* epoll = (PNTEpoll_t*)arg;
    net4GRild_t* rild = (net4GRild_t*)epoll->mOwner;

    if (event & EPOLLIN)
    {
        for (;;)
        {
            ssize_t ret = read(fd, rild->buffer + rild->size, ATREADBUFFER_SIZE - rild->size - 1);
            if (ret == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                printf("Uart Client close. %s", strerror(errno));
                break;
            }
            else if (ret == 0)
            {
                //printf("Uart Client close. %s", strerror(errno));
                break;
            }
            else
            {
                pthread_mutex_lock(&rild->mutex);
                rild->size += ret;

                net4GUartBufferParser(rild->buffer, &rild->size);

                if (rild->size >= ATREADBUFFER_SIZE)
                {
                    memset(rild->buffer, 0, ATREADBUFFER_SIZE);
                    rild->size = 0;
                }
                pthread_mutex_unlock(&rild->mutex);
            }
        }
    }

    return RET_SUCCESS;
}


void net4GReboot(void)
{
	char cmd[128] = { 0 };

	int udhcpcPid = getProcessPID("udhcpc -i usb0", "udhcpc -i usb0");

	if (-1 != udhcpcPid)
	{
		sprintf(cmd, "kill -9 %d", udhcpcPid);
		system(cmd);
		sleep(2);
	}
}
void* net4GRildLedStatus(void* arg)
{
	pthread_detach(pthread_self());

	while (gNet4GRild.fd >= 0)
	{
		if (gNet4GRild.netState)
		{
		}
		else
		{
       	 	SleepMs(1000);
    		setLedStatus(NET4G_LED, 0);
		}
   	 	SleepMs(1000);
		setLedStatus(NET4G_LED, 1);
	}

	return NULL;
}

void* net4GRildThread(void* arg)
{
    int fd = -1;
    int retry = 10;

open_uart:
	net4GReboot();
    setLedStatus(NET4G_LED, 0);
    fd = uartOpen("/dev/ttyUSB0", 115200);
    if (fd < 0)
    {
        SleepMs(1000);
        retry --;
        if (retry == 0)
        {
        	if (1 != getFlagStatus("restart"))
        	{
				playAudio("4g_abnormal.pcm");
        	}
            printf("fatal error !!!!!!!!!!!!! open /dev/ttyUSB0 failed.\n");
            while (1)
            {
                SleepMs(2000);
			    fd = uartOpen("/dev/ttyUSB0", 115200);
			    if (fd >= 0)
			    {
			    	break;
			    }
            }
        }
        goto open_uart;
    }

    pthread_t pid;
    int ret = pthread_create(&pid, 0, net4GRildLedStatus, NULL);

    gNet4GRild.fd = fd;
    gNet4GRild.size = 0;

    pthread_mutex_init(&gNet4GRild.mutex, NULL);

    LibPnt_EpollCreate(&gNet4GRild.epoll, 10, &gNet4GRild);
    LibPnt_EpollSetTimeout(&gNet4GRild.epoll, 100);
    LibPnt_EpollAddEvent(&gNet4GRild.epoll, gNet4GRild.fd, EPOLLIN | EPOLLET);
    LibPnt_EpollSetOnEvent(&gNet4GRild.epoll, net4GUartOnEpollEvent);
    LibPnt_EpollSetState(&gNet4GRild.epoll, TRUE);
    
    system("ifconfig usb0 up");

    const char* disableEcho = "ATE0\r\n";
    write(fd, disableEcho, strlen(disableEcho));
    SleepMs(10);

    const char* simDetCfg = "AT+QSIMDET=1,0;\r\n";
    write(fd, simDetCfg, strlen(simDetCfg));
    SleepMs(10);

    const char* qNetConnCfg= "AT+QNETDEVCTL=3,1,1;\r\n";
    write(fd, qNetConnCfg, strlen(qNetConnCfg));
    SleepMs(10);

    const char* simDetNotify = "AT+QSIMSTAT=1;\r\n";
    write(fd, simDetNotify, strlen(simDetNotify));
    SleepMs(10);

    // query simCard status
    const char* qSimState= "AT+QSIMSTAT?\r\n";
    write(fd, qSimState, strlen(qSimState));
    SleepMs(10);

	int tips = 0;
    while (!gNet4GRild.simState)
    {
        SleepMs(2000);
		if (!gNet4GRild.simState && 0 == tips)
		{
			tips = 1;
        	if (1 != getFlagStatus("restart"))
        	{
				playAudio("no_simcard.pcm");
        	}
		}
    }
	
    system("udhcpc -i usb0");

	gNet4GRild.netState = 1;

//    LibPnt_IPCCreate(&gNet4GRild.ipcServer, PNT_IPC_4G_NAME, NULL, NULL, &gNet4GRild);

    const char* signalQuery = "AT+CSQ\r\n";
    while (1)
    {
        write(fd, signalQuery, strlen(signalQuery));
        SleepMs(1000);
		if (currentTimeMillis()/1000 - gNet4GRild.lastATAckTime > 4)
		{
			PNT_LOGE("4G module error.");
    		LibPnt_EpollDelEvent(&gNet4GRild.epoll, gNet4GRild.fd, EPOLLIN | EPOLLET);
			LibPnt_EpollClose(&gNet4GRild.epoll);
			close(gNet4GRild.fd);
			gNet4GRild.fd = -1;
			sleep(1);
			goto open_uart;
		}
    }

    return NULL;
}

int net4gRildStart(void)
{
    pthread_t pid;

    memset(&gNet4GRild, 0, sizeof(gNet4GRild));

    int ret = pthread_create(&pid, 0, net4GRildThread, NULL);
    if (ret)
    {
        printf("errno=%d, reason(%s)\n", errno, strerror(errno));
    }

    return 0;
}
