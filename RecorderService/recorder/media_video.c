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
    int64_t   start_pts[2];
	volatile unsigned int timeStamp;
	
} sample_muxer_hdl_t;

MediaVideoStream_t gMediaStream[MAX_VIDEO_NUM*2] = { 0 };

static sample_muxer_hdl_t gRecordMuxerHandle[MAX_VIDEO_NUM] = { 0 };

void aenc_frame_callback(int bindVenc, uint8_t* data, int len);

static int MediaVideo_BuffpollSet(VBUF_CONFIG_S *vbuf_config)
{
    memset(vbuf_config, 0, sizeof(VBUF_CONFIG_S));

    int count = 0;

    vbuf_config->astFrameInfo[count].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Width = 1920;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Height = 1080;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[count].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    vbuf_config->astCommPool[count].u32BufCnt = 8;
    count ++;

    vbuf_config->astFrameInfo[count].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Width = 1280;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Height = 720;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[count].u32BufCnt = 12;
    vbuf_config->astCommPool[count].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    count ++;

#if SUBSTREAM
    vbuf_config->astFrameInfo[count].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Width = 640;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Height = 360;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[count].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    vbuf_config->astCommPool[count].u32BufCnt = 6;
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

	Vbuf_Pool_init(vbuf_config);

    return 0;
}

static int MediaVideo_VissConfigInit(SAMPLE_VISS_CONFIG_S *viss_config)
{
    SAMPLE_VISS_INFO_S *viss_info;
    memset(viss_config, 0, sizeof(SAMPLE_VISS_CONFIG_S));
	
#ifdef DISABLECH1

    viss_config->s32WorkingVissNum = 1;

    viss_info = &viss_config->astVissInfo[0];
	
    viss_info->stDevInfo.VissDev = 2;
    viss_info->stDevInfo.aBindPhyChn[0] = 1;
    viss_info->stDevInfo.aBindPhyChn[1] = 2;
    viss_info->stDevInfo.aBindPhyChn[2] = -1;
    viss_info->stDevInfo.aBindPhyChn[3] = -1;
    viss_info->stDevInfo.enOutPath = VISS_OUT_PATH_DMA;
    viss_info->stChnInfo.aVissChn[0] = 1;
    viss_info->stChnInfo.aVissChn[1] = 2;
    viss_info->stChnInfo.aVissChn[2] = -1;
    viss_info->stChnInfo.aVissChn[3] = -1;
    viss_info->stChnInfo.enWorkMode = VISS_WORK_MODE_2Chn;
    viss_info->stChnInfo.stChnAttr.u32Align = 32;
    viss_info->stChnInfo.stChnAttr.u32Depth = 2;
    viss_info->stSnsInfo.enSnsType = TP9963_MIPI_1280_720_25FPS_2CH_2LANE_YUV;

#else

	viss_config->s32WorkingVissNum = 2;

	viss_info = &viss_config->astVissInfo[0];
	viss_info->stDevInfo.VissDev = 1;
	viss_info->stDevInfo.aBindPhyChn[0] = 0;
	viss_info->stDevInfo.aBindPhyChn[1] = -1;
	viss_info->stDevInfo.aBindPhyChn[2] = -1;
	viss_info->stDevInfo.aBindPhyChn[3] = -1;
	viss_info->stDevInfo.enOutPath = VISS_OUT_PATH_PIXEL;
	viss_info->stChnInfo.aVissChn[0] = 0;
	viss_info->stChnInfo.aVissChn[1] = -1;
	viss_info->stChnInfo.aVissChn[2] = -1;
	viss_info->stChnInfo.aVissChn[3] = -1;
	viss_info->stChnInfo.enWorkMode = VISS_WORK_MODE_1Chn;
	viss_info->stChnInfo.stChnAttr.u32Align = 32;
	viss_info->stChnInfo.stChnAttr.u32Depth = 2;
	viss_info->stIspInfo.IspDev = 0;
	viss_info->stIspInfo.enRunningMode = ISP_MODE_RUNNING_ONLINE;
	if (0 == access("/data/gc2083", F_OK))
	{
		viss_info->stSnsInfo.enSnsType = GC2083_MIPI_1920_1080_25FPS_RAW10;
	}
	else// if (0 == access("/data/gc2053", F_OK))
	{
		system("echo 1 > /data/gc2053");
		viss_info->stSnsInfo.enSnsType = GC2053_MIPI_1920_1080_25FPS_RAW10;
	}
	viss_info->stIspInfo.stFrc.s32SrcFrameRate = SDR_VISS_FRAMERATE;
	viss_info->stIspInfo.stFrc.s32DstFrameRate = SDR_VISS_FRAMERATE;

	viss_info = &viss_config->astVissInfo[1];
	viss_info->stDevInfo.VissDev = 2;
	viss_info->stDevInfo.aBindPhyChn[0] = 1;
	viss_info->stDevInfo.aBindPhyChn[1] = 2;
	viss_info->stDevInfo.aBindPhyChn[2] = -1;
	viss_info->stDevInfo.aBindPhyChn[3] = -1;
	viss_info->stDevInfo.enOutPath = VISS_OUT_PATH_DMA;
	viss_info->stChnInfo.aVissChn[0] = 1;
	viss_info->stChnInfo.aVissChn[1] = 2;
	viss_info->stChnInfo.aVissChn[2] = -1;
	viss_info->stChnInfo.aVissChn[3] = -1;
	viss_info->stChnInfo.enWorkMode = VISS_WORK_MODE_2Chn;
	viss_info->stChnInfo.stChnAttr.u32Align = 32;
	viss_info->stChnInfo.stChnAttr.u32Depth = 2;
	viss_info->stSnsInfo.enSnsType = TP9963_MIPI_1280_720_25FPS_2CH_2LANE_YUV;

#endif

    return RET_SUCCESS;
}

