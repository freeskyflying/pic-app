#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include "utils.h"
#include "pnt_socket.h"
#include "pnt_log.h"

static int setSocketConfig(PNTSocket_t* sock)
{
    if (sock->mConfig.useRecvTimeout)
    {
        struct timeval timeout = {sock->mConfig.recvTimeout, 0};
        if (setsockopt(sock->mSockFd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval)) < 0)
        {
            PNT_LOGE("setsockopt SO_RCVTIMEO error:%s", strerror(errno));
            return -1;
        }
    }
    if (sock->mConfig.useSendTimeout)
    {
        struct timeval timeout = {sock->mConfig.sendTimeout, 0};
        if (setsockopt(sock->mSockFd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval)) < 0)
        {
            PNT_LOGE("setsockopt SO_SNDTIMEO error:%s", strerror(errno));
            return -1;
        }
    }
    if (sock->mConfig.useRecvBufSize)
    {
        if (setsockopt(sock->mSockFd, SOL_SOCKET, SO_RCVBUF, &sock->mConfig.recvBufSize, sizeof(sock->mConfig.recvBufSize)) < 0)
        {
            PNT_LOGE("setsockopt SO_RCVBUF error:%s", strerror(errno));
            return -1;
        }
    }
    if (sock->mConfig.useSendBufSize)
    {
        if (setsockopt(sock->mSockFd, SOL_SOCKET, SO_SNDBUF, &sock->mConfig.sendBufSize, sizeof(sock->mConfig.sendBufSize)) < 0)
        {
            PNT_LOGE("setsockopt SO_SNDBUF error:%s", strerror(errno));
            return -1;
        }
    }
    if (sock->mConfig.useNoDelay)
    {
        int nodelay = sock->mConfig.nodelay ? 1 : 0;
        if (setsockopt(sock->mSockFd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)) < 0)
        {
            PNT_LOGE("setsockopt TCP_NODELAY error:%s", strerror(errno));
        }
    }
    if (sock->mConfig.useReuseaddr)
    {
        int resumeAddr = sock->mConfig.resumeAddr ? 1 : 0;
        if (setsockopt(sock->mSockFd, SOL_SOCKET, SO_REUSEADDR, &resumeAddr, sizeof(resumeAddr)))
        {
            PNT_LOGE("setsockopt SO_REUSEADDR error:%s", strerror(errno));
        }
    }
    int size;
    socklen_t t = sizeof(int);
    if (getsockopt(sock->mSockFd, SOL_SOCKET, SO_SNDBUF, &size, &t) < 0)
    {
        PNT_LOGE("setsockopt SO_SNDBUF error:%s", strerror(errno));
        return -1;
    }

    //PNT_LOGI("SendBuf:%d %ld", size, sock->mConfig.sendBufSize);
    
    return 1;
}

static int bSockBindAddress(PNTSocket_t* sock, const char *host, int port, struct addrinfo *addrs)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;

    char portStr[16] = {0};
    sprintf(portStr, "%d", port);
    int result = 0;
    PNT_LOGD("BaseSocket start to get the address of %s:%d", host, port);
    result = getaddrinfo(host, portStr, &hints, &addrs);
    if (result != 0)
    {
        PNT_LOGE("BaseSocket fatal state error!!! fail to get the address info of %s:%d", host, port);
        return RET_FAILED;
    }

    for (struct addrinfo *addr = addrs; addr != NULL; addr = addr->ai_next)
    {
        sock->mSockFd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        if (sock->mSockFd < 0)
        {
            PNT_LOGE("BaseSocket fatal state error!!! create socket of [%s:%d] error, caused by %s", host, port,
                    strerror(errno));
            freeaddrinfo(addrs);
            return RET_FAILED;
        }

        PNT_LOGD("BaseSocket the socketFd of [%s:%d] are :%d", host, port, sock->mSockFd);
        setSocketConfig(sock);
        setNonBlocking(sock->mSockFd);

        if (connect(sock->mSockFd, addr->ai_addr, addr->ai_addrlen) < 0)
        {
            if (errno != EINPROGRESS)
            {
                PNT_LOGE("BaseSocket fatal state error!!! connect to [%s:%d] error! errno:%d %s", host, port, errno, strerror(errno));
                freeaddrinfo(addrs);
                return RET_FAILED;
            }
            else
            {
                PNT_LOGE("BaseSocket connection to [%s:%d] may established", host, port);
                break;
            }
        }
    }
    freeaddrinfo(addrs);

    return RET_SUCCESS;
}

