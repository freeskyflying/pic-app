/*
 *------------------------------------------------------------------------------
 * @File      :    sample_venc.c
 * @Date      :    2021-3-16
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
#include "sdrive_sample_muxer.h"

#define EI_TRACE_LOG(level, fmt, args...)\
do{ \
    EI_TRACE(EI_ID_LOG, level, fmt, ##args);\
} while (0)

typedef struct __sample_venc_para_t {
    EI_BOOL bThreadStart;
    VC_CHN VeChn;
    int bitrate;
    int file_idx;
    int muxer_type;
    int init_flag;

    FILE *flip_out;
    char *filebuf;
    int buf_offset;

    void *muxer_hdl;
    pthread_t gs_VencPid;
} sdr_sample_venc_para_t;

typedef struct {
    unsigned int total;
    unsigned int free;
} sdcard_info_t;

enum {
    AV_MUXER_TYPE_MP4 = 0X1000,
    AV_MUXER_TYPE_MOV,
    AV_MUXER_TYPE_MKV,
    AV_MUXER_TYPE_TS,
    AV_MUXER_TYPE_FLV,
    AV_MUXER_TYPE_AVI,
    AV_MUXER_TYPE_RAW,
    AV_MUXER_TYPE_ASF,
    AV_MUXER_TYPE_RM,
    AV_MUXER_TYPE_MPG,
};

#define USE_MUXER
#define WRITE_BUFFER_SIZE 1*1024*1024
#if 0
#define SDR_VISS_SNS_DVP TP9930_DVP_1920_1080_30FPS_4CH_YUV
#define SDR_VISS_SNS_MIPI TP2815_MIPI_1920_1080_30FPS_4CH_YUV
#define SDR_VISS_FRAMERATE 30
#else
#define SDR_VISS_SNS_DVP TP9930_DVP_1920_1080_25FPS_4CH_YUV
#define SDR_VISS_SNS_MIPI TP2815_MIPI_1920_1080_25FPS_4CH_YUV
#define SDR_VISS_FRAMERATE 25
#endif
#define MAIN_STREAM_BITRATE 4 * 1024 * 1024
#define MAIN_STREAM_FRAMERATE 20
#define MAIN_STREAM_MUXER_TYPE AV_MUXER_TYPE_TS
#define MAIN_STREAM_NUMBER 8
#define SUB_STREAM_NUMBER 8
#define SUB_STREAM_BITRATE 500 * 1024
#define SUB_STREAM_FRAMERATE 14
#define SUB_STREAM_MUXER_TYPE -1

#define SDR_SAMPLE_VENC_TIME 300
#define SDR_SAMPLE_FILE_NUM 6
#define PATH_ROOT "/mnt/card"

#define VO_DEVICE  OTM1289A_MIPI_720_1280_60FPS		/* TP2803_BT_720_576_25FPS */

static EI_U32 scn_w, scn_h, scn_rate;

void storage_info_get(const char *mount_point, sdcard_info_t *info)
{
    struct statfs disk_info;
    int blksize;
    FILE *fp;
    char split[] = " ";
    char linebuf[512];
    char *result = NULL;
    int mounted = 0;
    char mount_path[128];
    int len;
    len = strlen(mount_point);
    if (!len)
        return;
    strcpy(mount_path, mount_point);
    if (mount_path[len - 1] == '/')
        mount_path[len - 1] = '\0';
    fp = fopen("/proc/mounts", "r");
    if (fp == NULL) {
        EI_TRACE_LOG(EI_DBG_ERR, "open error mount proc");
        info->total = 0;
        info->free = 0;
        return;
    }
    while (fgets(linebuf, 512, fp)) {
        result = strtok(linebuf, split);
        result = strtok(NULL, split);
        if (!strncmp(result, mount_path, strlen(mount_path))) {
            mounted = 1;
            break;
        }
    }
    fclose(fp);
    if (mounted ==  0) {
        info->total = 0;
        info->free = 0;
        return;
    }

    memset(&disk_info, 0, sizeof(struct statfs));
    if (statfs(mount_path, &disk_info) < 0)
        return;
    if (disk_info.f_bsize >= (1 << 10)) {
        info->total = ((unsigned int)(disk_info.f_bsize >> 10) *
                disk_info.f_blocks);
        info->free = ((unsigned int)(disk_info.f_bsize >> 10) *
                disk_info.f_bfree);
    } else if (disk_info.f_bsize > 0) {
        blksize = ((1 << 10) / disk_info.f_bsize);
        info->total = (disk_info.f_blocks / blksize);
        info->free = (disk_info.f_bfree / blksize);
    }
}

void do_fallocate(const char *path, unsigned long fsize)
{
    int res;
    int fd;

    fd = open(path, O_RDWR, 0666);
    if (fd >= 0) {
        close(fd);
        return;
    }
    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        EI_TRACE_LOG(EI_DBG_ERR, "open %s fail\n", path);
        return;
    }
    res = fallocate(fd, FALLOC_FL_KEEP_SIZE, 0, fsize);
    close(fd);

    if (res)
        EI_TRACE_LOG(EI_DBG_ERR, "fallocate fail(%i)\n", res);
    else
        EI_TRACE_LOG(EI_DBG_INFO, "fallocate %s finish\n", path);
}