int MediaVideo_GC2053EncodeInit(SAMPLE_VISS_CONFIG_S* viss_config, VBUF_CONFIG_S* vbuf_config)
{
    int ret = 0;
    SAMPLE_VISS_INFO_S *viss_info = &viss_config->astVissInfo[0];

    EI_S32 vencChn = 0, devIppu = viss_info->stIspInfo.IspDev, chnIppu = 0;

    EI_BOOL chn_enabled[IPPU_PHY_CHN_MAX_NUM] = {EI_FALSE, EI_FALSE, EI_FALSE};
    IPPU_CHN_ATTR_S ippu_chn_attr[IPPU_PHY_CHN_MAX_NUM];
    IPPU_DEV_ATTR_S ippu_dev_attr = { 0 };

    VISS_PIC_TYPE_E stPicType;
    SAMPLE_COMM_VISS_GetPicTypeBySensor(viss_info->stSnsInfo.enSnsType, &stPicType);

    VISS_CHN_ATTR_S stChnAttr;
    SAMPLE_COMM_VISS_GetChnAttrBySns(viss_info->stSnsInfo.enSnsType, &stChnAttr);

    ippu_dev_attr.s32IppuDev        = devIppu;
    ippu_dev_attr.u32InputWidth     = stPicType.stSize.u32Width;
    ippu_dev_attr.u32InputHeight    = stPicType.stSize.u32Height;
    ippu_dev_attr.u32DataSrc        = viss_info->stDevInfo.VissDev;
    ippu_dev_attr.enRunningMode     = IPPU_MODE_RUNNING_ONLINE;

    memset(ippu_chn_attr, 0, sizeof(ippu_chn_attr));
    
    ippu_chn_attr[0].u32Width	= stPicType.stSize.u32Width;
    ippu_chn_attr[0].u32Height	= stPicType.stSize.u32Height;
    ippu_chn_attr[0].u32Align	= 32;
    ippu_chn_attr[0].enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    ippu_chn_attr[0].stFrameRate.s32SrcFrameRate = stChnAttr.stFrameRate.s32SrcFrameRate;
    ippu_chn_attr[0].stFrameRate.s32DstFrameRate = SDR_VENC_FRAMERATE_MAJOR;
    ippu_chn_attr[0].u32Depth	= 2;
    chn_enabled[0] = EI_TRUE;

#if SUBSTREAM
    ippu_chn_attr[1].u32Width	= 640;//stPicType.stSize.u32Width;
    ippu_chn_attr[1].u32Height	= 360;//stPicType.stSize.u32Height;
    ippu_chn_attr[1].u32Align	= 32;
    ippu_chn_attr[1].enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    ippu_chn_attr[1].stFrameRate.s32SrcFrameRate = stChnAttr.stFrameRate.s32SrcFrameRate;
    ippu_chn_attr[1].stFrameRate.s32DstFrameRate = SDR_VENC_FRAMERATE_SUB;
    ippu_chn_attr[1].u32Depth	= 0;
    chn_enabled[1] = EI_TRUE;
#endif

    ret = SAMPLE_COMM_IPPU_Start(devIppu, chn_enabled, &ippu_dev_attr, ippu_chn_attr);
    if (ret)
    {
        PNT_LOGE("SAMPLE_COMM_IPPU_Start:%d err\n", devIppu);
        SAMPLE_COMM_IPPU_Exit(ippu_dev_attr.s32IppuDev);
        return RET_FAILED;
    }
	
	MediaVideoStream_t* handle = &gMediaStream[vencChn];

	SAMPLE_VENC_CONFIG_S venc_config = {0};
    venc_config.enInputFormat   = PIX_FMT_YUV_SEMIPLANAR_420;
    venc_config.u32width	    = handle->param.width;
    venc_config.u32height	    = handle->param.height;
    venc_config.u32bitrate      = handle->param.height==1080?SDR_VENC_BITRATE_1080P:SDR_VENC_BITRATE_CH1720P;
	if (handle->param.framerate != SDR_VENC_FRAMERATE_MAJOR)
	{
   	 	venc_config.u32bitrate = venc_config.u32bitrate/(SDR_VENC_FRAMERATE_MAJOR/handle->param.framerate);
	}
    venc_config.u32srcframerate = ippu_chn_attr[0].stFrameRate.s32DstFrameRate;
    venc_config.u32dstframerate = handle->param.framerate;
    venc_config.u32IdrPeriod    = SDR_VENC_FRAMERATE_MAJOR;
	
	if (gGlobalSysParam->mirror[vencChn])
	{
		venc_config.enMirrorDirection = MIRDIRDIR_HOR;
		venc_config.enRotation = ROTATION_0;
	}
	else
	{
		venc_config.enMirrorDirection = MIRDIRDIR_NONE;
		venc_config.enRotation = ROTATION_0;
	}
		
#if (defined VIDEO_PT_H265)
    ret = SAMPLE_COMM_VENC_Start(vencChn, PT_H265, SAMPLE_RC_CBR, &venc_config, COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
#else
    ret = SAMPLE_COMM_VENC_Start(vencChn, PT_H264, SAMPLE_RC_CBR, &venc_config, COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
#endif
    if (EI_SUCCESS != ret)
    {
        PNT_LOGE("%d ret:%d \n", vencChn, ret);
        goto exit1;
    }
    
    MDP_CHN_S src_viss = { 0 }, sink_isp = { 0 };

    src_viss.enModId = EI_ID_IPPU;
    src_viss.s32DevId = devIppu;
    src_viss.s32ChnId = chnIppu;

    sink_isp.enModId = EI_ID_VPU;
    sink_isp.s32DevId = 0;
    sink_isp.s32ChnId = vencChn;
    ret = EI_MI_MLINK_Link(&src_viss, &sink_isp);
    if (ret)
    {
        PNT_LOGE("EI_MI_MLINK_Link:%d err\n", devIppu);
        goto exit2;
    }

#if SUBSTREAM
    vencChn = vencChn + MAX_VIDEO_NUM;
	handle = &gMediaStream[vencChn];
	SAMPLE_VENC_CONFIG_S venc_config_sub = {0};
    venc_config_sub.enInputFormat   = PIX_FMT_YUV_SEMIPLANAR_420;
    venc_config_sub.u32width	    = handle->param.width;
    venc_config_sub.u32height	    = handle->param.height;
    venc_config_sub.u32bitrate      = SDR_VENC_BITRATE_SUB;
    venc_config_sub.u32srcframerate = ippu_chn_attr[1].stFrameRate.s32DstFrameRate;
    venc_config_sub.u32dstframerate = handle->param.framerate;
    venc_config_sub.u32IdrPeriod    = venc_config_sub.u32dstframerate;

	if (gGlobalSysParam->mirror[vencChn%MAX_VIDEO_NUM])
	{
		venc_config_sub.enMirrorDirection = MIRDIRDIR_HOR;
		venc_config_sub.enRotation = ROTATION_0;
	}
	else
	{
		venc_config_sub.enMirrorDirection = MIRDIRDIR_NONE;
		venc_config_sub.enRotation = ROTATION_0;
	}
	
    ret = SAMPLE_COMM_VENC_Start(vencChn, PT_H264, SAMPLE_RC_CBR, &venc_config_sub, COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
    if (EI_SUCCESS != ret)
    {
        PNT_LOGE("%d ret:%d \n", vencChn, ret);
    }
    
    MDP_CHN_S sub_src_viss = { 0 }, sub_sink_isp = { 0 };

    sub_src_viss.enModId = EI_ID_IPPU;
    sub_src_viss.s32DevId = devIppu;
    sub_src_viss.s32ChnId = 1;

    sub_sink_isp.enModId = EI_ID_VPU;
    sub_sink_isp.s32DevId = 0;
    sub_sink_isp.s32ChnId = vencChn;
    ret = EI_MI_MLINK_Link(&sub_src_viss, &sub_sink_isp);
    if (ret)
    {
        PNT_LOGE("EI_MI_MLINK_Link:%d err\n", devIppu);
    }
#endif

    return RET_SUCCESS;

exit2:
    EI_MI_MLINK_UnLink(&src_viss, &sink_isp);

exit1:
    SAMPLE_COMM_IPPU_Stop(devIppu, chn_enabled);
    SAMPLE_COMM_IPPU_Exit(devIppu);

    return RET_FAILED;
}

int MediaVideo_GC2053EncodeDeInit(SAMPLE_VISS_CONFIG_S* viss_config, VBUF_CONFIG_S* vbuf_config)
{
    int ret = 0;
    SAMPLE_VISS_INFO_S *viss_info = &viss_config->astVissInfo[0];

    EI_S32 vencChn = 0, devIppu = viss_info->stIspInfo.IspDev, chnIppu = 0;
    EI_BOOL chn_enabled[IPPU_PHY_CHN_MAX_NUM] = {EI_FALSE, EI_FALSE, EI_FALSE};

    MDP_CHN_S src_viss = { 0 }, sink_isp = { 0 };
    src_viss.enModId = EI_ID_IPPU;
    src_viss.s32DevId = devIppu;
    src_viss.s32ChnId = chnIppu;
    sink_isp.enModId = EI_ID_VPU;
    sink_isp.s32DevId = 0;
    sink_isp.s32ChnId = vencChn;
    EI_MI_MLINK_UnLink(&src_viss, &sink_isp);
    EI_MI_VENC_DestroyChn(vencChn);
	
#if SUBSTREAM
    vencChn = vencChn + MAX_VIDEO_NUM;
    MDP_CHN_S sub_src_viss = { 0 }, sub_sink_isp = { 0 };
    sub_src_viss.enModId = EI_ID_IPPU;
    sub_src_viss.s32DevId = devIppu;
    sub_src_viss.s32ChnId = 1;
    sub_sink_isp.enModId = EI_ID_VPU;
    sub_sink_isp.s32DevId = 0;
    sub_sink_isp.s32ChnId = vencChn;
    EI_MI_MLINK_UnLink(&sub_src_viss, &sub_sink_isp);
    EI_MI_VENC_DestroyChn(vencChn);
#endif
	
    chn_enabled[0] = EI_TRUE;
#if SUBSTREAM
    chn_enabled[1] = EI_TRUE;
#endif

    SAMPLE_COMM_IPPU_Stop(devIppu, chn_enabled);
   	SAMPLE_COMM_IPPU_Exit(devIppu);

    return RET_SUCCESS;
}

extern EI_S32  SAMPLE_COMM_VENC_Creat(VC_CHN VencChn, PAYLOAD_TYPE_E enType,
    SAMPLE_RC_E RcMode, SAMPLE_VENC_CONFIG_S *pstVencConfig,
    COMPRESS_MODE_E enCompressMode, VENC_GOP_MODE_E GopMode);
int MediaVideo_TP9963EncodeInit(SAMPLE_VISS_CONFIG_S* viss_config, VBUF_CONFIG_S* vbuf_config)
{
	SAMPLE_VENC_CONFIG_S venc_config = {0};
	
#ifdef DISABLECH1
	int vechChnStart = 0;
    SAMPLE_VISS_INFO_S *viss_info = &viss_config->astVissInfo[0];
#else
	int vechChnStart = 1;
    SAMPLE_VISS_INFO_S *viss_info = &viss_config->astVissInfo[1];
#endif

	for (int i=0; i<(MAX_VIDEO_NUM-1); i++)
	{
	    int devChn = viss_info->stDevInfo.VissDev;
#ifdef DISABLECH2
	    int vissChn = viss_info->stChnInfo.aVissChn[1];
#else
	    int vissChn = viss_info->stChnInfo.aVissChn[i];
#endif
	    int vencChn = vechChnStart+i;
		if (-1 == vissChn)
		{
			break;
		}
		MediaVideoStream_t* handle = &gMediaStream[vencChn];

	    venc_config.enInputFormat   = PIX_FMT_YUV_SEMIPLANAR_420;
	    venc_config.u32width	    = handle->param.width;
	    venc_config.u32height	    = handle->param.height;
	    venc_config.u32bitrate      = SDR_VENC_BITRATE_720P;
		if (handle->param.framerate != SDR_VENC_FRAMERATE_MAJOR)
		{
	   	 	venc_config.u32bitrate = venc_config.u32bitrate/(SDR_VENC_FRAMERATE_MAJOR/handle->param.framerate);
		}
	    venc_config.u32srcframerate = SDR_VISS_FRAMERATE;
	    venc_config.u32dstframerate = handle->param.framerate;
	    venc_config.u32IdrPeriod    = SDR_VENC_FRAMERATE_MAJOR;
		
		if (gGlobalSysParam->mirror[vencChn])
		{
			venc_config.enMirrorDirection = MIRDIRDIR_HOR;
			venc_config.enRotation = ROTATION_0;
		}
		else
		{
			venc_config.enMirrorDirection = MIRDIRDIR_NONE;
			venc_config.enRotation = ROTATION_0;
		}
		
#if (defined VIDEO_PT_H265)
	    int ret = SAMPLE_COMM_VENC_Creat(vencChn, PT_H265, SAMPLE_RC_CBR, &venc_config, COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
#else
	    int ret = SAMPLE_COMM_VENC_Creat(vencChn, PT_H264, SAMPLE_RC_CBR, &venc_config, COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
#endif
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

	    vencChn = vencChn + MAX_VIDEO_NUM;
		handle = &gMediaStream[vencChn];

	    sub_venc_config.enInputFormat   = PIX_FMT_YUV_SEMIPLANAR_420;
	    sub_venc_config.u32width	    = handle->param.width;
	    sub_venc_config.u32height	    = handle->param.height;
	    sub_venc_config.u32bitrate      = SDR_VENC_BITRATE_SUB;
	    sub_venc_config.u32srcframerate = SDR_VISS_FRAMERATE;
	    sub_venc_config.u32dstframerate = handle->param.framerate;
	    sub_venc_config.u32IdrPeriod    = sub_venc_config.u32dstframerate;

		if (gGlobalSysParam->mirror[vencChn%MAX_VIDEO_NUM])
		{
			sub_venc_config.enMirrorDirection = MIRDIRDIR_HOR;
			sub_venc_config.enRotation = ROTATION_0;
		}
		else
		{
			venc_config.enMirrorDirection = MIRDIRDIR_NONE;
			venc_config.enRotation = ROTATION_0;
		}

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
	}

    return RET_SUCCESS;
}

int MediaVideo_TP9963EncodeDeInit(SAMPLE_VISS_CONFIG_S* viss_config, VBUF_CONFIG_S* vbuf_config)
{
#ifdef DISABLECH1
	int vechChnStart = 0;
    SAMPLE_VISS_INFO_S *viss_info = &viss_config->astVissInfo[0];
#else
	int vechChnStart = 1;
    SAMPLE_VISS_INFO_S *viss_info = &viss_config->astVissInfo[1];
#endif

	SAMPLE_VENC_CONFIG_S venc_config = {0};

	for (int i=0; i<(MAX_VIDEO_NUM-1); i++)
	{
	    int devChn = viss_info->stDevInfo.VissDev;
#ifdef DISABLECH2
	    int vissChn = viss_info->stChnInfo.aVissChn[1];
#else
	    int vissChn = viss_info->stChnInfo.aVissChn[i];
#endif
	    int vencChn = vechChnStart+i;
		if (-1 == vissChn)
		{
			break;
		}
		
	    SAMPLE_COMM_VISS_UnLink_VPU(devChn, vissChn, vencChn);
	    EI_MI_VENC_DestroyChn(vencChn);

#if SUBSTREAM
	    vencChn = vencChn + MAX_VIDEO_NUM;
	    SAMPLE_COMM_VISS_UnLink_VPU(devChn, vissChn, vencChn);
	    EI_MI_VENC_DestroyChn(vencChn);
#endif
	}

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

	if (RET_FAILED == fallocate_video_file(filename, currentTimeMillis()/1000, handle->venChn, handle->param.videoSize))
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

	muxer_hdl->timeStamp = currentTimeMillis()/1000;

    muxer_hdl->muxer_parm.nb_streams = (handle->venChn==AUDIO_BIND_CHN)?2:1;
	if (SDR_VENC_FRAMERATE_MAJOR != handle->param.framerate)
	{
		muxer_hdl->muxer_parm.nb_streams = 1;
	}
    muxer_hdl->start_pts[VIDEO_INDEX] = -1;
    muxer_hdl->start_pts[AUDIO_INDEX] = -1;
	
    muxer_hdl->muxer_parm.mode = AV_MUXER_TYPE_MP4;
    muxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].codec_type = AV_MEDIA_TYPE_VIDEO;
#if (defined VIDEO_PT_H265)
    muxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].codec_id = AV_CODEC_TYPE_HEVC;
#else
    muxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].codec_id = AV_CODEC_TYPE_H264;
#endif
    muxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].sample_rate = 20 * VIDEOTIMEFRAME;
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
    muxer_hdl->muxer_parm.fix_duration_param.file_duration = 65;

    muxer_context_ctrl(muxer_hdl->muxer, MUXER_CONTEXT_SET_FILE_NAME, filename);

    if (muxer_context_start(muxer_hdl->muxer, &muxer_hdl->muxer_parm))
    {
    	//muxer_context_destroy(muxer_hdl->muxer);
		//muxer_hdl->muxer = NULL;
    	PNT_LOGE("start muxer failed.");
		return RET_FAILED;
    }

	char thumbName[128] = { 0 };
	get_thumbname_by_videoname(thumbName, filename);
	MediaSnap_CreateSnap(handle->venChn, 1, 160, 120, 0, thumbName);

	return RET_SUCCESS;
}

static void FrameListContentFree(void* content)
{
	FrameInfo_t* frame = (FrameInfo_t*)content;

	if (NULL == frame)
	{
		return;
	}

	if (frame->frameBuffer)
		free(frame->frameBuffer);
	frame->frameBuffer = NULL;
	free(frame);
	content = NULL;
}

static void get_extradata(sample_muxer_hdl_t *pmuxer_hdl, FrameInfo_t* frame)
{
	int i = 0, idrOffset = 0;

	if (pmuxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].extradata != NULL)
	{
		return;
	}

	while (i<frame->frameSize)
	{
#if (defined VIDEO_PT_H265)
		if (0x00 == frame->frameBuffer[i] && 0x00 == frame->frameBuffer[i+1] && 0x00 == frame->frameBuffer[i+2] && 0x01 == frame->frameBuffer[i+3])
		{
			if (0x40 == frame->frameBuffer[i+4]) // vps
			{
			}
			else if (0x42 == frame->frameBuffer[i+4]) // sps
			{
			}
			else if (0x44 == frame->frameBuffer[i+4]) // pps
			{
			}
			else if (0x26 == frame->frameBuffer[i+4])
			{
				pmuxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].extradata_size = i;
				pmuxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].extradata = (uint8_t*)malloc(pmuxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].extradata_size);
				if (NULL != pmuxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].extradata)
				{
					memcpy(pmuxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].extradata, frame->frameBuffer, i);
				}
				break;
			}
		}