static int bSockConnectSock(PNTSocket_t* sock, char* host, int port)
{
    struct addrinfo *addrs = NULL;

    if (RET_FAILED == bSockBindAddress(sock, host, port, addrs))
    {
        PNT_LOGE("Socket[%s:%d] Bind failed.", host, port);
        close(sock->mSockFd);
        return RET_FAILED;
    }

    fd_set readfds;
    fd_set writefds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(sock->mSockFd, &readfds);
    FD_SET(sock->mSockFd, &writefds);
    timeout.tv_sec = 3; //timeout is 5 second
    timeout.tv_usec = 0;

    int ret = select(sock->mSockFd + 1, &readfds, &writefds, NULL, &timeout);
    if (ret <= 0)
    {
        PNT_LOGE("connection to [%s:%d] time out", host, port);
        return RET_FAILED;
    }

    if (FD_ISSET(sock->mSockFd, &readfds) ||FD_ISSET(sock->mSockFd, &writefds))
    {
        int error = 0;
        socklen_t length = sizeof(error);
        if (getsockopt(sock->mSockFd, SOL_SOCKET, SO_ERROR, &error, &length) < 0)
        {
            PNT_LOGE("get socket of [%s:%d] option failed", host, port);
            return RET_FAILED;
        }

        if (error != 0)
        {
            PNT_LOGE("connection of [%s:%d] failed after select with the error: %d %s", host, port, error, strerror(error));
            return RET_FAILED;
        }
    }
    else
    {
        PNT_LOGE("connection [%s:%d] no events on sockfd found", host, port);
        return RET_FAILED;
    }

    FD_CLR(sock->mSockFd, &readfds);
    FD_CLR(sock->mSockFd, &writefds);

    return RET_SUCCESS;
}

static int bSockOnEpollEvent(int fd, unsigned int event, void* arg)
{
    PNTEpoll_t* epoll = (PNTEpoll_t*)arg;
    PNTSocket_t* sock = (PNTSocket_t*)epoll->mOwner;

    if (NULL != sock)
    {
        sock->mTimeoutCounter = 0;
        
        if (event & EPOLLIN)
        {
            for (;;)
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
                    PNT_LOGE("0 Client close connection to [%s:%d], caused by %s", sock->mHost, sock->mPort, strerror(errno));
                    LibPnt_SocketClose(sock);
                    break;
                }
                else if (ret == 0)
                {
                    PNT_LOGE("1 Client close connection to [%s:%d], caused by %s", sock->mHost, sock->mPort, strerror(errno));
                    LibPnt_SocketClose(sock);
                    break;
                }
                else
                {
                    pthread_mutex_lock(&sock->mMutexRead);
                    
                    if (sock->mRcvDataLen + ret > sock->mRcvBufferSize)
                    {
                        sock->mRcvDataLen = 0;
                    }
                    memcpy(sock->mRcvBuffer+sock->mRcvDataLen, buffer, ret);
                    sock->mRcvDataLen += ret;

                    pthread_mutex_unlock(&sock->mMutexRead);
                }
            }
        }
        else if (event & EPOLLERR)
        {
            PNT_LOGD("EPOLLERR !");
        }
        else if (event & EPOLLOUT)
        {
        }
        else if (event & EPOLLRDHUP)
        {
            PNT_LOGD("EPOLLRDHUP");
        }
    }
	return 0;
}

static int bSockOnEpollTimeout(void* arg)
{
    PNTEpoll_t* epoll = (PNTEpoll_t*)arg;
    PNTSocket_t* sock = (PNTSocket_t*)epoll->mOwner;

    if (NULL != sock)
    {
        sock->mTimeoutCounter ++;
        if (sock->mTimeoutCounter >= sock->mTimeout/1000) // epoll 超时时间设置的1s
        {
            PNT_LOGE("connection [%s:%d][%d] timeout", sock->mHost, sock->mPort, sock->mSockFd);
            LibPnt_SocketClose(sock);
        }
    }
	return 0;
}

