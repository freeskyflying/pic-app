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

#include "sample_comm.h"
#include "typedef.h"
#include "pnt_log.h"
#include "queue_list.h"
#include "utils.h"
#include "media_utils.h"
#include "media_osd.h"
#include "media_video.h"
#include "media_audio.h"
#include "media_snap.h"
#include "osal.h"

#include "algo_common.h"
#include "media_storage.h"
#include "muxer_common.h"
#include "system_param.h"
#include "media_cache.h"
#include "jt808_controller.h"

typedef struct {
	
    void *muxer;
    muxer_param_t muxer_parm;
    char *filename;
    char *tmpbuf;
    int tmpbuf_len;
    int64_t   start_pts[2];
	
} sample_muxer_hdl_t;

MediaVideoStream_t gMediaStream[MAX_VIDEO_NUM*2] = { 0 };

pool_info_t pool_info[VBUF_POOL_MAX_NUM] = {0};

static sem_t gFrameSem[MAX_VIDEO_NUM];

static sample_muxer_hdl_t gRecordMuxerHandle[MAX_VIDEO_NUM] = { 0 };

void aenc_frame_callback(int bindVenc, uint8_t* data, int len);

static int MediaVideo_BuffpollSet(VBUF_CONFIG_S *vbuf_config)
{
    memset(vbuf_config, 0, sizeof(VBUF_CONFIG_S));

    int count = 0;

    vbuf_config->astFrameInfo[count].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Width = 1280;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Height = 720;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[count].u32BufCnt = 8;
    vbuf_config->astCommPool[count].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    count ++;

#if SUBSTREAM
    vbuf_config->astFrameInfo[count].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Width = 640;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Height = 360;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[count].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    vbuf_config->astCommPool[count].u32BufCnt = 2;
    count ++;
#endif
    vbuf_config->u32PoolCnt = count;

	return 0;
}

static int MediaVideo_BuffpollDeinit(VBUF_CONFIG_S *vbuf_config)
{
	MediaVideo_BuffpollSet(vbuf_config);

    return SAMPLE_COMM_SYS_Exit(vbuf_config);
}

static int MediaVideo_BuffpollInit(VBUF_CONFIG_S *vbuf_config)
{
	MediaVideo_BuffpollSet(vbuf_config);

    int s32Ret = 0;
	VBUF_POOL_CONFIG_S stPoolCfg = {0};

	EI_MI_MBASE_Init();
    EI_MI_VBUF_Init();
    EI_MI_MLINK_Init();
    
	for (int u32Idx = 0; u32Idx < vbuf_config->u32PoolCnt; u32Idx++)
    {
	    stPoolCfg.u32BufSize = EI_MI_VBUF_GetPicBufferSize(&vbuf_config->astFrameInfo[u32Idx]);
	    stPoolCfg.u32BufCnt = vbuf_config->astCommPool[u32Idx].u32BufCnt;
	    stPoolCfg.enRemapMode = vbuf_config->astCommPool[u32Idx].enRemapMode;
	    stPoolCfg.enVbufUid = VBUF_UID_COMMON;
	    strcpy(stPoolCfg.acPoolName, "common");

    	pool_info[u32Idx].Pool = EI_MI_VBUF_CreatePool(&stPoolCfg);

    	s32Ret = EI_MI_VBUF_SetFrameInfo(pool_info[u32Idx].Pool, &vbuf_config->astFrameInfo[u32Idx]);
		if (s32Ret)
        {
		    PNT_LOGE("%s %d\n", __FILE__, __LINE__);
		}

		pool_info[u32Idx].u32BufNum = vbuf_config->astCommPool[u32Idx].u32BufCnt;
		pool_info[u32Idx].enRemapMode = vbuf_config->astCommPool[u32Idx].enRemapMode;
		pool_info[u32Idx].pstBufMap = malloc(sizeof(VBUF_MAP_S) * pool_info[u32Idx].u32BufNum);

		for (int i = 0; i < vbuf_config->astCommPool[u32Idx].u32BufCnt; i++)
        {
            pool_info[u32Idx].Buffer[i] = EI_MI_VBUF_GetBuffer(pool_info[u32Idx].Pool, 10*1000);
            s32Ret = EI_MI_VBUF_GetFrameInfo(pool_info[u32Idx].Buffer[i], &vbuf_config->astFrameInfo[u32Idx]);
			if (s32Ret)
            {
		        PNT_LOGE("%s %d\n", __FILE__, __LINE__);
			}

            s32Ret = EI_MI_VBUF_FrameMmap(&vbuf_config->astFrameInfo[u32Idx], vbuf_config->astCommPool[u32Idx].enRemapMode);
			if (s32Ret)
            {
		        PNT_LOGE("%s %d\n", __FILE__, __LINE__);
			}
			pool_info[u32Idx].pstBufMap[vbuf_config->astFrameInfo[u32Idx].u32Idx].u32BufID = pool_info[u32Idx].Buffer[i];
			pool_info[u32Idx].pstBufMap[vbuf_config->astFrameInfo[u32Idx].u32Idx].stVfrmInfo = vbuf_config->astFrameInfo[u32Idx];
        }
		
        for (int i = 0; i < vbuf_config->astCommPool[u32Idx].u32BufCnt; i++)
        {
            s32Ret = EI_MI_VBUF_PutBuffer(pool_info[u32Idx].Buffer[i]);
			if (s32Ret)
            {
		        PNT_LOGE("%s %d\n", __FILE__, __LINE__);
				break;
			}
        }
	}

    return s32Ret;
}

