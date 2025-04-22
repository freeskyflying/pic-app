#include <sys/prctl.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "sample_comm.h"
#include "libuvcg.h"
#include "uvc_stream.h"

extern uvc_config_t g_cfg;
extern uvc_context_t g_ctx;
static EI_VOID *_video_thread(EI_VOID *arg)
{
	EI_S32 ret = -1;
	char name[32] = {0};
    stream_param_t *param = (stream_param_t *)arg;
    VENC_STREAM_S stStream = {0};
    VIDEO_FRAME_INFO_S stVFrameInfo = {0};
    video_cb_param_t cb_param = {0};
    stream_config_t *stream_cfg = &g_cfg.uvc_dev[param->dev].stream[param->stream];
    char *combin = NULL;
    char *tmpdata = NULL;
    VENC_STREAM_S stCombinStream = {0};
    VENC_STREAM_S *pstStream = NULL;
    EI_S32 combinSize = 0, packSize = 0;
    EI_S32 rebuild = 0;
    


	snprintf(name, sizeof(name), "video[%u][%u]", param->dev, param->stream);
	prctl(PR_SET_NAME, name);
    _DBG("\n");

	while (param->running) {
	
        if ((param->format == COMP_VIDEO_FORMAT_H264) ||
                (param->format == COMP_VIDEO_FORMAT_H265)) {
            memset(&stStream, 0, sizeof(VENC_STREAM_S));
            ret = EI_MI_VENC_GetStream(stream_cfg->venc_chn, &stStream, 300);
            if (ret == EI_ERR_VENC_NOBUF) {
                usleep(5 * 1000);
                continue;
            } else if (ret != EI_SUCCESS) {
                _DBG("get chn-%d error : %d\n", stream_cfg->venc_chn, ret);
                _DBG("[EI_MI_VENC_GetStream] get chn-%d error : %d \n", stream_cfg->venc_chn, ret);
                break;
            }
            if (stStream.pstPack.u32Len[0] == 0) {
                _DBG("ch %d pstPack.u32Len:%d-%d, ret:%x\n",
                    stream_cfg->venc_chn, stStream.pstPack.u32Len[0],
                    stStream.pstPack.u32Len[1], ret);
                _DBG("ch %d pstPack.u32Len:%d-%d, ret:%x\n",
                     stream_cfg->venc_chn, stStream.pstPack.u32Len[0],
                    stStream.pstPack.u32Len[1], ret);
            }
            pstStream = &stStream;
            if (!stStream.pstPack.bFrameEnd || rebuild) {
                _DBG("Need combin video frame.\n");
                packSize = stStream.pstPack.u32Len[0] + stStream.pstPack.u32Len[1];
                tmpdata = calloc(packSize + combinSize, 1);
                if (!tmpdata) {
                    _DBG("Request %d memory fail.\n", combinSize);
                    EI_MI_IPPU_ReleaseChnFrame(stream_cfg->ippu_dev, stream_cfg->ippu_chn, &stVFrameInfo);
                    continue;
                }
                if (combin && combinSize) {
                    memcpy(tmpdata, combin, combinSize);
                    memcpy(tmpdata + combinSize, stStream.pstPack.pu8Addr[0], stStream.pstPack.u32Len[0]);
                    if (stStream.pstPack.u32Len[1]) {
                        memcpy(tmpdata + combinSize + stStream.pstPack.u32Len[0], stStream.pstPack.pu8Addr[1], stStream.pstPack.u32Len[1]);
                    }
                    combinSize += packSize;
                    free(combin);
                } else {
                    memcpy(tmpdata, stStream.pstPack.pu8Addr[0], stStream.pstPack.u32Len[0]);
                    if (stStream.pstPack.u32Len[1]) {
                        memcpy(tmpdata + stStream.pstPack.u32Len[0], stStream.pstPack.pu8Addr[1], stStream.pstPack.u32Len[1]);
                    }
                    if (combin) {
                        free(combin);
                        combin = NULL;
                    }
                    combinSize = packSize;
                }
                combin = tmpdata;
                if (stStream.pstPack.bFrameEnd) {
                    memset(&stCombinStream, 0, sizeof(stCombinStream));
                    stCombinStream.pstPack.u32Len[0] = combinSize;
                    stCombinStream.pstPack.pu8Addr[0] = combin;
                    pstStream = &stCombinStream;
                } else {
                    EI_MI_IPPU_ReleaseChnFrame(stream_cfg->ippu_dev, stream_cfg->ippu_chn, &stVFrameInfo);
                    continue;
                }
            } 
            memset(&cb_param, 0, sizeof(cb_param));
        	cb_param.ippu_dev = stream_cfg->ippu_dev;
        	cb_param.ippu_chn = stream_cfg->ippu_chn;
        	cb_param.vc_chn = stream_cfg->venc_chn;
        	cb_param.format = param->format;
            cb_param.enc_stream[0] = pstStream;
            if (g_ctx.video_cb) {
                g_ctx.video_cb(&cb_param);
            }else {
                _DBG("Please config video callback.\n");
            }
            if (pstStream == &stStream) {
                ret = EI_MI_VENC_ReleaseStream(stream_cfg->venc_chn, pstStream);
                if (ret != EI_SUCCESS) {
                    _DBG("release stream chn-%d error : %d\n", stream_cfg->venc_chn, ret);
                    break;
                }
            } else {
                if (combin) {
                    free(combin);
                    combin = NULL;
                }
                combinSize = 0;
                rebuild = 0;
            }
        } else {
            ret = EI_MI_IPPU_GetChnFrame(stream_cfg->ippu_dev, stream_cfg->ippu_chn, &stVFrameInfo, 2000);
            if (ret == EI_ERR_VENC_NOBUF) {
                usleep(5 * 1000);
                continue;
            } else if (ret != EI_SUCCESS) {
                _DBG("[EI_MI_IPPU_GetChnFrame] get dev-%d chn-%d error : %d \n", stream_cfg->ippu_dev, stream_cfg->ippu_chn, ret);
                break;
            }
            EI_MI_VBUF_FrameMmap(&stVFrameInfo, VBUF_REMAP_MODE_CACHED);
        	cb_param.ippu_dev = stream_cfg->ippu_dev;
        	cb_param.ippu_chn = stream_cfg->ippu_chn;
        	cb_param.vc_chn = stream_cfg->venc_chn;
        	cb_param.format = param->format;
            cb_param.raw_frame = &stVFrameInfo;
            if (g_ctx.video_cb) {
                g_ctx.video_cb(&cb_param);
            } else {
                _DBG("Please config video callback.\n");
            }
            EI_MI_VBUF_FrameMunmap(&stVFrameInfo, VBUF_REMAP_MODE_CACHED);
            EI_MI_IPPU_ReleaseChnFrame(stream_cfg->ippu_dev, stream_cfg->ippu_chn, &stVFrameInfo);
        }
	}

	return NULL;
}