static void *bSockThreadLoop(void *arg)
{
    PNTSocket_t* sock = (PNTSocket_t*)arg;

    pthread_detach(pthread_self());

    pthread_mutex_lock(&sock->mMutex);

    sock->mRunning = TRUE;
    
    memset(&sock->mEpoll, 0, sizeof(sock->mEpoll));

    if(RET_FAILED == LibPnt_EpollCreate(&sock->mEpoll, MAX_EPOLL_SIZE, sock))
    {
        PNT_LOGE("[%s:%d] socket epoll_create error:%s", sock->mHost, sock->mPort, strerror(errno));
        sock->mRunning = FALSE;
        close(sock->mSockFd);
        goto exit;
    }

retry:
    
    if (RET_SUCCESS != bSockConnectSock(sock, sock->mHost, sock->mPort))
    {
        close(sock->mSockFd);

        if (!sock->mRunning)
        {
        	close(sock->mEpoll.mEpollFd);
            PNT_LOGE("the socket is stopped, do not need retry connecting.");
			goto exit;
        }
        else
        {
            SleepSs(5);
            goto retry;
        }
    }

    PNT_LOGI("the socket is connected, start to listen.");
    sock->mTimeoutCounter = 0;
    
    LibPnt_EpollAddEvent(&sock->mEpoll, sock->mSockFd, EPOLLIN | EPOLLET);
    LibPnt_EpollSetOnTimeout(&sock->mEpoll, bSockOnEpollTimeout);
    LibPnt_EpollSetOnEvent(&sock->mEpoll, bSockOnEpollEvent);
    LibPnt_EpollSetState(&sock->mEpoll, TRUE);

    sock->mConnectStatus = TRUE;
    if (NULL != sock->mOnConnectStatusChange)
    {
        sock->mOnConnectStatusChange(sock->mConnectStatus, sock);
    }

    int lastRcvDataLen = 0;

    while (sock->mRunning)
    {
        /* 接收数据处理 */
        if (sock->mRcvProcess && sock->mRcvDataLen > lastRcvDataLen)
        {
            pthread_mutex_lock(&sock->mMutexRead);
            
            int usedLen = sock->mRcvProcess(sock->mRcvBuffer, sock->mRcvDataLen, sock);
            if (usedLen >= sock->mRcvDataLen)
            {
                sock->mRcvDataLen = 0;
            }
            else
            {
                memcpy(sock->mRcvBuffer, sock->mRcvBuffer + usedLen, sock->mRcvDataLen - usedLen);
                sock->mRcvDataLen -= usedLen;
            }

            pthread_mutex_unlock(&sock->mMutexRead);

            lastRcvDataLen = sock->mRcvDataLen;
        }

        if (FALSE == sock->mConnectStatus)
        {
            if (sock->mRunning) // 重连
            {
                PNT_LOGI("the socket is disconnected, try to reconnected.");
                goto retry;
            }
        }
        else
        {
            SleepMs(10);
        }
    }

    LibPnt_SocketClose(sock);
	
    LibPnt_EpollClose(&sock->mEpoll);

exit:
    
    PNT_LOGE("[%s:%d] socket stop, task exit.", sock->mHost, sock->mPort);

    pthread_mutex_unlock(&sock->mMutex);
    pthread_exit(NULL);
    return NULL;
}

void LibPnt_SocketInit(PNTSocket_t* sock, char* host, int port, int timeout, void* owner)
{
    if (NULL == sock)
    {
        PNT_LOGE("paramete 1 cannot null.");
        return;
    }

    if(pthread_mutex_init(&sock->mMutex, NULL) != 0)
    {
        PNT_LOGE("[%s:%d]socket main mutex init failed.", host, port);
    }

    if(pthread_mutex_init(&sock->mMutexRead, NULL) != 0)
    {
        PNT_LOGE("[%s:%d]socket read mutex init failed.", host, port);
    }

    if(pthread_mutex_init(&sock->mMutexWrite, NULL) != 0)
    {
        PNT_LOGE("[%s:%d]socket write mutex init failed.", host, port);
    }

    memset(sock, 0, sizeof(PNTSocket_t));

    strcpy(sock->mHost, host);
    sock->mPort = port;
    sock->mOwner = owner;
    sock->mTimeout = timeout;

    sock->mRcvBufferSize = 4096;
    sock->mRcvBuffer = (char*)malloc(sock->mRcvBufferSize);
    if(NULL == sock->mRcvBuffer)
    {
        PNT_LOGE("[%s:%d]socket write buffer malloc failed.", host, port);
    }

    LibPnt_SocketSetConfig(sock, NULL);
}

