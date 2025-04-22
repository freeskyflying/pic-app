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

/* �������ݴ���ӿ�
* buff: �������ݻ���
* len����Ч�����ֽ�������
* arg��socketָ��
* ���أ�ʹ�õ��������ֽ���
 */
typedef int(*IRcvDataProcess)(char* buff, int len, void* arg);

typedef int(*OnConectStatusChange)(int status, void* arg);      /* ����״̬�仯�ص� */

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

    char                    mHost[128];                 /* IP������ */
    int                     mPort;                      /* �˿ں� */
    volatile int            mSockFd;

    pthread_t               mPid;                       /* �����߳̾�� */

    volatile int            mRunning;                   /* ���б�־��FALSE-�˳��߳� */
    volatile int            mConnectStatus;             /* ����״̬��FALSE-δ����  TRUE-������ */
    unsigned int            mTimeout;                   /* �ȴ���ʱʱ�䣨��λms�������ڹ̶�ʱ���ڵ������ж��Ƿ����ӶϿ���Ϊ-1��ʾ��ͨ�������ж�״̬ */
    unsigned int            mTimeoutCounter;            /* epoll��ʱ���������� */

    char*          			mRcvBuffer;
    int                     mRcvBufferSize;
    volatile int            mRcvDataLen;                /* ��ǰ����buffer����Ч���ݳ��� */

    pthread_mutex_t         mMutex;                     /* ���������������˳��߳��ж� */
    pthread_mutex_t         mMutexWrite;                /* �������ݹ����� */
    pthread_mutex_t         mMutexRead;                 /* �����ݹ����� */

    IRcvDataProcess         mRcvProcess;                /* �������ݴ���ӿ� */
    OnConectStatusChange    mOnConnectStatusChange;     /* ����״̬�ص����� */

    SocketConfig_t          mConfig;                    /* socket OPT���� */

    PNTEpoll_t              mEpoll;

    void*                   mOwner;                     /* ӵ���ߣ������� */

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