static EI_S32 _bufpool_init(uvc_context_t *ctx)
{
    EI_S32 i = 0;
    VBUF_CONFIG_S *vbuf_config = &ctx->vbufConfig;

    memset(vbuf_config, 0, sizeof(VBUF_CONFIG_S));
    vbuf_config->enFrmInfoType = VBUF_FRAME_INFO_TYPE_ARRAY;
  
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][0].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][0].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][0].stCommFrameInfo.u32Width = 1920;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][0].stCommFrameInfo.u32Height = 1080;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][0].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][0].stCommFrameInfo.enCompressMode = COMPRESS_MODE_NONE;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][1].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][1].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][1].stCommFrameInfo.u32Width = 1280;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][1].stCommFrameInfo.u32Height = 720;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][1].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][1].stCommFrameInfo.enCompressMode = COMPRESS_MODE_NONE;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][2].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][2].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][2].stCommFrameInfo.u32Width = 1024;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][2].stCommFrameInfo.u32Height = 576;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][2].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][2].stCommFrameInfo.enCompressMode = COMPRESS_MODE_NONE;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][3].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][3].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][3].stCommFrameInfo.u32Width = 960;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][3].stCommFrameInfo.u32Height = 540;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][3].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][3].stCommFrameInfo.enCompressMode = COMPRESS_MODE_NONE;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][4].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][4].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][4].stCommFrameInfo.u32Width = 640;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][4].stCommFrameInfo.u32Height = 480;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][4].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][4].stCommFrameInfo.enCompressMode = COMPRESS_MODE_NONE;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][5].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][5].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][5].stCommFrameInfo.u32Width = 640;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][5].stCommFrameInfo.u32Height = 360;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][5].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astFrameInfoArray[vbuf_config->u32PoolCnt][5].stCommFrameInfo.enCompressMode = COMPRESS_MODE_NONE;
    vbuf_config->au32FrmInfoCnt[vbuf_config->u32PoolCnt] = 6;
    vbuf_config->astCommPool[vbuf_config->u32PoolCnt].enRemapMode = VBUF_REMAP_MODE_CACHED;
    vbuf_config->astCommPool[vbuf_config->u32PoolCnt].u32BufCnt = 10;
    vbuf_config->u32PoolCnt++;

    return SAMPLE_COMM_SYS_Init(vbuf_config);
}

