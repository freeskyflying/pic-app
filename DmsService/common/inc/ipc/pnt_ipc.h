#ifndef _PNT_IPC_H_
#define _PNT_IPC_H_

#include <stdio.h>
#include <pthread.h>
#include "typedef.h"
#include "pnt_epoll.h"

/*
* 基于UNIXSOCKET实现进程间通讯
* 服务端IPCServer：消息/数据生产者，只向请求客户端发送数据，不处理接收
* 客户端IPCClient：消息/数据消费者，只处理服务端数据，不处理发送
* 一个进程可以创建多个服务端，多个客户端
*/

#ifdef __cplusplus
extern "C" {
#endif

#define IPC_NAME_LEN            20
#define IPC_SERVER_CLIENT_CNT   24

/* 接收数据处理接口
* buff: 接收数据缓存
* len：有效数据字节数长度
* fd: 客户fd（服务端有效）
* arg：ipc指针
* 返回：使用掉的数据字节数
 */
typedef int(*IPCIRcvDataProcess)(char* buff, int len, int fd, void* arg);

typedef int(*IPCOnConectStatusChange)(int status, int fd, void* arg);      /* 连接状态变化回调 */

typedef enum
{

    PNTIPC_SERVER,
    PNTIPC_CLIENT,

} PNTIPC_Role_e;

typedef struct
{
    
    pthread_t               mPid;                   /* 主处理线程句柄 */

    int                     mFd;

    char*          			mRcvBuffer;
    int                     mRcvBufferSize;
    volatile int            mRcvDataLen;            /* 当前接收buffer中有效数据长度 */
    pthread_mutex_t         mMutexRead;

    volatile bool_t         mConnectStatus;         /* 连接状态 */
    bool_t                  mRunning;               /* 运行标志 */

    PNTEpoll_t              mEpoll;                 /* Epoll处理消息接收等 */

} PNTIPC_Client_t;

typedef struct
{
    
    pthread_t               mPid;                   /* 主处理线程句柄 */

    int                     mFd;

    pthread_mutex_t         mMutexWrite;                /* 发送数据过程锁 */

    int                     mClientsFd[IPC_SERVER_CLIENT_CNT];

} PNTIPC_Server_t;

typedef struct 
{
    
    void*                   mSelf;                  /* 结构体指针 */

    char                    mName[IPC_NAME_LEN];    /* UnixSocket名字域 */

    int                     mRole;                  /* 详见@PNTIPC_Role_e */

    IPCIRcvDataProcess      mRcvDataProcess;        /* 接收数据处理 */

    IPCOnConectStatusChange mOnConnectStatusChange; /* 连接状态变化回调 */

    void*                   mOwner;                 /* 拥有者 */
    
} PNTIPC_t;

/*
* 创建服务端
*/
int LibPnt_IPCCreate(PNTIPC_t* ipc, char* name, IPCIRcvDataProcess rcvDataNotify, IPCOnConectStatusChange connectStatusNotify, void* owner);

/*
* 销毁客户端
*/
int LibPnt_IPCClientRelease(PNTIPC_t* ipc);

/*
* 创建客户端并连接服务端
*/
int LibPnt_IPCConnect(PNTIPC_t* ipc, char* name, IPCIRcvDataProcess rcvDataNotify, IPCOnConectStatusChange connectStatusNotify, int buffSize, void* owner);

/*
* 服务端发送数据
*/
int LibPnt_IPCSend(PNTIPC_t* ipc, char* data, int len, int fd /* 服务端有效,0-所有客户端 */);



#ifdef __cplusplus
}
#endif

#endif
