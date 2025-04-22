#ifndef _RILD_H_
#define _RILD_H_

#include "pnt_epoll.h"

#define ATREADBUFFER_SIZE     2048

typedef void*(*net4GRildThread)(void*);
typedef void(*net4GatCommandParser)(void*, char*);

typedef enum
{

	Net4GModule_NONE,
	Net4GModule_QUECTEL_800G,
	Net4GModule_YUGA_CLM920,
	Net4GModule_QUECTEL_EC200A,

} Net4GModuleType_e;

typedef enum
{

	CallType_RNDIS,
	CallType_ECM,

} Net4GCallType_e;

typedef struct
{
    
    int fd;
    int size;
    char buffer[ATREADBUFFER_SIZE];
    PNTEpoll_t epoll;
    pthread_mutex_t mutex;
    volatile int simState;
    volatile int netState;
    int signalStrength;

	unsigned int timeCalibFlag;

	char CCID[10];
	char IMEI[32];

	unsigned int lastATAckTime;

	net4GRildThread rildThread;
	net4GatCommandParser atCommandParser;

} net4GRild_t;

extern net4GRild_t gNet4GRild;

int net4GuartOpen(char* name, int bdr);

int net4GUartOnEpollEvent(int fd, unsigned int event, void* arg);

void net4GReboot(void);

void* net4GRildLedStatus(void* arg);

void net4GReboot(void);

int net4gRildStart(void);

int net4gRildStop(void);

void* net4G_Quectel800G_RildThread(void* arg);

void net4G_Quectel800G_atCommandParser(void* arg, char* atCmd);

void* net4G_YugaCML920_RildThread(void* arg);

void net4G_YugaCML920_atCommandParser(void* arg, char* atCmd);

void* net4G_QuectelEC200A_RildThread(void* arg);

void net4G_QuectelEC200A_atCommandParser(void* arg, char* atCmd);

#endif

