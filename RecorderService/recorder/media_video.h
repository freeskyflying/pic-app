#ifndef _MEDIA_VIDEO_H_
#define _MEDIA_VIDEO_H_

#include <pthread.h>
#include "pnt_ipc_stream.h"
#include "media_utils.h"
#include "media_cache.h"

#ifdef INDIA_2CH
#define SDR_VENC_BITRATE_1080P          (8*1024*1024/2)// 4.0M
#define SDR_VENC_BITRATE_CH1720P        (6*1024*1024/2)// 4.0M
#define SDR_VENC_BITRATE_720P           (5*1024*1024/2)// 2.5M
#else
#ifdef VIETNAM_2CH
#define SDR_VENC_BITRATE_1080P          (6*1024*1024)// 4.0M
#define SDR_VENC_BITRATE_CH1720P        (5*1024*1024)// 4.0M
#define SDR_VENC_BITRATE_720P           (3*1024*1024)// 2.5M
#else
#define SDR_VENC_BITRATE_1080P          (6*1024*1024/2)// 2.0M
#define SDR_VENC_BITRATE_CH1720P        (5*1024*1024/2)// 4.0M
#define SDR_VENC_BITRATE_720P           (4*1024*1024/2)// 1.5M
#endif
#endif
#define SDR_VENC_BITRATE_SUB            (1*1024*1024)
#define SDR_VISS_FRAMERATE              25
#ifdef VIETNAM_2CH
#define SDR_VENC_FRAMERATE_MAJOR        20//15
#else
#define SDR_VENC_FRAMERATE_MAJOR        15
#endif
#define SDR_VENC_FRAMERATE_SUB          15

#define VIDEO_THUMBNAIL_MAXLEN			(32*1024)

#ifdef ONLY_3rdCH
#define AUDIO_BIND_CHN					(0)
#else
#define AUDIO_BIND_CHN					(1)
#endif

#define SUBSTREAM                   1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{

	unsigned short srcwidth;
	unsigned short srcheight;

	unsigned short width;
	unsigned short height;
	unsigned char framerate;
	unsigned char needCache;
	unsigned char needRec;
	unsigned char isValid;
	unsigned int videoSize;

	unsigned int minutePerFile;

} MeidaVideoStreamParam_t;

typedef struct
{

    int venChn;
    int bThreadStart;
    pthread_t vencPid;
    pthread_t recPid;
    int type;

    volatile int recStatus;

	MediaCacheHandle_t cache;

	MeidaVideoStreamParam_t param;

	queue_list_t frameH264;
	uint64_t curFramePts;
	queue_list_t frameAudio;

    pthread_mutex_t mutex;
    
} MediaVideoStream_t;

void MediaVideo_SetRecordStatus(int status);

void MediaVideo_SetSyncCount(void);

int MediaVideo_Start(void);

int MediaVideo_RecorderInit(void);

int MediaVideo_Stop(void);

bool_t MediaVideo_GetRecordStatus(int chn);

unsigned int MediaVideo_GetRecordDuration(int chn);

int get_video_stream(int chn, FrameInfo_t* frame);

int get_audio_stream(int chn, FrameInfo_t* frame);

uint64_t get_curr_framepts(int chn);

extern MediaVideoStream_t gMediaStream[MAX_VIDEO_NUM*2];

#ifdef __cplusplus
}
#endif

#endif
