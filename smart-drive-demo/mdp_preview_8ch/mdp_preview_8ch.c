/*
 *------------------------------------------------------------------------------
 * @File      :    mdp_preview_8ch.c
 * @Date      :    2021-9-27
 * @Author    :    lomboswer <lomboswer@lombotech.com>
 * @Brief     :    Source file for MDP(Media Development Platform).
 *
 * Copyright (C) 2020-2021, LomboTech Co.Ltd. All rights reserved.
 *------------------------------------------------------------------------------
 */
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define _GNU_SOURCE
/* sample common venc test*/
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

#define EI_TRACE_LOG(level, fmt, args...)\
do{ \
    EI_TRACE(EI_ID_LOG, level, fmt, ##args);\
} while (0)

typedef struct preview_param {
    ISP_DEV dev;
    IPPU_CHN ch;
    ROTATION_E rot;
    EI_BOOL mirror;
    EI_BOOL flip;
    EI_U32 x;
    EI_U32 y;
    EI_U32 u32Width;
    EI_U32 u32Height;
} PREVIEW_PARAM_S;

#if 0
#define SDR_VISS_SNS_DVP TP9930_DVP_1920_1080_30FPS_4CH_YUV
#define SDR_VISS_SNS_MIPI TP2815_MIPI_1920_1080_30FPS_4CH_YUV
#define SDR_VISS_FRAMERATE 30
#else
#define SDR_VISS_SNS_DVP TP9930_DVP_1920_1080_25FPS_4CH_YUV
#define SDR_VISS_SNS_MIPI TP2815_MIPI_1920_1080_25FPS_4CH_YUV
#define SDR_VISS_FRAMERATE 25
#endif

#define VO_DEVICE  OTM1289A_MIPI_720_1280_60FPS	/* TP2803_BT_720_576_25FPS */

#define IPPU_FRAMERATE	15

static EI_U32 scn_w, scn_h, scn_rate;

EI_U32 SAMPLE_COMM_SYS_Init(VBUF_CONFIG_S *pstVbConfig)
{
    EI_S32 s32Ret = EI_SUCCESS;
    EI_S32 i = 0;

    EI_MI_MBASE_Init();
    EI_MI_VBUF_Init();
    EI_MI_MLINK_Init();

    s32Ret = EI_MI_VBUF_InitVbufConfig(pstVbConfig);
    if (s32Ret) {
        EI_TRACE_VBUF(EI_DBG_ERR, "create pool size %d failed!\n", pstVbConfig->astCommPool[i].u32BufSize);
        return s32Ret;
    }

    return s32Ret;
}

static int _get_screen_info(EI_U32 *scn_w, EI_U32 *scn_h, EI_U32 *scn_rate)
{
	return SAMPLE_COMM_DOSS_GetWH(VO_DEVICE, VO_OUTPUT_USER, scn_w, scn_h, scn_rate);
}

/* 8ch */
static int _link_4x2ch_display(VO_LAYER layer, EI_U32 width, EI_U32 hight, VO_CHN_ATTR_S *vo_chn_attr, IPPU_DEV *dev)
{
    int i;
    int ret = EI_FAILURE;
    MDP_CHN_S src_ippu[ISP_MAX_DEV_NUM], sink_doss[ISP_MAX_DEV_NUM];
    for (i = 0; i < 8; i++) {
        vo_chn_attr->stRect.s32X = i < 4 ? 0 : width/2;
        vo_chn_attr->stRect.s32Y = (i % 4) * hight/4;
        vo_chn_attr->stRect.u32Width = width/2;
        vo_chn_attr->stRect.u32Height = hight/4;

        EI_MI_VO_SetChnAttr(layer, i, vo_chn_attr);
        EI_MI_VO_EnableChn(layer, i);

        src_ippu[i].enModId = EI_ID_IPPU;
        src_ippu[i].s32ChnId = 1;
        src_ippu[i].s32DevId = dev[i];
        sink_doss[i].enModId = EI_ID_DOSS;
        sink_doss[i].s32ChnId = i;
        sink_doss[i].s32DevId = 0;
        ret = EI_MI_MLINK_Link(&src_ippu[i], &sink_doss[i]);
        if (ret != EI_SUCCESS) {
            EI_TRACE_LOG(EI_DBG_ERR, "\n");
            goto exit;
        }
    }
exit:
    return ret;
}

static int ippu_show_offline_8ch(void)
{
    EI_S32 s32Ret = EI_FAILURE;
    EI_S32 i;
    MDP_CHN_S stSrcViss[ISP_MAX_DEV_NUM], stSinkIsp[ISP_MAX_DEV_NUM];
    MDP_CHN_S stSrcIppu[ISP_MAX_DEV_NUM], stSinkDoss[ISP_MAX_DEV_NUM];
    EI_BOOL abChnEnable[IPPU_PHY_CHN_MAX_NUM] = {0};
    IPPU_DEV_ATTR_S stIppuDevAttr = {0};
    IPPU_CHN_ATTR_S astIppuChnAttr[IPPU_PHY_CHN_MAX_NUM] = {{0}};
    VO_VIDEO_LAYER_ATTR_S stVoLayerAttr = {0};
    VO_CHN_ATTR_S stVoChnAttr = {0};
    PANEL_TYPE_ATTR_S stPanelAttr = {0};
    SAMPLE_VISS_INFO_S *pstVissInfo;
    VBUF_CONFIG_S stVbConfig;
    SAMPLE_VISS_CONFIG_S stVissConfig;
	IPPU_DEV ippu_dev_4x2[8] = {0, 1, 2, 3, 4, 5, 6, 7};

	scn_w = scn_h = scn_rate = 0;
    _get_screen_info(&scn_w, &scn_h, &scn_rate);
	if ((scn_w == 0) || (scn_h == 0) || (scn_rate == 0)) {
	    EI_PRINT("%s %d get screen info fail. ", __func__, __LINE__);
	    goto exit0;
    }

    stVbConfig.astFrameInfo[0].enFrameType = MDP_FRAME_TYPE_COMMON;
    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Align = 32;
    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Width = 1920;
    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Height = 1080;
    stVbConfig.astFrameInfo[0].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stVbConfig.astCommPool[0].u32BufCnt = 40;
    stVbConfig.astCommPool[0].enRemapMode = VBUF_REMAP_MODE_NOCACHE;

    stVbConfig.astFrameInfo[1].enFrameType = MDP_FRAME_TYPE_COMMON;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Align = 32;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width = 640;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height = 360;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stVbConfig.astCommPool[1].u32BufCnt = 50;
    stVbConfig.astCommPool[1].enRemapMode = VBUF_REMAP_MODE_NOCACHE;

    stVbConfig.astFrameInfo[2].enFrameType = MDP_FRAME_TYPE_COMMON;
    stVbConfig.astFrameInfo[2].stCommFrameInfo.u32Align = 32;
    stVbConfig.astFrameInfo[2].stCommFrameInfo.u32Width = scn_w;
    stVbConfig.astFrameInfo[2].stCommFrameInfo.u32Height = scn_h;
    stVbConfig.astFrameInfo[2].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stVbConfig.astCommPool[2].u32BufCnt = 2;
    stVbConfig.astCommPool[2].enRemapMode = VBUF_REMAP_MODE_CACHED;

    stVbConfig.u32PoolCnt = 3;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConfig);
    if (s32Ret)
        goto exit0;

    stVissConfig.astVissInfo[0].stDevInfo.VissDev = 0; /*0: DVP, 1: MIPI*/
    stVissConfig.astVissInfo[0].stDevInfo.aBindPhyChn[0] = 0;
    stVissConfig.astVissInfo[0].stDevInfo.aBindPhyChn[1] = 1;
    stVissConfig.astVissInfo[0].stDevInfo.aBindPhyChn[2] = 2;
    stVissConfig.astVissInfo[0].stDevInfo.aBindPhyChn[3] = 3;
    stVissConfig.astVissInfo[0].stDevInfo.enOutPath = VISS_OUT_PATH_DMA;
    stVissConfig.astVissInfo[0].stChnInfo.aVissChn[0] = 0;
    stVissConfig.astVissInfo[0].stChnInfo.aVissChn[1] = 1;
    stVissConfig.astVissInfo[0].stChnInfo.aVissChn[2] = 2;
    stVissConfig.astVissInfo[0].stChnInfo.aVissChn[3] = 3;
    stVissConfig.astVissInfo[0].stChnInfo.enWorkMode = VISS_WORK_MODE_4Chn;
    stVissConfig.astVissInfo[0].stIspInfo.IspDev = 0;
    stVissConfig.astVissInfo[0].stIspInfo.enRunningMode = ISP_MODE_RUNNING_OFFLINE;
    stVissConfig.astVissInfo[0].stChnInfo.stChnAttr.u32Align = 32;
    stVissConfig.astVissInfo[0].stChnInfo.stChnAttr.u32Depth = 2;
    stVissConfig.astVissInfo[0].stSnsInfo.enSnsType = SDR_VISS_SNS_DVP;

    stVissConfig.astVissInfo[1].stDevInfo.VissDev = 1; /*0: DVP, 1: MIPI*/
    stVissConfig.astVissInfo[1].stDevInfo.aBindPhyChn[0] = 4;
    stVissConfig.astVissInfo[1].stDevInfo.aBindPhyChn[1] = 5;
    stVissConfig.astVissInfo[1].stDevInfo.aBindPhyChn[2] = 6;
    stVissConfig.astVissInfo[1].stDevInfo.aBindPhyChn[3] = 7;
    stVissConfig.astVissInfo[1].stDevInfo.enOutPath = VISS_OUT_PATH_DMA;
    stVissConfig.astVissInfo[1].stChnInfo.aVissChn[0] = 4;
    stVissConfig.astVissInfo[1].stChnInfo.aVissChn[1] = 5;
    stVissConfig.astVissInfo[1].stChnInfo.aVissChn[2] = 6;
    stVissConfig.astVissInfo[1].stChnInfo.aVissChn[3] = 7;
    stVissConfig.astVissInfo[1].stChnInfo.enWorkMode = VISS_WORK_MODE_4Chn;
    stVissConfig.astVissInfo[1].stIspInfo.IspDev = 0;
    stVissConfig.astVissInfo[1].stIspInfo.enRunningMode = ISP_MODE_RUNNING_OFFLINE;
    stVissConfig.astVissInfo[1].stChnInfo.stChnAttr.u32Align = 32;
    stVissConfig.astVissInfo[1].stChnInfo.stChnAttr.u32Depth = 2;
    stVissConfig.astVissInfo[1].stSnsInfo.enSnsType = SDR_VISS_SNS_MIPI;

    stVissConfig.astVissInfo[0].stChnInfo.stChnAttr.stFrameRate.s32DstFrameRate = IPPU_FRAMERATE;
    stVissConfig.astVissInfo[1].stChnInfo.stChnAttr.stFrameRate.s32DstFrameRate = IPPU_FRAMERATE;

    stVissConfig.s32WorkingVissNum = 2;

    for (i = 0; i < ISP_MAX_DEV_NUM; i++) {
        if (i > 3) {
            pstVissInfo = &stVissConfig.astVissInfo[1];
        } else {
            pstVissInfo = &stVissConfig.astVissInfo[0];
        }

        pstVissInfo->stIspInfo.IspDev = i;
        s32Ret = SAMPLE_COMM_IPPU_Init(pstVissInfo);
        if (s32Ret) {
            goto exit1;
        }

        stIppuDevAttr.s32IppuDev        = pstVissInfo->stIspInfo.IspDev;
        stIppuDevAttr.u32InputWidth     = stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Width;
        stIppuDevAttr.u32InputHeight    = stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Height;
        stIppuDevAttr.u32DataSrc        = pstVissInfo->stDevInfo.VissDev;
        stIppuDevAttr.enRunningMode     = IPPU_MODE_RUNNING_ONLINE;

        abChnEnable[1] = EI_TRUE;
        astIppuChnAttr[1].u32Width      = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
        astIppuChnAttr[1].u32Height     = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
        astIppuChnAttr[1].u32Align      = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Align;
        astIppuChnAttr[1].enPixelFormat = stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat;
        astIppuChnAttr[1].stFrameRate.s32SrcFrameRate = IPPU_FRAMERATE;
        astIppuChnAttr[1].stFrameRate.s32DstFrameRate = IPPU_FRAMERATE;
        astIppuChnAttr[1].u32Depth      = 0;

        s32Ret = SAMPLE_COMM_IPPU_Start(i, abChnEnable, &stIppuDevAttr, astIppuChnAttr);
        if (s32Ret) {
            goto exit2;
        }
    }

    s32Ret = SAMPLE_COMM_VISS_StartViss(&stVissConfig);
    if (s32Ret) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit2;
    }

    for (i = 0; i < ISP_MAX_DEV_NUM; i++) {
        stSrcViss[i].enModId = EI_ID_VISS;
        stSrcViss[i].s32ChnId = i;
        stSrcViss[i].s32DevId = i > 3 ? 1 : 0;
        stSinkIsp[i].enModId = EI_ID_ISP;
        stSinkIsp[i].s32ChnId = 0;
        stSinkIsp[i].s32DevId = i;
        EI_MI_MLINK_Link(&stSrcViss[i], &stSinkIsp[i]);
    }

    /*display begin*/
    EI_MI_DOSS_Init();

    s32Ret = SAMPLE_COMM_DOSS_GetPanelAttr(VO_DEVICE, &stPanelAttr);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit2;
    }

    s32Ret = SAMPLE_COMM_DOSS_StartDev(0, &stPanelAttr);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit2;
    }

    stVoLayerAttr.u32Align = stVbConfig.astFrameInfo[2].stCommFrameInfo.u32Align;
    stVoLayerAttr.stImageSize.u32Width = stVbConfig.astFrameInfo[2].stCommFrameInfo.u32Width;
    stVoLayerAttr.stImageSize.u32Height = stVbConfig.astFrameInfo[2].stCommFrameInfo.u32Height;
    stVoLayerAttr.enPixFormat = stVbConfig.astFrameInfo[2].stCommFrameInfo.enPixelFormat;
    stVoLayerAttr.stDispRect.s32X = 0;
    stVoLayerAttr.stDispRect.s32Y = 0;
    stVoLayerAttr.stDispRect.u32Width = scn_w;
    stVoLayerAttr.stDispRect.u32Height = scn_h;
    s32Ret = EI_MI_VO_SetVideoLayerAttr(0, &stVoLayerAttr);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit2;
    }

    s32Ret = EI_MI_VO_SetDisplayBufLen(0, 3);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit2;
    }

    s32Ret = EI_MI_VO_SetVideoLayerPartitionMode(0, VO_PART_MODE_SINGLE);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit2;
    }

    s32Ret = EI_MI_VO_EnableVideoLayer(0);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit2;
    }
	_link_4x2ch_display(0, scn_w, scn_h, &stVoChnAttr, ippu_dev_4x2);
	
