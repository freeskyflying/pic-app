#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <signal.h>
#include "pnt_ipc.h"
#include "pnt_log.h"
#include "utils.h"

void* pntIPCServerLoopThread(void* arg)
{
    PNTIPC_t* ipc = (PNTIPC_t*)arg;
    PNTIPC_Server_t* server = (PNTIPC_Server_t*)ipc->mSelf;

rebind:
	
	server->mFd = socket(AF_UNIX,SOCK_STREAM,0);
	if(0 > server->mFd)
	{
		PNT_LOGE("[%s] ipc server create fd failed.", ipc->mName);
		goto exit;
	}

	setNonBlocking(server->mFd);
 
	struct sockaddr_un addr = {};
    bzero(&addr, sizeof(addr));
	addr.sun_family = AF_UNIX;
 	strcpy((char*)(addr.sun_path+1), ipc->mName);
	socklen_t addrlen = 1 + strlen(ipc->mName) + sizeof(addr.sun_family);
 
	if(bind(server->mFd,(struct sockaddr*)&addr,addrlen))
	{
		PNT_LOGE("[%s] ipc server bind failed %s.", ipc->mName, strerror(errno));
		close(server->mFd);
		sleep(2);
		goto rebind;
	}
 
	if(listen(server->mFd, IPC_SERVER_CLIENT_CNT))  // 最大可以支持24个连接
	{
		PNT_LOGE("[%s] ipc server listen failed.", ipc->mName);
		goto exit;
	}
    
    if(pthread_mutex_init(&server->mMutexWrite, NULL) != 0)
    {
        PNT_LOGE("[%s] ipc write mutex init failed.", ipc->mName);
        goto exit;
    }
 
    while (server->mFd >= 0)
    {
        int con_fd = accept(server->mFd,(struct sockaddr*)&addr,&addrlen);
        if(0 < con_fd)
        {
		    PNT_LOGI("[%s] ipc server connect one client [%d].", ipc->mName, con_fd);
            
            pthread_mutex_lock(&server->mMutexWrite);
            for (int i = 0; i<IPC_SERVER_CLIENT_CNT; i++)
            {
                if (server->mClientsFd[i] <= 0)
                {
                    server->mClientsFd[i] = con_fd;
                    break;
                }
            }
            pthread_mutex_unlock(&server->mMutexWrite);
        }
		usleep(100*1000);
    }

 exit:
    
    if (0 <= server->mFd)
    {
        close(server->mFd);
    }

	PNT_LOGE("ipc server [%s] exit.", ipc->mName);

    free(ipc->mSelf);
    ipc->mSelf = NULL;

    pthread_exit(NULL);
    return NULL;
}

static int IPCClientClose(PNTIPC_t* ipc)
{
    PNTIPC_Client_t* client = (PNTIPC_Client_t*)ipc->mSelf;

    client->mConnectStatus = FALSE; // 断连，需要重新连接

    if (ipc->mOnConnectStatusChange)
    {
        ipc->mOnConnectStatusChange(client->mConnectStatus, client->mFd, ipc);
    }
    
    LibPnt_EpollDelEvent(&client->mEpoll, client->mFd, EPOLLIN | EPOLLET);
    LibPnt_EpollSetState(&client->mEpoll, FALSE);
    close(client->mFd);

	return 0;
}

static int IPCClientOnEpollEvent(int fd, unsigned int event, void* arg)
{
    PNTEpoll_t * ePoll = (PNTEpoll_t*)arg;
    PNTIPC_t* ipc = (PNTIPC_t*)ePoll->mOwner;
    PNTIPC_Client_t* client = (PNTIPC_Client_t*)ipc->mSelf;

    if (NULL != client)
    {
        if (event & EPOLLIN)
        {			
            while (client->mRunning)
            {
                unsigned char buffer[1024];
                memset(buffer, 0, 1024);
                ssize_t ret = read(fd, buffer, 1024);
                if (ret == -1)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        break;
                    }
                    PNT_LOGE("ipc client close connection to [%s], caused by %s", ipc->mName, strerror(errno));
                    IPCClientClose(ipc);
                    break;
                }
                else if (ret == 0)
                {
                    // PNT_LOGE("ipc client close connection to [%s], caused by %s", ipc->mName, strerror(errno));
                    // IPCClientClose(ipc);
                    break;
                }
                else
                {
            		pthread_mutex_lock(&client->mMutexRead);
					
                    if (client->mRcvDataLen + ret > client->mRcvBufferSize)
                    {
                        client->mRcvDataLen = 0;
                    }
                    memcpy(client->mRcvBuffer+client->mRcvDataLen, buffer, ret);
                    client->mRcvDataLen += ret;

					pthread_mutex_unlock(&client->mMutexRead);
                }
            }
        }
    }

    return RET_SUCCESS;
}