static int MediaVideo_VissConfigInit(SAMPLE_VISS_CONFIG_S *viss_config)
{
    SAMPLE_VISS_INFO_S *viss_info;
    memset(viss_config, 0, sizeof(SAMPLE_VISS_CONFIG_S));

    viss_config->s32WorkingVissNum = 1;

    viss_info = &viss_config->astVissInfo[0];
    viss_info->stDevInfo.VissDev = 2;
    viss_info->stDevInfo.aBindPhyChn[0] = 1;
    viss_info->stDevInfo.aBindPhyChn[1] = -1;//2;
    viss_info->stDevInfo.aBindPhyChn[2] = -1;
    viss_info->stDevInfo.aBindPhyChn[3] = -1;
    viss_info->stDevInfo.enOutPath = VISS_OUT_PATH_DMA;
    viss_info->stChnInfo.aVissChn[0] = 1;
    viss_info->stChnInfo.aVissChn[1] = -1;//2;
    viss_info->stChnInfo.aVissChn[2] = -1;
    viss_info->stChnInfo.aVissChn[3] = -1;
    viss_info->stChnInfo.enWorkMode = VISS_WORK_MODE_1Chn;
    viss_info->stChnInfo.stChnAttr.u32Align = 32;
    viss_info->stChnInfo.stChnAttr.u32Depth = 2;
    viss_info->stSnsInfo.enSnsType = TP9963_MIPI_1280_720_25FPS_1CH_2LANE_YUV;

    return RET_SUCCESS;
}

extern EI_S32  SAMPLE_COMM_VENC_Creat(VC_CHN VencChn, PAYLOAD_TYPE_E enType,
    SAMPLE_RC_E RcMode, SAMPLE_VENC_CONFIG_S *pstVencConfig,
    COMPRESS_MODE_E enCompressMode, VENC_GOP_MODE_E GopMode);