static EI_S32 _bufpool_deinit(uvc_context_t *ctx)
{
    _DBG("vbuf_config->u32PoolCnt = %d", ctx->vbufConfig.u32PoolCnt);
    return SAMPLE_COMM_SYS_Exit(&ctx->vbufConfig);
}

EI_S32 start_video_source(void)
{
    SAMPLE_VISS_CONFIG_S *pstVissConfig = &g_ctx.vissConfig;
    EI_S32 ispDev = 0, s32Ret = 0;
    IPPU_DEV_ATTR_S stIppuDevAttr = {0};
    VISS_PIC_TYPE_E stPicType;

    _bufpool_init(&g_ctx);
    pstVissConfig->astVissInfo[0].stDevInfo.VissDev = g_cfg.viss_dev; /*0: DVP, 1: MIPI*/
    pstVissConfig->astVissInfo[0].stDevInfo.aBindPhyChn[0] = 0;
    pstVissConfig->astVissInfo[0].stDevInfo.enOutPath = VISS_OUT_PATH_PIXEL;
    pstVissConfig->astVissInfo[0].stChnInfo.aVissChn[0] = 0;
    pstVissConfig->astVissInfo[0].stChnInfo.enWorkMode = VISS_WORK_MODE_1Chn;
    pstVissConfig->astVissInfo[0].stIspInfo.IspDev = ispDev;
    pstVissConfig->astVissInfo[0].stIspInfo.enRunningMode = ISP_MODE_RUNNING_ONLINE;
    pstVissConfig->astVissInfo[0].stSnsInfo.enSnsType = g_cfg.sesnor;
    pstVissConfig->s32WorkingVissNum = 1;

    g_ctx.camerNum = 1;

    s32Ret = SAMPLE_COMM_ISP_Start(&pstVissConfig->astVissInfo[0]);
    if (s32Ret != EI_SUCCESS) {
        goto EXIT;
    }

    s32Ret = SAMPLE_COMM_VISS_StartViss(pstVissConfig);
    if (s32Ret != EI_SUCCESS) {
        goto EXIT;
    }
    SAMPLE_COMM_VISS_GetPicTypeBySensor(pstVissConfig->astVissInfo[0].stSnsInfo.enSnsType, &stPicType);

    stIppuDevAttr.s32IppuDev        = pstVissConfig->astVissInfo[0].stIspInfo.IspDev;
    stIppuDevAttr.u32InputWidth     = stPicType.stSize.u32Width;
    stIppuDevAttr.u32InputHeight    = stPicType.stSize.u32Height;
    stIppuDevAttr.u32DataSrc        = pstVissConfig->astVissInfo[0].stDevInfo.VissDev;
    stIppuDevAttr.enRunningMode     = IPPU_MODE_RUNNING_ONLINE;

    s32Ret = EI_MI_IPPU_Create(ispDev, &stIppuDevAttr);
    if (s32Ret != EI_SUCCESS) {
        _DBG("EI_MI_IPPU_Create(dev:%d) failed with %#x!\n", ispDev, s32Ret);
        goto EXIT;
    }
    s32Ret = EI_MI_IPPU_Start(ispDev);
    _DBG("EI_MI_IPPU_Start s32Ret:%#x\n", s32Ret);

    return s32Ret;
EXIT:
    SAMPLE_COMM_VISS_StopViss(pstVissConfig);
    SAMPLE_COMM_ISP_Stop(ispDev);
    _bufpool_deinit(&g_ctx);
    return -1;
}