/*
    for (i = 0; i < ISP_MAX_DEV_NUM; i++) {
        if (i == 0) {
            stVoChnAttr.stRect.s32X = 0;
            stVoChnAttr.stRect.s32Y = 0;
        } else if (i == 1) {
            stVoChnAttr.stRect.s32X = 0;
            stVoChnAttr.stRect.s32Y = 320;
        } else if (i == 2) {
            stVoChnAttr.stRect.s32X = 0;
            stVoChnAttr.stRect.s32Y = 640;
        } else if (i == 3) {
            stVoChnAttr.stRect.s32X = 0;
            stVoChnAttr.stRect.s32Y = 960;
        } else if (i == 4) {
            stVoChnAttr.stRect.s32X = 360;
            stVoChnAttr.stRect.s32Y = 0;
        } else if (i == 5) {
            stVoChnAttr.stRect.s32X = 360;
            stVoChnAttr.stRect.s32Y = 320;
        } else if (i == 6) {
            stVoChnAttr.stRect.s32X = 360;
            stVoChnAttr.stRect.s32Y = 640;
        } else if (i == 7) {
            stVoChnAttr.stRect.s32X = 360;
            stVoChnAttr.stRect.s32Y = 960;
        }
        stVoChnAttr.stRect.u32Width = 360;
        stVoChnAttr.stRect.u32Height = 320;
        EI_MI_VO_SetChnAttr(0, i, &stVoChnAttr);
        EI_MI_VO_EnableChn(0, i);

        stSrcIppu[i].enModId = EI_ID_IPPU;
        stSrcIppu[i].s32ChnId = 1;
        stSrcIppu[i].s32DevId = i;
        stSinkDoss[i].enModId = EI_ID_DOSS;
        stSinkDoss[i].s32ChnId = i;
        stSinkDoss[i].s32DevId = 0;
        EI_MI_MLINK_Link(&stSrcIppu[i], &stSinkDoss[i]);
    }
*/
    printf("preview wait exit!\n");
    getchar();
    printf("preview start exit!\n");