void _fallocate_stream_file(sdr_sample_venc_para_t *sdr_venc_param, int number_stream)
{
    char aszFileName[128];
    char *szFilePostfix;
    int type;
    static sdcard_info_t info;
    int i;
    int size;
    sdr_sample_venc_para_t *venc_para;

    if (sdr_venc_param == NULL)
        return;

    storage_info_get(PATH_ROOT, &info);
    if (info.free < 500 * 1024)
        return;

    for (i = 0; i < number_stream; i++) {
        venc_para = &sdr_venc_param[i];
        type = venc_para->muxer_type;
        if (type == AV_MUXER_TYPE_RAW)
            szFilePostfix = ".h265";
        else if (type == AV_MUXER_TYPE_TS)
            szFilePostfix = ".ts";
        else if (type == AV_MUXER_TYPE_MP4)
            szFilePostfix = ".mp4";
        else {
            return;
        }
        snprintf(aszFileName, 128, PATH_ROOT"/stream_chn%02d-%03d%s",
            venc_para->VeChn,
            venc_para->file_idx, szFilePostfix);
        size = venc_para->bitrate * SDR_SAMPLE_VENC_TIME / 8 + 16 * 1024 *1024;
        do_fallocate(aszFileName, size);
    }

}


static inline int _save_raw_init(sdr_sample_venc_para_t *sdr_venc_para,
    char *aszFileName)
{
    sdr_venc_para->flip_out = fopen(aszFileName, "wb+");
    if (sdr_venc_para->flip_out == NULL) {
        EI_PRINT("open file %s failed!\n", aszFileName);
        return -1;
    }
    sdr_venc_para->file_idx++;
    if (sdr_venc_para->file_idx >= SDR_SAMPLE_FILE_NUM)
        sdr_venc_para->file_idx = 0;

    sdr_venc_para->buf_offset = 0;
    if (sdr_venc_para->filebuf) {
        EI_PRINT("filebuf is not null(%p)!\n", sdr_venc_para->filebuf);
        return 0;
    }
    sdr_venc_para->filebuf = malloc(WRITE_BUFFER_SIZE);
    if (!sdr_venc_para->filebuf) {
        fclose(sdr_venc_para->flip_out);
        sdr_venc_para->flip_out= NULL;
        EI_PRINT("malloc[%d] failed!\n", WRITE_BUFFER_SIZE);
        return -1;
    }
    return 0;
}

static inline int _save_raw_deinit(sdr_sample_venc_para_t *sdr_venc_para)
{
    if (sdr_venc_para->flip_out && sdr_venc_para->filebuf &&
            sdr_venc_para->buf_offset) {
        fwrite(sdr_venc_para->filebuf, 1, sdr_venc_para->buf_offset,
            sdr_venc_para->flip_out);
    }

    if (sdr_venc_para->flip_out) {
        fclose(sdr_venc_para->flip_out);
        sdr_venc_para->flip_out= NULL;
    }

    if (sdr_venc_para->filebuf) {
        free(sdr_venc_para->filebuf);
        sdr_venc_para->filebuf = NULL;
    }
    sdr_venc_para->buf_offset = 0;
    return 0;
}

static inline int __fwrite(sdr_sample_venc_para_t *sdr_venc_para,
    char *stream, int size)
{
    int ret = 0;
    if (sdr_venc_para->flip_out) {
        ret = fwrite(stream, 1, size, sdr_venc_para->flip_out);
        if (ret != size) {
            fclose(sdr_venc_para->flip_out);
            sdr_venc_para->flip_out = NULL;
            sdr_venc_para->buf_offset = 0;
        }
    }
    return ret;
}
static int __SaveStream2Buf(sdr_sample_venc_para_t *sdr_venc_para,
    char *stream, int size)
{
    int offset;
    char *buf;

    offset = sdr_venc_para->buf_offset;
    buf = sdr_venc_para->filebuf;
    if (size > WRITE_BUFFER_SIZE) {
        __fwrite(sdr_venc_para, buf, offset);
        __fwrite(sdr_venc_para, stream, size);
        offset = 0;
    } else if (offset + size >= WRITE_BUFFER_SIZE) {
        memcpy(buf + offset, stream, WRITE_BUFFER_SIZE - offset);
        __fwrite(sdr_venc_para, buf, WRITE_BUFFER_SIZE);
        if (size - (WRITE_BUFFER_SIZE - offset) > 0) {
            memcpy(buf, stream + WRITE_BUFFER_SIZE - offset,
                size - (WRITE_BUFFER_SIZE - offset));
        }
        offset = size - (WRITE_BUFFER_SIZE - offset);
    } else {
        memcpy(buf + offset, stream, size);
        offset += size;
    }

    sdr_venc_para->buf_offset = offset;
    return EI_SUCCESS;
}

static int _save_raw(sdr_sample_venc_para_t *sdr_venc_para, VENC_STREAM_S *pstStream)
{
    if (pstStream == NULL || sdr_venc_para == NULL || sdr_venc_para->filebuf == NULL)
        return EI_FAILURE;

    if ((pstStream->pstPack.pu8Addr[0] == EI_NULL) ||
        (pstStream->pstPack.u32Len[0] == 0) || sdr_venc_para->flip_out == NULL ||
        sdr_venc_para->buf_offset >= WRITE_BUFFER_SIZE) {
        EI_TRACE_LOG(EI_DBG_ERR, "save stream failed!\n");
        return EI_FAILURE;
    }
    __SaveStream2Buf(sdr_venc_para,
        (char *)pstStream->pstPack.pu8Addr[0], pstStream->pstPack.u32Len[0]);
    if ((pstStream->pstPack.pu8Addr[1] != EI_NULL) &&
        (pstStream->pstPack.u32Len[1] != 0)) {
        __SaveStream2Buf(sdr_venc_para, (char *)pstStream->pstPack.pu8Addr[1],
            pstStream->pstPack.u32Len[1]);
    }
    return EI_SUCCESS;
}

