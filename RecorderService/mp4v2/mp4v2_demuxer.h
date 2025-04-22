#ifndef _MP4V2_DEMUXER_H_
#define _MP4V2_DEMUXER_H_

#include <pthread.h>
#include "mp4v2/mp4v2.h"

typedef int(*mp4v2demuxer_vidcallback)(unsigned char* frame, int len, void* owner, unsigned int frameTime);

typedef int(*mp4v2demuxer_audcallback)(unsigned char* frame, int len, void* owner, unsigned int frameTime);

typedef enum
{

	mp4v2_demuxer_ctrl_start,
	mp4v2_demuxer_ctrl_pause,
	mp4v2_demuxer_ctrl_stop,
	mp4v2_demuxer_ctrl_fast,
	mp4v2_demuxer_ctrl_iframeback,
	mp4v2_demuxer_ctrl_drag,
	mp4v2_demuxer_ctrl_iframeplay,
	mp4v2_demuxer_ctrl_init,
	mp4v2_demuxer_ctrl_playing,
	mp4v2_demuxer_ctrl_release,

} MP4V2_DemuxerCtrl_e;

typedef struct
{

	pthread_t mPid;
	unsigned int trackid;

} MP4V2_EsDemuxer_t;

typedef struct
{

	volatile unsigned char mRunningFlag;
	pthread_t mPid;
	pthread_mutex_t mutex;

	mp4v2demuxer_vidcallback vidCb;
	mp4v2demuxer_audcallback audCb;

	MP4FileHandle mp4Handle;

	MP4V2_EsDemuxer_t mVidHandle;
	MP4V2_EsDemuxer_t mAudHandle;

	unsigned char mode;
	unsigned char gain;
	volatile unsigned char state;
	
	unsigned char dragOffset;	
	unsigned int startTimestamp;

	void* owner;

} MP4V2_Demuxer_t;

int mp4v2_Demuxer_Start(MP4V2_Demuxer_t* demuxer, char* filename);

void mp4v2_Demuxer_Stop(MP4V2_Demuxer_t* demuxer);

void mp4v2_Demuxer_SetCtrl(MP4V2_Demuxer_t* demuxer, unsigned char ctrl, unsigned char gain, unsigned char* timeNodeBCD);

#endif

