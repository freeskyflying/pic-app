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

#include "kwatch.h"
#include "pnt_log.h"
#include "algo_common.h"
#include "gpsReader.h"
#include "utils.h"

static AlarmInfo gAlarmInfo = { 0 };

static int gUartFd = -1;
volatile int gNeedReconnect = 0;

#define BIT_CHECK(a, b) (!!((a) & (1ULL<<(b))))
#define SET_BIT(var, bit)			do { var |= (0x1 << (bit)); } while(0)
#define CLR_BIT(var, bit)			do { var &= (~(0x1 << (bit))); } while(0)

#define CRC8_TABLE_SIZE			256
#define POLY        			0x4d
unsigned char  crc_table[CRC8_TABLE_SIZE];

static void crc8_populate_lsb(unsigned char* table, unsigned char polynomial)
{
    int i, j;
    unsigned char t = 1;
    table[0] = 0;

    for (i = (CRC8_TABLE_SIZE >> 1); i; i >>= 1)
	{
        t = (t >> 1) ^ (t & 1 ? polynomial : 0);
		
        for (j = 0; j < CRC8_TABLE_SIZE; j += 2*i)
            table[i+j] = table[j] ^ t;
    }
}

static unsigned char crc8( unsigned char *pdata, unsigned int nbytes, unsigned char crc)
{
    while (nbytes-- > 0)
	{
        crc = crc_table[(crc ^ *pdata++) & 0xff];
    }

    return crc;
}
static unsigned char crc8creator(unsigned char *msg,unsigned char start,unsigned int len)
{
    unsigned char crc=0;
    crc=crc8(&msg[start],len,0);
    return crc;
}

#if 0

static int detectcrc8(unsigned char *msg,unsigned char start,unsigned int len)
{
    unsigned char crc=crc8creator(msg,start,len-1);
	
    if(crc==msg[len-1])
	{
        return 1;
    }
	
    fprintf(stderr,"crc not right len %d %d %d\r\n",len ,crc,msg[len-1]);
	
    return 0;
}
#endif

static int uartOpen(char* name, int bdr)
{
    int fd = -1;
    
    fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd < 0)
    {
        PNT_LOGE("Open %s uart fail:%s ", name, strerror(errno));
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
	
	/* Data length setting section */
	uart_cfg_opt.c_cflag &= ~CSIZE;
	uart_cfg_opt.c_cflag |= CS8;
	uart_cfg_opt.c_iflag &= ~INPCK;
	uart_cfg_opt.c_cflag &= ~PARODD;
	uart_cfg_opt.c_cflag &= ~CSTOPB;
	
	/* Using raw data mode */
	uart_cfg_opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	uart_cfg_opt.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
	uart_cfg_opt.c_oflag &= ~(INLCR | IGNCR | ICRNL);
	uart_cfg_opt.c_oflag &= ~(ONLCR | OCRNL);
	
	/* Apply new settings */
	if (-1 == tcsetattr(fd, TCSANOW, &uart_cfg_opt))
		return -1;
	
	tcflush(fd, TCIOFLUSH);

    return fd;
}


static int send_aiscreen(AlarmInfo* info)
{
	int len = 0, datLen = 0;
	char buffer[128] = { 0 };

	if (-1 != gUartFd)
	{
		datLen = sizeof(AlarmInfo);
		
		buffer[len++] = 0xA0;
		buffer[len++] = 0xA1;
		buffer[len++] = 1;
		buffer[len++] = 23;
		buffer[len++] = (datLen >>24) & 0xFF;
		buffer[len++] = (datLen >>16) & 0xFF;
		buffer[len++] = (datLen >>8) & 0xFF;
		buffer[len++] = datLen & 0xFF;
		
		memcpy(&buffer[len], info, datLen);
		len+=datLen;
		
	    buffer[len++] = crc8creator((uint8_t *)&buffer[8],0,datLen);
	    buffer[len++] = 0x0D;
	    buffer[len++] = 0x0A;
		
		if (-1 == write(gUartFd, buffer, len))
		{
			gNeedReconnect = 1;
		}
	}

	return 0;
}

