/*
 *------------------------------------------------------------------------------
 * @File      :    mdp_venc.c
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
#include "sdrive_sample_muxer.h"
#include "water_mark.h"

#define EI_TRACE_LOG(level, fmt, args...)\
do{ \
    EI_TRACE(EI_ID_LOG, level, fmt, ##args);\
} while (0)
	
typedef enum muxer_codec_type {
	/* video codecs */
	CODEC_TYPE_H264 = 1,
	CODEC_TYPE_HEVC = 14
} muxer_codec_type_e;

typedef struct __sample_venc_para_t {
    EI_BOOL bThreadStart;
    VC_CHN VeChn;
    int bitrate;
    int file_idx;
    int muxer_type;
	int codec_type;
    int init_flag;
    pthread_mutex_t mutex;

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

typedef struct video_encode_info {
    EI_U32 width;
    EI_U32 hight;
    EI_U32 bitrate;
    EI_U32 framerate;
    PAYLOAD_TYPE_E enType;
    EI_U32 video_duration;
} video_encode_info_t;

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
#define USE_AUDIO_MUXER
#define WRITE_BUFFER_SIZE 1*1024*1024
#if 1
#define SDR_VISS_SNS_DVP TP9930_DVP_1920_1080_30FPS_4CH_YUV
#define SDR_VISS_SNS_MIPI TP2815_MIPI_1920_1080_30FPS_4CH_YUV
#define SDR_VISS_FRAMERATE 30
#else
#define SDR_VISS_SNS_DVP TP9930_DVP_1920_1080_25FPS_4CH_YUV
#define SDR_VISS_SNS_MIPI TP2815_MIPI_1920_1080_25FPS_4CH_YUV
#define SDR_VISS_FRAMERATE 25
#endif
#define MAIN_STREAM_NUMBER 8
#define SUB_STREAM_NUMBER 8

#define SDR_SAMPLE_FILE_NUM 6

#define IPPU_RGN_TEST_NUM 17

#define PATH_ROOT "/data/"

#ifdef USE_AUDIO_MUXER
pthread_t gs_AencPid = 0;
EI_BOOL AencThreadStart = EI_FALSE;
#endif


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
        if (result == NULL) {
            EI_TRACE_LOG(EI_DBG_ERR, "result is NULL");
            break;
        }
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

void _fallocate_stream_file(sdr_sample_venc_para_t *sdr_venc_param, int number_stream, int venc_time)
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
            if (venc_para->codec_type == PT_H265) 
				szFilePostfix = ".h265";
			else
				szFilePostfix = ".h264";
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
        size = venc_para->bitrate * venc_time / 8 + 16 * 1024 *1024;
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
    int type, codec_type;
    static sdcard_info_t info;
	struct tm *p_tm; /* time variable */
	time_t now;
	now = time(NULL);
	p_tm = localtime(&now);

    if (sdr_venc_para == NULL)
        return -1;
    type = sdr_venc_para->muxer_type;
	codec_type = sdr_venc_para->codec_type;
    if (type == AV_MUXER_TYPE_RAW) {
		if (codec_type == PT_H265) 
			szFilePostfix = ".h265";
		else
			szFilePostfix = ".h264";
    } else if (type == AV_MUXER_TYPE_TS)
        szFilePostfix = ".ts";
    else if (type == AV_MUXER_TYPE_MP4)
        szFilePostfix = ".mp4";
    else {
        return -1;
    }

    storage_info_get(PATH_ROOT, &info);
    if (info.free < 500 * 1024)
        return -1;
/*
    snprintf(aszFileName, 128, PATH_ROOT"/stream_chn%02d-%02d%02d%02d%02d%02d%02d%s",
        sdr_venc_para->VeChn, p_tm->tm_year + 1900, p_tm->tm_mon + 1, p_tm->tm_mday,
		p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec, szFilePostfix);
*/
	snprintf(aszFileName, 128, PATH_ROOT"/stream_chn%02d-%03d%s",
            sdr_venc_para->VeChn,
            sdr_venc_para->file_idx, szFilePostfix);
			
#ifdef USE_MUXER
	if (codec_type == PT_H265) {
		codec_type = CODEC_TYPE_HEVC;
	} else {
		codec_type = CODEC_TYPE_H264;
	}
    if (type == AV_MUXER_TYPE_TS || type == AV_MUXER_TYPE_MP4) {
        sdr_venc_para->muxer_hdl = sample_muxer_init(
            type, aszFileName, codec_type, sdr_venc_para->VeChn);
        if (sdr_venc_para->muxer_hdl)
            ret = 0;
    }

#else
    type = AV_MUXER_TYPE_RAW;
    sdr_venc_para->muxer_type = type;
#endif


    if (type == AV_MUXER_TYPE_RAW) {
        ret = _save_raw_init(sdr_venc_para, aszFileName);
        if (ret)
            return ret;
    } else {
        sdr_venc_para->file_idx++;
    }
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
static int save_astream(sdr_sample_venc_para_t *stream_para, AUDIO_STREAM_S *pstStream)
{
    int ret = 0;
    int i;
    int save_flag = 0;
    sdr_sample_venc_para_t *venc_para;
    if (pstStream == NULL || stream_para == NULL)
        return EI_FAILURE;

#ifdef USE_MUXER
    while(save_flag != 0xff) {
        for(i = 0; i < 8; i++) {
            venc_para = &stream_para[i];
            if (venc_para->init_flag) {
                if (pthread_mutex_trylock(&venc_para->mutex) == 0) {
                    ret = sample_muxer_write_aenc_packet(venc_para->muxer_hdl, pstStream);
                    save_flag |= 0x01 << i;
                    pthread_mutex_unlock(&venc_para->mutex);
                }
            } else {
                save_flag |= 0x01 << i;
            }
        }
        usleep(10000);
    }

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
            (stStream.pstPack.enDataType == DATA_TYPE_I_FRAME || stStream.pstPack.enDataType == DATA_TYPE_INITPACKET)) {
            save_stream_init(sdr_venc_para);
	    }
        if (sdr_venc_para->init_flag) {
            pthread_mutex_lock(&sdr_venc_para->mutex);
            ret = save_stream(sdr_venc_para, &stStream);
            pthread_mutex_unlock(&sdr_venc_para->mutex);
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

EI_VOID *_get_aenc_stream_proc(sdr_sample_venc_para_t *stream_para)
{
    VC_CHN VencChn;
    AENC_CHN    AeChn = 0;
    VENC_STREAM_S *pvstStream = NULL;
    int ret;
    char thread_name[16];
    VC_CHN AencChn = 0;
    EI_S32 AencFd;
    AUDIO_STREAM_S stStream;
    fd_set read_fds;
    struct timeval TimeoutVal;


    thread_name[15] = 0;
    snprintf(thread_name, 16, "streamamux");
    prctl(PR_SET_NAME, thread_name);
    FD_ZERO(&read_fds);
    AencFd = EI_MI_AENC_GetFd(AencChn);
    FD_SET(AencFd, &read_fds);

    while (EI_TRUE == AencThreadStart) {
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(AencFd, &read_fds);

        ret = select(AencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (ret < 0) {
            EI_PRINT("%s: get aenc stream failed\n", __FUNCTION__);
            break;
        } else if (0 == ret) {
            EI_PRINT("%s: get aenc stream select time out\n", __FUNCTION__);
            break;
        }

        if (FD_ISSET(AencFd, &read_fds)) {
            memset(&stStream, 0, sizeof(AUDIO_STREAM_S));
            ret = EI_MI_AENC_GetStream(AencChn, &stStream, -1);
            if (EI_SUCCESS != ret) {
                EI_PRINT("%s: EI_MI_AENC_GetStream(%d), failed with %#x!\n", \
                    __FUNCTION__, AencChn, ret);
            }
        }

        if (stStream.pStream == NULL) {
            usleep(10 * 1000);
            continue;
        }
        if (stStream.pStream) {
            ret = save_astream(stream_para, &stStream);
            if (ret)
                EI_PRINT("%s,%d save astream err!\n", __func__, __LINE__);

            ret = EI_MI_AENC_ReleaseStream(AencChn, &stStream);
            if (ret != EI_SUCCESS) {
                EI_TRACE_LOG(EI_DBG_ERR, "release stream chn-%d error : %d\n",
                    VencChn, ret);
                break;
            }
        }
    }
	
	Stop_AI_AENC_AEC(NULL, 16000, 0, PT_AAC, 0, 0);
    EI_PRINT("%s,%d\n", __func__, __LINE__);
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

static int _bufpool_init(VBUF_CONFIG_S *vbuf_config, EI_U32 width, EI_U32 hight)
{
    memset(vbuf_config, 0, sizeof(VBUF_CONFIG_S));

    vbuf_config->astFrameInfo[0].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[0].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[0].stCommFrameInfo.u32Width = 1920;
    vbuf_config->astFrameInfo[0].stCommFrameInfo.u32Height = 1080;
    vbuf_config->astFrameInfo[0].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[0].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    vbuf_config->astFrameInfo[0].stCommFrameInfo.enCompressMode = COMPRESS_MODE_NONE;
    if (width == 1920 && hight == 1080) {
        vbuf_config->astCommPool[0].u32BufCnt = 40;
        vbuf_config->u32PoolCnt = 1;
    } else {
        vbuf_config->astCommPool[0].u32BufCnt = 40;
        vbuf_config->u32PoolCnt = 2;

        vbuf_config->astFrameInfo[1].enFrameType = MDP_FRAME_TYPE_COMMON;
        vbuf_config->astFrameInfo[1].stCommFrameInfo.u32Align = 32;
        vbuf_config->astFrameInfo[1].stCommFrameInfo.u32Width = width;
        vbuf_config->astFrameInfo[1].stCommFrameInfo.u32Height = hight;
        vbuf_config->astFrameInfo[1].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
        vbuf_config->astCommPool[1].u32BufCnt = 24;
        vbuf_config->astCommPool[1].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    }

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
    viss_info->stChnInfo.stChnAttr.u32Depth = 2;
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
    viss_info->stChnInfo.stChnAttr.u32Depth = 2;
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
    ippu_chn_attr[1].stFrameRate.s32SrcFrameRate = viss_config->astVissInfo[0].stIspInfo.stFrc.s32DstFrameRate;
    ippu_chn_attr[1].stFrameRate.s32DstFrameRate = viss_config->astVissInfo[0].stIspInfo.stFrc.s32DstFrameRate;
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

#ifdef USE_AUDIO_MUXER
/***audio encode****/
static EI_S64 _get_time()
{
    struct timeval time;

    gettimeofday(&time, NULL);
    return time.tv_sec*1e6 + time.tv_usec;
}
/******************************************************************************
* function : Start Ai
******************************************************************************/
static EI_S32 AUDIO_StartAi(AUDIO_DEV AiDevId, EI_S32 s32AiChnCnt,
    AIO_ATTR_S *pstAioAttr, AUDIO_SAMPLE_RATE_E enOutSampleRate, EI_BOOL bResampleEn,
    EI_VOID *pstAiVqeAttr)
{
    EI_S32 i;
    EI_S32 s32Ret;
    /* Left 5dB Right 0dB, but real value is 5 * 1.5 = 7.5dB.*/
    EI_S32 as32VolumeDb[2] = {5, 0};
    EI_S32 s32Length = (sizeof(as32VolumeDb) / sizeof(as32VolumeDb[0]));

    s32Ret = EI_MI_AUDIOIN_Init(AiDevId);
    if (EI_SUCCESS != s32Ret) {
        printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return s32Ret;
    }

    s32Ret = EI_MI_AI_SetPubAttr(AiDevId, pstAioAttr);
    if (s32Ret) {
        printf("%s: EI_MI_AI_SetPubAttr(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
        return s32Ret;
    }

    s32Ret = EI_MI_AI_Enable(AiDevId);
    if (s32Ret) {
        printf("%s: EI_MI_AI_Enable(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
        return s32Ret;
    }

    s32Ret = EI_MI_AI_SetVolume(AiDevId, s32Length, as32VolumeDb);
    if (s32Ret) {
        printf("%s: EI_MI_AI_SetVolume(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
        return s32Ret;
    }

    for (i = 0; i < s32AiChnCnt >> pstAioAttr->enSoundmode; i++) {
        s32Ret = EI_MI_AI_EnableChn(AiDevId, i);
        if (s32Ret) {
            printf("%s: EI_MI_AI_EnableChn(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
            return s32Ret;
        }

        if (EI_TRUE == bResampleEn) {
            s32Ret = EI_MI_AI_EnableReSmp(AiDevId, i, enOutSampleRate);
            if (s32Ret) {
                printf("%s: EI_MI_AI_EnableReSmp(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
                return s32Ret;
            }
        }

        if (NULL != pstAiVqeAttr) {
            s32Ret = EI_MI_AI_SetVqeAttr(AiDevId, i, 0, 0, pstAiVqeAttr);
            if (s32Ret) {
                printf("%s: SetAiVqe(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
                return s32Ret;
            }

            s32Ret = EI_MI_AI_EnableVqe(AiDevId, i);
            if (s32Ret) {
                printf("%s: EI_MI_AI_EnableVqe(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
                return s32Ret;
            }
        }
    }

    return EI_SUCCESS;
}

/******************************************************************************
* function : Stop Ai
******************************************************************************/
static EI_S32 AUDIO_StopAi(AUDIO_DEV AiDevId, EI_S32 s32AiChnCnt,
    EI_BOOL bResampleEn, EI_BOOL bVqeEn)
{
    EI_S32 i;
    EI_S32 s32Ret;

    for (i = 0; i < s32AiChnCnt; i++) {
        if (EI_TRUE == bResampleEn) {
            s32Ret = EI_MI_AI_DisableReSmp(AiDevId, i);
            if (EI_SUCCESS != s32Ret) {
                printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
                return s32Ret;
            }
        }

        if (EI_TRUE == bVqeEn) {
            s32Ret = EI_MI_AI_DisableVqe(AiDevId, i);
            if (EI_SUCCESS != s32Ret) {
                printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
                return s32Ret;
            }
        }

        s32Ret = EI_MI_AI_DisableChn(AiDevId, i);
        if (EI_SUCCESS != s32Ret) {
            printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
            return s32Ret;
        }
    }

    s32Ret = EI_MI_AI_Disable(AiDevId);
    if (EI_SUCCESS != s32Ret) {
        printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return s32Ret;
    }

    s32Ret = EI_MI_AUDIOIN_Exit(AiDevId);
    if (EI_SUCCESS != s32Ret) {
        printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return s32Ret;
    }
    printf("EI_MI_AUDIOIN_Exit %d ok\n", __LINE__);

    return EI_SUCCESS;
}

#define AUDIO_ADPCM_TYPE ADPCM_TYPE_DVI4
#define G726_BPS MEDIA_G726_40K
static EI_S32 AUDIO_StartAenc(EI_S32 s32AencChnCnt, AIO_ATTR_S *pstAioAttr, PAYLOAD_TYPE_E enType)
{
    AENC_CHN AeChn;
    EI_S32 s32Ret, i;
    AENC_CHN_ATTR_S stAencAttr;
    AENC_ATTR_ADPCM_S stAdpcmAenc;
    AENC_ATTR_G711_S stAencG711;
    AENC_ATTR_G726_S stAencG726;
    AENC_ATTR_LPCM_S stAencLpcm;
    AENC_ATTR_AAC_S  stAencAac;

    stAencAttr.enType = enType;
    stAencAttr.u32BufSize = 30;
    stAencAttr.u32PtNumPerFrm = pstAioAttr->u32PtNumPerFrm;
    stAencAttr.enSoundmode = pstAioAttr->enSoundmode;
    stAencAttr.enSamplerate = pstAioAttr->enSamplerate;

    if (PT_ADPCMA == stAencAttr.enType) {
        stAencAttr.pValue       = &stAdpcmAenc;
        stAdpcmAenc.enADPCMType = AUDIO_ADPCM_TYPE;
    } else if (PT_G711A == stAencAttr.enType || PT_G711U == stAencAttr.enType) {
        stAencAttr.pValue       = &stAencG711;
    } else if (PT_G726 == stAencAttr.enType) {
        stAencAttr.pValue       = &stAencG726;
        stAencG726.enG726bps    = G726_BPS;
    } else if (PT_G729 == stAencAttr.enType) {
    } else if (PT_LPCM == stAencAttr.enType) {
        stAencAttr.pValue = &stAencLpcm;
    } else if (PT_AAC == stAencAttr.enType) {

        stAencAttr.pValue = &stAencAac;
        stAencAac.enBitRate = 128000;
        stAencAac.enBitWidth = AUDIO_BIT_WIDTH_16;
        stAencAac.enSmpRate = pstAioAttr->enSamplerate;
        stAencAac.enSoundMode = pstAioAttr->enSoundmode;
        stAencAac.s16BandWidth = 0;
    } else {
        printf("%s: invalid aenc payload type:%d\n", __FUNCTION__, stAencAttr.enType);
        return EI_FAILURE;
    }

    for (i = 0; i < s32AencChnCnt; i++) {
        AeChn = i;

        s32Ret = EI_MI_AENC_CreateChn(AeChn, &stAencAttr);
        if (EI_SUCCESS != s32Ret) {
            printf("%s: EI_MI_AENC_CreateChn(%d) failed with %#x!\n", __FUNCTION__,
                AeChn, s32Ret);
            return s32Ret;
        }
    }

    return EI_SUCCESS;
}
static EI_S32 AUDIO_StopAenc(EI_S32 s32AencChnCnt)
{
    EI_S32 i;
    EI_S32 s32Ret;

    for (i = 0; i < s32AencChnCnt; i++) {
        s32Ret = EI_MI_AENC_DestroyChn(i);
        if (EI_SUCCESS != s32Ret) {
            printf("%s: EI_MI_AENC_DestroyChn(%d) failed with %#x!\n", __FUNCTION__,
                i, s32Ret);
            return s32Ret;
        }

    }

    return EI_SUCCESS;
}
static EI_S32 AUDIO_AencLinkAi(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    MDP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId = EI_ID_AUDIOIN;
    stSrcChn.s32DevId = AiDev;
    stSrcChn.s32ChnId = AiChn;
    stDestChn.enModId = EI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AeChn;

    return EI_MI_MLINK_Link(&stSrcChn, &stDestChn);
}

static EI_S32 AUDIO_AencUnbindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    MDP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId = EI_ID_AUDIOIN;
    stSrcChn.s32DevId = AiDev;
    stSrcChn.s32ChnId = AiChn;
    stDestChn.enModId = EI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AeChn;

    return EI_MI_MLINK_UnLink(&stSrcChn, &stDestChn);
}

/*  record pcm data with aec processed, and encode it into g711a format. */
/*  we use linked mode here, so you don't need to deal with comunication */
/*  between ai and aenc. */
/*  you can use Sample_AUDIO_PlayWave to playa wavfile in the background, */
/*  then use this func to record a wavfile. The recorded */
/*  voice will cancel the the voice from playback */
EI_S32 Sample_AI_AENC_AEC(EI_CHAR *encfile, EI_U32 rate, EI_U32 duration, PAYLOAD_TYPE_E enType, AI_CHN AiChn, AENC_CHN AeChn)
{
    EI_S32 s32Ret;
    EI_S32      s32AiChnCnt;
    EI_S32      s32AencChnCnt;
    /* for aec, we only support one chn mono mode, 16 bit, 8k or 16k */
    AUDIO_DEV   AiDev = 0;
    FILE        *fp = EI_NULL;
    AIO_ATTR_S stAioAttr;
    AI_VQE_CONFIG_S stAiVqeAttr;
    EI_S64 start_time;
    AUDIO_STREAM_S stStream;
    fd_set read_fds;
    EI_S32 AencFd;
    struct timeval TimeoutVal;

#if 0
    fp = fopen(encfile, "wb+");
    if (fp == EI_NULL) {
        printf("open file %s err!!\n", encfile);
        return -1;
    }
#endif

    /*  1. start ai with vqe atrrs */
    stAioAttr.enSamplerate   = rate;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = 1024;
    stAioAttr.u32ChnCnt      = 1;
    s32AiChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;

    /*  if you want to enable vqe funcs, such as aec,anr, just past the */
    /*  AiVqeAttr to ai, and enable the vqe. */
    memset(&stAiVqeAttr, 0, sizeof(AI_VQE_CONFIG_S));
    stAiVqeAttr.s32WorkSampleRate    = rate;
    stAiVqeAttr.s32FrameSample       = stAioAttr.u32PtNumPerFrm; /* SAMPLE_AUDIO_PTNUMPERFRM; */
    stAiVqeAttr.enWorkstate          = VQE_WORKSTATE_COMMON;
    stAiVqeAttr.u32OpenMask = AI_VQE_MASK_AEC;/*  if you want to use anr, you can | AI_VQE_MASK_ANR here */
    s32Ret = AUDIO_StartAi(AiDev, stAioAttr.u32ChnCnt, &stAioAttr,
        AUDIO_SAMPLE_RATE_BUTT, EI_FALSE, &stAiVqeAttr);
    if (s32Ret) {
        printf("ai start err %d!!\n", s32Ret);
        return -1;
    }

    /*  2. start aenc with encode type ** as an example */
    s32AencChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    s32Ret = AUDIO_StartAenc(s32AencChnCnt, &stAioAttr, enType);
    if (s32Ret != EI_SUCCESS) {
        printf("AENC start err %d!!\n", s32Ret);
        goto EXIT1;
    }

    /*  3. link ai to aenc */
    s32Ret = AUDIO_AencLinkAi(AiDev, AiChn, AeChn);
    if (s32Ret != EI_SUCCESS) {
        printf("Aenc link to ai err %d!!\n", s32Ret);
        goto EXIT2;
    }

#if 0
    /*  4. get enc stream and write to file */
    start_time = _get_time();
    FD_ZERO(&read_fds);
    AencFd = EI_MI_AENC_GetFd(AeChn);
    FD_SET(AencFd, &read_fds);
    while (1) {
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(AencFd, &read_fds);

        s32Ret = select(AencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        /* printf("####func:%s,line:%d,ret:%d\n",__FUNCTION__,__LINE__,s32Ret); */
        if (s32Ret < 0) {
            break;
        } else if (0 == s32Ret) {
            printf("%s: get aenc stream select time out\n", __FUNCTION__);
            break;
        }

        if (FD_ISSET(AencFd, &read_fds)) {
            s32Ret = EI_MI_AENC_GetStream(AeChn, &stStream, -1);
            if (EI_SUCCESS != s32Ret) {
                printf("%s: EI_MI_AENC_GetStream(%d), failed with %#x!\n", \
                    __FUNCTION__, AeChn, s32Ret);
                goto EXIT3;
            }
            EI_PRINT("get:%llu,addr:%p\n", stStream.u64TimeStamp, stStream.pStream);

            /*if (fp) {
                s32Ret = fwrite(stStream.pStream, 1, stStream.u32Len, fp);
            }*/

            /* fflush(pstAencCtl->pfd); */
            /* EI_TRACE_ADEC(EI_DBG_ERR, "##########fwrite:%d,\n",stStream.u32Len); */

            s32Ret = EI_MI_AENC_ReleaseStream(AeChn, &stStream);
            if (EI_SUCCESS != s32Ret) {
                printf("%s: EI_MI_AENC_ReleaseStream(%d), failed with %#x!\n", \
                    __FUNCTION__, AeChn, s32Ret);
                goto EXIT3;
            }
        }
    }

    if (fp)
        fclose(fp);
#endif

    return 0;

//EXIT3:
    s32Ret = AUDIO_AencUnbindAi(AiDev, AiChn, AeChn);
    if (s32Ret != EI_SUCCESS) {
        printf("AUDIO_AencUnbindAi err %d\n", s32Ret);
    }
EXIT2:
    s32Ret = AUDIO_StopAenc(s32AencChnCnt);
    if (s32Ret != EI_SUCCESS) {
        printf("stop AENC err %d\n", s32Ret);
    }

EXIT1:
    s32Ret = AUDIO_StopAi(AiDev, s32AiChnCnt, EI_FALSE, EI_TRUE);
    if (s32Ret) {
        printf("stop ai err %d\n", s32Ret);
    }
    printf("Sample_AI_AENC_AEC end!!\n");

    return 0;
}
EI_S32 Stop_AI_AENC_AEC(EI_CHAR *encfile, EI_U32 rate, EI_U32 duration, PAYLOAD_TYPE_E enType, AI_CHN AiChn, AENC_CHN AeChn)
{
    EI_S32 s32Ret;
    EI_S32      s32AiChnCnt;
    EI_S32      s32AencChnCnt;
    /* for aec, we only support one chn mono mode, 16 bit, 8k or 16k */
    AUDIO_DEV   AiDev = 0;
    AIO_ATTR_S stAioAttr;

    /*  1. start ai with vqe atrrs */
    stAioAttr.enSamplerate   = rate;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = 1024;
    stAioAttr.u32ChnCnt      = 1;
    s32AiChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;

    /*  2. start aenc with encode type ** as an example */
    s32AencChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;

    s32Ret = AUDIO_AencUnbindAi(AiDev, AiChn, AeChn);
    if (s32Ret != EI_SUCCESS) {
        printf("AUDIO_AencUnbindAi err %d\n", s32Ret);
    }

    s32Ret = AUDIO_StopAenc(s32AencChnCnt);
    if (s32Ret != EI_SUCCESS) {
        printf("stop AENC err %d\n", s32Ret);
    }

    s32Ret = AUDIO_StopAi(AiDev, s32AiChnCnt, EI_FALSE, EI_TRUE);
    if (s32Ret) {
        printf("stop ai err %d\n", s32Ret);
    }
    printf("Stop_AI_AENC_AEC end!!\n");

    return 0;
}

#endif

static int _start_main_stream(sdr_sample_venc_para_t *stream_para,
    int num_stream, PAYLOAD_TYPE_E enType,
    SAMPLE_VISS_CONFIG_S *viss_config,
    SAMPLE_VENC_CONFIG_S *venc_config)
{
    int i;
    int ret = EI_SUCCESS;
    VISS_DEV viss_dev;
    VISS_CHN viss_chn;

    for (i = 0; i < num_stream; i++) {
        ret = pthread_mutex_init(&stream_para[i].mutex, NULL);
        stream_para[i].VeChn = i;
        ret = SAMPLE_COMM_VENC_Start(stream_para[i].VeChn, enType,
                SAMPLE_RC_CBR, venc_config,
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
#ifdef USE_AUDIO_MUXER
        Sample_AI_AENC_AEC(NULL, 16000, 0, PT_AAC, 0, 0);
        AencThreadStart = EI_TRUE;
        ret = pthread_create(&gs_AencPid, 0, _get_aenc_stream_proc,
            (EI_VOID *)stream_para);
        if (ret)
            EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));
#endif
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

#ifdef USE_AUDIO_MUXER
    AencThreadStart = EI_FALSE;
    if (gs_AencPid) {
        pthread_join(gs_AencPid, 0);
        gs_AencPid = 0;
    }
#endif

    for (i = 0; i < num_stream; i++) {
        save_stream_deinit(&stream_para[i]);
        pthread_mutex_destroy(&stream_para[i].mutex);
    }
    return 0;
}


static int _start_sub_stream(sdr_sample_venc_para_t *stream_para,
    int num_stream, PAYLOAD_TYPE_E enType,
    SAMPLE_VENC_CONFIG_S *venc_config)
{
    int i;
    int ret = EI_SUCCESS;

    for (i = 0; i < num_stream; i++) {
        ret = SAMPLE_COMM_VENC_Start(stream_para[i].VeChn, enType,
                SAMPLE_RC_CBR, venc_config,
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

static int _watermark_init(MDP_CHN_S *stRgnChn, EI_U32 handleId)
{
    EI_S32 s32Ret = EI_FAILURE;
    RGN_ATTR_S stRegion[IPPU_RGN_TEST_NUM] = {{0}};
    RGN_HANDLE Handle[IPPU_RGN_TEST_NUM];
    RGN_CHN_ATTR_S stRgnChnAttr[IPPU_RGN_TEST_NUM] = {{0}};
    EI_S32 s32BytesPerPix = 2;
    BITMAP_S stBitmap;

    stRegion[0].enType = OVERLAYEX_RGN;
    stRegion[0].unAttr.stOverlayEx.enPixelFmt = PIX_FMT_GREY_Y8;
    stRegion[0].unAttr.stOverlayEx.stSize.u32Width = ISP_WATERMARK_W0;
    stRegion[0].unAttr.stOverlayEx.stSize.u32Height = ISP_WATERMARK_H0;
    stRegion[0].unAttr.stOverlayEx.u32CanvasNum = 2;

    Handle[0] = handleId;
    s32Ret = EI_MI_RGN_Create(Handle[0], &stRegion[0]);
    if (s32Ret) {
        goto exit1;
    }

    stRgnChnAttr[0].bShow = EI_TRUE;
    stRgnChnAttr[0].enType = OVERLAYEX_RGN;
    stRgnChnAttr[0].unChnAttr.stOverlayExChn.stPoint.s32X = 30;
    stRgnChnAttr[0].unChnAttr.stOverlayExChn.stPoint.s32Y = 30;
    stRgnChnAttr[0].unChnAttr.stOverlayExChn.u32FgAlpha = 255;
    stRgnChnAttr[0].unChnAttr.stOverlayExChn.stInvertColorAttr.bInvertColorEn = EI_TRUE;

    s32Ret = EI_MI_RGN_AddToChn(Handle[0], stRgnChn, &stRgnChnAttr[0]);
    if (s32Ret) {
        goto exit1;
    }

    /* osd0 */
    stBitmap.enPixelFormat = stRegion[0].unAttr.stOverlayEx.enPixelFmt;
    stBitmap.u32Width = stRegion[0].unAttr.stOverlayEx.stSize.u32Width;
    stBitmap.u32Height = stRegion[0].unAttr.stOverlayEx.stSize.u32Height;
    switch (stBitmap.enPixelFormat) {
    case PIX_FMT_GREY_Y8: s32BytesPerPix = 1; break;
    case PIX_FMT_ARGB_4444: s32BytesPerPix = 2; break;
    case PIX_FMT_ARGB_1555: s32BytesPerPix = 2; break;
    case PIX_FMT_ARGB_8888: s32BytesPerPix = 4; break;
    default: break;
    }
    stBitmap.pData = malloc(stBitmap.u32Width * stBitmap.u32Height * s32BytesPerPix);
    memcpy(stBitmap.pData, water_mark_260_41, stBitmap.u32Width * stBitmap.u32Height * s32BytesPerPix);
    s32Ret = EI_MI_RGN_SetBitMap(Handle[0], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
exit2:
    free(stBitmap.pData);
exit1:
    return s32Ret;
}

static int _watermark_exit(MDP_CHN_S *stRgnChn, EI_U32 handleId)
{
    RGN_HANDLE Handle;
    Handle = handleId;

    EI_MI_RGN_DelFromChn(Handle, stRgnChn);
    EI_MI_RGN_Destroy(Handle);

    return EI_SUCCESS;
}

int mdp_venc_8ch(video_encode_info_t *vencInfo)
{
    int ret = EI_FAILURE;
    int i = 0;
    static int count = 0;
    SAMPLE_VISS_CONFIG_S viss_config = {0};
    VBUF_CONFIG_S vbuf_config = {0};
    SAMPLE_VENC_CONFIG_S venc_config = {0};
    MDP_CHN_S stRgnChn = {0};
    int loop_time;
    sdr_sample_venc_para_t main_stream_para[VISS_MAX_PHY_CHN_NUM] = {{0}};
    sdr_sample_venc_para_t sub_stream_para[ISP_MAX_DEV_NUM] = {{0}};
    EI_BOOL chn_enabled[IPPU_PHY_CHN_MAX_NUM] = {0};
    EI_BOOL isUseVissDataEnc = EI_FALSE;
    int muxer_type;
	int codec_type;
    if (vencInfo->width == 1920 && vencInfo->hight == 1080) {
        isUseVissDataEnc = EI_TRUE;
    }

    ret = _bufpool_init(&vbuf_config, vencInfo->width, vencInfo->hight);
    if (ret) {
        EI_PRINT("%s %d _bufpool_init err! \n", __func__, __LINE__);
        goto exit0;
    }

    EI_MI_MBASE_SetUsrLogLevel(EI_ID_VISS, 0);
    EI_MI_MBASE_SetKerLogLevel(EI_ID_VISS, 0);
    EI_MI_MBASE_SetUsrLogLevel(EI_ID_VPU, 0);
    EI_MI_MBASE_SetKerLogLevel(EI_ID_VPU, 0);

    ret = _start_viss(&viss_config);
    if (ret) {
        EI_PRINT("%s %d _start_viss err! \n", __func__, __LINE__);
        goto exit1;
    }

    if (isUseVissDataEnc != EI_TRUE) {
        viss_config.astVissInfo[0].stIspInfo.enRunningMode = ISP_MODE_RUNNING_OFFLINE;
        viss_config.astVissInfo[0].stIspInfo.stFrc.s32SrcFrameRate = SDR_VISS_FRAMERATE; /* only support offline */
        viss_config.astVissInfo[0].stIspInfo.stFrc.s32DstFrameRate = vencInfo->framerate;
        viss_config.astVissInfo[1].stIspInfo.enRunningMode = ISP_MODE_RUNNING_OFFLINE;
        viss_config.astVissInfo[1].stIspInfo.stFrc.s32SrcFrameRate = SDR_VISS_FRAMERATE; /* only support offline */
        viss_config.astVissInfo[1].stIspInfo.stFrc.s32DstFrameRate = vencInfo->framerate;

        ret = _start_ippu(chn_enabled, MAIN_STREAM_NUMBER, &viss_config,
                &vbuf_config.astFrameInfo[0].stCommFrameInfo,
                &vbuf_config.astFrameInfo[1].stCommFrameInfo);
        if (ret) {
            EI_PRINT("%s %d _start_ippu err!\n", __func__, __LINE__);
            goto exit2;
        }
    }

    for (i = 0; i < 8; i++) {
        /* region */
        stRgnChn.enModId = EI_ID_VISS;
        stRgnChn.s32ChnId = i;
        stRgnChn.s32DevId = i > 3 ? 1 : 0;
        ret = _watermark_init(&stRgnChn, i);
        if (ret) {
            EI_PRINT("%s %d _set_watermark:%d err\n", __func__, __LINE__, i);
            goto exit3;
        }
    }

    if (isUseVissDataEnc) {
        venc_config.enInputFormat    = vbuf_config.astFrameInfo[0].stCommFrameInfo.enPixelFormat;
        venc_config.u32width     = vbuf_config.astFrameInfo[0].stCommFrameInfo.u32Width;
        venc_config.u32height    = vbuf_config.astFrameInfo[0].stCommFrameInfo.u32Height;
        venc_config.u32bitrate   = vencInfo->bitrate;
        venc_config.u32srcframerate = SDR_VISS_FRAMERATE;
        venc_config.u32dstframerate = vencInfo->framerate;
        venc_config.u32IdrPeriod = 25;
    } else {
        venc_config.enInputFormat   = vbuf_config.astFrameInfo[1].stCommFrameInfo.enPixelFormat;
        venc_config.u32width	    = vbuf_config.astFrameInfo[1].stCommFrameInfo.u32Width;
        venc_config.u32height	    = vbuf_config.astFrameInfo[1].stCommFrameInfo.u32Height;
        venc_config.u32bitrate      = vencInfo->bitrate;
        venc_config.u32srcframerate = vencInfo->framerate;
        venc_config.u32dstframerate = vencInfo->framerate;
    }

    memset(main_stream_para, 0, sizeof(main_stream_para));
    memset(sub_stream_para, 0, sizeof(sub_stream_para));
    if (vencInfo->enType == PT_H265) {
		codec_type = PT_H265;
    } else {
		codec_type = PT_H264;
    }
#ifdef USE_MUXER
	muxer_type = AV_MUXER_TYPE_MP4;
#else
	muxer_type = AV_MUXER_TYPE_RAW;
#endif
	
    for (i = 0; i < 8; i++) {
        if (isUseVissDataEnc) {
            main_stream_para[i].VeChn = i;
            main_stream_para[i].bitrate = vencInfo->bitrate;
            main_stream_para[i].file_idx = 0;
            main_stream_para[i].muxer_type = muxer_type;
			main_stream_para[i].codec_type = codec_type;
        } else {
            sub_stream_para[i].VeChn = i;
            sub_stream_para[i].bitrate = vencInfo->bitrate;
            sub_stream_para[i].file_idx = 0;
            sub_stream_para[i].muxer_type = muxer_type;
			sub_stream_para[i].codec_type = codec_type;
        }
    }

    while (1) {
        if (isUseVissDataEnc) {
            _fallocate_stream_file(main_stream_para, MAIN_STREAM_NUMBER, vencInfo->video_duration);
            ret = _start_main_stream(main_stream_para, MAIN_STREAM_NUMBER, vencInfo->enType, &viss_config,
                    &venc_config);
            if (EI_SUCCESS != ret) {
                EI_TRACE_LOG(EI_DBG_ERR, " viss link vpu failed %d-%d-%d \n", i, 1, i);
                goto exit4;
            }
        } else {
            _fallocate_stream_file(sub_stream_para, SUB_STREAM_NUMBER, vencInfo->video_duration);
            ret = _start_sub_stream(sub_stream_para, SUB_STREAM_NUMBER, vencInfo->enType,
                    &venc_config);
            if (EI_SUCCESS != ret) {
                EI_TRACE_LOG(EI_DBG_ERR, " viss link vpu failed %d-%d-%d \n", i, 1, i);
                goto exit4;
            }
        }

        loop_time = vencInfo->video_duration;
        EI_PRINT("====count %d times\n", count++);
        while (loop_time--) {
            usleep(1000 * 1000);
            if (loop_time % 5 == 4)
                EI_PRINT("loop_time %d\n", loop_time);
            EI_TRACE_LOG(EI_DBG_ERR,"loop_time %d\n", loop_time);
        }

        if (isUseVissDataEnc) {
            _stop_main_stream(main_stream_para, MAIN_STREAM_NUMBER, &viss_config);
        } else {
            _stop_sub_stream(sub_stream_para, SUB_STREAM_NUMBER);
        }

        break;
    }

exit4:
    for (i = 0; i < 8; i++) {
        stRgnChn.enModId = EI_ID_VISS;
        stRgnChn.s32ChnId = i;
        stRgnChn.s32DevId = i > 3 ? 1 : 0;
        _watermark_exit(&stRgnChn, i);
    }
exit3:
    if (isUseVissDataEnc != EI_TRUE) {
        _stop_ippu(SUB_STREAM_NUMBER, chn_enabled);
    }
exit2:
    _stop_viss(&viss_config);
exit1:
    _bufpool_deinit(&vbuf_config);
exit0:
    EI_PRINT("8ch video enc end!\n");
    EI_PRINT("The generate files are in /data!\n");
    return 0;
}

static EI_VOID aenc_help(EI_VOID)
{
    printf("Version:1.0.0  \n");
    printf("such as: mdp_venc -w 1920 -h 1080 -r 15 -t 30\n");
    printf("such as: mdp_venc -w 1280 -h 720 -r 15 -t 30 -e 0 -b 1000000\n");
    printf("arguments:\n");
    printf("-w    select video width, support:0~1920\n");
    printf("-h    select video hight, support:0~1080\n");
    printf("-b    select video bitrate\n");
    printf("-r    select video record frame rate, default:20, support:1~30\n");
    printf("-e    select video type, default:0, support:0~1, 0:h265, 1: h264\n");
    printf("-t    select video record time, default:20, support:1~\n");
}

int main(int argc, char **argv)
{
    EI_S32 s32Ret = EI_FAILURE;
    video_encode_info_t vencInfo = {0};
    EI_U32 width = 1920;
    EI_U32 hight = 1080;
    EI_U32 bitrate = 0;
    EI_U32 framerate = 20;
    EI_U32 type = 0; /* 0:h265, h264 */
    EI_U32 video_duration = 20;
    int c;

    if (argc < 3) {
        aenc_help();
        goto EXIT;
    }

    while ((c = getopt(argc, argv, "w:h:b:r:e:t:")) != -1) {
        switch (c) {
        case 'w':
            width = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'h':
            hight = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'b':
            bitrate = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'r':
            framerate = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'e':
            type = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 't':
            video_duration = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        default:
            aenc_help();
            goto EXIT;
        }
    }

    vencInfo.width = width;
    vencInfo.hight = hight;
    vencInfo.framerate = framerate;
    vencInfo.video_duration = video_duration;
    if (bitrate != 0) {
        vencInfo.bitrate = bitrate;
    } else {
        vencInfo.bitrate = width*hight*framerate/10;
    }
    if (type == 0) {
        vencInfo.enType = PT_H265;
    } else if (type == 1) {
        vencInfo.enType = PT_H264;
    }

    EI_PRINT("width:%d,hight:%d,bitrate:%d,framerate:%d,enType:%s,video_time:%d\n",
        width, hight, vencInfo.bitrate, framerate, type?"H264":"H265", video_duration);

    s32Ret = mdp_venc_8ch(&vencInfo);

EXIT:
    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
