#include "sample_comm.h"
#include "osal.h"
#include "common_test.h"
#include "media_snap.h"
#include "media_osd.h"

#undef MAX_VIDEO_NUM
#define MAX_VIDEO_NUM   3

int gCameraInit = 0;
MediaVideoStream_t gMediaStreamTest[MAX_VIDEO_NUM*2] = { 0 };
unsigned int gCurrentSpeed = 30;

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
    vbuf_config->astCommPool[count].u32BufCnt = 16;
    count ++;

    vbuf_config->astFrameInfo[count].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Width = 1280;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Height = 720;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[count].u32BufCnt = 12;
    vbuf_config->astCommPool[count].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    count ++;

    vbuf_config->astFrameInfo[count].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Width = 640;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.u32Height = 360;
    vbuf_config->astFrameInfo[count].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[count].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    vbuf_config->astCommPool[count].u32BufCnt = 6;
    count ++;

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

    int s32Ret = SAMPLE_COMM_SYS_Init(vbuf_config);

    return s32Ret;
}

static int MediaVideo_VissConfigInit(SAMPLE_VISS_CONFIG_S *viss_config)
{
    SAMPLE_VISS_INFO_S *viss_info;
    memset(viss_config, 0, sizeof(SAMPLE_VISS_CONFIG_S));

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
    viss_info->stIspInfo.stFrc.s32SrcFrameRate = 25;
    viss_info->stIspInfo.stFrc.s32DstFrameRate = 15;
	if (0 == access("/data/gc2083", F_OK))
	{
    	viss_info->stSnsInfo.enSnsType = GC2083_MIPI_1920_1080_30FPS_RAW10;
	}
	else// if (0 == access("/data/gc2053", F_OK))
	{
		system("echo 1 > /data/gc2053");
    	viss_info->stSnsInfo.enSnsType = GC2053_MIPI_1920_1080_30FPS_RAW10;
	}

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
    
    ippu_chn_attr[1].u32Width	= stPicType.stSize.u32Width;
    ippu_chn_attr[1].u32Height	= stPicType.stSize.u32Height;
    ippu_chn_attr[1].u32Align	= 32;
    ippu_chn_attr[1].enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    ippu_chn_attr[1].stFrameRate.s32SrcFrameRate = stChnAttr.stFrameRate.s32SrcFrameRate;
    ippu_chn_attr[1].stFrameRate.s32DstFrameRate = 15;
    ippu_chn_attr[1].u32Depth	= 2;
    chn_enabled[1] = EI_TRUE;

    ippu_chn_attr[2].u32Width	= stPicType.stSize.u32Width;
    ippu_chn_attr[2].u32Height	= stPicType.stSize.u32Height;
    ippu_chn_attr[2].u32Align	= 32;
    ippu_chn_attr[2].enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    ippu_chn_attr[2].stFrameRate.s32SrcFrameRate = stChnAttr.stFrameRate.s32SrcFrameRate;
    ippu_chn_attr[2].stFrameRate.s32DstFrameRate = SDR_VENC_FRAMERATE_SUB;
    ippu_chn_attr[2].u32Depth	= 2;
    chn_enabled[2] = EI_TRUE;

    ret = SAMPLE_COMM_IPPU_Start(devIppu, chn_enabled, &ippu_dev_attr, ippu_chn_attr);
    if (ret)
    {
        PNT_LOGE("SAMPLE_COMM_IPPU_Start:%d err\n", devIppu);
        SAMPLE_COMM_IPPU_Exit(ippu_dev_attr.s32IppuDev);
        return RET_FAILED;
    }
	
	MediaVideoStream_t* handle = &gMediaStreamTest[vencChn];

	SAMPLE_VENC_CONFIG_S venc_config = {0};
    venc_config.enInputFormat   = PIX_FMT_YUV_SEMIPLANAR_420;
    venc_config.u32width	    = handle->param.width;
    venc_config.u32height	    = handle->param.height;
    venc_config.u32bitrate      = handle->param.height==1080?SDR_VENC_BITRATE_1080P:SDR_VENC_BITRATE_720P;
    venc_config.u32srcframerate = handle->param.framerate;
    venc_config.u32dstframerate = handle->param.framerate;
    venc_config.u32IdrPeriod    = venc_config.u32dstframerate;

    ret = SAMPLE_COMM_VENC_Start(vencChn, PT_H264, SAMPLE_RC_CBR, &venc_config, COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
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

    vencChn = vencChn + MAX_VIDEO_NUM;
	handle = &gMediaStreamTest[vencChn];
	SAMPLE_VENC_CONFIG_S venc_config_sub = {0};
    venc_config_sub.enInputFormat   = PIX_FMT_YUV_SEMIPLANAR_420;
    venc_config_sub.u32width	    = handle->param.width;
    venc_config_sub.u32height	    = handle->param.height;
    venc_config_sub.u32bitrate      = SDR_VENC_BITRATE_SUB;
    venc_config_sub.u32srcframerate = handle->param.framerate;
    venc_config_sub.u32dstframerate = handle->param.framerate;
    venc_config_sub.u32IdrPeriod    = venc_config_sub.u32dstframerate;

    ret = SAMPLE_COMM_VENC_Start(vencChn, PT_H264, SAMPLE_RC_CBR, &venc_config_sub, COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
    if (EI_SUCCESS != ret)
    {
        PNT_LOGE("%d ret:%d \n", vencChn, ret);
    }
    
    MDP_CHN_S sub_src_viss = { 0 }, sub_sink_isp = { 0 };

    sub_src_viss.enModId = EI_ID_IPPU;
    sub_src_viss.s32DevId = devIppu;
    sub_src_viss.s32ChnId = 2;

    sub_sink_isp.enModId = EI_ID_VPU;
    sub_sink_isp.s32DevId = 0;
    sub_sink_isp.s32ChnId = vencChn;
    ret = EI_MI_MLINK_Link(&sub_src_viss, &sub_sink_isp);
    if (ret)
    {
        PNT_LOGE("EI_MI_MLINK_Link:%d err\n", devIppu);
    }

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
    SAMPLE_COMM_VENC_Stop(vencChn);

    vencChn = vencChn + MAX_VIDEO_NUM;
    MDP_CHN_S sub_src_viss = { 0 }, sub_sink_isp = { 0 };
    sub_src_viss.enModId = EI_ID_IPPU;
    sub_src_viss.s32DevId = devIppu;
    sub_src_viss.s32ChnId = 2;
    sub_sink_isp.enModId = EI_ID_VPU;
    sub_sink_isp.s32DevId = 0;
    sub_sink_isp.s32ChnId = vencChn;
    EI_MI_MLINK_UnLink(&sub_src_viss, &sub_sink_isp);
    SAMPLE_COMM_VENC_Stop(vencChn);

    chn_enabled[0] = EI_TRUE;
    chn_enabled[1] = EI_TRUE;
    chn_enabled[2] = EI_TRUE;

    SAMPLE_COMM_IPPU_Stop(devIppu, chn_enabled);
    SAMPLE_COMM_IPPU_Exit(devIppu);

    return RET_SUCCESS;
}

extern EI_S32  SAMPLE_COMM_VENC_Creat(VC_CHN VencChn, PAYLOAD_TYPE_E enType,
    SAMPLE_RC_E RcMode, SAMPLE_VENC_CONFIG_S *pstVencConfig,
    COMPRESS_MODE_E enCompressMode, VENC_GOP_MODE_E GopMode);
int MediaVideo_TP9963EncodeInit(SAMPLE_VISS_CONFIG_S* viss_config, VBUF_CONFIG_S* vbuf_config)
{
    SAMPLE_VISS_INFO_S *viss_info = &viss_config->astVissInfo[1];

	SAMPLE_VENC_CONFIG_S venc_config = {0};

	for (int i=0; i<4; i++)
	{
	    int devChn = viss_info->stDevInfo.VissDev;
	    int vissChn = viss_info->stChnInfo.aVissChn[i];
	    int vencChn = 1+i;
		if (-1 == vissChn)
		{
			break;
		}
		MediaVideoStream_t* handle = &gMediaStreamTest[vencChn];

	    venc_config.enInputFormat   = PIX_FMT_YUV_SEMIPLANAR_420;
	    venc_config.u32width	    = handle->param.width;
	    venc_config.u32height	    = handle->param.height;
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

		SAMPLE_VENC_CONFIG_S sub_venc_config = {0};

	    vencChn = vencChn + MAX_VIDEO_NUM;
		handle = &gMediaStreamTest[vencChn];

	    sub_venc_config.enInputFormat   = PIX_FMT_YUV_SEMIPLANAR_420;
	    sub_venc_config.u32width	    = handle->param.width;
	    sub_venc_config.u32height	    = handle->param.height;
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
	}

    return RET_SUCCESS;
}

int MediaVideo_TP9963EncodeDeInit(SAMPLE_VISS_CONFIG_S* viss_config, VBUF_CONFIG_S* vbuf_config)
{
    SAMPLE_VISS_INFO_S *viss_info = &viss_config->astVissInfo[1];

	SAMPLE_VENC_CONFIG_S venc_config = {0};

	for (int i=0; i<4; i++)
	{
	    int devChn = viss_info->stDevInfo.VissDev;
	    int vissChn = viss_info->stChnInfo.aVissChn[i];
	    int vencChn = 1+i;
		if (-1 == vissChn)
		{
			break;
		}
		
	    SAMPLE_COMM_VISS_Link_VPU(devChn, vissChn, vencChn);
	    SAMPLE_COMM_VENC_Stop(vencChn);

	    vencChn = vencChn + MAX_VIDEO_NUM;
	    SAMPLE_COMM_VISS_Link_VPU(devChn, vissChn, vencChn);
	    SAMPLE_COMM_VENC_Stop(vencChn);
	}

    return RET_SUCCESS;
}

extern void RtspFramePushQueue(int channel, char* data, int len, char* data2, int len2);
extern void RtspAudioPushQueue(int channel, char* data, int len);

#define user_RB16(x) ((((const uint8_t *)(x))[0] << 8) | ((const uint8_t *)(x))[1])

void aenc_frame_callback(int bindVenc, uint8_t* data, int len)
{
	AUDIO_STREAM_S *pstStream = (AUDIO_STREAM_S *)data;

    MediaVideoStream_t *handle = (MediaVideoStream_t*)&gMediaStreamTest[bindVenc];

	RtspAudioPushQueue(bindVenc, (char*)pstStream->pStream, pstStream->u32Len);
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


int CameraTest_Snap(int chn, char* info)
{
	if (0 == gCameraInit)
	{
		sprintf(info, "抓拍失败，摄像头启动错误");
		return 0;
	}
	else
	{
		char filename[128] = { 0 };
		sprintf(filename, "/tmp/ch%d.jpg", chn+1);

		char cmd[128] = { 0 };
		sprintf(cmd, "rm -rf /tmp/ch%d*", chn+1);
		system(cmd);

		MediaSnap_CreateSnap(chn, 1, 640, 480, 0, filename);
		
		sprintf(info, "抓拍成功，请检查抓拍画面");

		usleep(100*1000);
	}
	
	return 1;
}

void MediaVideo_StartGetFrame(MediaVideoStream_t* handle, int type)
{
    handle->bThreadStart = EI_TRUE;
    handle->type = type;

    int ret = 0;

    pthread_mutex_init(&handle->mutex, NULL);
    
    if (CHANNEL_MASTER == type)
    {
        ret = pthread_create(&handle->vencPid, 0, _get_venc_stream_proc, (EI_VOID *)handle);
    }
    else
    {
        ret = pthread_create(&handle->vencPid, 0, _get_venc_substream_proc, (EI_VOID *)handle);
    }
    if (ret)
    {
        PNT_LOGE("errno=%d, reason(%s)\n", errno, strerror(errno));
    }
}

static void MediaVideo_ParamInit(void)
{
	MediaVideoStream_t* handle = &gMediaStreamTest[0];
	memset(handle, 0, sizeof(MediaVideoStream_t));
	handle->param.framerate = SDR_VENC_FRAMERATE_MAJOR;
	handle->param.width = CH1_VIDEO_WIDTH;
	handle->param.height = CH1_VIDEO_HEIGHT;
	handle->param.srcwidth = 1920;
	handle->param.srcheight = 1080;
	handle->param.needCache = 1;
	handle->param.isValid = 1;
	handle->param.needRec = 1;
	handle->param.videoSize = CH1_VIDEO_STREAM_SIZE;
	
	handle = &gMediaStreamTest[0+MAX_VIDEO_NUM];
	memset(handle, 0, sizeof(MediaVideoStream_t));
	handle->param.framerate = SDR_VENC_FRAMERATE_SUB;
	handle->param.width = CH1SUB_VIDEO_WIDTH;
	handle->param.height = CH1SUB_VIDEO_HEIGHT;
	handle->param.srcwidth = 1920;
	handle->param.srcheight = 1080;
	handle->param.needCache = 0;
	
#if (MAX_VIDEO_NUM > 1)
	handle = &gMediaStreamTest[1];
	memset(handle, 0, sizeof(MediaVideoStream_t));
	handle->param.framerate = SDR_VENC_FRAMERATE_MAJOR;
	handle->param.width = CH2_VIDEO_WIDTH;
	handle->param.height = CH2_VIDEO_HEIGHT;
	handle->param.srcwidth = 1280;
	handle->param.srcheight = 720;
	handle->param.needCache = 1;
	handle->param.isValid = 1;
	handle->param.needRec = 1;
	handle->param.videoSize = CH2_VIDEO_STREAM_SIZE;
	
	handle = &gMediaStreamTest[1+MAX_VIDEO_NUM];
	memset(handle, 0, sizeof(MediaVideoStream_t));
	handle->param.framerate = SDR_VENC_FRAMERATE_SUB;
	handle->param.width = CH2SUB_VIDEO_WIDTH;
	handle->param.height = CH2SUB_VIDEO_HEIGHT;
	handle->param.srcwidth = 1280;
	handle->param.srcheight = 720;
	handle->param.needCache = 0;
#endif
	
#if (MAX_VIDEO_NUM > 2)
	handle = &gMediaStreamTest[2];
	memset(handle, 0, sizeof(MediaVideoStream_t));
	handle->param.framerate = SDR_VENC_FRAMERATE_MAJOR;
	handle->param.width = CH3_VIDEO_WIDTH;
	handle->param.height = CH3_VIDEO_HEIGHT;
	handle->param.srcwidth = 1280;
	handle->param.srcheight = 720;
	handle->param.needCache = 1;
	handle->param.isValid = 1;
	handle->param.needRec = 1;
	handle->param.videoSize = CH3_VIDEO_STREAM_SIZE;
	
	handle = &gMediaStreamTest[2+MAX_VIDEO_NUM];
	memset(handle, 0, sizeof(MediaVideoStream_t));
	handle->param.framerate = SDR_VENC_FRAMERATE_SUB;
	handle->param.width = CH3SUB_VIDEO_WIDTH;
	handle->param.height = CH3SUB_VIDEO_HEIGHT;
	handle->param.srcwidth = 1280;
	handle->param.srcheight = 720;
	handle->param.needCache = 0;
#endif
}

void CameraTest_Start(void)
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
		return;
	}

	ret = SAMPLE_COMM_ISP_Start(&viss_config.astVissInfo[0]);
	if (ret) 
	{
		PNT_LOGE("SAMPLE_COMM_ISP_Start ret failed.");
		goto exit_fail_isp;
	}

	ret = SAMPLE_COMM_VISS_StartViss(&viss_config);
	if (RET_SUCCESS != ret)
	{
		PNT_LOGE("SAMPLE_COMM_VISS_StartViss ret failed.");
		goto exit_fail_viss;
	}

	ret = MediaVideo_GC2053EncodeInit(&viss_config, &vbuf_config);
	if (RET_SUCCESS != ret)
	{
		PNT_LOGE("MediaVideo_GC2053EncodeInit ret failed.");
		goto exit_fail_2053;
	}
	
	ret = MediaVideo_TP9963EncodeInit(&viss_config, &vbuf_config);
	if (RET_SUCCESS != ret)
	{
		PNT_LOGE("MediaVideo_TP9963EncodeInit ret failed.");
		goto exit_fail_9963;
	}

	MDP_CHN_S stRgnChn = { 0 };
	stRgnChn.enModId = EI_ID_IPPU;
	stRgnChn.s32DevId = 0;
	stRgnChn.s32ChnId = 0;
	MediaOsd_InitChn(&stRgnChn, 1920, OSD_AREA_HEIGHT, 0);
	stRgnChn.s32ChnId = 2;
	MediaOsd_InitChn(&stRgnChn, 1920, OSD_AREA_HEIGHT, MAX_VIDEO_NUM);
	
#if (MAX_VIDEO_NUM > 1)
	stRgnChn.enModId = EI_ID_VISS;
	stRgnChn.s32DevId = 2;
	stRgnChn.s32ChnId = 1;
	MediaOsd_InitChn(&stRgnChn, 1280, OSD_AREA_HEIGHT, 1);
#endif
	
#if (MAX_VIDEO_NUM > 2)
	stRgnChn.enModId = EI_ID_VISS;
	stRgnChn.s32DevId = 2;
	stRgnChn.s32ChnId = 2;
	MediaOsd_InitChn(&stRgnChn, 1280, OSD_AREA_HEIGHT, 2);
#endif

	MediaAudio_Start(AUDIO_BIND_CHN, aenc_frame_callback);

    MediaOsd_Start();

	MediaSnap_Init();

	for (int i=0; i<MAX_VIDEO_NUM; i++)
	{
		MediaVideoStream_t* handle = &gMediaStreamTest[i];
		
		handle->venChn = i;
	    MediaVideo_StartGetFrame(handle, CHANNEL_MASTER);
	}

	for (int i=MAX_VIDEO_NUM; i<2*MAX_VIDEO_NUM; i++)
	{
		MediaVideoStream_t* handle = &gMediaStreamTest[i];
		
		handle->venChn = i;
	    MediaVideo_StartGetFrame(handle, CHANNEL_CHILD);
	}

	gCameraInit = 1;

    return;

exit_fail_9963:
	MediaVideo_GC2053EncodeDeInit(&viss_config, &vbuf_config);

exit_fail_2053:
	SAMPLE_COMM_VISS_StopViss(&viss_config);

exit_fail_viss:
	SAMPLE_COMM_ISP_Stop(viss_config.astVissInfo[0].stIspInfo.IspDev);

exit_fail_isp:
	MediaVideo_BuffpollDeinit(&vbuf_config);

	if (0 == access("/data/gc2083", F_OK))
	{
		remove("/data/gc2083");
		system("echo 1 > /data/gc2053");
	}
	else// if (0 == access("/data/gc2053", F_OK))
	{
		remove("/data/gc2053");
		system("echo 1 > /data/gc2083");
	}
	
    return;
}