static int send_time(void)
{
	uint8_t  data[100];
	int size = 0;

	if (-1 != gUartFd)
	{
		time_t timep;
		struct tm *p;
		time(&timep);
		p = localtime(&timep);
		
		data[size++] = 0xA0;
		data[size++] = 0xA1;
		data[size++] = 1;
		data[size++] = 4;
		data[size++] = 0;
		data[size++] = 0;
		data[size++] = 0;
		data[size++] = 6;

		if (1900 + p->tm_year >= 2019)
		{
			data[size++] = 1900 + p->tm_year - 2000;
			data[size++] = 1 + p->tm_mon;
			data[size++] = p->tm_mday;
			data[size++] = p->tm_hour;
			data[size++] = p->tm_min;
			data[size++] = p->tm_sec;
			
		    data[size++] = crc8creator((uint8_t *)&data[8], 0, 6);
		    data[size++] = 0x0D;
		    data[size++] = 0x0A;
			
			if (-1 == write(gUartFd, data, size))
			{
				gNeedReconnect = 1;
			}
		}
	}

	return 0;
}

int KWatchReportAlarm(int level, int type, int extra, int extra2, float ttc)
{
    gAlarmInfo.fcwWarningValue = 0;
    gAlarmInfo.pcwWarningValue = 0;
    gAlarmInfo.bspWarningValue = 0;
    gAlarmInfo.yuliuwarningValue =0 ;
    gAlarmInfo.leftorright = 0;
    gAlarmInfo.tsValue = 0;
	
    CLR_BIT(gAlarmInfo.warningType, 1);
    CLR_BIT(gAlarmInfo.warningType, 11);
    CLR_BIT(gAlarmInfo.warningType, 7);
    CLR_BIT(gAlarmInfo.warningType, 14);
    CLR_BIT(gAlarmInfo.warningType, 4);
    CLR_BIT(gAlarmInfo.warningType, 15);
    CLR_BIT(gAlarmInfo.warningType, 5);
    CLR_BIT(gAlarmInfo.warningType, 17);
    CLR_BIT(gAlarmInfo.warningType, 10);
    CLR_BIT(gAlarmInfo.warningType, 13);

    CLR_BIT(gAlarmInfo.warningType, 21);
    CLR_BIT(gAlarmInfo.warningType, 23);
    CLR_BIT(gAlarmInfo.warningType, 20);
    CLR_BIT(gAlarmInfo.warningType, 22);
    CLR_BIT(gAlarmInfo.warningType, 6);
    CLR_BIT(gAlarmInfo.warningType, 16);
    CLR_BIT(gAlarmInfo.warningType, 12);
    CLR_BIT(gAlarmInfo.warningType, 19);
    CLR_BIT(gAlarmInfo.warningType, 2);
    CLR_BIT(gAlarmInfo.warningType, 9);
	
	CLR_BIT(gAlarmInfo.warningType, 3);
	CLR_BIT(gAlarmInfo.warningType, 8);
	CLR_BIT(gAlarmInfo.warningType, 18);
	
	switch (type)
	{
		case LB_WARNING_DMS_DRIVER_LEAVE:
			break;
		case LB_WARNING_DMS_CALL:
			{
                if(2 == level)
				{
                    SET_BIT(gAlarmInfo.warningType, 21);
                }
				else
				{
                    SET_BIT(gAlarmInfo.warningType, 23);
                }
			}
			break;
		case LB_WARNING_DMS_SMOKE:
			{
                if(2 == level)
				{
                    SET_BIT(gAlarmInfo.warningType, 20);
                }
				else 
				{
                    SET_BIT(gAlarmInfo.warningType, 22);
                }
			}
			break;
		case LB_WARNING_DMS_DRINK:
		case LB_WARNING_DMS_ATTATION:
			{
                if(2 == level)
				{
                    SET_BIT(gAlarmInfo.warningType, 6);
                }
				else
				{
                    SET_BIT(gAlarmInfo.warningType, 16);
                }
			}
			break;
		case LB_WARNING_DMS_REST:
			{
				if (extra2 > 0) // yawn
				{
	                if(2 == level)
					{
	                    SET_BIT(gAlarmInfo.warningType, 12);
	                }
					else
					{
	                    SET_BIT(gAlarmInfo.warningType, 19);
	                }
				}
				else // eyeclose
				{
	                if(2 == level)
					{
                        SET_BIT(gAlarmInfo.warningType, 2);
                    }
					else
					{
                        SET_BIT(gAlarmInfo.warningType, 9);
                    }
				}
			}
			break;
		case LB_WARNING_DMS_CAMERA_COVER:
			break;
		case LB_WARNING_DMS_INFRARED_BLOCK:
			break;
		case LB_WARNING_DMS_NO_BELT:
			break;
            
		case LB_WARNING_ADAS_LAUNCH:
			break;
		case LB_WARNING_ADAS_DISTANCE:
			{
                if(2 == level)
				{
                    SET_BIT(gAlarmInfo.warningType, 7);
                }
				else
				{
                    SET_BIT(gAlarmInfo.warningType, 14);
                }
                if(ttc > 0 )
				{
                    gAlarmInfo.fcwWarningValue = ttc * 10 >= 50 ? 50 : ttc * 10;
                }
			}
			break;
		case LB_WARNING_ADAS_COLLIDE:
			{
                if(2 == level)
				{
                    SET_BIT(gAlarmInfo.warningType, 1);
                }
				else
				{
                    SET_BIT(gAlarmInfo.warningType, 11);
                }
                if(ttc > 0 )
				{
                    gAlarmInfo.fcwWarningValue = ttc * 10 >= 50 ? 50 : ttc * 10;
                }
			}
			break;
		case LB_WARNING_ADAS_PRESSURE:
			{
                if(2 == level)
				{
                    SET_BIT(gAlarmInfo.warningType, 10);
                }
				else
				{
                    SET_BIT(gAlarmInfo.warningType, 17);
                }
                gAlarmInfo.leftorright =  extra;
			}
			break;
		case LB_WARNING_ADAS_PEDS:
			{
                if(2 == level)
				{
                    SET_BIT(gAlarmInfo.warningType, 4);
                }
				else
				{
                    SET_BIT(gAlarmInfo.warningType, 15);
                }
                if(ttc > 0 )
				{
                    gAlarmInfo.pcwWarningValue = ttc * 10 > 50 ?50:ttc *10;
                }
			}
			break;
		case LB_WARNING_ADAS_CHANGLANE:
			break;

		default:
			return 0;
	}

	gAlarmInfo.alertFlag = 10;

	return 0;
}

