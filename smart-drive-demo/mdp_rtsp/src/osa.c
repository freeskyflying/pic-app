
#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <asm/ioctl.h>
#include <errno.h>
#include <linux/rtc.h>
#include <sys/syscall.h>
#include <sys/msg.h>
#include <asm/param.h>
#include <semaphore.h>
#include <sched.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/time.h>

#include <sys/shm.h>

#define LOG_TAG "app_osa"
#include <log/log.h>

#include "live_video_priv.h"

#include "systype.h"
#include "osa.h"



#ifndef _SEMUN_H
#define _SEMUN_H

union sem_val
{
  int val;
};

#endif

sdword
mutex_sem_init (key_t key)
{
  sdword sdwSemId = 0;
  union sem_val sem_union;

  sdwSemId = semget (key, 1, 0);
  if (-1 != sdwSemId)
    {
      return SYS_OK;
    }
  else
    {
      sdwSemId = semget (key, 1, 0666 | IPC_CREAT);
      if (-1 == sdwSemId)
	{
	  TRACE_ERROR("Semget Error!\n");
	  return SYS_ERROR;
	}

      sem_union.val = 1;
      if (semctl (sdwSemId, 0, SETVAL, sem_union) == -1)
	{
	  TRACE_ERROR("semctl Error!\n");
	  return SYS_ERROR;
	}

      return sdwSemId;
    }
}

sdword
mutex_sem_get (key_t key)
{
  sdword sdwSemId = 0;
  struct sembuf sem_b;

  sdwSemId = semget (key, 1, 0);
  if (-1 == sdwSemId)
    {
      return SYS_ERROR;
    }

  sem_b.sem_num = 0;
  sem_b.sem_op = -1;
  sem_b.sem_flg = SEM_UNDO;
  sem_b.sem_flg &= ~IPC_NOWAIT;

  if (semop (sdwSemId, &sem_b, 1) == -1)
    {
      return SYS_ERROR;
    }

  return SYS_OK;
}

sdword
mutex_sem_put (key_t key)
{
  sdword sdwSemId = 0;
  struct sembuf sem_b;

  sdwSemId = semget (key, 1, 0);
  if (-1 == sdwSemId)
    {
      return SYS_ERROR;
    }

  sem_b.sem_num = 0;
  sem_b.sem_op = 1;
  sem_b.sem_flg = SEM_UNDO;
  sem_b.sem_flg &= ~IPC_NOWAIT;

  if (semop (sdwSemId, &sem_b, 1) == -1)
    {
      return SYS_ERROR;
    }

  return SYS_OK;
}

sdword
shm_init (key_t key, size_t size)
{
  sdword sdwShmId = 0;

  sdwShmId = shmget (key, size, 0666 | IPC_CREAT);
  if (sdwShmId < 0)
    {
      OS_ERROR("shmget error");
      return SYS_ERROR;
    }

  return sdwShmId;
}

byte *
shm_get (key_t key)
{
  sdword sdwShmId = 0;
  byte * pbyShMPtr;

  sdwShmId = shmget (key, 0, 0);
  if (sdwShmId < 0)
    {
      TRACE_ERROR("shmget error\n"); TRACE_ERROR("error=%d %d\n",errno, EACCES);
      perror ("");
      return SYS_NULL;
    }

  pbyShMPtr = shmat (sdwShmId, 0, 0);
  if (pbyShMPtr == (svoid *) -1)
    {
      TRACE_ERROR("shmat error\n");
      return SYS_NULL;
    }

  return pbyShMPtr;
}

sdword
shm_put (svoid * pvPtr)
{
  return shmdt (pvPtr);
}

sdword
shm_deinit (key_t key)
{
  sdword sdwShmId = 0;

  sdwShmId = shmget (key, 0, 0);
  if (sdwShmId < 0)
    {
      OS_ERROR("shmget error");
      return SYS_ERROR;
    }

  if (shmctl (sdwShmId, IPC_RMID, 0) < 0)
    {
      OS_ERROR("shmctl error");
      return SYS_ERROR;
    }

  return SYS_OK;
}

