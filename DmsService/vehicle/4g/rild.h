#ifndef _RILD_H_
#define _RILD_H_

#define ATREADBUFFER_SIZE     2048

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

	unsigned int lastATAckTime;
    
    PNTIPC_t ipcServer;

} net4GRild_t;

extern net4GRild_t gNet4GRild;

void net4GReboot(void);

int net4gRildStart(void);

#endif
