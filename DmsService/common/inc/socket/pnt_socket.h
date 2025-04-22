#ifndef _PNT_SOECKT_H_
#define _PNT_SOCKET_H_

#include <stdio.h>
#include <pthread.h>
#include "typedef.h"
#include "pnt_epoll.h"

#define MAX_EPOLL_SIZE          10

#ifdef __cplusplus
extern "C" {
#endif

#define     kMaxTimeOut             (60 * 1000)
#define     kMaxSocketBuffer        (8192 * 10)

/* 接收数据处理接口
* buff: 接收数据缓存
* len：有效数据字节数长度
* arg：socket指针
* 返回：使用掉的数据字节数
 */
typedef int(*IRcvDataProcess)(char* buff, int len, void* arg);

typedef int(*OnConectStatusChange)(int status, void* arg);      /* 连接状态变化回调 */

typedef struct
{
    bool_t      useRecvTimeout;
    int         recvTimeout;

    bool_t      useSendTimeout;
    int         sendTimeout;

    bool_t      useRecvBufSize;
    size_t      recvBufSize;

    bool_t      useSendBufSize;
    size_t      sendBufSize;

    bool_t      useNoDelay;
    bool_t      nodelay;

    bool_t      useReuseaddr;
    bool_t      resumeAddr;

} SocketConfig_t;

typedef struct 
{

    char                    mHost[128];                 /* IP或域名 */
    int                     mPort;                      /* 端口号 */
    int                     mSockFd;

    pthread_t               mPid;                       /* 处理线程句柄 */

    volatile int            mRunning;                   /* 运行标志，FALSE-退出线程 */
    volatile int            mConnectStatus;             /* 连接状态，FALSE-未连接  TRUE-已连接 */
    unsigned int            mTimeout;                   /* 等待超时时间（单位ms），用于固定时间内的心跳判断是否连接断开，为-1表示不通过心跳判断状态 */
    unsigned int            mTimeoutCounter;            /* epoll超时次数计数器 */

    char*          			mRcvBuffer;
    int                     mRcvBufferSize;
    volatile int            mRcvDataLen;                /* 当前接收buffer中有效数据长度 */

    pthread_mutex_t         mMutex;                     /* 主运行锁，用于退出线程判断 */
    pthread_mutex_t         mMutexWrite;                /* 发送数据过程锁 */
    pthread_mutex_t         mMutexRead;                 /* 读数据过程锁 */

    IRcvDataProcess         mRcvProcess;                /* 接收数据处理接口 */
    OnConectStatusChange    mOnConnectStatusChange;     /* 连接状态回调函数 */

    SocketConfig_t          mConfig;                    /* socket OPT配置 */

    PNTEpoll_t              mEpoll;

    void*                   mOwner;                     /* 拥有者（父级） */

} PNTSocket_t;

void LibPnt_SocketSetConfig(PNTSocket_t* sock, SocketConfig_t* config);

void LibPnt_SocketInit(PNTSocket_t* sock, char* host, int port, int timeout, void* owner);

int LibPnt_SocketStart(PNTSocket_t* sock);

void LibPnt_SocketStop(PNTSocket_t* sock);

int LibPnt_SocketWrite(PNTSocket_t* sock, void* data, int size);

void LibPnt_SocketSetRcvDataProcess(PNTSocket_t* sock, IRcvDataProcess rcvProcess);

void LibPnt_SocketSetOnConnStatusChange(PNTSocket_t* sock, OnConectStatusChange onConnStatusChange);

void LibPnt_SocketClose(PNTSocket_t* sock);

void LibPnt_SocketSetServer(PNTSocket_t* sock, char* host, int port);

#ifdef __cplusplus
}
#endif

#endif