int MediaVideo_TP9963EncodeInit(SAMPLE_VISS_CONFIG_S* viss_config, VBUF_CONFIG_S* vbuf_config)
{
    SAMPLE_VISS_INFO_S *viss_info = &viss_config->astVissInfo[0];

	SAMPLE_VENC_CONFIG_S venc_config = {0};
    VIDEO_FRAME_INFO_S *astFrameInfo = &vbuf_config->astFrameInfo[0];

    int devChn = viss_info->stDevInfo.VissDev;
    int vissChn = viss_info->stChnInfo.aVissChn[0];
    int vencChn = 0;

    venc_config.enInputFormat   = astFrameInfo->stCommFrameInfo.enPixelFormat;
    venc_config.u32width	    = astFrameInfo->stCommFrameInfo.u32Width;
    venc_config.u32height	    = astFrameInfo->stCommFrameInfo.u32Height;
    venc_config.u32bitrate      = SDR_VENC_BITRATE_720P;
    venc_config.u32srcframerate = SDR_VISS_FRAMERATE;
    venc_config.u32dstframerate = SDR_VENC_FRAMERATE_MAJOR;
    venc_config.u32IdrPeriod    = venc_config.u32dstframerate;

    int ret = SAMPLE_COMM_VENC_Creat(vencChn, PT_H264, SAMPLE_RC_CBR, &venc_config, COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
    if (EI_SUCCESS != ret)
    {
        PNT_LOGE("fatal error !!! SAMPLE_COMM_VENC_Creat failed[%d - %d - %d].", devChn, vissChn, vencChn);
        return RET_FAILED;
    }

    ret = SAMPLE_COMM_VISS_Link_VPU(devChn, vissChn, vencChn);
    if (EI_SUCCESS != ret)
    {
        PNT_LOGE("fatal error !!! SAMPLE_COMM_VISS_Link_VPU failed[%d - %d - %d].", devChn, vissChn, vencChn);
        SAMPLE_COMM_VENC_Stop(vencChn);
        return RET_FAILED;
    }

#if SUBSTREAM
	SAMPLE_VENC_CONFIG_S sub_venc_config = {0};
    astFrameInfo = &vbuf_config->astFrameInfo[0+MAX_VIDEO_NUM];

    vencChn = vencChn + MAX_VIDEO_NUM;

    sub_venc_config.enInputFormat   = astFrameInfo->stCommFrameInfo.enPixelFormat;
    sub_venc_config.u32width	    = astFrameInfo->stCommFrameInfo.u32Width;
    sub_venc_config.u32height	    = astFrameInfo->stCommFrameInfo.u32Height;
    sub_venc_config.u32bitrate      = SDR_VENC_BITRATE_SUB;
    sub_venc_config.u32srcframerate = SDR_VISS_FRAMERATE;
    sub_venc_config.u32dstframerate = SDR_VENC_FRAMERATE_SUB;
    sub_venc_config.u32IdrPeriod    = venc_config.u32dstframerate;

    ret = SAMPLE_COMM_VENC_Creat(vencChn, PT_H264, SAMPLE_RC_CBR, &sub_venc_config, COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
    if (EI_SUCCESS != ret)
    {
        PNT_LOGE("fatal error !!! SAMPLE_COMM_VENC_Creat failed[%d - %d - %d].", devChn, vissChn, vencChn);
        return RET_FAILED;
    }

    ret = SAMPLE_COMM_VISS_Link_VPU(devChn, vissChn, vencChn);
    if (EI_SUCCESS != ret)
    {
        PNT_LOGE("fatal error !!! SAMPLE_COMM_VISS_Link_VPU failed[%d - %d - %d].", devChn, vissChn, vencChn);
        SAMPLE_COMM_VENC_Stop(vencChn);
        return RET_FAILED;
    }
#endif

    return RET_SUCCESS;
}

static int closeVideoFile(void *hdl, char *filename)
{
    MediaVideoStream_t *handle = (MediaVideoStream_t*)hdl;

	close_video_file(handle->venChn, filename);

    return 0;
}

int MediaVideo_RecordMuxerInit(sample_muxer_hdl_t* muxer_hdl, int width, int height, void* param)
{
	char filename[128] = { 0 };
    MediaVideoStream_t *handle = (MediaVideoStream_t*)param;
	int videoSize = (handle->venChn==0)?CH1_VIDEO_STREAM_SIZE:CH2_VIDEO_STREAM_SIZE;

	if (RET_FAILED == fallocate_video_file(filename, currentTimeMillis()/1000, handle->venChn, videoSize))
	{
		return RET_FAILED;
	}

	if (NULL == muxer_hdl->muxer)
	{
	    memset(muxer_hdl, 0, sizeof(sample_muxer_hdl_t));
	    muxer_hdl->muxer = muxer_context_create();
	    if (muxer_hdl->muxer == NULL)
	    {
	    	PNT_LOGE("create muxer failed.");
	        return RET_FAILED;
	    }
	}

    muxer_hdl->muxer_parm.nb_streams = (handle->venChn==AUDIO_BIND_CHN)?2:1;
    muxer_hdl->start_pts[VIDEO_INDEX] = -1;
    muxer_hdl->start_pts[AUDIO_INDEX] = -1;
	
    muxer_hdl->muxer_parm.mode = AV_MUXER_TYPE_MP4;
    muxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].codec_type = AV_MEDIA_TYPE_VIDEO;
    muxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].codec_id = AV_CODEC_TYPE_H264;
    muxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].sample_rate = SDR_VENC_FRAMERATE_MAJOR;//20 * VIDEOTIMEFRAME;
    muxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].width = width;
    muxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].height = height;

    muxer_hdl->muxer_parm.stream_parm[AUDIO_INDEX].codec_type = AV_MEDIA_TYPE_AUDIO;
    muxer_hdl->muxer_parm.stream_parm[AUDIO_INDEX].codec_id = AV_CODEC_TYPE_AAC;
    muxer_hdl->muxer_parm.stream_parm[AUDIO_INDEX].sample_rate = MEDIA_AUDIO_RATE;
	muxer_hdl->muxer_parm.stream_parm[AUDIO_INDEX].channels = 1;

    muxer_hdl->muxer_parm.fix_duration_param.app_data = param;
    muxer_hdl->muxer_parm.fix_duration_param.cb_get_next_file = NULL;
    muxer_hdl->muxer_parm.fix_duration_param.cb_file_closed = closeVideoFile;
    muxer_hdl->muxer_parm.fix_duration_param.cb_get_user_data = NULL;
    muxer_hdl->muxer_parm.fix_duration_param.file_duration = 60;

    muxer_context_ctrl(muxer_hdl->muxer, MUXER_CONTEXT_SET_FILE_NAME, filename);

    if (muxer_context_start(muxer_hdl->muxer, &muxer_hdl->muxer_parm))
    {
    	muxer_context_destroy(muxer_hdl->muxer);
		muxer_hdl->muxer = NULL;
    	PNT_LOGE("start muxer failed.");
		return RET_FAILED;
    }

	char thumbName[128] = { 0 };
	get_thumbname_by_videoname(thumbName, filename);
	MediaSnap_CreateSnap(handle->venChn, 1, 160, 120, 0, thumbName);

	return RET_SUCCESS;
}