static inline int save_stream_init(sdr_sample_venc_para_t *sdr_venc_para)
{
    int ret = -1;
    char aszFileName[128];
    char *szFilePostfix;
    int type;
    static sdcard_info_t info;

    if (sdr_venc_para == NULL)
        return -1;
    type = sdr_venc_para->muxer_type;
    if (type == AV_MUXER_TYPE_RAW)
        szFilePostfix = ".h265";
    else if (type == AV_MUXER_TYPE_TS)
        szFilePostfix = ".ts";
    else if (type == AV_MUXER_TYPE_MP4)
        szFilePostfix = ".mp4";
    else {
        return -1;
    }

    storage_info_get(PATH_ROOT, &info);
    if (info.free < 500 * 1024)
        return -1;

    snprintf(aszFileName, 128, PATH_ROOT"/stream_chn%02d-%03d%s",
        sdr_venc_para->VeChn,
        sdr_venc_para->file_idx, szFilePostfix);
#ifdef USE_MUXER
    if (type == AV_MUXER_TYPE_TS || type == AV_MUXER_TYPE_MP4) {
        sdr_venc_para->muxer_hdl = sample_muxer_init(
            AV_MUXER_TYPE_TS, aszFileName, type, sdr_venc_para->VeChn);
        if (sdr_venc_para->muxer_hdl)
            ret = 0;
    }
#else
    type = AV_MUXER_TYPE_RAW;
    szFilePostfix = ".h265";
    sdr_venc_para->muxer_type = type;
#endif

    if (type == AV_MUXER_TYPE_RAW)
        ret = _save_raw_init(sdr_venc_para, aszFileName);
    if (ret)
        return ret;
    sdr_venc_para->file_idx++;
    if (sdr_venc_para->file_idx >= SDR_SAMPLE_FILE_NUM)
        sdr_venc_para->file_idx = 0;
    sdr_venc_para->init_flag = 1;

    return 0;
}

static int save_stream(sdr_sample_venc_para_t *sdr_venc_para, VENC_STREAM_S *pstStream)
{
    int ret = -1;
    if (pstStream == NULL || sdr_venc_para == NULL)
        return EI_FAILURE;

    if (sdr_venc_para->muxer_type== AV_MUXER_TYPE_RAW)
        ret = _save_raw(sdr_venc_para, pstStream);
#ifdef USE_MUXER
    if (sdr_venc_para->muxer_type == AV_MUXER_TYPE_TS ||
        sdr_venc_para->muxer_type == AV_MUXER_TYPE_MP4)
        ret = sample_muxer_write_packet(sdr_venc_para->muxer_hdl, pstStream);
#endif
    return ret;
}
static inline int save_stream_deinit(sdr_sample_venc_para_t *sdr_venc_para)
{
    int ret;
    if (sdr_venc_para == NULL)
        return EI_FAILURE;

    if (sdr_venc_para->muxer_type == AV_MUXER_TYPE_RAW)
        ret = _save_raw_deinit(sdr_venc_para);
#ifdef USE_MUXER
    if (sdr_venc_para->muxer_type == AV_MUXER_TYPE_TS ||
            sdr_venc_para->muxer_type == AV_MUXER_TYPE_MP4) {
        sample_muxer_deinit(sdr_venc_para->muxer_hdl);
        sdr_venc_para->muxer_hdl = NULL;
    }
#endif
    sdr_venc_para->muxer_type = 0;
    sdr_venc_para->init_flag = 0;
    return ret;
}
EI_VOID *_get_venc_stream_proc(EI_VOID *p)
{
    sdr_sample_venc_para_t *sdr_venc_para;
    VC_CHN VencChn;
    VENC_STREAM_S stStream;
    int ret, encoded_packet = 0;
    char thread_name[16];

    sdr_venc_para = (sdr_sample_venc_para_t *)p;
    if (!sdr_venc_para) {
        EI_TRACE_LOG(EI_DBG_ERR, "err param !\n");
        return NULL;
    }

    VencChn = sdr_venc_para->VeChn;

    thread_name[15] = 0;
    snprintf(thread_name, 16, "streamproc%d", VencChn);
    prctl(PR_SET_NAME, thread_name);
    while (EI_TRUE == sdr_venc_para->bThreadStart) {
        ret = EI_MI_VENC_GetStream(VencChn, &stStream, 100);
        if (ret == EI_ERR_VENC_NOBUF) {
            EI_TRACE_LOG(EI_DBG_INFO, "No buffer\n");
            usleep(5 * 1000);
            continue;
        } else if (ret != EI_SUCCESS) {
            EI_TRACE_LOG(EI_DBG_ERR, "get chn-%d error : %d\n", VencChn, ret);
            break;
        }
        if (stStream.pstPack.u32Len[0] == 0) {
            EI_PRINT("%d ch %d pstPack.u32Len:%d-%d, ofset:%d, ret:%x\n",
                __LINE__, VencChn, stStream.pstPack.u32Len[0],
                stStream.pstPack.u32Len[1],
                sdr_venc_para->buf_offset, ret);
        }
        if (sdr_venc_para->init_flag == 0 &&
            (stStream.pstPack.enDataType == DATA_TYPE_I_FRAME || stStream.pstPack.enDataType == DATA_TYPE_INITPACKET))
            save_stream_init(sdr_venc_para);
        if (sdr_venc_para->init_flag) {
            ret = save_stream(sdr_venc_para, &stStream);
            if (ret) {
                EI_PRINT("save stream err!");
                save_stream_deinit(sdr_venc_para);
            }
        }
        encoded_packet++;
        ret = EI_MI_VENC_ReleaseStream(VencChn, &stStream);
        if (ret != EI_SUCCESS) {
            EI_TRACE_LOG(EI_DBG_ERR, "release stream chn-%d error : %d\n",
                VencChn, ret);
            break;
        }
    }

    return NULL;
}