#else
		if (0x00 == frame->frameBuffer[i] && 0x00 == frame->frameBuffer[i+1] && 0x00 == frame->frameBuffer[i+2] && 0x01 == frame->frameBuffer[i+3])
		{
			if (0x67 == frame->frameBuffer[i+4])
			{
			}
			else if (0x68 == frame->frameBuffer[i+4])
			{
			}
			else if (0x65 == frame->frameBuffer[i+4])
			{
				pmuxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].extradata_size = i;
				pmuxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].extradata = (uint8_t*)malloc(pmuxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].extradata_size);
				if (NULL != pmuxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].extradata)
				{
					memcpy(pmuxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].extradata, frame->frameBuffer, i);
				}
				break;
			}
		}
#endif

		i++;
	}
}

#define user_RB16(x) ((((const uint8_t *)(x))[0] << 8) | ((const uint8_t *)(x))[1])

void aenc_frame_callback(int bindVenc, uint8_t* data, int len)
{
	AUDIO_STREAM_S *pstStream = (AUDIO_STREAM_S *)data;

    MediaVideoStream_t *handle = (MediaVideoStream_t*)&gMediaStream[bindVenc];

	FrameInfo_t* frame = (FrameInfo_t*)malloc(sizeof(FrameInfo_t));
	memset(frame, 0, sizeof(FrameInfo_t));
	frame->frameSize = pstStream->u32Len;
	frame->timeStamp = pstStream->u64TimeStamp;
	frame->frameType = (pstStream->u32Len > 2 && ((user_RB16(pstStream->pStream) & 0xfff0) == 0xfff0));
	frame->frameBuffer = (char*)malloc(frame->frameSize);
	if (NULL != frame->frameBuffer)
	{
		memcpy(frame->frameBuffer, pstStream->pStream, pstStream->u32Len);
	
		if (queue_list_push(&handle->frameAudio, frame))
		{
			FrameListContentFree(frame);
		}
	}
	else
	{
		free(frame);
	}
}

