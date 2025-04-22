#include <sys/epoll.h>
#include <sys/errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include "pnt_log.h"
#include "pnt_epoll.h"
#include "utils.h"

void* pntEpollLoopThread(void* arg)
{
    PNTEpoll_t* ePoll = (PNTEpoll_t*)arg;

    pthread_detach(pthread_self());
    
    struct epoll_event* events = NULL;

    ePoll->mExit = FALSE;

    int ret = 0, i = 0;
    
    PNT_LOGI("epoll[%d] start loop.", ePoll->mEpollFd);

    events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * ePoll->mEventSize);
    if (NULL == events)
    {
        PNT_LOGE("epoll[%d] events mem malloc failed.", ePoll->mEpollFd);
        ePoll->mLoop = FALSE;
    }

    while (ePoll->mLoop)
    {
        ret = epoll_wait(ePoll->mEpollFd, events, ePoll->mEventSize, ePoll->mTimeout);

        if(ret == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                break;
            }
        }
        else if(ret == 0)
        {//TIMEOUT
            if (ePoll->mOnEpollTimeout)
            {
                ePoll->mOnEpollTimeout(ePoll);
            }
            continue;
        }
        else
        {//NORMAL
            for(i = 0; i < ret; i++)
            {
                if (ePoll->mOnEpollEvent)
                {
                    ePoll->mOnEpollEvent(events[i].data.fd, events[i].events, ePoll);
                }
            }
        }
    }

    if (NULL != events)
    {
        free(events);
    }

    PNT_LOGI("epoll[%d] exist.", ePoll->mEpollFd);

    ePoll->mExit = TRUE;
    pthread_exit(NULL);
    return NULL;
}

int LibPnt_EpollCreate(PNTEpoll_t* ePoll, int eventSize, void* owner)
{
    int fd = epoll_create(eventSize);
    if(fd < 0)
    {
        PNT_LOGE("epoll_create error:%s", strerror(errno));
        return RET_FAILED;
    }
    
    memset(ePoll, 0, sizeof(PNTEpoll_t));

    ePoll->mEventSize = eventSize;
    ePoll->mEpollFd = fd;
    ePoll->mTimeout = 1000;
    ePoll->mOwner = owner;

	ePoll->mExit = TRUE;

    return RET_SUCCESS;
}

int LibPnt_EpollClose(PNTEpoll_t* ePoll)
{
    ePoll->mLoop = FALSE;
	
    while (FALSE == ePoll->mExit) SleepMs(10);       // 等待线程退出

    close(ePoll->mEpollFd);

    return RET_SUCCESS;
}

int LibPnt_EpollSetState(PNTEpoll_t* ePoll, bool_t state)
{
    ePoll->mLoop = state;

    if (state)
    {
	    if(ePoll->mEpollFd < 0)
	    {
	        return RET_FAILED;
	    }
        int ret = pthread_create(&ePoll->mPid, NULL, pntEpollLoopThread, ePoll);
        if (ret != 0)
        {
            PNT_LOGE("fail to create thead of epoll[%d] caused by:%s", ePoll->mEpollFd, strerror(ret));
            return RET_FAILED;
        }
    }

    return RET_SUCCESS;
}

int LibPnt_EpollSetTimeout(PNTEpoll_t* ePoll, int timeout)
{
    ePoll->mTimeout = timeout;

    return RET_SUCCESS;
}

int LibPnt_EpollAddEvent(PNTEpoll_t* ePoll, int fd, unsigned int event)
{
    if(ePoll->mEpollFd < 0)
    {
        PNT_LOGE("epoll not create.");
        return RET_FAILED;
    }

    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = event;

    if(epoll_ctl(ePoll->mEpollFd, EPOLL_CTL_ADD, fd, &ev) < 0)
    {
        PNT_LOGE("epoll_ctl EPOLL_CTL_ADD error:%s %d %d", strerror(errno), fd, ePoll->mEpollFd);
        return RET_FAILED;
    }

    return RET_SUCCESS;
}

int LibPnt_EpollUpdateEvent(PNTEpoll_t* ePoll, int fd, unsigned int event)
{
    if(ePoll->mEpollFd < 0)
    {
        PNT_LOGE("epoll not create.");
        return RET_FAILED;
    }

    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = event;

    if(epoll_ctl(ePoll->mEpollFd, EPOLL_CTL_MOD, fd, &ev) < 0)
    {
        PNT_LOGE("PNT_LOGE EPOLL_CTL_MOD error:%s %d", strerror(errno), fd);
        return RET_FAILED;
    }

    return RET_SUCCESS;
}

int LibPnt_EpollDelEvent(PNTEpoll_t* ePoll, int fd, unsigned int event)
{
    if(ePoll->mEpollFd < 0)
    {
        PNT_LOGE("epoll not create.");
        return RET_FAILED;
    }

    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = event;

    if(epoll_ctl(ePoll->mEpollFd, EPOLL_CTL_DEL, fd, &ev) < 0)
    {
        PNT_LOGE("epoll_ctl EPOLL_CTL_DEL error:%s %d %d", strerror(errno), fd, ePoll->mEpollFd);
        return RET_FAILED;
    }

    return RET_SUCCESS;
}

int LibPnt_EpollSetOnTimeout(PNTEpoll_t* ePoll, onEpollTimeout onTimeout)
{
    ePoll->mOnEpollTimeout = onTimeout;

    return RET_SUCCESS;
}

int LibPnt_EpollSetOnEvent(PNTEpoll_t* ePoll, onEpollEvent onEvent)
{
    ePoll->mOnEpollEvent = onEvent;

    return RET_SUCCESS;
}

int LibPnt_EpollCreateTimer(PNTEpoll_t* epoll, int timerInterval/* ms */)
{
	int timerFd;
	struct itimerspec timerSpec;
	
	//下面给定时器设定定时时间
	timerSpec.it_value.tv_sec = 1;
	timerSpec.it_value.tv_nsec = 0;
	timerSpec.it_interval.tv_sec = timerInterval/1000;
	timerSpec.it_interval.tv_nsec = (timerInterval%1000)*1000000;

	PNT_LOGI("Epoll[%d] timer[%d s, %d nsec]", epoll->mEpollFd, timerSpec.it_interval.tv_sec, timerSpec.it_interval.tv_nsec);

	// CLOCK_REALTIME 系统范围的实时时钟,设置此时钟需要适当的权限。 
	// CLOCK_MONOTONIC 无法设置的时钟，表示自某些未指定的起点以来的单调时间。  
	// CLOCK_PROCESS_CPUTIME_ID	来自CPU的高分辨率每进程计时器。
	// CLOCK_THREAD_CPUTIME_ID	 特定于线程的CPU时钟。
	timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
	
	timerfd_settime(timerFd, 0, &timerSpec, NULL);

	LibPnt_EpollAddEvent(epoll, timerFd, EPOLLIN);

	return timerFd;
}

int LibPnt_EpollDestroyTimer(PNTEpoll_t* epoll, int timerFd)
{
	LibPnt_EpollDelEvent(epoll, timerFd, EPOLLIN);

	close(timerFd);

	return RET_SUCCESS;
}


