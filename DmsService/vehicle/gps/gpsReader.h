#ifndef _GPSREADER_H_
#define _GPSREADER_H_

#include <pthread.h>
#include "pnt_epoll.h"
#include "pnt_ipc.h"

#define READBUFFER_SIZE     2048

#define GPS_MINUTE_TO_DEC(x)  ((((x) / 100.0 - (int)(x) / 100) * 100.0 / 60.0 + (int)(x) / 100))
#define GPS_SPEED_KNOT_TO_KM(x)  ((x) * 1.852f)

typedef struct
{
    
    int fd;
    int size;
    char buffer[READBUFFER_SIZE];
    PNTEpoll_t epoll;
    pthread_mutex_t mutex;

    PNTIPC_t ipcServer;

    int validFlag;
    int timeCaliFlag;
    int abnormal;

} gpsReader_t;


int gpsReaderStart(void);

#endif