#define user_RB16(x) ((((const uint8_t *)(x))[0] << 8) | ((const uint8_t *)(x))[1])

void aenc_frame_callback(int bindVenc, uint8_t* data, int len)
{
	AUDIO_STREAM_S *pstStream = (AUDIO_STREAM_S *)data;

    MediaVideoStream_t *handle = (MediaVideoStream_t*)&gMediaStream[bindVenc];

    sample_muxer_hdl_t *pmuxer_hdl = &gRecordMuxerHandle[bindVenc];
    muxer_packet_t pin = { 0 };
    int index = 1;
    int ret = 0;

    JT808Contorller_InsertAudio(bindVenc, pstStream->pStream, pstStream->u32Len);

	if (handle->recStatus != REC_RECORDING)
	{
		return;
	}

    if (pmuxer_hdl == NULL || pstStream == NULL)
	{
        return;
    }
    if (pmuxer_hdl->muxer_parm.stream_parm[AUDIO_INDEX].codec_id == AV_CODEC_TYPE_AAC && pstStream->u32Len > 2 && (user_RB16(pstStream->pStream) & 0xfff0) == 0xfff0)
	{
        pin.data = pstStream->pStream + 7;
        if (pstStream->u32Len - 7 > 0)
            pin.size = pstStream->u32Len - 7;
    }
	else
	{
        pin.data = pstStream->pStream;
        pin.size = pstStream->u32Len;
    }
    pin.stream_index = AUDIO_INDEX;
    if (pmuxer_hdl->start_pts[index] < 0)
        pmuxer_hdl->start_pts[index] = pstStream->u64TimeStamp;
    pin.timestamp = (pstStream->u64TimeStamp-pmuxer_hdl->start_pts[index]) / 1000;

	muxer_context_write(pmuxer_hdl->muxer, &pin);

    if (pmuxer_hdl->muxer_parm.stream_parm[AUDIO_INDEX].codec_id == AV_CODEC_TYPE_AAC && pstStream->u32Len > 2 && (user_RB16(pstStream->pStream) & 0xfff0) == 0xfff0)
	{
		if (pstStream->u32Len - 7 > 0)
		{
			MediaCache_InsertAudioFrame(bindVenc, pstStream->pStream+7, pstStream->u32Len - 7, NULL, 0, 0);
		}
    }
	else
	{
		MediaCache_InsertAudioFrame(bindVenc, pstStream->pStream, pstStream->u32Len, NULL, 0, 0);
    }
}

