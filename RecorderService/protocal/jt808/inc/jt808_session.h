#ifndef _JT808_SESSION_H_
#define _JT808_SESSION_H_

#include <pthread.h>
#include "typedef.h"
#include "jt808_cmd_node.h"
#include "queue_list.h"
#include "utils.h"
#include "pnt_socket.h"
#include "jt808_data.h"
#include "pnt_ipc.h"
#include "jt808_database.h"
#include "jt808_blind_upload.h"
#include "jt1078_rtsp.h"
#include "ftpUtil.h"
#include "jt808_activesafety_upload.h"
#include "jt808_mediaevent.h"
#include "jt1078_media_files.h"
#include "media_utils.h"
#include "jt808_circlearea.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#define ADD_CMD_NODE(session,id,param,type)     {\
    JT808_CmdNode_t* node##id = (JT808_CmdNode_t*)malloc(sizeof(JT808_CmdNode_t)); \
    if (NULL != node##id) \
    { \
        node##id->mEncoder = (JT808_CmdEncoder)JT808_##id##_CmdEncoder; \
        node##id->mParser = (JT808_CmdParser)JT808_##id##_CmdParser; \
        node##id->mParams = param; \
        node##id->mMsgID = 0x##id; \
        node##id->mType = type; \
\
        queue_list_push(&session->mJt808CmdList, node##id);\
    } \
    else \
    { \
        PNT_LOGE("add cmd node malloc failed.");\
    } \
}
#else

#define ADD_CMD_NODE(id,param,type)     \
{\
    .mEncoder= (JT808_CmdEncoder)JT808_##id##_CmdEncoder, \
    .mParser = (JT808_CmdParser)JT808_##id##_CmdParser, \
    .mParam = param, \
    .mMsgID = 0x##id, \
    .mType = type \
}

#endif

// this is the latest session type
typedef enum
{

	SESSION_TYPE_JT808					= 0, // the normal session type JT808-2013
	SESSION_TYPE_TIANMAI_808			= 1, // the tian-mai session type
	SESSION_TYPE_JT808_2019 			= 2, // the session type for JT808-2019
	
} JT808SessionType_e;

typedef enum
{

	JT808SessionState_Waiting,								/* 等待状态 */

	JT808SessionState_StartRegister,						/* 发送注册消息 */
	
	JT808SessionState_StartAuthentication,					/* 发送鉴权消息 */
	
	JT808SessionState_Running,								/* 连接正常处理 */

	JT808SessionState_Offline,								/* 连接断开处理 */

} JT808SessionState_e;

typedef struct
{

	volatile uint16_t		mRunningFlag;					/* 运行标志 */
    uint16_t                mMsgSerialNum;					/* 消息序列号 */
    queue_list_t            mSendMsgQueue;					/* 待发送的消息队列 */
	queue_list_t			mPacketedSendMsgQueue;			/* 分包发送的消息队列 */
	queue_list_t			mPacketedRecvMsgQueue;			/* 分包接收的消息队列 */
    queue_list_t            mRecvMsgQueue;					/* 接收待处理的消息队列 */
	queue_list_t			mWaitAckMsgQueue;				/* 发送后等待应答的消息队列 */
    PNTSocket_t             mMainSocket;					/* 终端与808平台主连接socket */

	int						mLocationReportTimerFd;			/* 位置上报定时器描述符 */
	int						mHeartBeatTimerFd;				/* 心跳定时器描述符 */

	PNTEpoll_t 				mTimerEpoll;					/* 定时器Epoll */

	JT808SessionData_t*		mSessionData;					/* 当前Session参数 */

	BYTE					mPhoneNum[PHONE_NUM_LEN_MAX];	/* 平台电话号码 */

	queue_list_t			mLocationExtraInfoList;			/* 位置附加信息列表 */

    pthread_t               mSendQueueProcessPid;			/* 发送消息队列处理线程PID */
    pthread_t               mRecvQueueProcessPid;			/* 接收消息队列处理线程PID */
    pthread_t               mWaitAckMsgQueueProcessPid;		/* 等待应答消息队列处理线程PID */
    pthread_t               mMainProcessPid;				/* 主处理线程PID */
	pthread_t				mLocationReportProcessPid;		/* 位置上报处理线程PID */

	pthread_mutex_t			mSendMsgMutex;					/* 发送消息互斥量，保证同一时间只有一个线程访问发送队列消息入队函数 */
	
	int						mMainProcessState;				/* 主处理逻辑状态 */

	JT808BlindUpload_t		mBlineGpsData;					/* GPS盲区数据库 */

	JT1078RtspControl_t		mRtspController;				/* RTSP控制句柄 */

	JT808ActivesafetyUploadHandle_t mAlarmUpload;			/* 报警附件上传 */
	
	JT808MediaEvent_t			mMediaEvent;

//	JT1078MediaPlaybackCtrl_t mMediaPlayback[MAX_VIDEO_NUM];				/* 多媒体回放 */
	queue_list_t 			mMediaPlaybackList;

	volatile unsigned short	mTempReportIntervel;
	unsigned int			mTempReportValidTime;
	unsigned int			mTempReportStartTime;

	volatile unsigned char 	mFtpDownloadFlag;

	JT808MsgBody_9205_t* 	mMediaQueryParam;
	unsigned short			mMediaQuerySerialNum;

    void*                   mOnwer;							/* 拥有者 */

} JT808Session_t;

typedef struct
{
    JT808MsgHeader_t*       mMsgHead;						/* 消息头信息 */
    void*                   mMsgBody;						/* 消息体/消息体数据 */
	int						mMsgBodyLen;					/* 消息体数据长度 */

} JT808MsgNode_t;

typedef struct
{

	uint32_t				mLastTimeStamp;					/* 前一次发送的时间戳 */
	uint16_t				mMsgId;							/* 消息ID */
	uint16_t				mMsgSerilNum;					/* 消息序列号 */
	uint16_t				mTimeoutValue;					/* 超时时间，根据Tn+1=Tn*(n+1)计算 */
	uint16_t				mReplyCount;					/* 当前重传次数 */
	uint16_t				mMsgDataLen;					/* 消息数据长度 */
	uint8_t					mTimeBCD[6];					/* 消息发送BCD时间 */
	uint8_t*				mMsgData;						/* 消息数据，可以直接发送的 */

} JT808MsgWaitAckNode_t;

typedef struct
{

	JT808MsgHeader_t		mMsgHead;						/* 首包消息头 */
	BYTE*					mPackIdxList;					/* 已收到的包序号列表 */
    void*                   mMsgBody;						/* 消息体/消息体数据 */
	int						mMsgBodyLen;					/* 消息体数据长度 */
	uint32_t				mLastTimeStamp;					/* 前一次接收包的时间戳 */
	BYTE					mFinishFlag;					/* 最后一包接收标志 */

} JT808MsgRecvPacketNode_t;

JT808Session_t* JT808Session_Create(JT808SessionData_t* sessionData, void* owner);

int JT808Session_SendMsg(JT808Session_t* session, WORD msgId, void* msgBody, int msgBodyLen, char msgBodyEncFlag/* 数据量比较大的在调用此函数前将消息体编码 */);

void JT808Session_InsertLocationExtra(JT808Session_t* session, JT808MsgBody_0200Ex_t* ext);

int JT808Session_Start(JT808Session_t* session);

int JT808Session_Release(JT808Session_t* session);

void JT808Session_ReportPassengers(JT808Session_t* session, JT808MsgBody_1005_t* data);

#ifdef __cplusplus
}
#endif

#endif