int _start_get_stream(sdr_sample_venc_para_t *venc_para)
{
    int ret;
    if (venc_para == NULL || venc_para->VeChn < 0 ||
            venc_para->VeChn >= VC_MAX_CHN_NUM)
        return EI_FAILURE;

    venc_para->bThreadStart = EI_TRUE;

    ret = pthread_create(&venc_para->gs_VencPid, 0, _get_venc_stream_proc,
            (EI_VOID *)venc_para);
    if (ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));
    return ret;
}

int _stop_get_stream(sdr_sample_venc_para_t *venc_para)
{
    if (venc_para == NULL ||
        venc_para->VeChn < 0 || venc_para->VeChn >= VC_MAX_CHN_NUM) {
        EI_PRINT("%s %d  venc_para:%p:%d\n", __func__, __LINE__,
            venc_para, venc_para->VeChn);
        return EI_FAILURE;
    }
    venc_para->bThreadStart = EI_FALSE;
    if (venc_para->gs_VencPid) {
        pthread_join(venc_para->gs_VencPid, 0);
        venc_para->gs_VencPid = 0;
    }
    return EI_SUCCESS;
}

static int _bufpool_init(VBUF_CONFIG_S *vbuf_config)
{
    memset(vbuf_config, 0, sizeof(VBUF_CONFIG_S));
    vbuf_config->u32PoolCnt = 3;

    vbuf_config->astFrameInfo[0].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[0].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[0].stCommFrameInfo.u32Width = 1920;
    vbuf_config->astFrameInfo[0].stCommFrameInfo.u32Height = 1080;
    vbuf_config->astFrameInfo[0].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[0].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    vbuf_config->astFrameInfo[0].stCommFrameInfo.enCompressMode = COMPRESS_MODE_NONE;
    vbuf_config->astCommPool[0].u32BufCnt = 40;

    vbuf_config->astFrameInfo[1].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[1].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[1].stCommFrameInfo.u32Width = 640;
    vbuf_config->astFrameInfo[1].stCommFrameInfo.u32Height = 360;
    vbuf_config->astFrameInfo[1].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[1].u32BufCnt = 45;
    vbuf_config->astCommPool[1].enRemapMode = VBUF_REMAP_MODE_NOCACHE;

    vbuf_config->astFrameInfo[2].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[2].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[2].stCommFrameInfo.u32Width = 720;
    vbuf_config->astFrameInfo[2].stCommFrameInfo.u32Height = 1280;
    vbuf_config->astFrameInfo[2].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[2].u32BufCnt = 2;
    vbuf_config->astCommPool[2].enRemapMode = VBUF_REMAP_MODE_CACHED;

    return SAMPLE_COMM_SYS_Init(vbuf_config);
}

static int _bufpool_deinit(VBUF_CONFIG_S *vbuf_config)
{
    return SAMPLE_COMM_SYS_Exit(vbuf_config);
}

static int _start_viss(SAMPLE_VISS_CONFIG_S *viss_config)
{
    SAMPLE_VISS_INFO_S *viss_info;
    memset(viss_config, 0, sizeof(SAMPLE_VISS_CONFIG_S));
    viss_info = viss_config->astVissInfo;

    viss_config->s32WorkingVissNum = 2;
    viss_info = &viss_config->astVissInfo[0];
    viss_info->stDevInfo.VissDev = 0; /*0: DVP, 1: MIPI*/
    viss_info->stDevInfo.aBindPhyChn[0] = 0;
    viss_info->stDevInfo.aBindPhyChn[1] = 1;
    viss_info->stDevInfo.aBindPhyChn[2] = 2;
    viss_info->stDevInfo.aBindPhyChn[3] = 3;
    viss_info->stDevInfo.enOutPath = VISS_OUT_PATH_DMA;
    viss_info->stChnInfo.aVissChn[0] = 0;
    viss_info->stChnInfo.aVissChn[1] = 1;
    viss_info->stChnInfo.aVissChn[2] = 2;
    viss_info->stChnInfo.aVissChn[3] = 3;
    viss_info->stChnInfo.enWorkMode = VISS_WORK_MODE_4Chn;
    viss_info->stChnInfo.stChnAttr.u32Align = 32;
    viss_info->stSnsInfo.enSnsType = SDR_VISS_SNS_DVP;

    viss_info = &viss_config->astVissInfo[1];
    viss_info->stDevInfo.VissDev = 1; /*0: DVP, 1: MIPI*/
    viss_info->stDevInfo.aBindPhyChn[0] = 4;
    viss_info->stDevInfo.aBindPhyChn[1] = 5;
    viss_info->stDevInfo.aBindPhyChn[2] = 6;
    viss_info->stDevInfo.aBindPhyChn[3] = 7;
    viss_info->stDevInfo.enOutPath = VISS_OUT_PATH_DMA;
    viss_info->stChnInfo.aVissChn[0] = 4;
    viss_info->stChnInfo.aVissChn[1] = 5;
    viss_info->stChnInfo.aVissChn[2] = 6;
    viss_info->stChnInfo.aVissChn[3] = 7;
    viss_info->stChnInfo.enWorkMode = VISS_WORK_MODE_4Chn;
    viss_info->stChnInfo.stChnAttr.u32Align = 32;
    viss_info->stSnsInfo.enSnsType = SDR_VISS_SNS_MIPI;

    return SAMPLE_COMM_VISS_StartViss(viss_config);
}

static int _stop_viss(SAMPLE_VISS_CONFIG_S *viss_config)
{
    return SAMPLE_COMM_VISS_StopViss(viss_config);
}

