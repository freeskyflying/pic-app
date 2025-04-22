#ifndef _PNT_EPOLL_H_
#define _PNT_EPOLL_H_

#include <pthread.h>
#include "typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int(*onEpollEvent)(int fd, unsigned int event, void* arg);
typedef int(*onEpollTimeout)(void* epoll);

typedef struct
{

    pthread_t       mPid;                   /* 处理线程句柄 */
    
    int             mEpollFd;               /* epoll句柄 */  
    bool_t          mLoop;                  /* 处理线程循环标志 */
    volatile bool_t mExit;                  /* 处理线程退出标志 */
    int             mEventSize;             /* 支持最大的事件数 */
    unsigned int    mTimeout;               /* 事件等待超时时间，默认1000ms */

    onEpollEvent    mOnEpollEvent;          /* 事件回调处理函数 */
    onEpollTimeout  mOnEpollTimeout;        /* 超时回调处理函数 */

    void*           mOwner;                 /* 拥有者 */

} PNTEpoll_t;

int LibPnt_EpollCreate(PNTEpoll_t* epoll, int eventSize, void* owner);

int LibPnt_EpollSetState(PNTEpoll_t* epoll, bool_t state/* TRUE: start FALSE: stop */);

int LibPnt_EpollClose(PNTEpoll_t* epoll);

int LibPnt_EpollSetTimeout(PNTEpoll_t* epoll, int timeout);

int LibPnt_EpollAddEvent(PNTEpoll_t* epoll, int fd, unsigned int event);

int LibPnt_EpollUpdateEvent(PNTEpoll_t* epoll, int fd, unsigned int event);

int LibPnt_EpollDelEvent(PNTEpoll_t* epoll, int fd, unsigned int event);

int LibPnt_EpollSetOnTimeout(PNTEpoll_t* epoll, onEpollTimeout onTimeout);

int LibPnt_EpollSetOnEvent(PNTEpoll_t* epoll, onEpollEvent onEvent);

/* 返回定时器fd */
int LibPnt_EpollCreateTimer(PNTEpoll_t* epoll, int timerInterval/* ms */);

int LibPnt_EpollDestroyTimer(PNTEpoll_t* epoll, int timerFd);

#ifdef __cplusplus
}
#endif

#endif
