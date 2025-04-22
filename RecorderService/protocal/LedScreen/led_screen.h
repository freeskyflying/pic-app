#ifndef _LED_SCN_H_
#define _LED_SCN_H_

#include <pthread.h>
#include "pnt_epoll.h"

#define LEDSCN_PARAMS_FILE			"/data/etc/LedScreen.bin"

#define UARTBUFFER_SIZE				4096

typedef struct
{
	
	char mUrl[128];			// LED屏幕平台地址
	int  mPort;				// LED屏幕平台端口
	int  mBaudrate;			// 串口波特率
	char mTermID[20];		// 终端ID
	char mFactory;			// 厂家标识
	
} LedScnParam_t;

/* 终端与LED平台数据类型 */
typedef struct
{
	
	unsigned char head[2];			// 'B''B'
	unsigned short packLen;			// 包长：从保留位开始到校验和的长度， 不包含自身和校验和。
	unsigned char resv[10];
	unsigned char flag;				// 厂家标识
	unsigned char termID[20];		// 终端ID，字符串类型，utf8编码
	unsigned short cmdID;			// 指令标识
	unsigned char time[8];			// 年+月+日+时+分+秒+毫秒（2byte）
	unsigned short dataLen;			// 内容长度
	unsigned char* data;			// 信息内容
	unsigned char crc;				// 校验和： 取包长到校验和之前的数据的异或和运算
	unsigned char tail[2];			// 0x0D 0x0A
	
} LedScnPlatCmd_t;

/* 终端与LED屏数据类型 */
typedef struct
{
	
	unsigned char head;				// 帧头 0x7E
	unsigned char type;				// 类型 0x44、0x43
	unsigned short packLen;			// 包长：从resv到crc + 1
	unsigned char resv[2];
	unsigned char cmd[4];
	unsigned char data[256];
	unsigned short crc;				// crc：参与校验的数据是从第二个字节开始到 crc 校验字节前
	unsigned char resv2[6];			// 0x44类型有，0x43类型无
	unsigned char tail;
	
} LedScnDevCmd_t;

typedef struct
{

	/* 串口对接-Led屏 */
	int uartFd;
    char buffer[UARTBUFFER_SIZE];
    pthread_mutex_t mutex;

	/* UDP对接-Led平台 */
	int udpFd;
    char udpBuffer[UARTBUFFER_SIZE];
    pthread_mutex_t udpMutex;
	uint32_t lastPacketTime;

	LedScnParam_t* param;
	
} LedScnHandle_t;

extern LedScnParam_t* gLedScnParams;

void LedScreenParamInit(void);

void LedScreenParamDeInit(void);

void LedScreenStart(void);

#endif

