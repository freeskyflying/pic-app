#ifndef _GPS_RECORDER_H_
#define _GPS_RECORDER_H_

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "media_storage.h"
#include "pnt_ipc.h"
#include "pnt_ipc_gps.h"

#define GPSDATA_PATH					VIDEO_PATH"/GPSDATA"
#define GPSDATA_FILENAME_FORMAT       	GPSDATA_PATH"/%4d%02d%02d-%02d%02d%02d.txt"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{

	int runFlag;
	volatile int recStatus;
	FILE* pfile;

	time_t fileTime;
	PNTIPC_t locationIPC;
	GpsLocation_t location;

	pthread_mutex_t mutex;
	pthread_t pid;

} GpsRecorder_t;


void gps_file_query(void);

void gps_file_get_name(time_t timeval, char* filename);

void GpsRec_SetRecordStatus(bool_t status);

void GpsRec_SetSyncCount(void);

void gps_file_recorder_stop(void);

void gps_file_recorder_init(void);


#ifdef __cplusplus
}
#endif

extern queue_list_t gGpsFileListQueue;

#endif

