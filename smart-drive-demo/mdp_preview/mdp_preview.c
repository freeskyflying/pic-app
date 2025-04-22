/*
 *------------------------------------------------------------------------------
 * @File      :    mdp_preview.c
 * @Date      :    2021-9-24
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

static EI_U32 scn_w, scn_h, scn_rate;

void preview_help(void)
{
    printf("usage: mdp_preview \n");
    printf("such as: mdp_preview\n");
    printf("such as: mdp_preview -d 0 -c 2\n");
    printf("such as: mdp_preview -d 1 -c 2 -r 90 -m 1 -f 0 -x 90 -y 180 -w 540 -h 960\n");
    printf("arguments:\n");
    printf("-d    select input device, Dev:0~1\n");
    printf("-c    select chn id, support:0~3\n");
    printf("-r    select rotation, 0/90/180/270\n");
    printf("-m    enable mirror, 0/1 \n");
    printf("-f    enable flip, 0/1 \n");
    printf("-x    select display rect X position, support:0~720\n");
    printf("-y    select display rect Y position, support:0~1280\n");
    printf("-w    select display rect width, support:0~720\n");
    printf("-h    select display rect hight, support:0~1280\n");
}

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

static int ippu_show_offline_1ch(VISS_DEV	dev, VISS_CHN chn, ROTATION_E rot, EI_BOOL mirror, EI_BOOL flip,
    unsigned int rect_x, unsigned int rect_y, unsigned int width, unsigned int hight)
{
    EI_S32 s32Ret = EI_FAILURE;
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
    IPPU_ROTATION_ATTR_S astIppuChnRot[IPPU_PHY_CHN_MAX_NUM] = {{0}};
	
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
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width = scn_w;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height = scn_h;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stVbConfig.astCommPool[1].u32BufCnt = 12;
    stVbConfig.astCommPool[1].enRemapMode = VBUF_REMAP_MODE_CACHED;

    stVbConfig.u32PoolCnt = 2;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConfig);
    if (s32Ret)
        goto exit0;

    if (dev == 0) {
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
        stVissConfig.astVissInfo[0].stSnsInfo.enSnsType = SDR_VISS_SNS_DVP;
        stVissConfig.astVissInfo[0].stChnInfo.stChnAttr.stFrameRate.s32DstFrameRate = 15;
    }else if (dev == 1) {
        stVissConfig.astVissInfo[0].stDevInfo.VissDev = 1; /*0: DVP, 1: MIPI*/
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
        stVissConfig.astVissInfo[0].stSnsInfo.enSnsType = SDR_VISS_SNS_MIPI;
        stVissConfig.astVissInfo[0].stChnInfo.stChnAttr.stFrameRate.s32DstFrameRate = 15;
    }
    stVissConfig.astVissInfo[0].stChnInfo.stChnAttr.u32Depth = 2;
    stVissConfig.s32WorkingVissNum = 1;

    pstVissInfo = &stVissConfig.astVissInfo[0];
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
    astIppuChnAttr[1].stFrameRate.s32SrcFrameRate = 30;
    astIppuChnAttr[1].stFrameRate.s32DstFrameRate = 30;
    astIppuChnAttr[1].bFlip = flip;
    astIppuChnAttr[1].bMirror = mirror;
    astIppuChnAttr[1].u32Depth      = 0;

    s32Ret = SAMPLE_COMM_IPPU_Start(0, abChnEnable, &stIppuDevAttr, astIppuChnAttr);
    if (s32Ret) {
        goto exit2;
    }

    if (rot == ROTATION_90 || rot == ROTATION_180 || rot == ROTATION_270) {
        astIppuChnRot[0].bEnable = EI_TRUE;
        astIppuChnRot[0].eRotation = rot;
        EI_MI_IPPU_SetChnRotation(0, 1, &astIppuChnRot[0]);
    }

    s32Ret = SAMPLE_COMM_VISS_StartViss(&stVissConfig);
    if (s32Ret) {
        goto exit2;
    }

    stSrcViss[0].enModId = EI_ID_VISS;
    stSrcViss[0].s32ChnId = chn;
    stSrcViss[0].s32DevId = dev;
    stSinkIsp[0].enModId = EI_ID_ISP;
    stSinkIsp[0].s32ChnId = 0;
    stSinkIsp[0].s32DevId = 0;
    EI_MI_MLINK_Link(&stSrcViss[0], &stSinkIsp[0]);


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

    stVoLayerAttr.u32Align = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Align;
    stVoLayerAttr.stImageSize.u32Width = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
    stVoLayerAttr.stImageSize.u32Height = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
    stVoLayerAttr.enPixFormat = stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat;
    stVoLayerAttr.stDispRect.s32X = rect_x;
    stVoLayerAttr.stDispRect.s32Y = rect_y;
    stVoLayerAttr.stDispRect.u32Width = width;
    stVoLayerAttr.stDispRect.u32Height = hight;
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

    s32Ret = EI_MI_VO_SetVideoLayerPartitionMode(0, VO_PART_MODE_MULTI);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit2;
    }

    s32Ret = EI_MI_VO_EnableVideoLayer(0);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit2;
    }

    stVoChnAttr.stRect.s32X = rect_x;
    stVoChnAttr.stRect.s32Y = rect_y;
    stVoChnAttr.stRect.u32Width = width;
    stVoChnAttr.stRect.u32Height = hight;

    EI_MI_VO_SetChnAttr(0, 0, &stVoChnAttr);
    EI_MI_VO_EnableChn(0, 0);

    stSrcIppu[0].enModId = EI_ID_IPPU;
    stSrcIppu[0].s32ChnId = 1;
    stSrcIppu[0].s32DevId = 0;
    stSinkDoss[0].enModId = EI_ID_DOSS;
    stSinkDoss[0].s32ChnId = 0;
    stSinkDoss[0].s32DevId = 0;
    EI_MI_MLINK_Link(&stSrcIppu[0], &stSinkDoss[0]);


    printf("ISP%d wait exit!\n", 0);
    getchar();
    printf("ISP%d start exit!\n", 0);