static void *venc_muxer_recorder_task(void* p)
{
    MediaVideoStream_t *handle = (MediaVideoStream_t*)p;
	
	int initFlag = 0, newFileFlag = 0, wait = 0;
	muxer_packet_t pin = {0};
    sample_muxer_hdl_t *pmuxer_hdl = &gRecordMuxerHandle[handle->venChn];
	
	memset(pmuxer_hdl, 0, sizeof(sample_muxer_hdl_t));

	FrameInfo_t frame = { 0 };
	FrameInfo_t frameAudio = { 0 };
	
	pmuxer_hdl->muxer = muxer_context_create();

	while (EI_TRUE == handle->bThreadStart)
	{
		if (REC_STOP == handle->recStatus && !initFlag)
		{
			SleepMs(40);
			continue;
		}

		wait = 2;
		
		if (0 == get_video_stream(handle->venChn, &frame))
		{
			pthread_mutex_lock(&handle->mutex);
			
			if (frame.frameType == DATA_TYPE_I_FRAME || frame.frameType == DATA_TYPE_INITPACKET)
	        {
	        	get_extradata(pmuxer_hdl, &frame);

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
					if (RET_SUCCESS == MediaVideo_RecordMuxerInit(pmuxer_hdl, handle->param.width, handle->param.height, handle))
					{
						initFlag = 1;
						handle->recStatus = REC_RECORDING;
						
						if (SDR_VENC_FRAMERATE_MAJOR == handle->param.framerate)
						{
							pmuxer_hdl->start_pts[0] = frame.timeStamp/1000;
						}
						else
						{
							pmuxer_hdl->start_pts[0] = 0;//frame.timeStamp/1000;
						}
					}
	            }
	        }

			if (initFlag)
			{
				memset(&pin, 0, sizeof(pin));
		    	pin.stream_index = VIDEO_INDEX;				
		        pin.data = (uint8_t*)frame.frameBuffer;
		        pin.size = frame.frameSize;
				
				if (SDR_VENC_FRAMERATE_MAJOR == handle->param.framerate)
				{
			    	pin.timestamp = (frame.timeStamp / 1000 - pmuxer_hdl->start_pts[0]);
				}
				else
				{
			    	pin.timestamp = pmuxer_hdl->start_pts[0];
				}
				
			    if (frame.frameType == DATA_TYPE_I_FRAME)
				{
			        pin.flags |= AV_PKT_FLAG_KEY;
			    }
			    if (frame.frameType == DATA_TYPE_INITPACKET)
				{
			        pin.flags |= AV_PKT_INIT_FRAME;
			    }
			    if (frame.frameType == DATA_TYPE_P_FRAME)
			    {
			    	pin.flags |= AV_PKT_P_FRAME;
			    }
			    if (frame.frameType == DATA_TYPE_B_FRAME)
			    {
			    	pin.flags |= AV_PKT_B_FRAME;
			    }
			   	pin.flags |= AV_PKT_ENDING_FRAME;

			    muxer_context_write(pmuxer_hdl->muxer, &pin);
				
				if (SDR_VENC_FRAMERATE_MAJOR != handle->param.framerate)
				{
					pmuxer_hdl->start_pts[0] += 66;
				}
			}
			
			pthread_mutex_unlock(&handle->mutex);

			wait --;
		}

		if (pmuxer_hdl->muxer_parm.nb_streams >= 2)
		{
			if (handle->recStatus == REC_RECORDING && initFlag)
			{
				if (0 == get_audio_stream(handle->venChn, &frameAudio))
				{
					memset(&pin, 0, sizeof(pin));
					
				    if (frameAudio.frameType)
					{
				        pin.data = (uint8_t*)(frameAudio.frameBuffer + 7);
				        if (frameAudio.frameSize - 7 > 0)
				            pin.size = frameAudio.frameSize - 7;
				    }
					else
					{
				        pin.data = (uint8_t*)frameAudio.frameBuffer;
				        pin.size = frameAudio.frameSize;
				    }
				    pin.stream_index = AUDIO_INDEX;
				    if (pmuxer_hdl->start_pts[1] < 0)
				        pmuxer_hdl->start_pts[1] = frameAudio.timeStamp;
				    pin.timestamp = (frameAudio.timeStamp-pmuxer_hdl->start_pts[1]) / 1000;

					muxer_context_write(pmuxer_hdl->muxer, &pin);
				}
			}
			
			wait --;
		}

		if (2 == wait)
		{
			SleepMs(30);
		}
	}
	
	if (frame.frameBuffer)
		free(frame.frameBuffer);
	if (frameAudio.frameBuffer)
		free(frameAudio.frameBuffer);

	return NULL;
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

	unsigned int streamSize = 0, bufferSize = 0, lost_frame_count = 0;
	unsigned char* streamBuffer = NULL;

	if (handle->param.width == 1920)
	{
		streamBuffer = (unsigned char*)malloc(800*1024);
		bufferSize = 800*1024;
	}
	else
	{
		streamBuffer = (unsigned char*)malloc(400*1024);
		bufferSize = 400*1024;
	}
    while (EI_TRUE == handle->bThreadStart)
    {
        ret = EI_MI_VENC_GetStream(handle->venChn, &stStream, 100);
        if (ret == EI_ERR_VENC_NOBUF)
        {
        	lost_frame_count ++;
			if (lost_frame_count > 50)
			{
				PNT_LOGE("--fatal error, venc[%d] continue lost frames count overflow. reboot system force", handle->venChn);
        		system("reboot -f");
			}
            usleep(5 * 1000);
            continue;
        }
        else if (ret != EI_SUCCESS)
        {
        	//system("reboot -f");
            PNT_LOGE("get chn-%d error : %d\n", handle->venChn, ret);
            //break;
        }

		lost_frame_count = 0;
		
        if (stStream.pstPack.u32Len[0] == 0)
        {
            PNT_LOGE("%d ch %d pstPack.u32Len:%d-%d, ret:%x\n", __LINE__, handle->venChn, stStream.pstPack.u32Len[0], stStream.pstPack.u32Len[1], ret);
        }
		
		if (NULL != streamBuffer)
		{
		    if (stStream.pstPack.pu8Addr[1])
			{
				if (streamSize+stStream.pstPack.u32Len[0] <= bufferSize)
				{
					memcpy(streamBuffer + streamSize, stStream.pstPack.pu8Addr[0], stStream.pstPack.u32Len[0]);
					streamSize += stStream.pstPack.u32Len[0];
					
					if (streamSize+stStream.pstPack.u32Len[1] <= bufferSize)
					{
				        memcpy(streamBuffer + streamSize, stStream.pstPack.pu8Addr[1], stStream.pstPack.u32Len[1]);
						streamSize += stStream.pstPack.u32Len[1];
					}
				}
				else
				{
					PNT_LOGE("buffer overflow.");
				}
		    }
			else
			{
				if (streamSize+stStream.pstPack.u32Len[0] <= bufferSize)
				{
			        memcpy(streamBuffer + streamSize, stStream.pstPack.pu8Addr[0], stStream.pstPack.u32Len[0]);
					streamSize += stStream.pstPack.u32Len[0];
				}
				else
				{
					PNT_LOGE("buffer overflow.");
				}
		    }
		}

		if (stStream.pstPack.bFrameEnd)
		{
			FrameInfo_t* frame = (FrameInfo_t*)malloc(sizeof(FrameInfo_t));
			memset(frame, 0, sizeof(FrameInfo_t));
			frame->frameSize = streamSize;
			frame->timeStamp = stStream.pstPack.u64PTS;
			frame->frameType = stStream.pstPack.enDataType;
			frame->frameBuffer = (char*)malloc(frame->frameSize);
			if (NULL != frame->frameBuffer)
			{
				memcpy(frame->frameBuffer, streamBuffer, streamSize);

			    if (queue_list_push(&handle->frameH264, frame))
			    {
			    	FrameListContentFree(frame);
			    }
			}
			else
			{
				free(frame);
			}

			handle->curFramePts = stStream.pstPack.u64PTS;

			streamSize = 0;
		}

        ret = EI_MI_VENC_ReleaseStream(handle->venChn, &stStream);
        if (ret != EI_SUCCESS)
        {
            PNT_LOGE("release stream chn-%d error : %d\n", handle->venChn, ret);
            break;
        }
    }

	EI_MI_VENC_StopRecvFrame(handle->venChn);

	if (NULL != streamBuffer)
	{
		free(streamBuffer);
	}
    PNT_LOGE("task %s exit\n", thread_name);

    return NULL;
}

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

	unsigned int streamSize = 0, bufferSize = 0;
	unsigned char* streamBuffer = NULL;
	
	streamBuffer = (unsigned char*)malloc(300*1024);
	bufferSize = 300*1024;

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

		if (NULL != streamBuffer)
		{
		    if (stStream.pstPack.pu8Addr[1])
			{
				if (streamSize+stStream.pstPack.u32Len[0] <= bufferSize)
				{
					memcpy(streamBuffer + streamSize, stStream.pstPack.pu8Addr[0], stStream.pstPack.u32Len[0]);
					streamSize += stStream.pstPack.u32Len[0];
					
					if (streamSize+stStream.pstPack.u32Len[1] <= bufferSize)
					{
				        memcpy(streamBuffer + streamSize, stStream.pstPack.pu8Addr[1], stStream.pstPack.u32Len[1]);
						streamSize += stStream.pstPack.u32Len[1];
					}
				}
				else
				{
					PNT_LOGE("buffer overflow.");
				}
		    }
			else
			{
				if (streamSize+stStream.pstPack.u32Len[0] <= bufferSize)
				{
			        memcpy(streamBuffer + streamSize, stStream.pstPack.pu8Addr[0], stStream.pstPack.u32Len[0]);
					streamSize += stStream.pstPack.u32Len[0];
				}
				else
				{
					PNT_LOGE("buffer overflow.");
				}
		    }
		}

		if (stStream.pstPack.bFrameEnd)
		{
			FrameInfo_t* frame = (FrameInfo_t*)malloc(sizeof(FrameInfo_t));
			memset(frame, 0, sizeof(FrameInfo_t));
			frame->frameSize = streamSize;
			frame->timeStamp = stStream.pstPack.u64PTS;
			frame->frameType = stStream.pstPack.enDataType;
			frame->frameBuffer = (char*)malloc(frame->frameSize);
			if (NULL != frame->frameBuffer)
			{
				memcpy(frame->frameBuffer, streamBuffer, streamSize);

			    if (queue_list_push(&handle->frameH264, frame))
			    {
			    	FrameListContentFree(frame);
			    }
			}
			else
			{
				free(frame);
			}

			handle->curFramePts = stStream.pstPack.u64PTS;

			streamSize = 0;
		}

        ret = EI_MI_VENC_ReleaseStream(handle->venChn, &stStream);
        if (ret != EI_SUCCESS)
        {
            PNT_LOGE("release stream chn-%d error : %d\n", handle->venChn, ret);
            break;
        }
    }

	EI_MI_VENC_StopRecvFrame(handle->venChn);

	if (streamBuffer)
		free(streamBuffer);

    PNT_LOGI("task %s exit\n", thread_name);

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