static EI_VOID *_get_venc_stream_proc(EI_VOID *p)
{
    MediaVideoStream_t *handle = (MediaVideoStream_t*)p;
    VENC_STREAM_S stStream;
    int ret;
    char thread_name[16];
    thread_name[15] = 0;
    snprintf(thread_name, 16, "mstreamproc%d", handle->venChn);
    prctl(PR_SET_NAME, thread_name);

    VENC_RECV_PIC_PARAM_S  stRecvParam;
    stRecvParam.s32RecvPicNum = -1;
    ret = EI_MI_VENC_StartRecvFrame(handle->venChn, &stRecvParam);
    if (EI_SUCCESS != ret)
    {
        PNT_LOGE( "EI_MI_VENC_StartRecvFrame faild with%#x! \n", ret);
        return NULL;
    }


    handle->recStatus = REC_STOP;
	int initFlag = 0;
	muxer_packet_t pin = {0};
    sample_muxer_hdl_t *pmuxer_hdl = &gRecordMuxerHandle[handle->venChn];
	
	memset(pmuxer_hdl, 0, sizeof(sample_muxer_hdl_t));

	const int vid_width = (handle->venChn==0)?CH1_VIDEO_WIDTH:CH2_VIDEO_WIDTH;
	const int vid_height = (handle->venChn==0)?CH1_VIDEO_HEIGHT:CH2_VIDEO_HEIGHT;

    while (EI_TRUE == handle->bThreadStart)
    {
        ret = EI_MI_VENC_GetStream(handle->venChn, &stStream, 100);
        if (ret == EI_ERR_VENC_NOBUF)
        {
            usleep(5 * 1000);
            continue;
        }
        else if (ret != EI_SUCCESS)
        {
            PNT_LOGE("get chn-%d error : %d\n", handle->venChn, ret);
            break;
        }
        if (stStream.pstPack.u32Len[0] == 0)
        {
            PNT_LOGE("%d ch %d pstPack.u32Len:%d-%d, ret:%x\n", __LINE__, handle->venChn, stStream.pstPack.u32Len[0], stStream.pstPack.u32Len[1], ret);
        }
		
        pthread_mutex_lock(&handle->mutex);
		
		memset(&pin, 0, sizeof(pin));
			
		if (stStream.pstPack.enDataType == DATA_TYPE_I_FRAME || stStream.pstPack.enDataType == DATA_TYPE_INITPACKET)
        {
            if (REC_START == handle->recStatus || REC_STOP == handle->recStatus)
            {
                if (initFlag)
                {
					muxer_context_stop(pmuxer_hdl->muxer);
					initFlag = 0;
                }
            }
            if (REC_START == handle->recStatus && !initFlag)
            {
				if (RET_SUCCESS == MediaVideo_RecordMuxerInit(pmuxer_hdl, vid_width, vid_height, handle))
				{
					initFlag = 1;
					handle->recStatus = REC_RECORDING;
			        pin.flags |= AV_PKT_INIT_FRAME;
				}
            }
        }

		if (handle->recStatus == REC_RECORDING && initFlag)
		{
			MediaCache_InsertH264Frame(handle->venChn, stStream.pstPack.pu8Addr[0], stStream.pstPack.u32Len[0], 
							stStream.pstPack.pu8Addr[1], stStream.pstPack.u32Len[1], stStream.pstPack.enDataType);
			
	    	pin.stream_index = VIDEO_INDEX;
		    if (stStream.pstPack.pu8Addr[1])
			{
		        if (stStream.pstPack.u32Len[0] + stStream.pstPack.u32Len[1] > pmuxer_hdl->tmpbuf_len)
				{
		            pmuxer_hdl->tmpbuf_len = stStream.pstPack.u32Len[0] + stStream.pstPack.u32Len[1];
		            if (pmuxer_hdl->tmpbuf)
		                free(pmuxer_hdl->tmpbuf);
		            pmuxer_hdl->tmpbuf = malloc(pmuxer_hdl->tmpbuf_len);
		            if (pmuxer_hdl->tmpbuf == NULL)
					{
		                pmuxer_hdl->tmpbuf_len = 0;
		            }
		        }
		        memcpy(pmuxer_hdl->tmpbuf, stStream.pstPack.pu8Addr[0], stStream.pstPack.u32Len[0]);
		        memcpy(pmuxer_hdl->tmpbuf + stStream.pstPack.u32Len[0], stStream.pstPack.pu8Addr[1], stStream.pstPack.u32Len[1]);
		        pin.data = (unsigned char *)pmuxer_hdl->tmpbuf;
		        pin.size = stStream.pstPack.u32Len[0] + stStream.pstPack.u32Len[1];
		    }
			else
			{
		        pin.data = stStream.pstPack.pu8Addr[0];
		        pin.size = stStream.pstPack.u32Len[0];
		    }
			
		    if (pmuxer_hdl->start_pts[0] < 0)
		        pmuxer_hdl->start_pts[0] = stStream.pstPack.u64PTS;
	    	pin.timestamp = (stStream.pstPack.u64PTS-pmuxer_hdl->start_pts[0]) / 1000;
			
		    if (stStream.pstPack.enDataType == DATA_TYPE_I_FRAME)
			{
		        pin.flags |= AV_PKT_FLAG_KEY;
		    }
		    if (stStream.pstPack.enDataType == DATA_TYPE_INITPACKET)
			{
		        pin.flags |= AV_PKT_INIT_FRAME;
		    }
		    if (stStream.pstPack.enDataType == DATA_TYPE_P_FRAME)
		    {
		    	pin.flags |= AV_PKT_P_FRAME;
		    }
		    if (stStream.pstPack.enDataType == DATA_TYPE_B_FRAME)
		    {
		    	pin.flags |= AV_PKT_B_FRAME;
		    }
		    if (stStream.pstPack.bFrameEnd)
		        pin.flags |= AV_PKT_ENDING_FRAME;

		    muxer_context_write(pmuxer_hdl->muxer, &pin);
		}
		
        pthread_mutex_unlock(&handle->mutex);

		JT808Contorller_InsertFrame(handle->venChn, 0, stStream.pstPack.pu8Addr[0], stStream.pstPack.u32Len[0], stStream.pstPack.pu8Addr[1], stStream.pstPack.u32Len[1]);

        ret = EI_MI_VENC_ReleaseStream(handle->venChn, &stStream);
        if (ret != EI_SUCCESS)
        {
            PNT_LOGE("release stream chn-%d error : %d\n", handle->venChn, ret);
            break;
        }
    }

    printf("task %s exit\n", thread_name);

    return NULL;
}

