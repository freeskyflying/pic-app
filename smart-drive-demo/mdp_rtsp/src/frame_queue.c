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
#include "systype.h"

#define LOG_TAG "app_frame_queue"
#include <log/log.h>

#include "live_video_priv.h"

#include "frame_queue.h"
#include "mem_pool.h"
#include "osa.h"

#define		FRAME_QUEUE_SHM_KEY_BASE		9500

#define		FRAME_QUEUE_SEM_KEY_BASE		5020

#define		FRAME_QUEUE_SYNC_SEM_KEY_PATH	"/tmp/"

#define		MEM_POOL_START_OFFSET			4096

#define		MEM_BLOCK_SIZE_2048K			0x200000
#define		MEM_BLOCK_SIZE_1024K			0x100000
#define		MEM_BLOCK_SIZE_512K				0x80000
#define		MEM_BLOCK_SIZE_256K				0x40000
#define		MEM_BLOCK_SIZE_128K				0x20000

#define		MEM_BLOCK_SIZE_16K				0x4000      /*20*/
#define		MEM_BLOCK_SIZE_32K				0x8000      /*20*/
#define		MEM_BLOCK_SIZE_64K				0x10000     /*30*/
#define		MEM_BLOCK_SIZE_128K				0x20000     /*30*/
//0x6E0000

typedef struct
{
	dword dwFrameMemPoolSize;
	dword dwFrameMemBlockType;
	mem_block_dsc_s stMemBlockDsc[6];

}frame_mem_pool_dsc_s;

byte * g_pbyFrameMemPoolBase[FRAME_QUEUE_NUM_MAX] = {NULL};

frame_mem_pool_dsc_s g_stFrameMemPoolDsc =
{
	.dwFrameMemPoolSize = 0x1610000,
	.dwFrameMemBlockType = 4,
	.stMemBlockDsc = {
		[0] = {
			.dwMemBlockSize 	= MEM_BLOCK_SIZE_128K,
			.dwMemBlockNum		= 16,
		},
		[1] = {
			.dwMemBlockSize 	= MEM_BLOCK_SIZE_256K,
			.dwMemBlockNum		= 8,
		},
		[2] = {
			.dwMemBlockSize 	= MEM_BLOCK_SIZE_1024K,
			.dwMemBlockNum		= 10,
		},
		[3] = {
			.dwMemBlockSize 	= MEM_BLOCK_SIZE_2048K,
			.dwMemBlockNum		= 4,
		},
    }
};

typedef	struct
{
	dword dwQueueLen;
	dword dwCounter;
	dword dwReadPos;
	dword dwWritePos;
	dword dwQueue[128];

}frame_queue_dsc_s;

static sdword frame_mem_pool_create(dword dwFrameMemPoolNum, dword dwMemSize)
{
	key_t ShMKey;
	sdword sdwShmId;


	ShMKey	= dwFrameMemPoolNum + FRAME_QUEUE_SHM_KEY_BASE;

	sdwShmId = shm_init(ShMKey, dwMemSize);
	if(sdwShmId == SYS_ERROR)
	{
		TRACE_ERROR("ShMInit error\n");
		return SYS_ERROR;
	}

	return SYS_OK;
}

static byte * frame_mem_pool_get_base(dword dwFrameMemPoolNum)
{
	key_t ShMKey;
	byte * pbyShMPtr;


	if(g_pbyFrameMemPoolBase[dwFrameMemPoolNum] != NULL)
	{
	}
	else
	{

		ShMKey	= dwFrameMemPoolNum + FRAME_QUEUE_SHM_KEY_BASE;

		pbyShMPtr = shm_get(ShMKey);
		if(pbyShMPtr == SYS_NULL)
		{
			TRACE_ERROR("ShMGetPtr error\n");
			return SYS_NULL;
		}

		g_pbyFrameMemPoolBase[dwFrameMemPoolNum] = pbyShMPtr;
	}

	return g_pbyFrameMemPoolBase[dwFrameMemPoolNum];
}