bool_t MediaVideo_GetRecordStatus(int chn)
{
	if (chn >= MAX_VIDEO_NUM)
	{
		return 1;
	}
	PNT_LOGE("recorder status : %d", gMediaStream[chn].recStatus);
	return (gMediaStream[chn].recStatus != REC_STOP);
}

unsigned int MediaVideo_GetRecordDuration(int chn)
{
    sample_muxer_hdl_t *pmuxer_hdl = &gRecordMuxerHandle[chn];
	
	if (gMediaStream[chn].recStatus == REC_STOP)
	{
		return 0;
	}
	else if (gMediaStream[chn].recStatus == REC_START)
	{
		return 0;
	}
	else
	{
		return (currentTimeMillis()/1000 - pmuxer_hdl->timeStamp);
	}
}

void MediaVideo_StartGetFrame(MediaVideoStream_t* handle, int type)
{
    handle->bThreadStart = EI_TRUE;
    handle->type = type;

    int ret = 0;

    pthread_mutex_init(&handle->mutex, NULL);
    
    if (CHANNEL_MASTER == type)
    {
		queue_list_create(&handle->frameH264, 32);
		queue_list_set_free(&handle->frameH264, FrameListContentFree);
		queue_list_create(&handle->frameAudio, 16);
		queue_list_set_free(&handle->frameAudio, FrameListContentFree);
	
        ret = pthread_create(&handle->vencPid, 0, _get_venc_stream_proc, (EI_VOID *)handle);

		pthread_t pid;
        ret = pthread_create(&pid, 0, venc_muxer_recorder_task, (EI_VOID *)handle);
    }
    else
    {
		queue_list_create(&handle->frameH264, 32);
		queue_list_set_free(&handle->frameH264, FrameListContentFree);
		queue_list_create(&handle->frameAudio, 16);
		queue_list_set_free(&handle->frameAudio, FrameListContentFree);
		
        ret = pthread_create(&handle->vencPid, 0, _get_venc_substream_proc, (EI_VOID *)handle);
    }
    if (ret)
    {
        PNT_LOGE("errno=%d, reason(%s)\n", errno, strerror(errno));
    }
}

