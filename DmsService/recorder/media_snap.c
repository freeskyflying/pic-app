#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <errno.h>
#include <sys/prctl.h>
#include "dirent.h"
#include <pthread.h>

#include "media_snap.h"

#include "sample_comm.h"
#include "typedef.h"
#include "pnt_log.h"
#include "queue_list.h"
#include "osal.h"

#include "utils.h"

static SnapController_t gSnapController[MAX_VIDEO_NUM] = { 0 };

EI_VOID *snap_proc(EI_VOID *p)
{
    EI_S32 s32Ret;
    AREA_CODE_S stAreaCode[1] = {0};
    VIDEO_FRAME_INFO_S snapFrame = {0};
    VENC_CHN_STATUS_S stStatus;
    VENC_STREAM_S stStream;
    int count = 0;

	SnapController_t* handle = (SnapController_t*)p;

	char prname[16] = { 0 };
	sprintf(prname, "snap_proc%d", handle->vissChn);
	prctl(PR_SET_NAME, prname);

    usleep(1000 * 1000);

	while (handle->bThreadFlag)
	{
		SnapRequest_t* req = (SnapRequest_t*)queue_list_popup(&handle->reqList);
		if (NULL == req)
		{
			usleep(50*1000);
			continue;
		}

		if (req->snapTime > currentTimeMillis())
		{
			queue_list_push(&handle->reqList, req);
			usleep(100*1000);
			continue;
		}

		int retry = 2;
retry_ones:
        memset(&snapFrame, 0, sizeof(VIDEO_FRAME_INFO_S));

		if (0 == handle->vissDev)
		{
        	s32Ret = EI_MI_IPPU_GetChnFrame(handle->vissDev, handle->vissChn, &snapFrame, 1000);
		}
		else
		{
        	s32Ret = EI_MI_VISS_GetChnFrame(handle->vissDev, handle->vissChn, &snapFrame, 1000);
		}
		
        if (s32Ret != EI_SUCCESS)
		{
            PNT_LOGE("get frame error\n");
            usleep(50 * 1000);
			if (0 == retry) { retry ++; goto retry_ones; }
            continue;
        }
		
        snapFrame.stVencFrameInfo.stAreaCodeInfo.u32AreaNum = 1;
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode = &stAreaCode[0];
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].stRegion.s32X = 0;
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].stRegion.s32Y = 0;
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].stRegion.u32Width  = handle->width;
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].stRegion.u32Height = handle->height;
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].u32OutWidth = req->width;
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].u32OutHeight = req->height;
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].u32AreaCodeInfo = 0;
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].u32Quality = 90;

        snapFrame.stVencFrameInfo.bEnableScale = EI_TRUE;

        s32Ret = EI_MI_VENC_SendFrame(handle->vencChn, &snapFrame, 1000);
        if (s32Ret != EI_SUCCESS)
		{
            PNT_LOGE("send frame error\n");
            EI_MI_VISS_ReleaseChnFrame(0, 0, &snapFrame);
            usleep(50 * 1000);
			if (0 == retry) { retry ++; goto retry_ones; }
            continue;
        }
        usleep(20 * 1000);

		if (0 == handle->vissDev)
		{
        	EI_MI_IPPU_ReleaseChnFrame(handle->vissDev, handle->vissChn, &snapFrame);
		}
		else
		{
        	EI_MI_VISS_ReleaseChnFrame(handle->vissDev, handle->vissChn, &snapFrame);
		}
		
        s32Ret = EI_MI_VENC_QueryStatus(handle->vencChn, &stStatus);
        if (s32Ret != EI_SUCCESS)
		{
            PNT_LOGE("query status chn-%d error : %d\n", 0, s32Ret);
            usleep(50 * 1000);
			if (0 == retry) { retry ++; goto retry_ones; }
			break;
        }

        s32Ret = EI_MI_VENC_GetStream(handle->vencChn, &stStream, 1000);

        if (s32Ret == EI_ERR_VENC_NOBUF)
		{
            PNT_LOGE("No buffer\n");
            usleep(50 * 1000);
			if (0 == retry) { retry ++; goto retry_ones; }
            continue;
        }
		else if (s32Ret != EI_SUCCESS)
		{
            PNT_LOGE("get venc chn-%d error : %d\n", handle->vencChn, s32Ret);
            usleep(50 * 1000);
			if (0 == retry) { retry ++; goto retry_ones; }
            continue;
        }

		if (strlen(req->filename) > 0)
		{
	        int fd = open(req->filename, O_WRONLY | O_CREAT);
			
	        if (fd < 0)
			{
	            EI_MI_VENC_ReleaseStream(handle->vencChn, &stStream);
	            PNT_LOGE("open file[%s] failed!\n", req->filename);
	            continue;
	        }
			else
			{
				if ((stStream.pstPack.pu8Addr[0] != EI_NULL) && (stStream.pstPack.u32Len[0] != 0))
				{
			        write(fd, stStream.pstPack.pu8Addr[0], stStream.pstPack.u32Len[0]);
			        if ((stStream.pstPack.pu8Addr[1] != EI_NULL) && (stStream.pstPack.u32Len[1] != 0) && (stStream.PackType != PACKET_TYPE_JPEG))
					{
			            write(fd, stStream.pstPack.pu8Addr[1], stStream.pstPack.u32Len[1]);
			        }
		    	}
	            close(fd);
				PNT_LOGE("create %s photo (%dx%d) success.", req->filename, req->width, req->height);
	        }
		}
		
        s32Ret = EI_MI_VENC_ReleaseStream(handle->vencChn, &stStream);

        if (s32Ret != EI_SUCCESS)
		{
            PNT_LOGE("release venc chn-%d error : %d\n", handle->vencChn, s32Ret);
            usleep(50 * 1000);
			if (0 == retry) { retry ++; goto retry_ones; }
            continue;
        }

		free(req);
		req = NULL;
	}

	return NULL;
}