exit2:
    EI_MI_MLINK_UnLink(&stSrcViss[0], &stSinkIsp[0]);
    SAMPLE_COMM_VISS_StopViss(&stVissConfig);

    EI_MI_MLINK_UnLink(&stSrcIppu[0], &stSinkDoss[0]);
    EI_MI_VO_DisableChn(0, 0);

    EI_MI_VO_DisableVideoLayer(0);
    SAMPLE_COMM_DOSS_StopDev(0);
    EI_MI_VO_CloseFd();
    EI_MI_DOSS_Exit();

    SAMPLE_COMM_IPPU_Stop(0, abChnEnable);
exit1:
    SAMPLE_COMM_IPPU_Exit(0);
exit0:
    SAMPLE_COMM_SYS_Exit(&stVbConfig);

    printf("preview end exit!\n");
    exit(EXIT_SUCCESS);

    return s32Ret;

}


int main(int argc, char **argv)
{
    EI_S32 s32Ret = EI_FAILURE;
    VISS_DEV dev = 0;
    VISS_CHN chn = 0;
    ROTATION_E rot = ROTATION_0;
    EI_BOOL flip = EI_FALSE;
    EI_BOOL mirror = EI_FALSE;
    unsigned int rect_x = 0;
    unsigned int rect_y = 0;
    unsigned int width = 720;
    unsigned int hight = 1280;
    int c;

    if (argc < 3) {
        preview_help();
        goto EXIT;
    }

    while ((c = getopt(argc, argv, "d:c:r:m:f:x:y:w:h:")) != -1) {
        switch (c) {
        case 'd':
            dev = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'c':
            chn = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'r':
            rot = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'm':
            mirror = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'f':
            flip = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'x':
            rect_x = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'y':
            rect_y = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'w':
            width = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'h':
            hight = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        default:
            preview_help();
            goto EXIT;
        }
    }

    /* signal(SIGINT, isp_stop); */
    /* signal(SIGTERM, isp_stop); */
    if (rot == 90) {
        rot = ROTATION_90;
    }else if (rot == 180) {
        rot = ROTATION_180;
    }else if (rot == 270) {
        rot = ROTATION_270;
    }

    s32Ret = ippu_show_offline_1ch(dev, chn, rot, mirror, flip, rect_x, rect_y, width, hight);

EXIT:
    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