static int _stop_ippu(int num_stream, EI_BOOL *chn_enabled)
{
    IPPU_DEV dev;
    MDP_CHN_S src_viss, sink_isp;

    for (dev = 0; dev < num_stream; dev++) {
        src_viss.enModId = EI_ID_VISS;
        src_viss.s32ChnId = dev;
        src_viss.s32DevId = dev > 3 ? 1 : 0;
        sink_isp.enModId = EI_ID_ISP;
        sink_isp.s32ChnId = 0;
        sink_isp.s32DevId = dev;
        EI_MI_MLINK_UnLink(&src_viss, &sink_isp);
    }
    for (dev = 0; dev < num_stream; dev++) {
        SAMPLE_COMM_IPPU_Stop(dev, chn_enabled);
        SAMPLE_COMM_IPPU_Exit(dev);
    }
    return EI_SUCCESS;
}

static int _start_ippu(EI_BOOL *chn_enabled,
    int num_stream,
    SAMPLE_VISS_CONFIG_S *viss_config,
    COMMON_FRAME_INFO_S *in_frame_info,
    COMMON_FRAME_INFO_S *out_frame_info)
{
    int i;
    int ret = 0;
    SAMPLE_VISS_INFO_S *viss_info;
    IPPU_DEV_ATTR_S ippu_dev_attr;
    IPPU_CHN_ATTR_S ippu_chn_attr[IPPU_PHY_CHN_MAX_NUM];
    MDP_CHN_S src_viss[ISP_MAX_DEV_NUM], sink_isp[ISP_MAX_DEV_NUM];
    int dev_num = 0;

    viss_config->astVissInfo[0].stIspInfo.enRunningMode = ISP_MODE_RUNNING_OFFLINE;
    viss_config->astVissInfo[0].stIspInfo.stFrc.s32SrcFrameRate =
        SDR_VISS_FRAMERATE; /* only support offline */
    viss_config->astVissInfo[0].stIspInfo.stFrc.s32DstFrameRate = SUB_STREAM_FRAMERATE;
    viss_config->astVissInfo[1].stIspInfo.enRunningMode = ISP_MODE_RUNNING_OFFLINE;
    viss_config->astVissInfo[1].stIspInfo.stFrc.s32SrcFrameRate =
        SDR_VISS_FRAMERATE; /* only support offline */
    viss_config->astVissInfo[1].stIspInfo.stFrc.s32DstFrameRate = SUB_STREAM_FRAMERATE;

    memset(&ippu_dev_attr, 0, sizeof(ippu_dev_attr));
    ippu_dev_attr.u32InputWidth	= in_frame_info->u32Width;
    ippu_dev_attr.u32InputHeight	= in_frame_info->u32Height;
    ippu_dev_attr.enRunningMode	= IPPU_MODE_RUNNING_ONLINE;

    memset(ippu_chn_attr, 0, sizeof(ippu_chn_attr));
    chn_enabled[0] = EI_FALSE;
    chn_enabled[1] = EI_TRUE;
    chn_enabled[2] = EI_FALSE;
    ippu_chn_attr[1].u32Width	= out_frame_info->u32Width;
    ippu_chn_attr[1].u32Height	= out_frame_info->u32Height;
    ippu_chn_attr[1].u32Align	= out_frame_info->u32Align;
    ippu_chn_attr[1].enPixelFormat = out_frame_info->enPixelFormat;
    ippu_chn_attr[1].stFrameRate.s32SrcFrameRate = SUB_STREAM_FRAMERATE;
    ippu_chn_attr[1].stFrameRate.s32DstFrameRate = SUB_STREAM_FRAMERATE;
    ippu_chn_attr[1].u32Depth	= 0;

    for (i = 0; i < num_stream; i++) {
        if (i > 3)
            viss_info = &viss_config->astVissInfo[1];
        else
            viss_info = &viss_config->astVissInfo[0];

        viss_info->stIspInfo.IspDev = i;
        ret = SAMPLE_COMM_IPPU_Init(viss_info);
        if (ret) {
            EI_PRINT("%s %d SAMPLE_COMM_IPPU_Init:%d err\n", __func__, __LINE__, i);
            goto exit;
        }

        ippu_dev_attr.s32IppuDev	    = i;
        ippu_dev_attr.u32DataSrc	    = viss_info->stDevInfo.VissDev;
        ret = SAMPLE_COMM_IPPU_Start(i, chn_enabled, &ippu_dev_attr, ippu_chn_attr);
        if (ret) {
            EI_PRINT("%s %d SAMPLE_COMM_IPPU_Start:%d err\n", __func__, __LINE__, i);
            SAMPLE_COMM_IPPU_Exit(ippu_dev_attr.s32IppuDev);
            goto exit;
        }
        dev_num++;
    }

    dev_num = 0;
    for (i = 0; i < num_stream; i++) {
        src_viss[i].enModId = EI_ID_VISS;
        src_viss[i].s32ChnId = i;
        src_viss[i].s32DevId = i > 3 ? 1 : 0;
        sink_isp[i].enModId = EI_ID_ISP;
        sink_isp[i].s32ChnId = 0;
        sink_isp[i].s32DevId = i;
        ret = EI_MI_MLINK_Link(&src_viss[i], &sink_isp[i]);
        if (ret) {
            EI_PRINT("%s %d EI_MI_MLINK_Link:%d err\n", __func__, __LINE__, i);
            goto exit1;
        }
        dev_num++;
    }
    return ret;
exit1:
    for (i = 0; i < dev_num; i++)
        EI_MI_MLINK_UnLink(&src_viss[i], &sink_isp[i]);
    dev_num = ISP_MAX_DEV_NUM;

exit:
    for (i = 0; i < dev_num; i++) {
        SAMPLE_COMM_IPPU_Stop(i, chn_enabled);
        SAMPLE_COMM_IPPU_Exit(i);
    }
    return 0;

}


