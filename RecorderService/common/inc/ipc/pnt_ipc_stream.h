#ifndef _PNT_IPC_STREAM_H_
#define _PNT_IPC_STREAM_H_

#include "pnt_ipc.h"

#define PNTIPC_STREAMNAME_V_FORMAT			"video_%02d_%s"
#define PNTIPC_STREAMNAME_A_FORMAT			"audio_%02d"

#define PNTIPC_STREAMSHM_ID					(10)
#define PNTIPC_STREAMSHM_SIZE				(256*1024)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{

	PNTIPC_t mHandler;			// 句柄
	uint8_t mChannelNum;		// 通道号
	uint8_t mStreamType;		// 0-视频，1-音频
	uint8_t mStreamSub;		// 0-主码流，1-子码流

	char* mShmaddr;

} PNTIPC_StreamServer_t;

typedef struct
{

	PNTIPC_t mHandler;			// 句柄
	uint8_t mChannelNum;		// 通道号
	uint8_t mStreamType;		// 0-视频，1-音频
	uint8_t mStreamSub;		// 0-主码流，1-子码流

	char* mShmaddr;

} PNTIPC_StreamClient_t;

void PNTIPCStream_GetName(char* name, uint8_t channelNum, uint8_t type, uint8_t sub);

void PNTIPCStream_ServerCreate(PNTIPC_StreamServer_t* server, uint8_t channelNum, uint8_t type, uint8_t sub, void* owner);

void PNTIPCStream_ServerSend(PNTIPC_StreamServer_t* server, char* data, int len, char* data2, int len2);

void PNTIPCStream_ClientConnect(PNTIPC_StreamClient_t* client, uint8_t channelNum, uint8_t type, uint8_t sub, IPCIRcvDataProcess rcvDataNotify, IPCOnConectStatusChange connectStatusNotify, void* owner);

void PNTIPCStream_ClientRelease(PNTIPC_StreamClient_t* client);

#ifdef __cplusplus
}
#endif

#endif

