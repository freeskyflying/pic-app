#ifndef _MEDIA_CACHE_H_
#define _MEDIA_CACHE_H_


#include <pthread.h>
#include "media_storage.h"

#define CHCHE_FRAME_SECOND				(20)
#define CACHE_FRAME_COUNT				(SDR_VENC_FRAMERATE_MAJOR*CHCHE_FRAME_SECOND)

#define CACHE_VIDEO_FORMAT				VIDEO_PATH"/cache%d.h264"
#define CACHE_AUDIO_FORMAT				VIDEO_PATH"/cache%d.aac"

#define CACHEAUDIO_CACHE_SIZE			(128*1024)
#define CACHEVIDEO_CACHE_SIZE			(1024*1024)

typedef struct
{

	unsigned char beforeTime;
	unsigned char afterTime;
	unsigned int startTime;
	char filename[64];

} CacheVideoReq_t;

typedef struct
{

    uint32_t OFFSET;//偏移
    uint32_t frameSize;//帧长
    uint8_t  frameType;//帧类

} CacheIndex_t;

typedef struct
{

    int cacheFd;

    char* dataCache;
    unsigned int dataSize;

    CacheIndex_t* indexCache;
	unsigned int curSecond;

} MediaStreamWriter_t;

typedef struct
{

	int chnNum;

	pthread_t pid;

	queue_list_t videoReqs;
	
	MediaStreamWriter_t writerVideo;
	MediaStreamWriter_t writerAudio;

	void* stream;

} MediaCacheHandle_t;

void MediaCache_FileInit(void);

void MediaCache_Create(MediaCacheHandle_t* handle, int chnNum, void* onwer);

int MediaCache_VideoCreate(unsigned char before_time, unsigned char after_time, int chnNum, char* filename);

#endif

