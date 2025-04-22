

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define LOG_TAG "app_mem_pool"
#include <log/log.h>

#include "live_video_priv.h"

#include "mem_pool.h"




typedef struct 
{
	dword dwMemPoolBaseAddr;			
	dword dwMemPoolHeadId;				
	dword dwMemPoolNodeOffset;			
	dword dwMemBlockCounter;
	dword dwMemBlockNum;				
	dword dwMemBlockSize;				
	
}mem_pool_list_head_s;



typedef struct 
{
	dword dwMemPoolBaseOffset;			
	dword dwMemPoolHeadId;			
	dword dwNodeMemSize;				
	dword dwUsed;						
	byte byBuffer[0];					
	
}mem_pool_list_node_s;



static svoid mem_pool_info_print(mem_pool_dsc_s * pstMemPoolDsc)
{
	
}



static sdword mem_pool_list_init(mem_pool_list_head_s * pstMemPoolListHead)
{

	dword dwCounter;
	dword dwMemPoolListOffsetAddr = 0;
	byte * pbyShMPoolBase;
	mem_pool_list_node_s * pstMemPoolListNode;
	dword dwMemBlockNum;
	dword dwMemBlockSize;

	dwMemBlockSize = pstMemPoolListHead->dwMemBlockSize;
	dwMemBlockNum = pstMemPoolListHead->dwMemBlockNum;


	if(dwMemBlockNum == 0)				
	{
		return SYS_OK;
	}

	pbyShMPoolBase = (byte *)(pstMemPoolListHead->dwMemPoolBaseAddr + pstMemPoolListHead->dwMemPoolNodeOffset);

	for(dwCounter = 0; dwCounter < dwMemBlockNum; dwCounter++)
	{
		dwMemPoolListOffsetAddr = (sizeof(mem_pool_list_node_s) + dwMemBlockSize) * dwCounter;
		
		pstMemPoolListNode = (mem_pool_list_node_s *)(pbyShMPoolBase + dwMemPoolListOffsetAddr);

		pstMemPoolListNode->dwUsed = 0;
		pstMemPoolListNode->dwMemPoolBaseOffset = dwMemPoolListOffsetAddr + pstMemPoolListHead->dwMemPoolNodeOffset;
		pstMemPoolListNode->dwMemPoolHeadId = pstMemPoolListHead->dwMemPoolHeadId;

	}

	return SYS_OK;
}


static sdword mem_pool_list_alloc(mem_pool_list_head_s * pstMemPoolListHead, byte ** ppbyBuffer)
{
	sdword sdwRetn = SYS_ERROR;
	dword dwCounter;
	byte * pbyMemPoolListBase;
	mem_pool_list_node_s * pstMemPoolListNode;

	pbyMemPoolListBase = (byte *)(pstMemPoolListHead->dwMemPoolBaseAddr + pstMemPoolListHead->dwMemPoolNodeOffset);

	if(pstMemPoolListHead->dwMemBlockCounter > 0)	
	{
		for(dwCounter = 0; dwCounter < pstMemPoolListHead->dwMemBlockNum; dwCounter++)
		{
			pstMemPoolListNode = (mem_pool_list_node_s *)(pbyMemPoolListBase + (sizeof(mem_pool_list_node_s) + pstMemPoolListHead->dwMemBlockSize) * dwCounter);

			if(pstMemPoolListNode->dwUsed == 0)
			{
				*ppbyBuffer = pstMemPoolListNode->byBuffer - pstMemPoolListHead->dwMemPoolBaseAddr;
				pstMemPoolListNode->dwUsed = 1;
				pstMemPoolListHead->dwMemBlockCounter--;

				sdwRetn = SYS_OK;

				break;
			}
		}
	}
	
	return sdwRetn;
}