static sdword frame_mem_pool_init(dword dwFrameMemPoolNum, frame_mem_pool_dsc_s * pstFrameMemPoolDsc)
{
	sdword sdwRetn;
	dword dwCounter;
	byte * pbyFrameMemPoolBase;
	mem_pool_dsc_s stMemPoolDsc;


	sdwRetn = frame_mem_pool_create(dwFrameMemPoolNum, pstFrameMemPoolDsc->dwFrameMemPoolSize);
	if(sdwRetn == SYS_ERROR)
	{
		TRACE_ERROR("FrameMemPoolCreate Error\n");
		return SYS_ERROR;
	}

	pbyFrameMemPoolBase = frame_mem_pool_get_base(dwFrameMemPoolNum);


	stMemPoolDsc.pbyMemPoolBase = pbyFrameMemPoolBase + MEM_POOL_START_OFFSET;
	stMemPoolDsc.dwMemPoolSize = pstFrameMemPoolDsc->dwFrameMemPoolSize - MEM_POOL_START_OFFSET;
	stMemPoolDsc.dwMemBlockType = pstFrameMemPoolDsc->dwFrameMemBlockType;

	for(dwCounter = 0; dwCounter < pstFrameMemPoolDsc->dwFrameMemBlockType; dwCounter++)
	{
		stMemPoolDsc.stMemBlockDsc[dwCounter].dwMemBlockSize = pstFrameMemPoolDsc->stMemBlockDsc[dwCounter].dwMemBlockSize;
		stMemPoolDsc.stMemBlockDsc[dwCounter].dwMemBlockNum = pstFrameMemPoolDsc->stMemBlockDsc[dwCounter].dwMemBlockNum;
	}

	return mem_pool_init(&stMemPoolDsc);
}

static svoid * frame_mem_pool_alloc(dword dwFrameMemPoolNum, dword dwAllocMemSize)
{
	byte * pbyShMPtr;
	byte * pbyBuffer;
	mem_pool_dsc_s stMemPoolDsc;

	pbyShMPtr = frame_mem_pool_get_base(dwFrameMemPoolNum);


	stMemPoolDsc.pbyMemPoolBase = pbyShMPtr + MEM_POOL_START_OFFSET;


	pbyBuffer = mem_pool_malloc(&stMemPoolDsc, dwAllocMemSize);
	if(pbyBuffer == SYS_NULL)
	{
		TRACE_ERROR("MemPoolMalloc error dwAllocMemSize = %d\n",dwAllocMemSize);
		return SYS_NULL;
	}
	else
	{
		return (pbyBuffer + (dword)stMemPoolDsc.pbyMemPoolBase);
	}
}

static sdword frame_mem_pool_free(svoid * pvBuffer)
{
	mem_pool_free(pvBuffer);
	return SYS_OK;
}

sdword frame_queue_init_default()
{
	int pool[FRAME_QUEUE_NUM_MAX] = {0};
	for(int i = 0; i < FRAME_QUEUE_NUM_MAX; i++)
	{
		pool[i] = FRAME_QUEUE_SIZE;
	}

	return frame_queue_init(FRAME_QUEUE_NUM_MAX, pool);
}

sdword frame_queue_init(int pool_size, int *queue_depth)
{
	frame_mem_pool_dsc_s * pstFrameMemPoolDsc;
	frame_queue_dsc_s * pstFrameQueueDsc = NULL;
	byte * pbyBuffer;
	dword dwCounter;
	TRACE_DEBUG("FrameQueueInit\n");
	for(dwCounter = 0; dwCounter < pool_size; dwCounter++)
	{
		if(SYS_ERROR == mutex_sem_init(FRAME_QUEUE_SEM_KEY_BASE + dwCounter))
		{
			TRACE_ERROR("MutexSemInit Error\n");
			return SYS_ERROR;
		}

		pstFrameMemPoolDsc = &g_stFrameMemPoolDsc;

		if(SYS_ERROR == frame_mem_pool_init(dwCounter, pstFrameMemPoolDsc))
		{
			TRACE_ERROR("FrameMemPoolInit Error\n");
			return SYS_ERROR;
		}

		pbyBuffer = frame_mem_pool_get_base(dwCounter);
		pstFrameQueueDsc = (frame_queue_dsc_s *)pbyBuffer;

		bzero(pstFrameQueueDsc, sizeof(frame_queue_dsc_s));
		pstFrameQueueDsc->dwQueueLen = queue_depth[dwCounter];

		g_pbyFrameMemPoolBase[dwCounter] = NULL;
	}

	return SYS_OK;
}


