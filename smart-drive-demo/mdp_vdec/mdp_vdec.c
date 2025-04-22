/*
 *------------------------------------------------------------------------------
 * @File      :    mdp_vdec.c
 * @Date      :    2021-9-27
 * @Author    :    lomboswer <lomboswer@lombotech.com>
 * @Brief     :    Source file for MDP(Media Development Platform).
 *
 * Copyright (C) 2020-2021, LomboTech Co.Ltd. All rights reserved.
 *------------------------------------------------------------------------------
 */
#include "mi_vdec.h"
#include "mi_vbuf.h"
#include "mi_mfake.h"
#include "ei_comm_vc.h"
#include "sample_comm.h"
#include "ei_comm_vdec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#ifndef __EOS__
#define DATA_ROOT "/data/"
#else
#define DATA_ROOT "/mnt/card/"
#endif
#define VC_USER_DST_FILE_PATH DATA_ROOT"raw.yuv"

#define VDEC_OUTPUT_FORMAT PIX_FMT_YUV_PLANAR_420
#define VDEC_OUT_WIDTH  1280
#define VDEC_OUT_HEIGHT 720

#define DECODE_FRAME_CNT 100
typedef struct thread_ch_param {
    VC_CHN ch;
    EI_S32 vdecStreamBytes;
    PAYLOAD_TYPE_E enPixelFormat;
    EI_BOOL thread_start;
    EI_BOOL stream_end;
    EI_S32 vdecframeRate;
    EI_CHAR *videoFilePath;
} thread_dec_param_t;

typedef struct video_file_info {
    EI_S32 width;
    EI_S32 hight;
    EI_CHAR *videoFilePath;
    EI_S32 type;
    EI_S32 frameRate;
} video_file_info_t;

#define VO_DEVICE  OTM1289A_MIPI_720_1280_60FPS
#define FREAD_PKT_LEN 600*1024
static EI_U32 scn_w, scn_h, scn_rate;

int read_stream_from_file(FILE *fin, unsigned char *buf, unsigned len, int pkt_ts, VDEC_STREAM_S *input_stream)
{
    int ret;

    input_stream->bDisplay  = 1;
    input_stream->pu8Addr = buf;
    input_stream->u64PTS = pkt_ts;

    ret = fread(input_stream->pu8Addr, 1, len, fin);
    input_stream->u32Len = ret;
    EI_TRACE_VDEC(EI_DBG_ERR, "read %d\n",ret);
    return ret;
}

static void *vdec_buffer_process(void *param)
{
    thread_dec_param_t *thread_parm = param;
    VC_CHN VdecChn = thread_parm->ch;
    VDEC_CHN_STATUS_S status;
    VIDEO_FRAME_INFO_S frame_info;
    EI_S32 s32Ret;
    int ret;
    int frame_num = 0;
    int frameRate = thread_parm->vdecframeRate;
    int frame_interval = 1000*1000/frameRate;

    while (thread_parm->thread_start == EI_TRUE) {
        s32Ret =  EI_MI_VDEC_QueryStatus(VdecChn, &status);
        if (s32Ret != EI_SUCCESS)
        {
            EI_TRACE_VDEC(EI_DBG_ERR, "EI_MI_VDEC_QueryStatus ERR\n");
            break;
        }
        EI_TRACE_VDEC(EI_DBG_ERR, "u32DecodeStreamFrames  %d u32LeftStreamBytes %d, errset %d\n", status.u32DecodeStreamFrames,
                                    status.u32LeftStreamBytes, status.stVdecDecErr.s32PicBufSizeErrSet);

        if (status.u32DecodeStreamFrames > 0 && status.u32LeftStreamBytes <= 0
            && thread_parm->stream_end)
        {
            EI_TRACE_VDEC(EI_DBG_ERR, "buffer end, exit\n");
            break;
        }

        s32Ret = EI_MI_VDEC_GetFrame(VdecChn, &frame_info, 500);
        if (EI_ERR_VDEC_BUF_EMPTY == s32Ret)
        {
            EI_TRACE_VDEC(EI_DBG_ERR, "wait for buffer\n");
            usleep(50 * 1000);
            thread_parm->stream_end = EI_TRUE;
            continue;
        }

        frame_info.enFrameType = MDP_FRAME_TYPE_DOSS;
        ret = EI_MI_VO_SendFrame(0, 0, &frame_info, 100);
        if (ret != EI_SUCCESS) {
            EI_PRINT("EI_MI_VO_SendFrame FAILURE!\n");
        } else {
            frame_num++;
            EI_PRINT("EI_MI_VO_SendFrame SUCCESS %d  %d!\n", frame_num, frame_interval);
        }
        EI_MI_VDEC_ReleaseFrame(VdecChn, &frame_info);

        usleep(frame_interval);
    }

    thread_parm->thread_start = EI_FALSE;

    EI_TRACE_VDEC(EI_DBG_ERR, "exit dec buffer\n");
    pthread_exit(NULL);
    return NULL;
}

