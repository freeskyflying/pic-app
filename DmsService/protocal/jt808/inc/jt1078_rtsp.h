#ifndef _JT1078_RTSTP_H_
#define _JT1078_RTSTP_H_

#include <pthread.h>
#include "typedef.h"
#include "jt808_data.h"
#include "pnt_ipc_stream.h"

#define PACKET_BODY_LEN			950

typedef struct
{
	DWORD frameHead; // 0: 帧头标识，固定为0x30, 0x31, 0x63, 0x64
	uint8_t V: 2; // 4: V 固定为2
	uint8_t P: 1; // 固定为0
	uint8_t X: 1; // RTP头是否需要扩展位,固定为0
	uint8_t CC: 4; // 固定为1
	uint8_t M: 1; // 5: 标志位，确定是否为完整数据帧的边界
	uint8_t PT: 7; // 负载类型
	WORD packetSerialNum; // 6: 包序号，初始为0,每发送一个RTP数据包，序列号加1
	BYTE simCardNum[10]; // 8: 终端设备SIM卡号
	BYTE simCardNumLen;
	BYTE logicChannel; // 14: 逻辑通道号，按照JT/T 1076-2016中的表2(我们需要注意的是，苏标当中的逻辑通道号定义和1076当中的逻辑通道号定义是有冲突的.
	// 例如在苏标当中，64是adas,65是dms,但是在1076当中，adas是10以内的值)
	uint8_t dataType: 4; // 15 数据类型,
	// 0000 - 视频I帧
	// 0001 - 视频P帧
	// 0010 - 视频B帧
	// 0011 - AUDIO Frame
	// 0100 - 透传数据
	uint8_t separatePacketTag: 4; // 分包处理标记
	// 0000 - 原子包,不可拆分
	// 0001 - 分包处理时的第一个分包
	// 0010 - 分包处理时的最后一个分包
	// 0100 - 分包处理时的中间包
	uint64_t timeStamp; // 16: 时间戳，标识此RTP数据包当前帧的相对时间，单位毫秒。当数据类型为0100时，则没有该字段
	// 在1078文档的原始定义当中，timestamp时间戳的定义是BYTE[8],即是8个字节的数组，但是关于时间戳具体如何定义并没有具体的描述，因此我们这里指定定义成uint64_t.
	// 在不同的平台当中可能会有歧义，因此对于不同的平台需要单独分析
	WORD lastIFrameInterval; // 24: 该帧与上一个关键帧之间的时间间隔，单位毫秒，当前数据类型为非视频帧时，则没有该字段
	WORD lastFrameInterval; // 26: 该帧与上一帧之间的时间间隔，单位毫秒，当数据类型为非视频帧时，则没有该字段
	WORD dataBodyLen; // 28: 数据体长度
	BYTE dataBody[PACKET_BODY_LEN]; // 30 音视频数据或者透传数据，长度不超过 950 byte
} RTPStreamingPacket;

typedef enum
{

	JT1078RtspStateInit = 0,
	JT1078RtspStatePlaying,
	JT1078RtspStatePause,
	JT1078RtspStateStop,

} JT1078RtspState_e;

typedef struct
{

	volatile unsigned char mState;			// 状态
	volatile unsigned char mStreamState;	// 音视频流状态，bit0-视频流，bit1-音频流
	unsigned char mLogicChannel;			// 逻辑通道号
	unsigned char mStreamType;				// 码流类型：0-主码流，1-子码流
	unsigned char mDataType;				// 数据类型：0-音视频，1-视频，2-双向对讲，3-监听，4-中心广播，5-透传
	unsigned char mServerUrl[64];			// 服务器地址
	unsigned short mServerTCPPort;			// 服务器端口号
	unsigned short mServerUDPPort;			// 服务器端口号

	int	mSocketFd;

	pthread_mutex_t	mWriteMutex;

	WORD	mRtpSerialNum;
	uint32_t	mLastIFrameTime;
	uint32_t	mLastFrameTime;

	queue_list_t mFrameListVideo;
	
	queue_list_t mFrameListAudio;

	void* mOwner;

} JT1078Rtsp_t;

typedef struct
{

	queue_list_t mRtspHandleList;			// JT1078Rtsp_t

	uint32_t	mTimestamp;

	void* mOwner;

} JT1078RtspControl_t;

int jt1078ConnectServer(uint8_t* url, uint16_t port);

void JT1078Rtsp_ControlInit(JT1078RtspControl_t* controller, void* owner);

int JT1078Rtsp_ChannelCreate(JT1078RtspControl_t* controller, JT808MsgBody_9101_t* param);

void JT1078Rtsp_PlayControl(JT1078RtspControl_t* controller, JT808MsgBody_9102_t* param);

void JT1078Rtsp_InsertFrame(JT1078RtspControl_t* controller, int chnNum, int sub, unsigned char* buff1, int len1, unsigned char* buff2, int len2);

void JT1078Rtsp_InsertAudio(JT1078RtspControl_t* controller, int chnNum, unsigned char* buff, int len);

#endif