sdword frame_queue_get_node_frame(dword dwFrameQueueNum, frame_dsc_s * pstFrameDsc, frame_queue_wait_e eFrameQueueWait)
{
	byte * pbyBuffer;
	byte * pbyBuffer1;
	frame_queue_dsc_s * pstFrameQueueDsc = NULL;
	frame_dsc_s * pstFrameDscTemp;
	sdword sdwRetn;

	if(dwFrameQueueNum >= FRAME_QUEUE_NUM_MAX)
	{
		TRACE_ERROR("dwFrameQueueNum Error\n");
		return SYS_ERROR;
	}

	if(eFrameQueueWait == FRAME_QUEUE_WAIT)
	{
	    TRACE_ERROR("FRAME_QUEUE_WAIT no support\n");
	    return SYS_ERROR;
	}
	else
	{
	}


	mutex_sem_get(FRAME_QUEUE_SEM_KEY_BASE + dwFrameQueueNum);

	pbyBuffer1 = frame_mem_pool_get_base(dwFrameQueueNum);

	if(NULL == pbyBuffer1)
	{
		mutex_sem_put(FRAME_QUEUE_SEM_KEY_BASE + dwFrameQueueNum);
		return SYS_ERROR;
	}

	pstFrameQueueDsc = (frame_queue_dsc_s *)pbyBuffer1;

	pbyBuffer1 += MEM_POOL_START_OFFSET;

	if(pstFrameQueueDsc->dwCounter > 0)
	{
		pbyBuffer = (byte *)pstFrameQueueDsc->dwQueue[pstFrameQueueDsc->dwReadPos];
		pbyBuffer += (dword)pbyBuffer1;
		pstFrameDscTemp = (frame_dsc_s *)pbyBuffer;
		memcpy((byte *)pstFrameDsc, (byte *)pstFrameDscTemp, sizeof(frame_dsc_s) - 4);
		memcpy(pstFrameDsc->pbyData, pstFrameDscTemp->byData, pstFrameDscTemp->dwDataLen);
		frame_mem_pool_free(pbyBuffer);
		pstFrameQueueDsc->dwReadPos = (pstFrameQueueDsc->dwReadPos + 1) % pstFrameQueueDsc->dwQueueLen;
		pstFrameQueueDsc->dwCounter--;

		sdwRetn = SYS_OK;
  	}
	else
	{
		sdwRetn = SYS_ERROR;
  	}
	mutex_sem_put(FRAME_QUEUE_SEM_KEY_BASE + dwFrameQueueNum);

	return sdwRetn;
}

sdword frame_queue_counter(dword dwFrameQueueNum)
{
	byte * pbyBuffer;
	frame_queue_dsc_s * pstFrameQueueDsc = NULL;

	pstFrameQueueDsc = (frame_queue_dsc_s *)frame_mem_pool_get_base(dwFrameQueueNum);
	if(NULL == pstFrameQueueDsc)
		return SYS_ERROR;
	else
		return pstFrameQueueDsc->dwCounter;
}