extern void RtspFramePushQueue(int channel, char* data, int len, char* data2, int len2);
extern void RtspAudioPushQueue(int channel, char* data, int len);

static EI_VOID *_get_venc_substream_proc(EI_VOID *p)
{
    MediaVideoStream_t *handle = (MediaVideoStream_t*)p;
    VENC_STREAM_S stStream;
    int ret;
    char thread_name[16];

    thread_name[15] = 0;
    snprintf(thread_name, 16, "sstreamproc%d", handle->venChn);
    prctl(PR_SET_NAME, thread_name);

    VENC_RECV_PIC_PARAM_S  stRecvParam;
    stRecvParam.s32RecvPicNum = -1;
    ret = EI_MI_VENC_StartRecvFrame(handle->venChn, &stRecvParam);
    if (EI_SUCCESS != ret)
    {
        PNT_LOGE( "EI_MI_VENC_StartRecvFrame faild with%#x! \n", ret);
        return NULL;
    }

    while (EI_TRUE == handle->bThreadStart)
    {
        ret = EI_MI_VENC_GetStream(handle->venChn, &stStream, 100);
        if (ret == EI_ERR_VENC_NOBUF)
        {
            usleep(5 * 1000);
            continue;
        }
        else if (ret != EI_SUCCESS)
        {
            PNT_LOGE("get chn-%d error : %d\n", handle->venChn, ret);
            break;
        }
        if (stStream.pstPack.u32Len[0] == 0)
        {
            PNT_LOGE("%d ch %d pstPack.u32Len:%d-%d, ret:%x\n", __LINE__, handle->venChn, stStream.pstPack.u32Len[0], stStream.pstPack.u32Len[1], ret);
        }

        RtspFramePushQueue(handle->venChn%MAX_VIDEO_NUM, (char*)stStream.pstPack.pu8Addr[0], stStream.pstPack.u32Len[0], (char*)stStream.pstPack.pu8Addr[1], stStream.pstPack.u32Len[1]);

		JT808Contorller_InsertFrame(handle->venChn%MAX_VIDEO_NUM, 1, stStream.pstPack.pu8Addr[0], stStream.pstPack.u32Len[0], stStream.pstPack.pu8Addr[1], stStream.pstPack.u32Len[1]);

        ret = EI_MI_VENC_ReleaseStream(handle->venChn, &stStream);
        if (ret != EI_SUCCESS)
        {
            PNT_LOGE("release stream chn-%d error : %d\n", handle->venChn, ret);
            break;
        }
    }

    printf("task %s exit\n", thread_name);

    return NULL;
}