EI_S32 stop_video_source(void)
{
    SAMPLE_VISS_CONFIG_S *pstVissConfig = &g_ctx.vissConfig;
    EI_S32 ispDev = 0, s32Ret = 0;

    EI_MI_IPPU_Stop(ispDev);
    EI_MI_IPPU_Destroy(ispDev);
    SAMPLE_COMM_VISS_StopViss(pstVissConfig);
    SAMPLE_COMM_ISP_Stop(ispDev);
    _bufpool_deinit(&g_ctx);
    
    return 0;
}

EI_S32 start_uvc_stream(stream_param_t *param)
{
    EI_S32 i = 0, s32Ret = 0;
    IPPU_CHN PhyChn = 0;
    VISS_CHN_ATTR_S stChnAttr = {0};
    IPPU_CHN_ATTR_S astIppuChnAttr[IPPU_PHY_CHN_MAX_NUM] = {{0}};
    IPPU_ROTATION_ATTR_S astIppuChnRot[IPPU_PHY_CHN_MAX_NUM] = {{0}};
    stream_config_t *stream_cfg = &g_cfg.uvc_dev[param->dev].stream[param->stream];

    
    _DBG("\n");
    SAMPLE_COMM_VISS_GetChnAttrBySns(g_ctx.vissConfig.astVissInfo[0].stSnsInfo.enSnsType, &stChnAttr);

    if (stream_cfg->ippu_chn >= IPPU_PHY_CHN_MAX_NUM) {
        _DBG("Unsupport input ext channel.\n");
        return -1;
    } else if (LIBUVCG_VIDEO_FMT_MJPEG == param->info.format) {
        _DBG("Unsupport mjpeg encode.\n");
        return -1;
    }
    PhyChn = stream_cfg->ippu_chn;

    astIppuChnAttr[PhyChn].u32Width      = param->info.width;
    astIppuChnAttr[PhyChn].u32Height     = param->info.height;
    astIppuChnAttr[PhyChn].u32Align      = 32;
    astIppuChnAttr[PhyChn].enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    astIppuChnAttr[PhyChn].stFrameRate.s32SrcFrameRate = stChnAttr.stFrameRate.s32SrcFrameRate;
    astIppuChnAttr[PhyChn].stFrameRate.s32DstFrameRate = param->framerate;
    astIppuChnAttr[PhyChn].u32Depth      = 2;
    
    if ((stream_cfg->rotation == ROTATION_90) || (stream_cfg->rotation == ROTATION_180) || (stream_cfg->rotation == ROTATION_270)) {
        astIppuChnRot[PhyChn].bEnable = EI_TRUE;
        astIppuChnRot[PhyChn].eRotation = stream_cfg->rotation;
        EI_MI_IPPU_SetChnRotation(stream_cfg->ippu_dev, PhyChn, &astIppuChnRot[PhyChn]);
    }
    s32Ret = EI_MI_IPPU_SetChnAttr(stream_cfg->ippu_dev, stream_cfg->ippu_chn, &astIppuChnAttr[PhyChn]);
    if (s32Ret != EI_SUCCESS) {
        _DBG("EI_MI_IPPU_SetChnAttr failed with %#x\n", s32Ret);
        return -1;
    }
    
    s32Ret = EI_MI_IPPU_EnableChn(stream_cfg->ippu_dev, stream_cfg->ippu_chn);
    if (s32Ret != EI_SUCCESS) {
        _DBG("EI_MI_IPPU_EnableChn failed with %#x\n", s32Ret);
        return -1;
    }
    _DBG("stream>>format:%d framerate:%d width:%d height:%d\n", param->info.format, param->framerate, param->info.width, param->info.height);

    if ((LIBUVCG_VIDEO_FMT_H264 == param->info.format) ||
        (LIBUVCG_VIDEO_FMT_H265 == param->info.format)) {
        SAMPLE_VENC_CONFIG_S venc_config = {0};
        PAYLOAD_TYPE_E pt_type;

        venc_config.enInputFormat   = astIppuChnAttr[PhyChn].enPixelFormat;
        venc_config.u32width        = astIppuChnAttr[PhyChn].u32Width;
        venc_config.u32height       = astIppuChnAttr[PhyChn].u32Height;
        venc_config.u32bitrate      = stream_cfg->ippu_chn;
        venc_config.u32srcframerate = param->framerate;
        venc_config.u32dstframerate = param->framerate;
        if (LIBUVCG_VIDEO_FMT_H264 == param->info.format) {
            if ((1920 == venc_config.u32width) && (1080 == venc_config.u32height)) {
        		venc_config.u32bitrate = 12000000;
        	} else if ((1280 == venc_config.u32width) && (720 == venc_config.u32height)) {
        		venc_config.u32bitrate = 8000000;
        	} else {
        		venc_config.u32bitrate = 6000000;
        	}
        	pt_type = PT_H264;
            param->format = COMP_VIDEO_FORMAT_H264;
    	} else if (LIBUVCG_VIDEO_FMT_H265 == param->info.format) {
            if ((1920 == venc_config.u32width) && (1080 == venc_config.u32height)) {
                venc_config.u32bitrate = 8000000;
            } else if ((1280 == venc_config.u32width) && (720 == venc_config.u32height)) {
                venc_config.u32bitrate = 6000000;
            } else {
                venc_config.u32bitrate = 4000000;
            }
            pt_type = PT_H265;
            param->format = COMP_VIDEO_FORMAT_H265;
    	}
        s32Ret = SAMPLE_COMM_VENC_Start(stream_cfg->venc_chn, pt_type,
                SAMPLE_RC_CBR, &venc_config,
                COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
        if (EI_SUCCESS != s32Ret) {
            _DBG("venc:%d ret:%#x\n", stream_cfg->venc_chn, s32Ret);
            return -1;
        }

        s32Ret = SAMPLE_COMM_IPPU_Link_VPU(stream_cfg->ippu_dev, stream_cfg->ippu_chn, stream_cfg->venc_chn);
        if (EI_SUCCESS != s32Ret) {
            _DBG("viss link vpu failed %d-%d-%d \n",
                i, 1, stream_cfg->venc_chn);
            SAMPLE_COMM_VENC_Stop(stream_cfg->venc_chn);
            return -1;
        }
    } else {
        param->format = COMP_VIDEO_FORMAT_NV12;
    }
    
    _DBG("\n");
    param->running = 1;
    pthread_create(&g_ctx.video_pid[param->dev][param->stream], NULL, _video_thread, param);

    return 0;
}

EI_S32 stop_uvc_stream(stream_param_t *param)
{
    stream_config_t *stream_cfg = &g_cfg.uvc_dev[param->dev].stream[param->stream];

    if (!param->running) {
        return -1;
    }
    param->running = 0;
    EI_MI_IPPU_DisableChn(stream_cfg->ippu_dev, stream_cfg->ippu_chn);
    pthread_join(g_ctx.video_pid[param->dev][param->stream], NULL);
    if ((LIBUVCG_VIDEO_FMT_H264 == param->info.format) ||
        (LIBUVCG_VIDEO_FMT_H265 == param->info.format)) {
        SAMPLE_COMM_VENC_Stop(stream_cfg->venc_chn);
        SAMPLE_COMM_IPPU_UnLink_VPU(stream_cfg->ippu_dev, stream_cfg->ippu_chn, stream_cfg->venc_chn);
    }

    return 0;
}