static int _start_disp(VO_DEV dev, VO_LAYER layer,
    COMMON_FRAME_INFO_S *disp_frame_info)
{
    int i;
    int ret = EI_FAILURE;
    VO_VIDEO_LAYER_ATTR_S layer_attr;
    VO_CHN_ATTR_S vo_chn_attr;
    PANEL_TYPE_ATTR_S panel_attr;
    MDP_CHN_S src_ippu[ISP_MAX_DEV_NUM], sink_doss[ISP_MAX_DEV_NUM];

    memset(&layer_attr, 0, sizeof(layer_attr));
    memset(&vo_chn_attr, 0, sizeof(vo_chn_attr));
    memset(&panel_attr, 0, sizeof(panel_attr));

    /*display begin*/
    EI_MI_DOSS_Init();
    ret = SAMPLE_COMM_DOSS_GetPanelAttr(VO_DEVICE, &panel_attr);
    if (ret != EI_SUCCESS) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
        goto exit;
    }

    ret = SAMPLE_COMM_DOSS_StartDev(dev, &panel_attr);
    if (ret != EI_SUCCESS) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
        goto exit;
    }

    layer_attr.u32Align = disp_frame_info->u32Align;
    layer_attr.stImageSize.u32Width = scn_w;
    layer_attr.stImageSize.u32Height = scn_h;
    layer_attr.enPixFormat = disp_frame_info->enPixelFormat;
    layer_attr.stDispRect.s32X = 0;
    layer_attr.stDispRect.s32Y = 0;
    layer_attr.stDispRect.u32Width = scn_w;		//720;
    layer_attr.stDispRect.u32Height = scn_h;	//1280;
    ret = EI_MI_VO_SetVideoLayerAttr(layer, &layer_attr);
    if (ret != EI_SUCCESS) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
        goto exit1;
    }

    ret = EI_MI_VO_SetDisplayBufLen(layer, 3);
    if (ret != EI_SUCCESS) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
        goto exit1;
    }

    ret = EI_MI_VO_SetVideoLayerPartitionMode(layer, VO_PART_MODE_SINGLE);
    if (ret != EI_SUCCESS) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
        goto exit1;
    }

    ret = EI_MI_VO_EnableVideoLayer(layer);
    if (ret != EI_SUCCESS) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
        goto exit1;
    }

    for (i = 0; i < ISP_MAX_DEV_NUM; i++) {
        vo_chn_attr.stRect.s32X = i < 4 ? 0 : scn_w/2;
        vo_chn_attr.stRect.s32Y = (i % 4) * scn_h/4;
        vo_chn_attr.stRect.u32Width = scn_w/2;
        vo_chn_attr.stRect.u32Height = scn_h/4;
        EI_MI_VO_SetChnAttr(layer, i, &vo_chn_attr);
        EI_MI_VO_SetChnFrameRate(layer, i, SUB_STREAM_FRAMERATE);
        EI_MI_VO_EnableChn(layer, i);

        src_ippu[i].enModId = EI_ID_IPPU;
        src_ippu[i].s32ChnId = 1;
        src_ippu[i].s32DevId = i;
        sink_doss[i].enModId = EI_ID_DOSS;
        sink_doss[i].s32ChnId = i;
        sink_doss[i].s32DevId = 0;
        EI_MI_MLINK_Link(&src_ippu[i], &sink_doss[i]);
    }
    return ret;
exit1:
    SAMPLE_COMM_DOSS_StopDev(dev);
exit:
    EI_MI_DOSS_Exit();
    return ret;
}

static void _stop_disp(VO_DEV dev, VO_LAYER layer)
{
    int i;
    MDP_CHN_S src_ippu[ISP_MAX_DEV_NUM], sink_doss[ISP_MAX_DEV_NUM];

    for (i = 0; i < ISP_MAX_DEV_NUM; i++) {
        src_ippu[i].enModId = EI_ID_IPPU;
        src_ippu[i].s32ChnId = 1;
        src_ippu[i].s32DevId = i;
        sink_doss[i].enModId = EI_ID_DOSS;
        sink_doss[i].s32ChnId = i;
        sink_doss[i].s32DevId = 0;
        EI_MI_MLINK_UnLink(&src_ippu[i], &sink_doss[i]);
        EI_MI_VO_DisableChn(layer, i);
    }
    EI_MI_VO_DisableVideoLayer(layer);
    SAMPLE_COMM_DOSS_StopDev(dev);
    EI_MI_DOSS_Exit();
}