static void MediaVideo_ParamInit(void)
{
	int chn = 0;

	int major_framrates[] = { SDR_VENC_FRAMERATE_MAJOR, 1, 3, 5 };
	
#ifdef DISABLECH1
#else
	MediaVideoStream_t* handle = &gMediaStream[chn];
	memset(handle, 0, sizeof(MediaVideoStream_t));

	handle->param.framerate = major_framrates[gGlobalSysParam->timelapse_rate];
	handle->param.width = CH1_VIDEO_WIDTH;
	handle->param.height = CH1_VIDEO_HEIGHT;
	handle->param.srcwidth = 1920;
	handle->param.srcheight = 1080;
	handle->param.needCache = 1;
	handle->param.isValid = 1;
	handle->param.needRec = 1;
	handle->param.videoSize = CH1_VIDEO_STREAM_SIZE;
	handle->param.minutePerFile = SDR_VENC_FRAMERATE_MAJOR/handle->param.framerate;

	handle->recStatus = REC_STOP;
	
	handle = &gMediaStream[chn+MAX_VIDEO_NUM];
	memset(handle, 0, sizeof(MediaVideoStream_t));
	handle->param.framerate = SDR_VENC_FRAMERATE_SUB;
	handle->param.width = CH1SUB_VIDEO_WIDTH;
	handle->param.height = CH1SUB_VIDEO_HEIGHT;
	handle->param.srcwidth = 1920;
	handle->param.srcheight = 1080;
	handle->param.needCache = 0;
	chn ++;
#endif
	
#ifdef DISABLECH2
#else
	handle = &gMediaStream[chn];
	memset(handle, 0, sizeof(MediaVideoStream_t));
	handle->param.framerate = major_framrates[gGlobalSysParam->timelapse_rate];
	handle->param.width = CH2_VIDEO_WIDTH;
	handle->param.height = CH2_VIDEO_HEIGHT;
	handle->param.srcwidth = 1280;
	handle->param.srcheight = 720;
	handle->param.needCache = 1;
	handle->param.isValid = 1;
	handle->param.needRec = 1;
	handle->param.videoSize = CH2_VIDEO_STREAM_SIZE;
	handle->recStatus = REC_STOP;
	handle->param.minutePerFile = SDR_VENC_FRAMERATE_MAJOR/handle->param.framerate;
	
	handle = &gMediaStream[chn+MAX_VIDEO_NUM];
	memset(handle, 0, sizeof(MediaVideoStream_t));
	handle->param.framerate = SDR_VENC_FRAMERATE_SUB;
	handle->param.width = CH2SUB_VIDEO_WIDTH;
	handle->param.height = CH2SUB_VIDEO_HEIGHT;
	handle->param.srcwidth = 1280;
	handle->param.srcheight = 720;
	handle->param.needCache = 0;
	chn ++;
#endif
	
#ifdef DISABLECH3
#else
	handle = &gMediaStream[chn];
	memset(handle, 0, sizeof(MediaVideoStream_t));
	handle->param.framerate = major_framrates[gGlobalSysParam->timelapse_rate];
	handle->param.width = CH3_VIDEO_WIDTH;
	handle->param.height = CH3_VIDEO_HEIGHT;
	handle->param.srcwidth = 1280;
	handle->param.srcheight = 720;
	handle->param.needCache = 1;
	handle->param.isValid = 1;
	handle->param.needRec = 1;
	handle->param.videoSize = CH3_VIDEO_STREAM_SIZE;
	handle->recStatus = REC_STOP;
	handle->param.minutePerFile = SDR_VENC_FRAMERATE_MAJOR/handle->param.framerate;
	
	handle = &gMediaStream[chn+MAX_VIDEO_NUM];
	memset(handle, 0, sizeof(MediaVideoStream_t));
	handle->param.framerate = SDR_VENC_FRAMERATE_SUB;
	handle->param.width = CH3SUB_VIDEO_WIDTH;
	handle->param.height = CH3SUB_VIDEO_HEIGHT;
	handle->param.srcwidth = 1280;
	handle->param.srcheight = 720;
	handle->param.needCache = 0;
	chn ++;
#endif
}