static void *vdec_frame_process(void *param)
{

    thread_dec_param_t *thread_parm = param;
    VC_CHN VdecChn = thread_parm->ch;
    /* PAYLOAD_TYPE_E enType = thread_parm->enPixelFormat; */
    EI_CHAR *videoFilePath = thread_parm->videoFilePath;
    FILE *fin;
    VDEC_STREAM_S input_stream;
    unsigned char *pkt_data;
    int pkt_len;
    int decode_cnt = 0;
    int ret;
#if 1
    fin = fopen(videoFilePath, "rb");
#else
    if (enType == PT_H265) {
        fin = fopen(videoFilePath, "rb"); /* fopen(DATA_ROOT"raw.h265", "rb"); */
    } else if (enType == PT_H264) {
        fin = fopen(DATA_ROOT"raw.h264", "rb");
    } else if ((enType == PT_MJPEG) || (enType == PT_JPEG)) {
        fin = fopen(DATA_ROOT"raw.mjpeg", "rb");
    } else {
        fin = fopen(DATA_ROOT"raw.data", "rb");
    }
#endif
    if (fin == NULL) {
        EI_TRACE_VDEC(EI_DBG_ERR, "input file (stream) open error\n");
        EI_PRINT("input file (stream) open error\n");
        thread_parm->thread_start = EI_FALSE;
        return NULL;
    }

    pkt_data = malloc(FREAD_PKT_LEN);
    pkt_len = FREAD_PKT_LEN;

    EI_TRACE_VDEC(EI_DBG_ERR, "enter\n");
    while (thread_parm->thread_start == EI_TRUE)
    {
        EI_TRACE_VDEC(EI_DBG_ERR, "start read file\n");
        ret = read_stream_from_file(fin, pkt_data, pkt_len, decode_cnt, &input_stream);
        if (ret == 0)
        {
            break;
        }

        EI_MI_VDEC_SendStream(VdecChn, &input_stream, -1);
    }

    EI_TRACE_VDEC(EI_DBG_ERR, "bitstream send end of stream\n");
    memset(&input_stream, 0, sizeof(VDEC_STREAM_S));
    input_stream.bEndOfStream = 1;
    EI_MI_VDEC_SendStream(VdecChn, &input_stream, -1);
    /* thread_parm->stream_end = EI_TRUE; */

    free(pkt_data);
    fclose(fin);
    EI_TRACE_VDEC(EI_DBG_ERR, "bitstream read end\n");
    pthread_exit(NULL);
    return NULL;
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

int mdp_vdec_display(video_file_info_t *videoFileInfo)
{
    MDP_CHN_S stSrcChn;
    MDP_CHN_S stSinkChn;
    VBUF_POOL_CONFIG_S stPoolCfg = {0};
    /* MFAKE_SINK_CHN_ATTR_S stSinkChnAttr = {0}; */
    VIDEO_FRAME_INFO_S stVideoFrameInfo = {0};
    VC_CHN VdecChn;
    VBUF_POOL Pool;
    VDEC_CHN_ATTR_S stVdecAttr = {0};
    VDEC_CHN_PARAM_S stVdecParam = {0};
    EI_S32 s32Ret;
    int ret;
    pthread_t buffer_thread_id;
    pthread_t frameSend_thread_id;
    thread_dec_param_t thread_parm = {0};
    VO_VIDEO_LAYER_ATTR_S stVoLayerAttr = {0};
    PANEL_TYPE_ATTR_S stPanelAttr = {0};
    VO_CHN_ATTR_S stVoChnAttr = {0};

	scn_w = scn_h = scn_rate = 0;
    _get_screen_info(&scn_w, &scn_h, &scn_rate);
    if ((scn_w == 0) || (scn_h == 0) || (scn_rate == 0)) {
	    EI_TRACE_VDEC(EI_DBG_ERR, "%s %d get screen info fail. \n");
	    return -1;
    }

    EI_MI_MBASE_Init();
    EI_MI_VBUF_Init();
    EI_MI_MLINK_Init();

    EI_MI_VC_Init();
    EI_MI_MBASE_SetUsrLogLevel(EI_ID_JVC, 7);

    stVideoFrameInfo.enFrameType = MDP_FRAME_TYPE_COMMON;
    stVideoFrameInfo.stCommFrameInfo.u32Align = 16;
    stVideoFrameInfo.stCommFrameInfo.u32Width = videoFileInfo->width;
    stVideoFrameInfo.stCommFrameInfo.u32Height = videoFileInfo->hight;
    stVideoFrameInfo.stCommFrameInfo.enPixelFormat = VDEC_OUTPUT_FORMAT;
    stPoolCfg.u32BufSize = EI_MI_VBUF_GetPicBufferSize(&stVideoFrameInfo);

    EI_TRACE_VDEC(EI_DBG_ERR, "stPoolCfg.u32BufSize : %d\n", stPoolCfg.u32BufSize);

    stPoolCfg.u32BufCnt = 16;
    stPoolCfg.enRemapMode = 1;
    stPoolCfg.enVbufUid = VBUF_UID_COMMON;
    EI_TRACE_VDEC(EI_DBG_ERR, "creat pool start\n");
    Pool = EI_MI_VBUF_CreatePool(&stPoolCfg);
    EI_MI_VBUF_SetFrameInfo(Pool, &stVideoFrameInfo);

    memset(&stVdecAttr, 0, sizeof(VDEC_CHN_ATTR_S));
    stVdecAttr.u32Width = videoFileInfo->width;
    stVdecAttr.u32Height = videoFileInfo->hight;
    stVdecAttr.enType = videoFileInfo->type;
    stVdecAttr.enPixelFormat = VDEC_OUTPUT_FORMAT;
    stVdecAttr.u32Align = 16;
    stVdecAttr.u32StreamBufSize = videoFileInfo->width*videoFileInfo->hight*1.5;
    stVdecAttr.enMode = VIDEO_MODE_STREAM;
    stVdecAttr.enVdecPoolMode = VIDEO_POOL_USER;
    stVdecAttr.u32FrameBufCnt = 12;
    stVdecAttr.u32UserDisFrameNum = 6;

    stVdecAttr.stVdecVideoAttr.stVdecScale.bScaleEnable = 0;
    VdecChn = 1;

    s32Ret = EI_MI_VDEC_CreateChn(VdecChn, &stVdecAttr);
    if (s32Ret != EI_SUCCESS)
    {
        EI_TRACE_VDEC(EI_DBG_ERR, "vdec create chn error\n");
        return -1;
    }

    stVdecParam.u32DisplayFrameNum = 6;
    s32Ret = EI_MI_VDEC_SetChnParam(VdecChn, &stVdecParam);
    if (s32Ret != EI_SUCCESS)
    {
        EI_TRACE_VDEC(EI_DBG_ERR, "vdec set chn error\n");
        return -1;
    }

    s32Ret = EI_MI_VDEC_StartRecvStream(VdecChn);
        if (s32Ret != EI_SUCCESS)
    {
        EI_TRACE_VDEC(EI_DBG_ERR, "vdec create chn error\n");
        return -1;
    }

    EI_TRACE_VDEC(EI_DBG_DEBUG, "EI_MI_DOSS_Init\n");

    /*display begin*/
    EI_MI_DOSS_Init();

    s32Ret = SAMPLE_COMM_DOSS_GetPanelAttr(VO_DEVICE, &stPanelAttr);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        /* goto exit3; */
    }

    s32Ret = SAMPLE_COMM_DOSS_StartDev(0, &stPanelAttr);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        /* goto exit3; */
    }

    stVoLayerAttr.u32Align = 16;
    stVoLayerAttr.stImageSize.u32Width = videoFileInfo->width;
    stVoLayerAttr.stImageSize.u32Height = videoFileInfo->hight;
    stVoLayerAttr.enPixFormat = VDEC_OUTPUT_FORMAT;
    stVoLayerAttr.stDispRect.s32X = 0;
    stVoLayerAttr.stDispRect.s32Y = 0;
    stVoLayerAttr.stDispRect.u32Width = scn_w;    /* 屏幕显示区域 */
    stVoLayerAttr.stDispRect.u32Height = scn_h;  /* 屏幕显示区域 */
    s32Ret = EI_MI_VO_SetVideoLayerAttr(0, &stVoLayerAttr);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        /* goto EXIT; */
    }

    s32Ret = EI_MI_VO_SetChnRotation(0, 0, ROTATION_0);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        /* goto EXIT; */
    }

    s32Ret = EI_MI_VO_SetDisplayBufLen(0, 3);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        /* goto exit3; */
    }

    s32Ret = EI_MI_VO_SetVideoLayerPartitionMode(0, VO_PART_MODE_MULTI);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        /* goto exit3; */
    }

    s32Ret = EI_MI_VO_EnableVideoLayer(0);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        /* goto exit3; */
    }

    stVoChnAttr.stRect.s32X = 0;
    stVoChnAttr.stRect.s32Y = 0;
    stVoChnAttr.stRect.u32Width = videoFileInfo->width;
    stVoChnAttr.stRect.u32Height = videoFileInfo->hight;

    EI_MI_VO_SetChnAttr(0, 0, &stVoChnAttr);
    EI_MI_VO_EnableChn(0, 0);

    thread_parm.ch = VdecChn;
    thread_parm.enPixelFormat = stVdecAttr.enType;
    thread_parm.thread_start = EI_TRUE;
    thread_parm.vdecframeRate = videoFileInfo->frameRate;
    thread_parm.videoFilePath = videoFileInfo->videoFilePath;
    thread_parm.stream_end = EI_FALSE;

    ret = pthread_create(&buffer_thread_id, NULL, vdec_buffer_process, &thread_parm);
    if (ret != 0) {
        EI_TRACE_VDEC(EI_DBG_ERR, "Thread creat error!");
        goto EXIT;
    }

    ret = pthread_create(&frameSend_thread_id, NULL, vdec_frame_process, &thread_parm);
    if (ret != 0) {
        EI_TRACE_VDEC(EI_DBG_ERR, "Thread creat error!");
        goto EXIT;
    }
    EI_TRACE_VDEC(EI_DBG_DEBUG, "vdec_frame_process\n");

    while (!thread_parm.stream_end) {
        usleep(1000 * 1000);
    }

    thread_parm.thread_start = EI_FALSE;
    pthread_join(frameSend_thread_id, NULL);

    EI_MI_MLINK_UnLink(&stSrcChn, &stSinkChn);

    EI_MI_VO_DisableChn(0, 0);
    EI_MI_VO_DisableVideoLayer(0);
    SAMPLE_COMM_DOSS_StopDev(0);
    EI_MI_DOSS_Exit();