void* KwatchUartReadThread(void* arg)
{
	int count = 0;
	char buffer[128] = { 0 };
	char str[512] = { 0 };

	while (1)
	{
		if (gUartFd > 0)
		{
			count = read(gUartFd, buffer, sizeof(buffer));
			if (count > 0)
			{
				memset(str, 0, sizeof(str));
				for (int i=0; i<count; i++)
				{
					sprintf(str, "%s%02X ", str, buffer[i]);
				}
				PNT_LOGI("kwatch recieve: %s\n", str);
			}
		}
		SleepMs(100);
	}
}

void* KWatchUartThread(void* arg)
{
	int flag = 0;
	char kwatchPoint[128] = { 0 };
	
open_uart:

	flag = 0;
	gUartFd = -1;

	for (int i=0; i<25; i++)
	{
		sprintf(kwatchPoint, "/dev/ttyACM%d", i);
		if (0 == access(kwatchPoint, F_OK))
		{
			PNT_LOGI("find kwatch node %s", kwatchPoint);
			flag = 1;
			break;
		}
	}

	if (flag)
	{
	    gUartFd = uartOpen(kwatchPoint, 115200);
	    if (gUartFd < 0)
	    {
	        sleep(2);
	        goto open_uart;
	    }
	}
	else
	{
		sleep(2);
		goto open_uart;
	}

	PNT_LOGI("kwatch start.");

	int count = 0;

	while (1)
	{
		if (-1 == access(kwatchPoint, F_OK) || gNeedReconnect)
		{
			gNeedReconnect = 0;
			gAlarmInfo.alertFlag = 0;
			close(gUartFd);
			sleep(2);
			PNT_LOGE("kwatch error, reconnect.");
			goto open_uart;
		}

		if (gAlarmInfo.alertFlag > 0)
		{
			send_aiscreen(&gAlarmInfo);
			SleepMs(200);
			gAlarmInfo.alertFlag --;
			//PNT_LOGI("kwatch send alarm %d.", gAlarmInfo.alertFlag);
		}
		else
		{
			if (count >= 20)
			{
				send_time();
				count = 0;
				//PNT_LOGI("kwatch send time %d.", gAlarmInfo.alertFlag);
			}
			SleepMs(100);
			count ++;
		}
	}
}

void KWatchInit(void)
{
    pthread_t pid;

	crc8_populate_lsb(crc_table, POLY);

    int ret = pthread_create(&pid, 0, KWatchUartThread, NULL);
    if (ret)
    {
        printf("errno=%d, reason(%s)\n", errno, strerror(errno));
    }
/*
	ret = pthread_create(&pid, 0, KwatchUartReadThread, NULL);
    if (ret)
    {
        printf("errno=%d, reason(%s)\n", errno, strerror(errno));
    }
   */
}

