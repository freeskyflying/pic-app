#ifndef _MEDIA_RECORD_SNAP_H_
#define _MEDIA_RECORD_SNAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include "media_video.h"
#include "media_utils.h"
#include "queue_list.h"

typedef struct
{

	unsigned int snapTime; // ms
		
	int width;
	int height;
	char filename[64];

} SnapRequest_t;

typedef struct
{

	int vissDev;
	int vissChn;
	int vencChn;
	int width;
	int height;

	int bThreadFlag;
	pthread_t pid;
	queue_list_t reqList;

} SnapController_t;

int MediaSnap_Init(void);

int MediaSnap_CreateSnap(int chn, int count, int width, int height, int interval, char* arg);

void MediaSnap_RenameBySequence(char* outname, char* orgname, int total, int seq);

#ifdef __cplusplus
}
#endif

#endif

