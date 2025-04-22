#ifndef _MEDIA_VIDEO_H_
#define _MEDIA_VIDEO_H_

#include <pthread.h>
#include "pnt_ipc_stream.h"
#include "media_utils.h"

#define SDR_VENC_BITRATE_1080P          (4*1024*1024/2)// 2.0M
#define SDR_VENC_BITRATE_720P           (3*1024*1024/2)// 1.5M
#define SDR_VENC_BITRATE_SUB            (1*1024*1024)
#define SDR_VISS_FRAMERATE              25
#define SDR_VENC_FRAMERATE_MAJOR        15
#define SDR_VENC_FRAMERATE_SUB          15

#define VIDEO_THUMBNAIL_MAXLEN			(32*1024)

#define AUDIO_BIND_CHN					(0)

#define SUBSTREAM                   1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    
    int bitrate;
    int muxer_type;
    int venChn;

} MeidaVideoStreamParam_t;

typedef struct
{

    int venChn;
    int bThreadStart;
    pthread_t vencPid;
    pthread_t recPid;
    int type;

    volatile int recStatus;

    pthread_mutex_t mutex;
    
} MediaVideoStream_t;

void MediaVideo_SetRecordStatus(int status);

void MediaVideo_SetSyncCount(void);

int MediaVideo_Start(void);

int MediaVideo_RecorderInit(void);

int MediaVideo_Stop(void);


#ifdef __cplusplus
}
#endif

#endif
