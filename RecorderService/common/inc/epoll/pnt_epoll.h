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

    pthread_t       mPid;                   /* �����߳̾�� */
    
    int             mEpollFd;               /* epoll��� */  
    bool_t          mLoop;                  /* �����߳�ѭ����־ */
    volatile bool_t mExit;                  /* �����߳��˳���־ */
    int             mEventSize;             /* ֧�������¼��� */
    unsigned int    mTimeout;               /* �¼��ȴ���ʱʱ�䣬Ĭ��1000ms */

    onEpollEvent    mOnEpollEvent;          /* �¼��ص������� */
    onEpollTimeout  mOnEpollTimeout;        /* ��ʱ�ص������� */

    void*           mOwner;                 /* ӵ���� */

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

/* ���ض�ʱ��fd */
int LibPnt_EpollCreateTimer(PNTEpoll_t* epoll, int timerInterval/* ms */);

int LibPnt_EpollDestroyTimer(PNTEpoll_t* epoll, int timerFd);

#ifdef __cplusplus
}
#endif

#endif