int MediaVideo_RecorderInit(void)
{
    int ret = 0;
    SAMPLE_VISS_CONFIG_S viss_config = { 0 };
    VBUF_CONFIG_S vbuf_config = {0};
    video_encode_info_t vencInfo = { 0 };

	MediaVideo_ParamInit();

    MediaVideo_VissConfigInit(&viss_config);

    ret = MediaVideo_BuffpollInit(&vbuf_config);
    if (RET_SUCCESS != ret)
    {
        PNT_LOGE("MediaVideo_BuffpollInit ret failed.");
        return RET_FAILED;
    }
	
#ifdef DISABLECH1
#else
    ret = SAMPLE_COMM_ISP_Start(&viss_config.astVissInfo[0]);
    if (ret) 
    {
        PNT_LOGE("SAMPLE_COMM_ISP_Start ret failed.");
        goto exit_fail_isp;
    }
#endif

    ret = SAMPLE_COMM_VISS_StartViss(&viss_config);
    if (RET_SUCCESS != ret)
    {
        PNT_LOGE("SAMPLE_COMM_VISS_StartViss ret failed.");
        goto exit_fail_viss;
    }

#ifdef DISABLECH1
#else
    ret = MediaVideo_GC2053EncodeInit(&viss_config, &vbuf_config);
    if (RET_SUCCESS != ret)
    {
        PNT_LOGE("MediaVideo_GC2053EncodeInit ret failed.");
        goto exit_fail_2053;
    }
#endif

    ret = MediaVideo_TP9963EncodeInit(&viss_config, &vbuf_config);
    if (RET_SUCCESS != ret)
    {
        PNT_LOGE("MediaVideo_TP9963EncodeInit ret failed.");
        goto exit_fail_9963;
    }

    MDP_CHN_S stRgnChn = { 0 };
	int osdChn = 0;

#ifdef DISABLECH1
#else
    stRgnChn.enModId = EI_ID_IPPU;
    stRgnChn.s32DevId = 0;
    stRgnChn.s32ChnId = 0;
    MediaOsd_InitChn(&stRgnChn, 1920, OSD_AREA_HEIGHT, osdChn);
	osdChn ++;
#endif
	
#ifdef DISABLECH2
#else
    stRgnChn.enModId = EI_ID_VISS;
    stRgnChn.s32DevId = 2;
    stRgnChn.s32ChnId = 1;
    MediaOsd_InitChn(&stRgnChn, 1280, OSD_AREA_HEIGHT, osdChn);
	osdChn ++;
#endif
	
#ifdef DISABLECH3
#else
    stRgnChn.enModId = EI_ID_VISS;
    stRgnChn.s32DevId = 2;
    stRgnChn.s32ChnId = 2;
    MediaOsd_InitChn(&stRgnChn, 1280, OSD_AREA_HEIGHT, osdChn);
	osdChn ++;
#endif

#if SUBSTREAM
#ifdef DISABLECH1
#else
	stRgnChn.enModId = EI_ID_IPPU;
	stRgnChn.s32DevId = 0;
	stRgnChn.s32ChnId = 1;
	MediaOsd_InitChn(&stRgnChn, 640, 72, osdChn);
	osdChn ++;
#endif
#endif

	if (gGlobalSysParam->micEnable)
	{
   	 	MediaAudio_Start(AUDIO_BIND_CHN, aenc_frame_callback);
	}

    return RET_SUCCESS;

exit_fail_9963:
	MediaVideo_GC2053EncodeDeInit(&viss_config, &vbuf_config);
	
#ifdef DISABLECH1
#else
exit_fail_2053:
	SAMPLE_COMM_VISS_StopViss(&viss_config);

exit_fail_viss:
	SAMPLE_COMM_ISP_Stop(viss_config.astVissInfo[0].stIspInfo.IspDev);
#endif

exit_fail_isp:
    MediaVideo_BuffpollDeinit(&vbuf_config);

    return RET_FAILED;
}