int MediaSnap_Init(void)
{
    int ret;
    SAMPLE_VENC_CONFIG_S stSnapJVC[MAX_VIDEO_NUM];
    VENC_JPEG_PARAM_S       stJpegParam = {0};

    stSnapJVC[0].enInputFormat   = PIX_FMT_YUV_SEMIPLANAR_420;
    stSnapJVC[0].u32width        = 1280;
    stSnapJVC[0].u32height       = 720;
    stSnapJVC[0].u32bitrate      = 10*1024*1024;
    ret = SAMPLE_COMM_VENC_Start(5, PT_JPEG, SAMPLE_RC_FIXQP, &stSnapJVC[0], COMPRESS_MODE_NONE, VENC_GOPMODE_DUAL_P);
    if (EI_SUCCESS != ret)
	{
        PNT_LOGE("SAMPLE_COMM_VENC_Start error\n");
        return EI_FAILURE;
    }

    EI_MI_VENC_GetJpegParam(5, &stJpegParam);
    stJpegParam.bEnableAreaCode = 1;
    EI_MI_VENC_SetJpegParam(5, &stJpegParam);

	memset(gSnapController, 0, sizeof(gSnapController));
	for (int i=0; i<sizeof(gSnapController)/sizeof(gSnapController[0]); i++)
	{
		gSnapController[i].vencChn = 5;
		gSnapController[i].vissDev = 2;
		gSnapController[i].vissChn = 1;
		gSnapController[i].width = 1280;
		gSnapController[i].height = 720;
		
		gSnapController[i].bThreadFlag = 1;

		queue_list_create(&gSnapController[i].reqList, 10);

	    ret = pthread_create(&gSnapController[i].pid, NULL, snap_proc, &gSnapController[i]);
	}
	
    return EI_SUCCESS;
}

int MediaSnap_CreateSnap(int chn, int count, int width, int height, int interval, char* arg)
{
	if (chn >= MAX_VIDEO_NUM)
	{
		return RET_FAILED;
	}

	for (int i = 0; i<count; i++)
	{
		SnapRequest_t* req = (SnapRequest_t*)malloc(sizeof(SnapRequest_t));

		if (NULL == req)
		{
			PNT_LOGE("create snap request failed chn[%d] [%s - %d].", chn, arg, i);
			continue;
		}

		memset(req, 0, sizeof(SnapRequest_t));

		req->snapTime = currentTimeMillis()+interval;
		req->width = width;
		req->height = height;

		MediaSnap_RenameBySequence(req->filename, arg, count, i);

		if (queue_list_push(&gSnapController[chn].reqList, req))
		{
			free(req);
			PNT_LOGE("push snap request failed chn[%d] [%s - %d].", chn, arg, i);
			continue;
		}
	}

	return RET_SUCCESS;
}

void MediaSnap_RenameBySequence(char* outname, char* orgname, int total, int seq)
{
	if (0 == seq)
	{
		strcpy(outname, orgname);
	}
	else
	{
		int i = 0;
		while (orgname[i] != '.')
		{
			i++;
		}

		memcpy(outname, orgname, i);
		outname[i] = 0;

		sprintf(outname, "%s_%d%s", outname, seq, &orgname[i]);
	}
}


