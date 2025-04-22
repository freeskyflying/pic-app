#ifndef _JT1078_MEDIA_FILES_H_
#define _JT1078_MEDIA_FILES_H_

#include <pthread.h>
#include "jt808_data.h"
#include "mp4v2_demuxer.h"


typedef struct
{

	MP4V2_Demuxer_t mp4Demuxer;

	unsigned int filetime;
	unsigned char serverAddr[64];
	int serverPort;
	int socketFd;
	
	int mRtpSerialNum;
	int mLogicChannel;
	uint32_t mLastIFrameTime;
	uint32_t mLastFrameTime;

	pthread_mutex_t mutex;

	void* owner;

} JT1078MediaPlaybackCtrl_t;

void JT1078MediaFiles_PlaybackInit(void* owner);

void JT1078MediaFiles_QueryFiles(JT808MsgBody_9205_t* reqParam, JT808MsgBody_1205_t* ackParam);

void JT1078MediaFiles_PlaybackRequest(void* owner, JT808MsgBody_9201_t* reqParam, JT808MsgBody_1205_t* ackParam);

void JT1078MediaFiles_PlaybackControl(void* owner, JT808MsgBody_9202_t* ctrlParam);

#endif

