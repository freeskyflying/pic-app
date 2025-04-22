#ifndef _JT808_ACTIVESAFETY_UPLOAD_H_
#define _JT808_ACTIVESAFETY_UPLOAD_H_

#include "jt808_data.h"
#include "queue_list.h"

#define FILE_PACK_SIZE		(64*1024)

typedef struct
{

	char mIp[64];
	WORD mTCPPort;
	BYTE mAlertFlag[16];
	BYTE mAlertID[32];

} JT808ActivesafetyRequest_t;

typedef enum
{

	upload_state_connect_server,
	upload_state_report_attinfo,
	upload_state_report_fileinfo,
	upload_state_running,
	upload_state_complete,
	upload_state_exit,

} ActivesafetyUploadState_e;

typedef struct
{

	queue_list_t reqList;
	int socketFd;

	int msgSerialNum;

	volatile int mState;

	JT808MsgBody_1210_File_t* fileList;
	int currenFileIdx;
	int currentFileCount;

	void* owner;

} JT808ActivesafetyUploadHandle_t;

void JT808ActiveSafety_UploadRequest(JT808ActivesafetyUploadHandle_t* handle, JT808MsgBody_9208_t* req);

void JT808ActiveSafety_UploadInit(JT808ActivesafetyUploadHandle_t * handle, void* owner);

#endif

