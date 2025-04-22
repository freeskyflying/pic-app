#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <string.h>
#include "pnt_log.h"
#include "pnt_ipc_stream.h"

void PNTIPCStream_GetName(char* name, uint8_t channelNum, uint8_t type, uint8_t sub)
{
	if (0 == type)
	{
		sprintf(name, PNTIPC_STREAMNAME_V_FORMAT, channelNum, (0==sub)?"m":"s");
	}
	else
	{
		sprintf(name, PNTIPC_STREAMNAME_A_FORMAT, channelNum);
	}
}

void PNTIPCStream_ServerCreate(PNTIPC_StreamServer_t* server, uint8_t channelNum, uint8_t type, uint8_t sub, void* owner)
{
	if (NULL == server)
	{
		PNT_LOGE("param 1 cannot be NULL.");
		return;
	}
	
	char name[128] = { 0 };
	PNTIPCStream_GetName(name, channelNum, type, sub);

	int ret = LibPnt_IPCCreate(&server->mHandler, name, NULL, NULL, owner);

	if (RET_SUCCESS == ret && 0 == type) // 视频帧通过shm
	{
		key_t key=ftok("/", PNTIPC_STREAMSHM_ID+(channelNum*2+sub));
		if (key<0)
		{
			PNT_LOGE("ftok failed [%s] %d.", name, key);
			return;
		}
		
		int shmid=shmget(key, PNTIPC_STREAMSHM_SIZE, IPC_CREAT/*|IPC_EXCL*/|0666);
	    if(shmid<0)
		{
			PNT_LOGE("shmgets failed [%s].", name);
			return;
	    }
		
		server->mShmaddr = (char*)shmat(shmid, NULL, 0);
	    if(server->mShmaddr == (char*)-1)
		{
			server->mShmaddr = NULL;
	        PNT_LOGE("shmat failed [%s]", name);
			return;
	    }
		
		PNT_LOGI("%s ipcServer create success.", name);
	}
	else
	{
		PNT_LOGE("%s ipcServer create failed.", name);
	}
}

void PNTIPCStream_ServerSend(PNTIPC_StreamServer_t* server, char* data, int len, char* data2, int len2)
{
	int flag = 0;
    PNTIPC_Server_t* ipcserver = (PNTIPC_Server_t*)server->mHandler.mSelf;

	// no client , don't copy
	for (int i = 0; i<IPC_SERVER_CLIENT_CNT; i++)
	{
		if (ipcserver->mClientsFd[i] > 0)
		{
			flag = 1;
			break;
		}
	}

	if (0 == flag)
	{
		return;
	}

	if (NULL != server->mShmaddr)
	{
		int total_len = len + len2;

		memcpy(server->mShmaddr, data, len);
		if (data2 && len2 > 0)
		{
			memcpy(server->mShmaddr+len, data2, len2);
		}
		LibPnt_IPCSend(&server->mHandler, (char*)&total_len, sizeof(int), 0);
	}
	else
	{
		LibPnt_IPCSend(&server->mHandler, data, len, 0);
	}
}

void PNTIPCStream_ClientConnect(PNTIPC_StreamClient_t* client, uint8_t channelNum, uint8_t type, uint8_t sub, IPCIRcvDataProcess rcvDataNotify, IPCOnConectStatusChange connectStatusNotify, void* owner)
{
	if (NULL == client)
	{
		PNT_LOGE("param 1 cannot be NULL.");
		return;
	}
	
	char name[128] = { 0 };
	PNTIPCStream_GetName(name, channelNum, type, sub);

	int ret = LibPnt_IPCConnect(&client->mHandler, name, rcvDataNotify, connectStatusNotify, 0, owner);
	if (RET_SUCCESS == ret && 0 == type) /* 视频流采用内存共享方式 */
	{
		key_t key=ftok("/", PNTIPC_STREAMSHM_ID+(channelNum*2+sub));
		if (key<0)
		{
			PNT_LOGE("ftok failed [%s].", name);
			return;
		}
		
		int shmid=shmget(key, PNTIPC_STREAMSHM_SIZE, IPC_CREAT);
	    if(shmid<0)
		{
			PNT_LOGE("shmgets failed [%s].", name);
			return;
	    }
		
		client->mShmaddr = (char*)shmat(shmid, NULL, 0);
	    if(client->mShmaddr == (char*)-1)
		{
			client->mShmaddr = NULL;
	        PNT_LOGE("shmat failed [%s]", name);
			return;
	    }
	}
}

void PNTIPCStream_ClientRelease(PNTIPC_StreamClient_t* client)
{
	LibPnt_IPCClientRelease(&client->mHandler);

	if (NULL != client->mShmaddr)
	{
		shmdt(client->mShmaddr);
		client->mShmaddr = NULL;
	}
}