void* pntIPCClientLoopThread(void* arg)
{
    PNTIPC_t* ipc = (PNTIPC_t*)arg;
    PNTIPC_Client_t* client = (PNTIPC_Client_t*)ipc->mSelf;
 
    memset(&client->mEpoll, 0, sizeof(client->mEpoll));

    if(RET_FAILED == LibPnt_EpollCreate(&client->mEpoll, 10, ipc))
    {
        PNT_LOGE("[%s] ipc epoll_create error:%s", ipc->mName, strerror(errno));
        goto exit;
    }
    
    if(pthread_mutex_init(&client->mMutexRead, NULL) != 0)
    {
        PNT_LOGE("[%s] ipc read mutex init failed.", ipc->mName);
        goto exit;
    }

	if (0 == client->mRcvBufferSize)
	{
    	client->mRcvBufferSize = 4096;
	}

    client->mRcvBuffer = (char*)malloc(client->mRcvBufferSize);
    if(NULL == client->mRcvBuffer)
    {
        PNT_LOGE("[%s] ipc recv buffer malloc failed.", ipc->mName);
        goto exit;
    }

    client->mRunning = TRUE;
    client->mConnectStatus = FALSE;
  
retry:

	client->mFd = socket(AF_UNIX,SOCK_STREAM,0);
	if(0 > client->mFd)
	{
		PNT_LOGE("client[%s] socket create failed.", ipc->mName);
		goto exit;
	}

	struct sockaddr_un addr = {};
    bzero(&addr, sizeof(addr));
	addr.sun_family = AF_UNIX;
 	strcpy((char*)(addr.sun_path+1), ipc->mName);
	socklen_t addrlen = 1 + strlen(ipc->mName) + sizeof(addr.sun_family);

	if(connect(client->mFd,(struct sockaddr*)&addr,addrlen))
	{
        close(client->mFd);
		if (!client->mRunning)
		{
			goto exit;
		}
        SleepMs(100);
		goto retry;
	}

    LibPnt_EpollAddEvent(&client->mEpoll, client->mFd, EPOLLIN | EPOLLET);
    LibPnt_EpollSetOnEvent(&client->mEpoll, IPCClientOnEpollEvent);
    LibPnt_EpollSetState(&client->mEpoll, TRUE);

    client->mConnectStatus = TRUE;
    if (ipc->mOnConnectStatusChange)
    {
        ipc->mOnConnectStatusChange(client->mConnectStatus, client->mFd, ipc);
    }

    int lastRcvDataLen = 0;

    while (client->mRunning)
    {
        /* 接收数据处理 */
        if (ipc->mRcvDataProcess && client->mRcvDataLen > lastRcvDataLen)
        {
            pthread_mutex_lock(&client->mMutexRead);
            
            int usedLen = ipc->mRcvDataProcess(client->mRcvBuffer, client->mRcvDataLen, client->mFd, ipc);
            if (usedLen >= client->mRcvDataLen)
            {
                client->mRcvDataLen = 0;
            }
            else
            {
                memcpy(client->mRcvBuffer, client->mRcvBuffer + usedLen, client->mRcvDataLen - usedLen);
                client->mRcvDataLen -= usedLen;
            }

            pthread_mutex_unlock(&client->mMutexRead);

            lastRcvDataLen = client->mRcvDataLen;
        }

        if (FALSE == client->mConnectStatus) // 断连，需要重新连接，除非线程退出
        {
            if (client->mRunning)
            {
                goto retry;
            }
        }
        else
        {
            SleepMs(10);
        }
    }

    LibPnt_EpollClose(&client->mEpoll);
    IPCClientClose(ipc);

exit:

	PNT_LOGE("client[%s] thread exit.", ipc->mName);

    if (NULL != client->mRcvBuffer)
    {
        free(client->mRcvBuffer);
        client->mRcvBuffer = NULL;
    }
	
	pthread_mutex_destroy(&client->mMutexRead);

    free(ipc->mSelf);
    ipc->mSelf = NULL;

    pthread_exit(NULL);
    return NULL;
}

int LibPnt_IPCCreate(PNTIPC_t* ipc, char* name, IPCIRcvDataProcess rcvDataNotify, IPCOnConectStatusChange connectStatusNotify, void* owner)
{
    if (NULL == ipc)
    {
        PNT_LOGE("param1 cannot null.");
        return RET_FAILED;
    }
    
    signal(SIGPIPE, SIG_IGN);

    memset(ipc, 0, sizeof(PNTIPC_t));

    ipc->mRole = PNTIPC_SERVER;
    ipc->mOwner = owner;

    strncpy(ipc->mName, name, IPC_NAME_LEN-1);

    ipc->mSelf = (PNTIPC_Server_t*)malloc(sizeof(PNTIPC_Server_t));
    if (NULL == ipc->mSelf)
    {
        PNT_LOGE("param1 cannot null.");
        return RET_FAILED;
    }

    memset(ipc->mSelf, 0, sizeof(PNTIPC_Server_t));

    ipc->mRcvDataProcess = rcvDataNotify;
    ipc->mOnConnectStatusChange = connectStatusNotify;
    
    int ret = pthread_create(&((PNTIPC_Server_t*)ipc->mSelf)->mPid, NULL, pntIPCServerLoopThread, ipc);
    if (ret != 0)
    {
        PNT_LOGE("fail to create thead of server ipc[%s] caused by:%s", name, strerror(ret));
        return RET_FAILED;
    }

    return RET_SUCCESS;
}

