#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#include "pnt_log.h"
#include "utils.h"

#include "algo_common.h"
#include "adas_nn.h"
#include "dms_nn.h"
#include "media_video.h"
#include "system_param.h"

AlCommonParam_t gAlComParam = { 0 };

int Vbuf_Pool_init(VBUF_CONFIG_S *pstVbConfig)
{
	int s32Ret = 0;
	VBUF_POOL_CONFIG_S stPoolCfg = {0};
	pool_info_t* pool_info = gAlComParam.pool_info;

	if (pstVbConfig->u32PoolCnt > VBUF_POOL_MAX_NUM)
	{
		PNT_LOGE("VBUF MAX ERROE %s %d %d\n", __FILE__, __LINE__, pstVbConfig->u32PoolCnt);
		return RET_FAILED;
	}
	
	EI_MI_MBASE_Init();
    EI_MI_VBUF_Init();
    EI_MI_MLINK_Init();
	
	for (int u32Idx = 0; u32Idx < pstVbConfig->u32PoolCnt; ++u32Idx)
	{
	    stPoolCfg.u32BufSize = EI_MI_VBUF_GetPicBufferSize(&pstVbConfig->astFrameInfo[u32Idx]);
	    stPoolCfg.u32BufCnt = pstVbConfig->astCommPool[u32Idx].u32BufCnt;
	    stPoolCfg.enRemapMode = pstVbConfig->astCommPool[u32Idx].enRemapMode;
	    stPoolCfg.enVbufUid = VBUF_UID_COMMON;
	    strcpy(stPoolCfg.acPoolName, "common");
		
    	pool_info[u32Idx].Pool = EI_MI_VBUF_CreatePool(&stPoolCfg);
		
    	s32Ret = EI_MI_VBUF_SetFrameInfo(pool_info[u32Idx].Pool, &pstVbConfig->astFrameInfo[u32Idx]);
		if (s32Ret)
		{
			PNT_LOGE("%s %d\n", __FILE__, __LINE__);
		}
		
		pool_info[u32Idx].u32BufNum = pstVbConfig->astCommPool[u32Idx].u32BufCnt;
		pool_info[u32Idx].enRemapMode = pstVbConfig->astCommPool[u32Idx].enRemapMode;
		pool_info[u32Idx].pstBufMap = malloc(sizeof(VBUF_MAP_S) * pool_info[u32Idx].u32BufNum);
		
		/* 这里在初始化的时候保存映射的虚拟地址，是避免在算法使用的时候映射会占用 cpu 资源 */
		for (int i = 0; i < pstVbConfig->astCommPool[u32Idx].u32BufCnt; i++)
		{
            pool_info[u32Idx].Buffer[i] = EI_MI_VBUF_GetBuffer(pool_info[u32Idx].Pool, 10*1000);
			
            s32Ret = EI_MI_VBUF_GetFrameInfo(pool_info[u32Idx].Buffer[i], &pstVbConfig->astFrameInfo[u32Idx]);
			if (s32Ret)
			{
		        PNT_LOGE("%s %d\n", __FILE__, __LINE__);
			}
			
            s32Ret = EI_MI_VBUF_FrameMmap(&pstVbConfig->astFrameInfo[u32Idx], pstVbConfig->astCommPool[u32Idx].enRemapMode);
			if (s32Ret)
			{
		        PNT_LOGE("%s %d\n", __FILE__, __LINE__);
			}
			pool_info[u32Idx].pstBufMap[pstVbConfig->astFrameInfo[u32Idx].u32Idx].u32BufID = pool_info[u32Idx].Buffer[i];
			pool_info[u32Idx].pstBufMap[pstVbConfig->astFrameInfo[u32Idx].u32Idx].stVfrmInfo = pstVbConfig->astFrameInfo[u32Idx];
        }
		
        for (int i = 0; i < pstVbConfig->astCommPool[u32Idx].u32BufCnt; i++)
		{
            s32Ret = EI_MI_VBUF_PutBuffer(pool_info[u32Idx].Buffer[i]);
			if (s32Ret)
			{
		        PNT_LOGE("%s %d\n", __FILE__, __LINE__);
				return RET_FAILED;
			}
        }
	}

	return RET_SUCCESS;
}

int Vbuf_Pool_Deinit(VBUF_CONFIG_S *pstVbConfig)
{
	pool_info_t* pool_info = gAlComParam.pool_info;
	
	for (int u32Idx = 0; u32Idx < pstVbConfig->u32PoolCnt; ++u32Idx)
	{
		for (int i = 0; i < pstVbConfig->astCommPool[u32Idx].u32BufCnt; i++)
		{
            EI_MI_VBUF_FrameMunmap(&pool_info[u32Idx].pstBufMap[i].stVfrmInfo, pool_info[u32Idx].enRemapMode);
        }
		
		free(pool_info[u32Idx].pstBufMap);
    	pool_info[u32Idx].pstBufMap = NULL;
		
		EI_MI_VBUF_DestroyPool(pool_info[u32Idx].Pool);
	}

	return RET_SUCCESS;
}

void Algo_Start(void)
{
#ifdef ONLY_3rdCH

	if (gGlobalSysParam->dsmEnable)
	{
		dms_nn_init(2, 2, VISS_MAX_PHY_CHN_NUM, 1280, 720);
	}

#else

#if (MAX_VIDEO_NUM > 1)
	if (gGlobalSysParam->dsmEnable)
	{
#if (DMS_CAM_CHANNEL==1)
		dms_nn_init(2, 1, VISS_MAX_PHY_CHN_NUM, 1280, 720);
#else
		dms_nn_init(0/*ippu dev*/, 0, 1/*ippu chn*/, 1920, 1080);
#endif
	}
#endif
	if (gGlobalSysParam->adasEnable)
	{
#if (ADAS_CAM_CHANNEL==1)
		adas_nn_init(2, 1, VISS_MAX_PHY_CHN_NUM, 1280, 720);
#else
		adas_nn_init(0/*ippu dev*/, 0, 1/*ippu chn*/, 1920, 1080);
#endif
	}

#endif
}

int al_warning_proc(WARNING_MSG_TYPE warnType, int extra, int extra2, float ttc)
{
	int ret = 0;

	if (0 == gAlComParam.mGpsValid && LB_WARNING_ADAS_LAUNCH == warnType)
	{
		return ret;
	}

	return ret;
}

void Algo_Stop(void)
{
	if (gGlobalSysParam->adasEnable)
	{
		adas_nn_stop();
	}
	
	if (gGlobalSysParam->dsmEnable)
	{
		dms_nn_stop();
	}
	
}