void MediaVideo_SetRecordStatus(bool_t status)
{
    for (int ch=0; ch<MAX_VIDEO_NUM; ch++)
    {
        pthread_mutex_lock(&gMediaStream[ch].mutex);
        if (status)
        {
            if (gMediaStream[ch].recStatus == REC_STOP)
            {
                gMediaStream[ch].recStatus = REC_START;
            }
        }
        else
        {
            gMediaStream[ch].recStatus = REC_STOP;
        }
        pthread_mutex_unlock(&gMediaStream[ch].mutex);
    }
}

void MediaVideo_SetSyncCount(void)
{
    for (int ch=0; ch<MAX_VIDEO_NUM; ch++)
    {
        pthread_mutex_lock(&gMediaStream[ch].mutex);
    
        if (gMediaStream[ch].recStatus == REC_RECORDING)
        {
            gMediaStream[ch].recStatus = REC_START;
        }
            
        pthread_mutex_unlock(&gMediaStream[ch].mutex);
    }
}

void MediaVideo_StartGetFrame(int chnNum, int type)
{
    gMediaStream[chnNum].bThreadStart = EI_TRUE;
    gMediaStream[chnNum].venChn = chnNum;
    gMediaStream[chnNum].type = type;

    int ret = 0;

    pthread_mutex_init(&gMediaStream[chnNum].mutex, NULL);
    
    if (CHANNEL_MASTER == type)
    {
        ret = pthread_create(&gMediaStream[chnNum].vencPid, 0, _get_venc_stream_proc, (EI_VOID *)&gMediaStream[chnNum]);
    }
    else
    {
        ret = pthread_create(&gMediaStream[chnNum].vencPid, 0, _get_venc_substream_proc, (EI_VOID *)&gMediaStream[chnNum]);
    }
    if (ret)
    {
        PNT_LOGE("errno=%d, reason(%s)\n", errno, strerror(errno));
    }
}