EXIT:
    EI_MI_VDEC_StopRecvStream(VdecChn);
    EI_MI_VDEC_DestroyChn(VdecChn);
    EI_MI_VBUF_DestroyPool(Pool);
    EI_MI_VC_Exit();
    EI_MI_VBUF_Exit();
    EI_MI_MLINK_Exit();
    EI_MI_MBASE_Exit();
    EI_PRINT("video dec end, exit\n");
    return 0;
}

static EI_VOID vdec_help(EI_VOID)
{
    printf("Version:1.0.0  \n");
    printf("such as: mdp_vdec -w 1920 -h 1080 -e 0 -r 20 -f /data/raw.h265\n");
    printf("such as: mdp_vdec -w 1280 -h 720 -e 0 -r 25 -f /data/raw.h265\n");
    printf("arguments:\n");
    printf("-w    select video width, support:0~1920\n");
    printf("-h    select video hight, support:0~1080\n");
    printf("-f    select file path, no default\n");
    printf("-e    select video type, default:0, support:0~1, 0:h265, 1h264\n");
    printf("-r    select video playback frame rate, default:20, support:1~30\n");
}

int main(int argc, char **argv)
{
    EI_S32 s32Ret = EI_FAILURE;
    video_file_info_t videoFileInfo = {0};
    EI_S32 width = 720;
    EI_S32 hight = 1280;
    EI_CHAR *videoFilePath = NULL;
    EI_S32 type = 0;    /* 0:h265 1:h264 */
    EI_S32 frameRate = 20;
    int c;

    if (argc < 3) {
        vdec_help();
        goto EXIT;
    }

    while ((c = getopt(argc, argv, "w:h:f:e:r:")) != -1) {
        switch (c) {
        case 'w':
            width = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'h':
            hight = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'f':
            videoFilePath = (EI_CHAR *)argv[optind - 1];
            break;
        case 'e':
            type = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'r':
            frameRate = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        default:
            vdec_help();
            goto EXIT;
        }
    }

    if (videoFilePath == NULL) {
        EI_PRINT("The video file path cannot be empty!");
        goto EXIT;
    }

    videoFileInfo.width = width;
    videoFileInfo.hight = hight;
    videoFileInfo.videoFilePath = videoFilePath;
    videoFileInfo.frameRate = frameRate;
    if (type == 0) {
        videoFileInfo.type = PT_H265;
    } else if (type == 1) {
        videoFileInfo.type = PT_H264;
    }
    EI_PRINT("width:%d,hight:%d,videoFilePath:%s,enType:%s,frameRate:%d\n",
        width, hight, videoFilePath, type?"H264":"H265", frameRate);

    s32Ret = mdp_vdec_display(&videoFileInfo);

EXIT:
    return s32Ret;
}