int LibPnt_IPCClientRelease(PNTIPC_t* ipc)
{
    if (NULL == ipc)
    {
        PNT_LOGE("param1 cannot null.");
        return RET_FAILED;
    }

    if (NULL != ipc->mSelf)
    {
    	PNTIPC_Client_t* client = (PNTIPC_Client_t*)ipc->mSelf;
		client->mRunning = FALSE;
		sleep(1);
		//while (NULL != ipc->mSelf) SleepMs(10);
		PNT_LOGE("IPC client closed.");
    }

    return RET_SUCCESS;
}

int LibPnt_IPCServerRelease(PNTIPC_t* ipc)
{
    if (NULL == ipc)
    {
        PNT_LOGE("param1 cannot null.");
        return RET_FAILED;
    }

    if (NULL != ipc->mSelf)
    {
    	PNTIPC_Server_t* server = (PNTIPC_Server_t*)ipc->mSelf;
		if (server->mFd >= 0)
		{
			close(server->mFd);
			server->mFd = -1;
		}
		while (NULL != ipc->mSelf) SleepMs(10);
		PNT_LOGE("IPC server closed.");
    }

    return RET_SUCCESS;
}

int LibPnt_IPCConnect(PNTIPC_t* ipc, char* name, IPCIRcvDataProcess rcvDataNotify, IPCOnConectStatusChange connectStatusNotify, int buffSize, void* owner)
{
    if (NULL == ipc)
    {
        PNT_LOGE("param1 cannot null.");
        return RET_FAILED;
    }

    memset(ipc, 0, sizeof(PNTIPC_t));

    ipc->mRole = PNTIPC_CLIENT;
    ipc->mOwner = owner;

    memcpy(ipc->mName, name, strlen(name)>=(IPC_NAME_LEN-1)?(IPC_NAME_LEN-1):strlen(name));

    ipc->mSelf = (PNTIPC_Client_t*)malloc(sizeof(PNTIPC_Client_t));
    if (NULL == ipc->mSelf)
    {
        PNT_LOGE("param1 cannot null.");
        return RET_FAILED;
    }

    memset(ipc->mSelf, 0, sizeof(PNTIPC_Client_t));

    ipc->mRcvDataProcess = rcvDataNotify;
    ipc->mOnConnectStatusChange = connectStatusNotify;
	((PNTIPC_Client_t*)ipc->mSelf)->mRcvBufferSize = buffSize;
    
    int ret = pthread_create(&((PNTIPC_Client_t*)ipc->mSelf)->mPid, NULL, pntIPCClientLoopThread, ipc);
    if (ret != 0)
    {
        PNT_LOGE("fail to create thead of client ipc[%s] caused by:%s", name, strerror(ret));
        return RET_FAILED;
    }

    return RET_SUCCESS;
}

int LibPnt_IPCSend(PNTIPC_t* ipc, char* data, int len, int fd /* 服务端有效,0-所有客户端 */)
{
    if (NULL == ipc)
    {
        PNT_LOGE("param1 cannot null.");
        return RET_FAILED;
    }
    if (PNTIPC_CLIENT == ipc->mRole)
    {
        PNT_LOGE("[%s]this ipc is a ipc client, cannot send.", ipc->mName);
        return RET_FAILED;
    }
    else
    {
        PNTIPC_Server_t* server = (PNTIPC_Server_t*)ipc->mSelf;
        pthread_mutex_lock(&server->mMutexWrite);
        
        for (int i = 0; i<IPC_SERVER_CLIENT_CNT; i++)
        {
            if (server->mClientsFd[i] > 0)
            {
                int ret = write(server->mClientsFd[i], data, len);
                if (ret < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        continue;
                    }
                    else
                    {
                        PNT_LOGE("[%s] ipc write client [%d] failed, caused by %s", ipc->mName, strerror(errno));
						if (ipc->mOnConnectStatusChange)
					    {
					        ipc->mOnConnectStatusChange(0, server->mClientsFd[i], ipc);
					    }
                        close(server->mClientsFd[i]);
                        server->mClientsFd[i] = -1;
                    }
                }
            }
        }

        pthread_mutex_unlock(&server->mMutexWrite);
    }

    return RET_SUCCESS;
}