static int _start_main_stream(sdr_sample_venc_para_t *stream_para,
    int num_stream,
    SAMPLE_VISS_CONFIG_S *viss_config,
    COMMON_FRAME_INFO_S *pstVissFrameInfo)
{
    int i;
    int ret = EI_SUCCESS;
    SAMPLE_VENC_CONFIG_S venc_config;
    VISS_DEV viss_dev;
    VISS_CHN viss_chn;

    venc_config.enInputFormat	 = pstVissFrameInfo->enPixelFormat;
    venc_config.u32width	 = pstVissFrameInfo->u32Width;
    venc_config.u32height	 = pstVissFrameInfo->u32Height;
    venc_config.u32bitrate	 = MAIN_STREAM_BITRATE;
    venc_config.u32srcframerate = SDR_VISS_FRAMERATE;
    venc_config.u32dstframerate = MAIN_STREAM_FRAMERATE;

    for (i = 0; i < num_stream; i++) {
        stream_para[i].VeChn = i;
        ret = SAMPLE_COMM_VENC_Start(stream_para[i].VeChn, PT_H265,
                SAMPLE_RC_CBR, &venc_config,
                COMPRESS_MODE_NONE, VENC_GOPMODE_ADV_P);
        if (EI_SUCCESS != ret) {
            EI_PRINT("%s %d VENC_Start %d fail\n", __func__, __LINE__, i);
            while (1);
            break;
        }

        if (i > 3) {
            viss_dev = viss_config->astVissInfo[1].stDevInfo.VissDev;
            viss_chn = viss_config->astVissInfo[1].stChnInfo.aVissChn[i - 4];
        } else {
            viss_dev = viss_config->astVissInfo[0].stDevInfo.VissDev;
            viss_chn = viss_config->astVissInfo[0].stChnInfo.aVissChn[i];
        }
        ret = SAMPLE_COMM_VISS_Link_VPU(viss_dev, viss_chn, stream_para[i].VeChn);
        if (EI_SUCCESS != ret) {
            EI_PRINT("viss link vpu failed %d-%d-%d \n",
                viss_dev, viss_chn, stream_para[i].VeChn);
            while (1);
            SAMPLE_COMM_VENC_Stop(stream_para[i].VeChn);
            break;
        }

        stream_para[i].muxer_type = MAIN_STREAM_MUXER_TYPE;
        stream_para[i].bitrate = venc_config.u32bitrate;
        ret = _start_get_stream(&stream_para[i]);
        if (EI_SUCCESS != ret) {
            EI_PRINT("viss link vpu failed %d-%d-%d \n",
                viss_dev, viss_chn, stream_para[i].VeChn);
            while (1);
            SAMPLE_COMM_VISS_UnLink_VPU(viss_dev, viss_chn, stream_para[i].VeChn);
            SAMPLE_COMM_VENC_Stop(stream_para[i].VeChn);
            break;
        }
    }
    return ret;
}

static int _stop_main_stream(sdr_sample_venc_para_t *stream_para,
    int num_stream,
    SAMPLE_VISS_CONFIG_S *viss_config)
{
    int i;
    VISS_DEV viss_dev;
    VISS_CHN viss_chn;
    for (i = 0; i < num_stream; i++) {
        if (i > 3) {
            viss_dev = viss_config->astVissInfo[1].stDevInfo.VissDev;
            viss_chn = viss_config->astVissInfo[1].stChnInfo.aVissChn[i - 4];
        } else {
            viss_dev = viss_config->astVissInfo[0].stDevInfo.VissDev;
            viss_chn = viss_config->astVissInfo[0].stChnInfo.aVissChn[i];
        }
        stream_para[i].bThreadStart = EI_FALSE;
        _stop_get_stream(&stream_para[i]);
        SAMPLE_COMM_VISS_UnLink_VPU(viss_dev, viss_chn, stream_para[i].VeChn);
        SAMPLE_COMM_VENC_Stop(stream_para[i].VeChn);
    }
    for (i = 0; i < num_stream; i++)
        save_stream_deinit(&stream_para[i]);
    return 0;
}


static int _start_sub_stream(sdr_sample_venc_para_t *stream_para,
    int num_stream,
    COMMON_FRAME_INFO_S *ippu_frame_info)
{
    int i;
    SAMPLE_VENC_CONFIG_S venc_config;
    int ret = EI_SUCCESS;

    venc_config.enInputFormat   = ippu_frame_info->enPixelFormat;
    venc_config.u32width	     = ippu_frame_info->u32Width;
    venc_config.u32height	     = ippu_frame_info->u32Height;
    venc_config.u32bitrate      = SUB_STREAM_BITRATE;
    venc_config.u32srcframerate = SUB_STREAM_FRAMERATE;
    venc_config.u32dstframerate = SUB_STREAM_FRAMERATE;

    for (i = 0; i < num_stream; i++) {
        ret = SAMPLE_COMM_VENC_Start(stream_para[i].VeChn, PT_H265,
                SAMPLE_RC_CBR, &venc_config,
                COMPRESS_MODE_NONE, VENC_GOPMODE_ADV_P);
        if (EI_SUCCESS != ret) {
            EI_PRINT("%s %d %d ret:%d \n", __func__, __LINE__, stream_para[i].VeChn, ret);
            break;
        }

        ret = SAMPLE_COMM_IPPU_Link_VPU(i, 1, stream_para[i].VeChn);
        if (EI_SUCCESS != ret) {
            EI_PRINT("viss link vpu failed %d-%d-%d \n",
                i, 1, stream_para[i].VeChn);
            SAMPLE_COMM_VENC_Stop(stream_para[i].VeChn);
            break;
        }

        stream_para[i].muxer_type = SUB_STREAM_MUXER_TYPE;
        stream_para[i].bitrate = venc_config.u32bitrate;
        ret = _start_get_stream(&stream_para[i]);
        if (EI_SUCCESS != ret) {
            EI_PRINT("_StartGetStream failed %d \n", stream_para[i].VeChn);
            SAMPLE_COMM_VISS_UnLink_VPU(i, 1, stream_para[i].VeChn);
            SAMPLE_COMM_VENC_Stop(stream_para[i].VeChn);
            break;
        }
    }
    return ret;
}