sdword mem_pool_init(mem_pool_dsc_s * pstMemPoolDsc)
{
	dword dwCounter;
	dword dwReqMemPoolSize = 0;	
	dword dwReqMemBlockSize = 0;
	byte * pbyMemPoolBase;									
	dword dwMemPoolSize;									
	sdword sdwRetn = SYS_OK;
	mem_pool_list_head_s * pstMemPoolListHead[18];			
	mem_pool_dsc_s * pstMemPoolDscTemp;
	dword dwAddrOffset = 0;

	mem_pool_info_print(pstMemPoolDsc);

	if((pstMemPoolDsc->dwMemBlockType == 0) || (pstMemPoolDsc->dwMemBlockType > 18))
	{
		TRACE_ERROR("MemPoolDsc Para Error\n");
		return SYS_ERROR;
	}
	
	
	pbyMemPoolBase = pstMemPoolDsc->pbyMemPoolBase;
	dwMemPoolSize = pstMemPoolDsc->dwMemPoolSize;

	dwReqMemPoolSize += sizeof(mem_pool_dsc_s);
	for(dwCounter = 0; dwCounter < pstMemPoolDsc->dwMemBlockType; dwCounter++)
	{
		dwReqMemPoolSize += sizeof(mem_pool_list_head_s);
		dwReqMemBlockSize = sizeof(mem_pool_list_node_s) + pstMemPoolDsc->stMemBlockDsc[dwCounter].dwMemBlockSize;
		dwReqMemPoolSize += dwReqMemBlockSize * pstMemPoolDsc->stMemBlockDsc[dwCounter].dwMemBlockNum;
	}
	if(dwReqMemPoolSize > dwMemPoolSize)
	{
		TRACE_ERROR("dwReqMemPoolSize > pstMemPoolDsc->dwMemPoolSize\n");
		return SYS_ERROR;
	}
	
	
	pstMemPoolDscTemp = (mem_pool_dsc_s *)(pbyMemPoolBase);
	memcpy(pstMemPoolDscTemp, pstMemPoolDsc, sizeof(mem_pool_dsc_s));
	
	pbyMemPoolBase += sizeof(mem_pool_dsc_s);


	dwAddrOffset += sizeof(mem_pool_dsc_s);
	dwAddrOffset += sizeof(mem_pool_list_head_s) * pstMemPoolDsc->dwMemBlockType;
			

	for(dwCounter = 0; dwCounter < pstMemPoolDsc->dwMemBlockType; dwCounter++)
	{
		pstMemPoolListHead[dwCounter] = (mem_pool_list_head_s *)(pbyMemPoolBase + (sizeof(mem_pool_list_head_s) * dwCounter));

		pstMemPoolListHead[dwCounter]->dwMemBlockNum = pstMemPoolDsc->stMemBlockDsc[dwCounter].dwMemBlockNum;
		pstMemPoolListHead[dwCounter]->dwMemBlockCounter = pstMemPoolDsc->stMemBlockDsc[dwCounter].dwMemBlockNum;
		pstMemPoolListHead[dwCounter]->dwMemBlockSize = pstMemPoolDsc->stMemBlockDsc[dwCounter].dwMemBlockSize;
		pstMemPoolListHead[dwCounter]->dwMemPoolBaseAddr = (dword)pstMemPoolDsc->pbyMemPoolBase;
		pstMemPoolListHead[dwCounter]->dwMemPoolHeadId = dwCounter;		

		if(dwCounter == 0)
		{
			dwAddrOffset += 0;
		}
		else
		{
			dwAddrOffset += (sizeof(mem_pool_list_node_s) + (pstMemPoolListHead[dwCounter - 1]->dwMemBlockSize)) * (pstMemPoolListHead[dwCounter - 1]->dwMemBlockNum);
		}

		pstMemPoolListHead[dwCounter]->dwMemPoolNodeOffset = dwAddrOffset;
	}


	for(dwCounter = 0; dwCounter < pstMemPoolDsc->dwMemBlockType; dwCounter++)
	{
		sdwRetn |= mem_pool_list_init(pstMemPoolListHead[dwCounter]);
	}
	

	if(sdwRetn != SYS_OK)
	{
		TRACE_ERROR("MemPoolInit Error\n");
		return SYS_ERROR;
	}

	return SYS_OK;
}

svoid * mem_pool_malloc(mem_pool_dsc_s * pstMemPoolDsc, dword dwMemSize)
{
	dword dwCounter;
	byte * pbyBuffer;
	byte * pbyMemPoolBase;									
	mem_pool_dsc_s * pstMemPoolDscTemp;						
	mem_pool_list_head_s * pstMemPoolListHead[18];			
	sdword sdwRetn = SYS_ERROR;


	pbyMemPoolBase = pstMemPoolDsc->pbyMemPoolBase;


	pstMemPoolDscTemp = (mem_pool_dsc_s *)(pbyMemPoolBase);
	
	
	pbyMemPoolBase += sizeof(mem_pool_dsc_s);

	for(dwCounter = 0; dwCounter < pstMemPoolDscTemp->dwMemBlockType; dwCounter++)
	{
		pstMemPoolListHead[dwCounter] = (mem_pool_list_head_s *)(pbyMemPoolBase + (sizeof(mem_pool_list_head_s) * dwCounter));

		pstMemPoolListHead[dwCounter]->dwMemPoolBaseAddr = (dword)pstMemPoolDsc->pbyMemPoolBase;

		if(dwMemSize <= pstMemPoolListHead[dwCounter]->dwMemBlockSize)
		{
			sdwRetn = mem_pool_list_alloc(pstMemPoolListHead[dwCounter], &pbyBuffer);
			if(sdwRetn == SYS_OK)
			{
				break;
			}
		}
	}
	
	if(sdwRetn == SYS_OK)
	{
		return (svoid *)(pbyBuffer);
	}
	else
	{
		return SYS_NULL;
	}
}



sdword mem_pool_free(svoid * pvBuffer)
{	
	mem_pool_list_node_s * pstMemPoolListNode;
	mem_pool_list_head_s * pstMemPoolListHead;
 	byte * pbyMemPoolBase;									
	
	pstMemPoolListNode = (mem_pool_list_node_s *)(((dword)pvBuffer) - sizeof(mem_pool_list_node_s));

	pbyMemPoolBase = (byte *)((dword)pstMemPoolListNode - pstMemPoolListNode->dwMemPoolBaseOffset);
	pbyMemPoolBase += sizeof(mem_pool_dsc_s);
	pbyMemPoolBase += sizeof(mem_pool_list_head_s) * pstMemPoolListNode->dwMemPoolHeadId;
	
	pstMemPoolListNode->dwUsed = 0;

	pstMemPoolListHead = (mem_pool_list_head_s *)pbyMemPoolBase;
	pstMemPoolListHead->dwMemBlockCounter++;

	return SYS_OK;
}