int MediaVideo_Start(void)
{	
    MediaOsd_Start();

	MediaSnap_Init();

	for (int i=0; i<MAX_VIDEO_NUM; i++)
	{
		MediaVideoStream_t* handle = &gMediaStream[i];
		
		handle->venChn = i;
	    MediaVideo_StartGetFrame(handle, CHANNEL_MASTER);
		if (handle->param.needCache)
		{
			MediaCache_Create(&handle->cache, i, handle);
		}
	}

	for (int i=MAX_VIDEO_NUM; i<2*MAX_VIDEO_NUM; i++)
	{
		MediaVideoStream_t* handle = &gMediaStream[i];
		
		handle->venChn = i;
	    MediaVideo_StartGetFrame(handle, CHANNEL_CHILD);
		if (handle->param.needCache)
		{
			MediaCache_Create(&handle->cache, i, handle);
		}
	}

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
	
	MDP_CHN_S stRgnChn = { 0 };
	int osdChn = 0;

#ifdef DISABLECH1
#else
    stRgnChn.enModId = EI_ID_IPPU;
    stRgnChn.s32DevId = 0;
    stRgnChn.s32ChnId = 0;
    MediaOsd_DeInitChn(&stRgnChn, osdChn);
	osdChn ++;
#endif
	
#ifdef DISABLECH2
#else
    stRgnChn.enModId = EI_ID_VISS;
    stRgnChn.s32DevId = 2;
    stRgnChn.s32ChnId = 1;
    MediaOsd_DeInitChn(&stRgnChn, osdChn);
	osdChn ++;
#endif
	
#ifdef DISABLECH3
#else
    stRgnChn.enModId = EI_ID_VISS;
    stRgnChn.s32DevId = 2;
    stRgnChn.s32ChnId = 2;
    MediaOsd_DeInitChn(&stRgnChn, osdChn);
	osdChn ++;
#endif

#if SUBSTREAM
#ifdef DISABLECH1
#else
	stRgnChn.enModId = EI_ID_IPPU;
	stRgnChn.s32DevId = 0;
	stRgnChn.s32ChnId = 1;
	MediaOsd_DeInitChn(&stRgnChn, osdChn);
	osdChn ++;
#endif
#endif

	MediaSnap_Stop();
	Algo_Stop();

	sleep(1);

    for (int ch=0; ch<MAX_VIDEO_NUM; ch++)
    {
    	if (RET_SUCCESS == checkTFCard(NULL) && NULL != gRecordMuxerHandle[ch].muxer)
    	{
    		muxer_context_destroy(gRecordMuxerHandle[ch].muxer);
    	}
    }

    MediaVideo_VissConfigInit(&viss_config);
	
#ifdef DISABLECH1
#else
    MediaVideo_GC2053EncodeDeInit(&viss_config, &vbuf_config);
#endif

    MediaVideo_TP9963EncodeDeInit(&viss_config, &vbuf_config);
	
	SAMPLE_COMM_VISS_StopViss(&viss_config);
	
#ifdef DISABLECH1
#else
	SAMPLE_COMM_ISP_Stop(viss_config.astVissInfo[0].stIspInfo.IspDev);
#endif

//	MediaVideo_BuffpollDeinit(&vbuf_config);

	sleep(2);

	return 0;
}

int get_video_stream(int chn, FrameInfo_t* frame)
{
	int ret = -1;

	if (chn >= MAX_VIDEO_NUM*2)
	{
		return -1;
	}
	
	MediaVideoStream_t* handle = &gMediaStream[chn];

	pthread_mutex_lock(&handle->frameH264.m_mutex);

	queue_t* head = handle->frameH264.m_head;
	while (head)
	{
		FrameInfo_t* node = (FrameInfo_t*)head->m_content;
		if (NULL != node)
		{
			if (node->timeStamp > frame->timeStamp)
			{
				frame->frameSize = node->frameSize;
				frame->timeStamp = node->timeStamp;
				frame->frameType = node->frameType;
				if (frame->frameBuffer == NULL)
				{
					if (handle->param.width == 1920)
					{
						frame->frameBuffer = (char*)malloc(800*1024);
					}
					else
					{
						frame->frameBuffer = (char*)malloc(400*1024);
					}
				}
				memcpy(frame->frameBuffer, node->frameBuffer, node->frameSize);
				ret = 0;
				break;
			}
		}
		head = head->next;
	}

	pthread_mutex_unlock(&handle->frameH264.m_mutex);

	return ret;
}

int get_audio_stream(int chn, FrameInfo_t* frame)
{
	int ret = -1;

	if (chn >= MAX_VIDEO_NUM*2)
	{
		return ret;
	}
	
	MediaVideoStream_t* handle = &gMediaStream[chn];

	pthread_mutex_lock(&handle->frameAudio.m_mutex);

	queue_t* head = handle->frameAudio.m_head;
	while (head)
	{
		FrameInfo_t* node = (FrameInfo_t*)head->m_content;
		if (NULL != node)
		{
			if (node->timeStamp > frame->timeStamp)
			{
				frame->frameSize = node->frameSize;
				frame->timeStamp = node->timeStamp;
				frame->frameType = node->frameType;
				if (frame->frameBuffer == NULL)
				{
					frame->frameBuffer = (char*)malloc(2*1024);
				}
				memcpy(frame->frameBuffer, node->frameBuffer, (node->frameSize>2*1024)?(2*1024):node->frameSize);
				ret = 0;
				break;
			}
		}
		head = head->next;
	}

	pthread_mutex_unlock(&handle->frameAudio.m_mutex);

	return ret;
}

uint64_t get_curr_framepts(int chn)
{
	if (chn >= MAX_VIDEO_NUM*2)
	{
		return -1;
	}
	
	return gMediaStream[chn].curFramePts;
}