static int _stop_sub_stream(sdr_sample_venc_para_t *stream_para,
    int num_stream)
{
    int i;
    for (i = 0; i < num_stream; i++) {
        stream_para[i].bThreadStart = EI_FALSE;
        _stop_get_stream(&stream_para[i]);
        SAMPLE_COMM_IPPU_UnLink_VPU(i, 1, stream_para[i].VeChn);
        SAMPLE_COMM_VENC_Stop(stream_para[i].VeChn);
    }
    for (i = 0; i < num_stream; i++)
        save_stream_deinit(&stream_para[i]);
    return 0;
}

static int _get_screen_info(EI_U32 *scn_w, EI_U32 *scn_h, EI_U32 *scn_rate)
{
	return SAMPLE_COMM_DOSS_GetWH(VO_DEVICE, VO_OUTPUT_USER, scn_w, scn_h, scn_rate);
}

int viss_venc_8ch(void)
{
    int ret = EI_FAILURE;
    int i = 0;
    static int count = 0;
    SAMPLE_VISS_CONFIG_S viss_config = {0};
    VBUF_CONFIG_S vbuf_config = {0};
    int loop_time;
    sdr_sample_venc_para_t main_stream_para[VISS_MAX_PHY_CHN_NUM];
    sdr_sample_venc_para_t sub_stream_para[ISP_MAX_DEV_NUM];
    EI_BOOL chn_enabled[IPPU_PHY_CHN_MAX_NUM] = {0};
	
	scn_w = scn_h = scn_rate = 0;
    _get_screen_info(&scn_w, &scn_h, &scn_rate);
    if ((scn_w == 0) || (scn_h == 0) || (scn_rate == 0)) {
	    EI_PRINT("%s %d get screen info fail. ", __func__, __LINE__);
	    goto exit0;
    }

    ret = _bufpool_init(&vbuf_config);
    if (ret) {
        EI_PRINT("%s %d  \n", __func__, __LINE__);
        goto exit0;
    }

    ret = _start_viss(&viss_config);
    if (ret) {
        EI_PRINT("%s %d  \n", __func__, __LINE__);
        goto exit1;
    }

    ret = _start_ippu(chn_enabled, MAIN_STREAM_NUMBER, &viss_config,
            &vbuf_config.astFrameInfo[0].stCommFrameInfo,
            &vbuf_config.astFrameInfo[1].stCommFrameInfo);
    if (ret) {
        EI_PRINT("%s %d  \n", __func__, __LINE__);
        goto exit2;
    }

    ret = _start_disp(0, 0, &vbuf_config.astFrameInfo[1].stCommFrameInfo);
    if (ret) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
        goto exit3;
    }

    memset(main_stream_para, 0, sizeof(main_stream_para));
    memset(sub_stream_para, 0, sizeof(sub_stream_para));
    for (i = 0; i < 8; i++) {
        main_stream_para[i].VeChn = i;
        main_stream_para[i].bitrate = MAIN_STREAM_BITRATE;
        main_stream_para[i].file_idx = 0;
        main_stream_para[i].muxer_type = MAIN_STREAM_MUXER_TYPE;
        sub_stream_para[i].VeChn = i + 8;
        sub_stream_para[i].bitrate = SUB_STREAM_BITRATE;
        sub_stream_para[i].file_idx = 0;
        sub_stream_para[i].muxer_type = SUB_STREAM_MUXER_TYPE;
    }

    while (1) {
        _fallocate_stream_file(main_stream_para, MAIN_STREAM_NUMBER);
        _fallocate_stream_file(sub_stream_para, SUB_STREAM_NUMBER);
        ret = _start_main_stream(main_stream_para, MAIN_STREAM_NUMBER, &viss_config,
                &vbuf_config.astFrameInfo[0].stCommFrameInfo);
        if (EI_SUCCESS != ret) {
            EI_TRACE_LOG(EI_DBG_ERR, " viss link vpu failed %d-%d-%d \n", i, 1, i);
            goto exit4;
        }

        ret = _start_sub_stream(sub_stream_para, SUB_STREAM_NUMBER,
                &vbuf_config.astFrameInfo[1].stCommFrameInfo);
        if (EI_SUCCESS != ret) {
            EI_TRACE_LOG(EI_DBG_ERR, " viss link vpu failed %d-%d-%d \n", i, 1, i);
            goto exit5;
        }


        loop_time = SDR_SAMPLE_VENC_TIME;
        EI_PRINT("====count %d times\n", count++);
        while (loop_time--) {
            usleep(1000 * 1000);
            if (loop_time % 5 == 4)
                EI_PRINT("loop_time %d\n", loop_time);
            EI_TRACE_LOG(EI_DBG_ERR,"loop_time %d\n", loop_time);
        }

        _stop_main_stream(main_stream_para, MAIN_STREAM_NUMBER, &viss_config);
        _stop_sub_stream(sub_stream_para, SUB_STREAM_NUMBER);
    }
    _stop_disp(0, 0);
    _stop_ippu(MAIN_STREAM_NUMBER, chn_enabled);
    _stop_viss(&viss_config);
    ret = _bufpool_deinit(&vbuf_config);
    if (ret)
        EI_PRINT("====_bufpool_deinit failed\n");

    return 0;
exit5:
    _stop_main_stream(main_stream_para, 8, &viss_config);
exit4:
    _stop_disp(0, 0);
exit3:
    _stop_ippu(SUB_STREAM_NUMBER, chn_enabled);
exit2:
    EI_PRINT("%s %d  \n", __func__, __LINE__);
    _stop_viss(&viss_config);
exit1:
    EI_PRINT("%s %d  \n", __func__, __LINE__);
    _bufpool_deinit(&vbuf_config);
exit0:
    EI_PRINT("%s %d  \n", __func__, __LINE__);
    return 0;
}

int main(int argc, char **argv)
{
    int ret = EI_FAILURE;

    ret = viss_venc_8ch();

    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