int MediaVideo_RecorderInit(void)
{
    int ret = 0;
    SAMPLE_VISS_CONFIG_S viss_config = { 0 };
    VBUF_CONFIG_S vbuf_config = {0};
    video_encode_info_t vencInfo = { 0 };

    MediaVideo_VissConfigInit(&viss_config);

    ret = MediaVideo_BuffpollInit(&vbuf_config);
    if (RET_SUCCESS != ret)
    {
        PNT_LOGE("MediaVideo_BuffpollInit ret failed.");
        return RET_FAILED;
    }

    ret = SAMPLE_COMM_VISS_StartViss(&viss_config);
    if (RET_SUCCESS != ret)
    {
        PNT_LOGE("SAMPLE_COMM_VISS_StartViss ret failed.");
        goto exit_fail_viss;
    }

    MediaVideo_TP9963EncodeInit(&viss_config, &vbuf_config);
	
    MDP_CHN_S stRgnChn = { 0 };
    stRgnChn.enModId = EI_ID_VISS;
    stRgnChn.s32DevId = 2;
    stRgnChn.s32ChnId = 1;
    MediaOsd_InitChn(&stRgnChn, 1280, OSD_AREA_HEIGHT, 0);

    al_warning_init();

	if (gGlobalSysParam->dsmEnable)
	{
    	dms_nn_init(2, 1, 1280, 720);
	}

	if (gGlobalSysParam->micEnable)
	{
   	 	MediaAudio_Start(AUDIO_BIND_CHN, aenc_frame_callback);
	}

	MediaSnap_Init();

    return RET_SUCCESS;

exit_fail_viss:
    MediaVideo_BuffpollDeinit(&vbuf_config);

    return RET_FAILED;
}

int MediaVideo_Start(void)
{
    MediaOsd_Start();

    MediaVideo_StartGetFrame(0, CHANNEL_MASTER);
    
#if SUBSTREAM
    MediaVideo_StartGetFrame(0+MAX_VIDEO_NUM, CHANNEL_CHILD);
#endif

	MediaCache_Start(gMediaStream);

    return RET_SUCCESS;
}

int MediaVideo_Stop(void)
{
    SAMPLE_VISS_CONFIG_S viss_config = { 0 };
	VBUF_CONFIG_S vbuf_config = {  0 };

	int wait = 1;
	while (1)
	{
		wait = 0;
	    for (int ch=0; ch<MAX_VIDEO_NUM; ch++)
	    {
	    	if (gMediaStream[ch].recStatus != REC_STOP)
	    	{
	    		wait = 1;
				usleep(100*1000);
	    	}
	    }
		if (!wait)
		{
			break;
		}
	}

    for (int ch=0; ch<MAX_VIDEO_NUM*2; ch++)
    {
    	gMediaStream[ch].bThreadStart = 0;
    }

	MediaOsd_Stop();
	dms_nn_stop();

	sleep(1);

    for (int ch=0; ch<MAX_VIDEO_NUM; ch++)
    {
    	if (RET_SUCCESS == checkTFCard(NULL) && NULL != gRecordMuxerHandle[ch].muxer)
    	{
    		muxer_context_destroy(gRecordMuxerHandle[ch].muxer);
    	}
    }

    MediaVideo_VissConfigInit(&viss_config);

	SAMPLE_COMM_VISS_StopViss(&viss_config);

	MediaVideo_BuffpollDeinit(&vbuf_config);

	sleep(2);

	return 0;
}

