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

net4GRild_t gNet4GRild = { 0 };

static int get4GModuleType(void)
{
	char buffer[1024] = { 0 };

	FILE *p = NULL;
	memset(buffer, 0, sizeof(buffer));

	p = popen("lsusb", "r");

	if (NULL == p)
	{
		printf("command failed.\n");
		return -1;
	}
	
	memset(buffer, 0, sizeof(buffer));
	fread(buffer, 1, sizeof(buffer), p);
	pclose(p);
	p = NULL;

	if (strstr(buffer, "1286:4e3c"))
	{
		return Net4GModule_YUGA_CLM920;
	}
	else if (strstr(buffer, "2c7c:0904"))
	{
		return Net4GModule_QUECTEL_800G;
	}
	else if (strstr(buffer, "2c7c:6005"))
	{
		return Net4GModule_QUECTEL_EC200A;
	}
	else
	{
		return Net4GModule_NONE;
	}
}

int net4GuartOpen(char* name, int bdr)
{
    int fd = -1;
    
    fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd < 0)
    {
        RILD_DBG("Open %s uart fail:%s ", name, strerror(errno));
        return -1;
    }

	struct termios uart_cfg_opt;
	speed_t speed;
	
	if (-1 == tcgetattr(fd, &uart_cfg_opt))
		return -1;
	//Set speed.
	tcflush(fd, TCIOFLUSH);
	speed = bdr==115200?B115200:B9600;
	if (CBAUDEX != speed)
	{
		cfsetospeed(&uart_cfg_opt, speed);
		cfsetispeed(&uart_cfg_opt, speed);
	}
	if (-1 == tcsetattr(fd, TCSANOW, &uart_cfg_opt))
		return -1;
	
	tcflush(fd, TCIOFLUSH);

    uart_cfg_opt.c_cc[VTIME] = 30;
    uart_cfg_opt.c_cc[VMIN] = 0;

    uart_cfg_opt.c_cflag|=(CLOCAL|CREAD );

	uart_cfg_opt.c_cflag = B115200;

    uart_cfg_opt.c_cflag &= ~CSIZE;
    uart_cfg_opt.c_cflag |= CS8;

    uart_cfg_opt.c_cflag &= ~PARENB;

    uart_cfg_opt.c_cflag &= ~CSTOPB;

	/* Using raw data mode */
	uart_cfg_opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	uart_cfg_opt.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
	uart_cfg_opt.c_oflag &= ~(INLCR | IGNCR | ICRNL);
	uart_cfg_opt.c_oflag &= ~(ONLCR | OCRNL);

	int err = tcsetattr(fd, TCSANOW, &uart_cfg_opt);
	if(err == -1 || err == EINTR)
    {
		RILD_DBG("fialed to set attr to fd\n");
		return -1;
	}

    return fd;
}

void net4GUartBufferParser(net4GRild_t* rild, char* buffer, int* bufferLen)
{
    int i = 0, start = 0, cmdLen = 0, passed = 0;
    int len = *bufferLen;
    char atCmd[128] = { 0 };

    while (i<len)
    {
        if (/*'\r' == buffer[i] && */'\n' == buffer[i])
        {
        	if ('\r' == buffer[i-1])
        	{
        		cmdLen -= 1;
        	}
            if (cmdLen >= 2)
            {
                memset(atCmd, 0, sizeof(atCmd));
                memcpy(atCmd, buffer+start, (cmdLen>(sizeof(atCmd)-1))?(sizeof(atCmd)-1):(cmdLen));
				//PNT_LOGI("start:%d cmdLen:%d buffer:%s AT: %s", start, cmdLen, buffer, atCmd);
                rild->atCommandParser(rild, atCmd);
            }

            passed += (cmdLen+2);
            cmdLen = 0;
           	i += 1;
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
                break;
            }
            else
            {
                pthread_mutex_lock(&rild->mutex);
                rild->size += ret;

                net4GUartBufferParser(rild, rild->buffer, &rild->size);

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

	int udhcpcPid = getProcessPID("/sbin/udhcpc", "");

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
	net4GRild_t* rild = (net4GRild_t*)arg;

	while (1)
	{
		if (rild->fd >= 0)
		{
			if (!rild->netState)
			{
				sleep(1);
	    		setLedStatus(NET4G_LED, 0);
			}
			sleep(1);
			setLedStatus(NET4G_LED, 1);
		}
		else
		{
			sleep(1);
			setLedStatus(NET4G_LED, 0);
		}
	}

	return NULL;
}

static void* net4GFindThread(void* arg)
{
	net4GRild_t* rild = (net4GRild_t*)arg;
	int type = Net4GModule_NONE;
	int count = 0;
	
	while (1)
	{
		type = get4GModuleType();
		if (type != Net4GModule_NONE)
		{
			PNT_LOGE("find 4g module type %d.", type);
			break;
		}

		if (count == 4)
		{
			setLedStatus("/sys/class/leds/4gpwr", 0);
			sleep(1);
			setLedStatus("/sys/class/leds/4gpwr", 1);
		}
		
		if (count++ == 10)
		{
			playAudio("4g_abnormal.pcm");
		}
		
		sleep(1);
	}

	if (Net4GModule_QUECTEL_800G == type)
	{
		rild->rildThread = net4G_Quectel800G_RildThread;
		rild->atCommandParser = net4G_Quectel800G_atCommandParser;
	}
	else if (Net4GModule_YUGA_CLM920 == type)
	{
		rild->rildThread = net4G_YugaCML920_RildThread;
		rild->atCommandParser = net4G_YugaCML920_atCommandParser;
	}
	else if (Net4GModule_QUECTEL_EC200A == type)
	{
		rild->rildThread = net4G_QuectelEC200A_RildThread;
		rild->atCommandParser = net4G_QuectelEC200A_atCommandParser;
	}

	rild->rildThread(rild);

	return NULL;
}

int net4gRildStart(void)
{
    pthread_t pid;

    memset(&gNet4GRild, 0, sizeof(gNet4GRild));

	gNet4GRild.fd = -1;

    int ret = pthread_create(&pid, 0, net4GFindThread, &gNet4GRild);
    if (ret)
    {
        printf("errno=%d, reason(%s)\n", errno, strerror(errno));
    }

    pthread_create(&pid, 0, net4GRildLedStatus, &gNet4GRild);

    return 0;
}

int net4gRildStop(void)
{
	char cmd[128] = { 0 };

	int udhcpcPid = getProcessPID("/sbin/udhcpc", "");

	if (-1 != udhcpcPid)
	{
		sprintf(cmd, "kill -9 %d", udhcpcPid);
		system(cmd);
	}

	return 0;
}