void LibPnt_SocketSetConfig(PNTSocket_t* sock, SocketConfig_t* config)
{
    if (NULL == config)
    {
        config = &sock->mConfig;
        
        config->useNoDelay = TRUE;
        config->nodelay = TRUE;

        config->useReuseaddr = TRUE;
        config->resumeAddr = TRUE;

        config->useSendTimeout = TRUE;
        config->sendTimeout = kMaxTimeOut;

        config->useRecvTimeout = TRUE;
        config->recvTimeout = kMaxTimeOut;

        config->useSendBufSize = TRUE;
        config->sendBufSize = kMaxSocketBuffer;

        config->useRecvBufSize = TRUE;
        config->recvBufSize = kMaxSocketBuffer;
    }
    else
    {
        sock->mConfig = *config;
    }
}

int LibPnt_SocketStart(PNTSocket_t* sock)
{
    if (0 == sock->mPort)
    {
        PNT_LOGE("baseSocket not init.");
        return RET_FAILED;
    }

    if (sock->mRunning)
    {
        PNT_LOGE("the connection[%s:%d] is running.", sock->mHost, sock->mPort);
        return RET_SUCCESS;
    }

    int connThreadRet = pthread_create(&sock->mPid, NULL, bSockThreadLoop, sock);
    if (connThreadRet != 0)
    {
        PNT_LOGE("fail to create new thead of connection [%s:%d], caused by:%s", sock->mHost, sock->mPort, strerror(connThreadRet));
        return RET_FAILED;
    }

    return RET_SUCCESS;
}

void LibPnt_SocketClose(PNTSocket_t* sock)
{
    sock->mConnectStatus = FALSE;
    if (NULL != sock->mOnConnectStatusChange)
    {
        sock->mOnConnectStatusChange(sock->mConnectStatus, sock);
    }

    LibPnt_EpollDelEvent(&sock->mEpoll, sock->mSockFd, EPOLLIN | EPOLLET);

    close(sock->mSockFd);

    LibPnt_EpollSetState(&sock->mEpoll, FALSE);

    sock->mSockFd = -1;
    sock->mTimeoutCounter = 0;
}

void LibPnt_SocketStop(PNTSocket_t* sock)
{
    sock->mRunning = FALSE;

    pthread_mutex_lock(&sock->mMutex);
    
    pthread_mutex_unlock(&sock->mMutex);       // 等待线程退出

	pthread_mutex_destroy(&sock->mMutex);
	pthread_mutex_destroy(&sock->mMutexWrite);
	pthread_mutex_destroy(&sock->mMutexRead);

	if (NULL != sock->mRcvBuffer)
	{
		free(sock->mRcvBuffer);
	}

	PNT_LOGI("socket[%d] stop success.", sock->mSockFd);
}

void LibPnt_SocketSetServer(PNTSocket_t* sock, char* host, int port)
{
    if (NULL == sock)
    {
        PNT_LOGE("paramete 1 cannot null.");
        return;
    }

    strcpy(sock->mHost, host);
    sock->mPort = port;
}

void LibPnt_SocketSetRcvDataProcess(PNTSocket_t* sock, IRcvDataProcess rcvProcess)
{
    sock->mRcvProcess = rcvProcess;
}

void LibPnt_SocketSetOnConnStatusChange(PNTSocket_t* sock, OnConectStatusChange onConnStatusChange)
{
    sock->mOnConnectStatusChange = onConnStatusChange;
}

int LibPnt_SocketWrite(PNTSocket_t* sock, void* data, int size)
{
	if (FALSE == sock->mRunning)
	{
		return 0;
	}

    pthread_mutex_lock(&sock->mMutexWrite);

	int ret = write(sock->mSockFd, data, size);
	if (ret <= 0)
	{
		PNT_LOGE("send data failed %08X[%d].", data, size);
	}

    pthread_mutex_unlock(&sock->mMutexWrite);
	return ret;
}