exit2:
    for (i = 0; i < ISP_MAX_DEV_NUM; i++) {
        EI_MI_MLINK_UnLink(&stSrcViss[i], &stSinkIsp[i]);
    }
    SAMPLE_COMM_VISS_StopViss(&stVissConfig);
    for (i = 0; i < ISP_MAX_DEV_NUM; i++) {
		stSrcIppu[i].enModId = EI_ID_IPPU;
        stSrcIppu[i].s32ChnId = 1;
        stSrcIppu[i].s32DevId = i;
        stSinkDoss[i].enModId = EI_ID_DOSS;
        stSinkDoss[i].s32ChnId = i;
        stSinkDoss[i].s32DevId = 0;
        EI_MI_MLINK_UnLink(&stSrcIppu[i], &stSinkDoss[i]);
        EI_MI_VO_DisableChn(0, i);
    }
    EI_MI_VO_DisableVideoLayer(0);
    SAMPLE_COMM_DOSS_StopDev(0);
    EI_MI_VO_CloseFd();
    EI_MI_DOSS_Exit();
    for (i = 0; i < ISP_MAX_DEV_NUM; i++)
        SAMPLE_COMM_IPPU_Stop(i, abChnEnable);
exit1:
    for (i = 0; i < ISP_MAX_DEV_NUM; i++)
        SAMPLE_COMM_IPPU_Exit(i);
exit0:
    SAMPLE_COMM_SYS_Exit(&stVbConfig);

    printf("8ch preview end exit!\n");
    exit(EXIT_SUCCESS);

    return s32Ret;

}

int main(int argc, char **argv)
{
    int ret = EI_FAILURE;

    ret = ippu_show_offline_8ch();
    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
