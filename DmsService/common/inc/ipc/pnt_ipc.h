#ifndef _PNT_IPC_H_
#define _PNT_IPC_H_

#include <stdio.h>
#include <pthread.h>
#include "typedef.h"
#include "pnt_epoll.h"

/*
* ����UNIXSOCKETʵ�ֽ��̼�ͨѶ
* �����IPCServer����Ϣ/���������ߣ�ֻ������ͻ��˷������ݣ����������
* �ͻ���IPCClient����Ϣ/���������ߣ�ֻ�����������ݣ���������
* һ�����̿��Դ����������ˣ�����ͻ���
*/

#ifdef __cplusplus
extern "C" {
#endif

#define IPC_NAME_LEN            20
#define IPC_SERVER_CLIENT_CNT   24

/* �������ݴ���ӿ�
* buff: �������ݻ���
* len����Ч�����ֽ�������
* fd: �ͻ�fd���������Ч��
* arg��ipcָ��
* ���أ�ʹ�õ��������ֽ���
 */
typedef int(*IPCIRcvDataProcess)(char* buff, int len, int fd, void* arg);

typedef int(*IPCOnConectStatusChange)(int status, int fd, void* arg);      /* ����״̬�仯�ص� */

typedef enum
{

    PNTIPC_SERVER,
    PNTIPC_CLIENT,

} PNTIPC_Role_e;

typedef struct
{
    
    pthread_t               mPid;                   /* �������߳̾�� */

    int                     mFd;

    char*          			mRcvBuffer;
    int                     mRcvBufferSize;
    volatile int            mRcvDataLen;            /* ��ǰ����buffer����Ч���ݳ��� */
    pthread_mutex_t         mMutexRead;

    volatile bool_t         mConnectStatus;         /* ����״̬ */
    bool_t                  mRunning;               /* ���б�־ */

    PNTEpoll_t              mEpoll;                 /* Epoll������Ϣ���յ� */

} PNTIPC_Client_t;

typedef struct
{
    
    pthread_t               mPid;                   /* �������߳̾�� */

    int                     mFd;

    pthread_mutex_t         mMutexWrite;                /* �������ݹ����� */

    int                     mClientsFd[IPC_SERVER_CLIENT_CNT];

} PNTIPC_Server_t;

typedef struct 
{
    
    void*                   mSelf;                  /* �ṹ��ָ�� */

    char                    mName[IPC_NAME_LEN];    /* UnixSocket������ */

    int                     mRole;                  /* ���@PNTIPC_Role_e */

    IPCIRcvDataProcess      mRcvDataProcess;        /* �������ݴ��� */

    IPCOnConectStatusChange mOnConnectStatusChange; /* ����״̬�仯�ص� */

    void*                   mOwner;                 /* ӵ���� */
    
} PNTIPC_t;

/*
* ���������
*/
int LibPnt_IPCCreate(PNTIPC_t* ipc, char* name, IPCIRcvDataProcess rcvDataNotify, IPCOnConectStatusChange connectStatusNotify, void* owner);

/*
* ���ٿͻ���
*/
int LibPnt_IPCClientRelease(PNTIPC_t* ipc);

/*
* �����ͻ��˲����ӷ����
*/
int LibPnt_IPCConnect(PNTIPC_t* ipc, char* name, IPCIRcvDataProcess rcvDataNotify, IPCOnConectStatusChange connectStatusNotify, int buffSize, void* owner);

/*
* ����˷�������
*/
int LibPnt_IPCSend(PNTIPC_t* ipc, char* data, int len, int fd /* �������Ч,0-���пͻ��� */);



#ifdef __cplusplus
}
#endif

#endif