sdword frame_queue_push_node(dword dwFrameQueueNum, frame_dsc_s * pstFrameDsc, dword dwTimeOut)
{
	sdword sdwRetn = SYS_ERROR;
	byte * pbyBuffer;
	byte * pbyData;
	frame_queue_dsc_s * pstFrameQueueDsc = NULL;
	dword dwCounter = 0;

	(void)dwCounter;
	if(dwFrameQueueNum >= FRAME_QUEUE_NUM_MAX)
	{
		TRACE_ERROR("dwFrameQueueNum Error\n");
		return SYS_ERROR;
	}

	if(pstFrameDsc == SYS_NULL)
	{
		TRACE_ERROR("pstFrameDsc == NULL\n");
		return SYS_ERROR;
	}

	pbyBuffer = frame_mem_pool_get_base(dwFrameQueueNum);
	if(NULL == pbyBuffer)
		return SYS_ERROR;
	pstFrameQueueDsc = (frame_queue_dsc_s *)pbyBuffer;

	pbyBuffer += MEM_POOL_START_OFFSET;

	while(1)
	{
		if(pstFrameQueueDsc->dwCounter < pstFrameQueueDsc->dwQueueLen)
		{
			break;
		}
		else
		{
				/*TRACE_DEBUG_RATE(100,"FrameQueue full \n");*/
				return SYS_ERROR;
		}
	}


	mutex_sem_get(FRAME_QUEUE_SEM_KEY_BASE + dwFrameQueueNum);

	if(pstFrameQueueDsc->dwCounter < pstFrameQueueDsc->dwQueueLen)
	{
		pbyData = frame_mem_pool_alloc(dwFrameQueueNum, sizeof(frame_dsc_s) + pstFrameDsc->dwDataLen);

		if(pbyData != SYS_NULL)
		{
			memcpy(pbyData, pstFrameDsc, sizeof(frame_dsc_s));
			memcpy(pbyData + sizeof(frame_dsc_s), pstFrameDsc->pbyData, pstFrameDsc->dwDataLen);

			pstFrameQueueDsc->dwQueue[pstFrameQueueDsc->dwWritePos] = (dword)pbyData  - (dword)pbyBuffer;

			pstFrameQueueDsc->dwWritePos = (pstFrameQueueDsc->dwWritePos + 1) % pstFrameQueueDsc->dwQueueLen;

			pstFrameQueueDsc->dwCounter++;

			sdwRetn = SYS_OK;
		}
		else
		{
			TRACE_DEBUG("pbyData == NULL\n");
			sdwRetn = SYS_ERROR;
		}
	}
	else
	{
		TRACE_DEBUG("dwCounter >= dwQueueLen\n");
		sdwRetn = SYS_ERROR;
	}

	mutex_sem_put(FRAME_QUEUE_SEM_KEY_BASE + dwFrameQueueNum);

	return sdwRetn;
}

sbool frame_queue_is_empty(dword dwFrameQueueNum)
{
	sdword sdwRetn;
	byte * pbyBuffer;
	frame_queue_dsc_s * pstFrameQueueDsc = NULL;

	if(dwFrameQueueNum >= FRAME_QUEUE_NUM_MAX)
	{
		TRACE_ERROR("dwFrameQueueNum Error\n");
		return SYS_ERROR;
	}



	pbyBuffer = frame_mem_pool_get_base(dwFrameQueueNum);
	pstFrameQueueDsc = (frame_queue_dsc_s *)pbyBuffer;


	if(pstFrameQueueDsc->dwCounter > 0)
	{
		sdwRetn = SYS_OK;
  	}
	else
	{
		sdwRetn = SYS_ERROR;
  	}


	if(sdwRetn == SYS_OK)
		return SYS_FAILS;
	else
		return SYS_TRUE;
}

sdword frame_queue_flush(dword dwFrameQueueNum)
{
	byte * pbyBuffer;
	byte * pbyBuffer1;
	frame_queue_dsc_s * pstFrameQueueDsc = NULL;

	if(dwFrameQueueNum >= FRAME_QUEUE_NUM_MAX)
	{
		TRACE_ERROR("dwFrameQueueNum Error\n");
		return SYS_ERROR;
	}

	mutex_sem_get(FRAME_QUEUE_SEM_KEY_BASE + dwFrameQueueNum);


	pbyBuffer1 = frame_mem_pool_get_base(dwFrameQueueNum);
	pstFrameQueueDsc = (frame_queue_dsc_s *)pbyBuffer1;
	pbyBuffer1 += MEM_POOL_START_OFFSET;


	while(pstFrameQueueDsc->dwCounter > 0)
	{
		pbyBuffer = (byte *)pstFrameQueueDsc->dwQueue[pstFrameQueueDsc->dwReadPos];
		pbyBuffer += (dword)pbyBuffer1;

		frame_mem_pool_free(pbyBuffer);

		pstFrameQueueDsc->dwReadPos = (pstFrameQueueDsc->dwReadPos + 1) % pstFrameQueueDsc->dwQueueLen;
		pstFrameQueueDsc->dwCounter--;
	}


	mutex_sem_put(FRAME_QUEUE_SEM_KEY_BASE + dwFrameQueueNum);

	return SYS_OK;
}

