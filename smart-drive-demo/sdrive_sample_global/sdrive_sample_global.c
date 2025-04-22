/*
 *------------------------------------------------------------------------------
 * @File      :    sdrive_sample_global.c
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
#include <mntent.h>

#include "ei_posix.h"
#include "sample_comm.h"
#include "sdrive_sample_muxer.h"
#include "osal.h"

#include "nn_adas_api.h"
#include "nn_bsd_api.h"
#include "nn_dms_api.h"
#include "nn_facerecg_api.h"


#include "mdp_dms_config.h"
#include "water_mark.h"
#include <sqlite3.h>


#define EI_TRACE_LOG(level, fmt, args...)\
do{ \
    EI_TRACE(EI_ID_LOG, level, fmt, ##args);\
} while(0)
	
typedef enum _WARNING_MSG_TYPE_ {
	LB_WARNING_BEGIN = 0xF000,
	LB_WARNING_DMS_NO_FACE,
	LB_WARNING_DMS_CALL,
	LB_WARNING_DMS_DRINK,
	LB_WARNING_DMS_YAWN,
	LB_WARNING_DMS_SMOKE,
	LB_WARNING_DMS_ATTATION,
	LB_WARNING_DMS_REST,
	LB_WARNING_DMS_CAMERA_COVER,
	LB_WARNING_DMS_INFRARED_BLOCK,
	LB_WARNING_DMS_CALIBRATE_START,
	LB_WARNING_DMS_CALIBRATE_SUCCESS,
	LB_WARNING_DMS_CALIBRATE_FAIL,
	LB_WARNING_USERDEF,
	LB_WARNING_END,
} WARNING_MSG_TYPE;

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

typedef enum {
    DISPLAY_MODE_1x1 = 1,
    DISPLAY_MODE_2x2,
    DISPLAY_MODE_3x3,
    DISPLAY_MODE_2x4,
    DISPLAY_MODE_MAX
} DISPLAY_MODE_E;

typedef enum {
    NN_DMS = 0,
    NN_BSD0,
    NN_BSD1,
    NN_ADAS,
    NN_MAX
} AXNU_E;

typedef struct _axframe_info_cache_s{
    VIDEO_FRAME_INFO_S axFrame;
    EI_U32 axframe_id;
} axframe_info_cache_s;

typedef struct _camera_info_s{
    SIZE_S stSize;
    EI_U32 u32FrameRate;
} camera_info_s;

typedef struct _display_info_s {
    EI_BOOL bThreadStart;
    DISPLAY_MODE_E mode;
    SIZE_S stDispSize;
    unsigned int frameRate;
    VO_DEV dev;
    VO_LAYER layer; 
    VO_CHN_ATTR_S *vo_chn_attr;
    DISPLAY_MODE_E current_display_mode;
    EI_U32 display_switch_flag;
    pthread_t gs_DispPid;
    pthread_mutex_t g_stLock;
} display_info_s;

typedef struct _snap_info_s {
    EI_BOOL bThreadStart;
    camera_info_s *camera_info;
        
    RECT_S stSnapRect;
    VC_CHN VeChn[2];
    VISS_DEV dev; 
    VISS_CHN chn;
    EI_BOOL snap_flag;
    EI_U32 width;
    EI_U32 hight;
    pthread_t gs_SnapPid;
} snap_info_s;

typedef enum _DMS_ACTION_ {
	DMS_RECOGNITION,
	DMS_CALIB,
	DMS_ACTION_END,
} DMS_ACTION;

typedef struct _axframe_info_s {
	DMS_ACTION action;
    AXNU_E nn_type;
    EI_BOOL bThreadStart;

    VISS_CHN phyChn;
    VISS_DEV vissDev;
    VISS_CHN vissExtChn;

    RECT_S stFrameSize;
    unsigned int frameRate;

    pthread_t gs_axframePid;
    sem_t frame_del_sem;
    pthread_mutex_t frame_stLock;
} axframe_info_s;
typedef struct _facerecgframe_info_s {
    VISS_CHN phyChn;
    VISS_DEV dev;
    VISS_CHN chn;
    EI_U32 u32Width;
    EI_U32 u32Height;
 
	VIDEO_FRAME_INFO_S frame_info;
	nn_facerecg_cfg_t facerecg_cfg;
	nn_facerecg_in_t recg_in;
	nn_facerecg_faces_t faces;
	nn_facerecg_features_t features;
	nn_facerecg_features_t *match_features;
	nn_facerecg_match_result_t result;
	int m_face_rect[4];
	void *nn_hdl;
	int input_id;
	char input_name[30];
	sqlite3 *drivers_db;
	char file_name[30];
	int match_id; /* the match id insert to db */
	char match_name[128]; /* the match name insert to db */
	float match_score;

	nn_facerecg_faces_t *p_det_info;
} facerecgframe_info;

typedef struct _warning_info_s {
    EI_BOOL bThreadStart;
    int mq_id;
	int64_t last_warn_time;
	int last_warn_status;
    pthread_t g_Pid;
    sem_t frame_del_sem;
} warning_info_s;

typedef struct tag_warn_msg {
	int32_t		type;
	int32_t		len;
	char		data[120];
} tag_warn_msg_t;

static warning_info_s warning_info;
facerecgframe_info facerecg_info;
#define USE_MUXER
#define WRITE_BUFFER_SIZE 1*1024*1024
#if 1
#define SDR_VISS_SNS_DVP TP9930_DVP_1920_1080_30FPS_4CH_YUV
#define SDR_VISS_SNS_MIPI TP2815_MIPI_ADAPTIVE_MASHUP_4CH_YUV   /* TP2815_MIPI_ADAPTIVE_MASHUP_4CH_YUV銆乀P2815_MIPI_1920_1080_30FPS_4CH_YUV */
#define SDR_VISS_FRAMERATE 30
#else
#define SDR_VISS_SNS_DVP TP9930_DVP_1920_1080_25FPS_4CH_YUV
#define SDR_VISS_SNS_MIPI TP2815_MIPI_ADAPTIVE_MASHUP_4CH_YUV  /* TP2815_MIPI_1920_1080_25FPS_4CH_YUV */
#define SDR_VISS_FRAMERATE 25
#endif

#define VO_DEVICE  OTM1289A_MIPI_720_1280_60FPS

#define MAIN_STREAM_BITRATE 4 * 1024 * 1024
#define MAIN_STREAM_FRAMERATE 20
#define MAIN_STREAM_MUXER_TYPE AV_MUXER_TYPE_RAW
#define MAIN_STREAM_NUMBER 8
#define SUB_STREAM_NUMBER 8
#define SUB_STREAM_BITRATE 500 * 1024
#define SUB_STREAM_FRAMERATE 16
#define SUB_STREAM_MUXER_TYPE -1

#define SDR_SAMPLE_VENC_TIME 300
#define SDR_SAMPLE_FILE_NUM 6
#define PATH_ROOT "/data"   /* "/mnt/card" */
#define MQ_MSG_NUM 128 /* message queue number */
#define MQ_MSG_LEN 128 /* every message max lenght */
#define ALARM_INTERVAL 5000 /* same warning message interval time unit:ms */

#define LOOKAROUND_STAT_TIME    2000
#define LOOKDOWN_STAT_TIME      2000
#define NN_BSD_AX_PATH      "/usr/share/ax/bsd"
#define NN_ADAS_AX_PATH     "/usr/share/ax/adas"
#define DMS_AX_PATH     "/usr/share/ax/dms"
#define NN_FACERECG_AX_PATH	 "/usr/share/ax/dms"


#define SAVE_OUTPUT_STREAM 1
/*#define SAVE_AX_YUV_SP*/

EI_BOOL disp_switch_flag = EI_FALSE;
EI_BOOL snap_flag = EI_FALSE;
EI_BOOL app_exit_flag = EI_FALSE;

#define NN_AX_NUM   5
axframe_info_s axframe_info[NN_AX_NUM];

nn_adas_out_t *p_adas_out = NULL;
static int adas_cnt = 0;

nn_bsd_out_t *p_bsd0_out = NULL;
static int bsd_cnt = 0;

nn_bsd_out_t *p_bsd1_out = NULL;
static int bsd1_cnt = 0;

nn_dms_out_t *p_dms_out = NULL;
static int dms_cnt = 0;

int adas_before_time = 0;
static EI_U32 scn_w, scn_h, scn_rate;

static EI_S32 VISS_MASHUP_IPPU_Init(SAMPLE_VISS_INFO_S *pstVissInfo, camera_info_s *camera_info)
{
    EI_S32 s32Ret = EI_SUCCESS;
    IPPU_DEV dev = pstVissInfo->stIspInfo.IspDev;
    ISP_PUB_ATTR_S stPubAttr = {0};
    VISS_DEV_ATTR_S stSnsAttr = {0};

    s32Ret = EI_MI_ISP_MemInit(dev);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "Init memory failed with %#x!\n", s32Ret);
        SAMPLE_COMM_ISP_Stop(dev);
        return EI_FAILURE;
    }

    SAMPLE_COMM_VISS_GetDevAttrBySns(pstVissInfo->stSnsInfo.enSnsType, &stSnsAttr);
    stPubAttr.s32IspDev     = dev;
    stPubAttr.stSnsSize.u32Height   = camera_info->stSize.u32Height;
    stPubAttr.stSnsSize.u32Width    = camera_info->stSize.u32Width;
    stPubAttr.pcSnsName     = stSnsAttr.pcSnsName;
    stPubAttr.u32DataSrc    = pstVissInfo->stDevInfo.VissDev;
    switch (stSnsAttr.enPixelFormat) {
    case PIX_FMT_YUV_SEMIPLANAR_420:
    case PIX_FMT_YVU_SEMIPLANAR_420:
    case PIX_FMT_YUV_PLANAR_420:
    case PIX_FMT_YVU_PLANAR_420:
        stPubAttr.enInFmt = ISP_IN_FMT_YUV420;
        break;
    case PIX_FMT_YUV_SEMIPLANAR_422:
    case PIX_FMT_YVU_SEMIPLANAR_422:
    case PIX_FMT_YUV_PLANAR_422:
    case PIX_FMT_YVU_PLANAR_422:
        stPubAttr.enInFmt = ISP_IN_FMT_YUV422;
        break;
    default:
        stPubAttr.enInFmt = ISP_IN_FMT_YUV420;
        break;
    }
    stPubAttr.u32IspClk   = stSnsAttr.u32IspClk;
    stPubAttr.u32IppuClk   = stSnsAttr.u32IppuClk;
    //stPubAttr.u32SnsFps   = stSnsAttr.u32Fps;
    stPubAttr.u32SnsFps   = camera_info->u32FrameRate;
    stPubAttr.enRunningMode = pstVissInfo->stIspInfo.enRunningMode;
    //stPubAttr.stFrc = pstVissInfo->stIspInfo.stFrc;
    stPubAttr.stFrc.s32SrcFrameRate = camera_info->u32FrameRate;
    stPubAttr.stFrc.s32DstFrameRate = SUB_STREAM_FRAMERATE;
    stPubAttr.stWndRect = pstVissInfo->stIspInfo.stCrop;
    stPubAttr.pcParamName   = pstVissInfo->stIspInfo.pcParamName;
    s32Ret = EI_MI_ISP_SetPubAttr(dev, &stPubAttr);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "SetPubAttr failed with %#x!\n", s32Ret);
        SAMPLE_COMM_ISP_Stop(dev);
        return EI_FAILURE;
    }

    s32Ret = EI_MI_ISP_Init(dev);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "ISP Init failed with %#x!\n", s32Ret);
        SAMPLE_COMM_ISP_Stop(dev);
        return EI_FAILURE;
    }

    s32Ret = EI_MI_ISP_Run(dev);
    if (EI_SUCCESS != s32Ret) {
        EI_TRACE_IPPU(EI_DBG_ERR, "failed with %#x!\n", s32Ret);
        return EI_FAILURE;
    }

    return s32Ret;
}

static int64_t _osal_get_msec(void)
{
	struct timespec t;
	t.tv_sec = t.tv_nsec = 0;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	EI_PRINT("t.tv_sec:%ld, t.tv_nsec:%ld \n", t.tv_sec, t.tv_nsec);
	return (int64_t)(t.tv_sec)*1000LL + t.tv_nsec/1000000LL;
}

int storage_info_get(const char *mount_point, sdcard_info_t *info)
{
	struct statfs disk_info;
	int blksize;
	struct mntent *ent;
	FILE *fp;
	char *str_buf = NULL;
	struct mntent tmp_store;
	int str_buf_size = 512;
	char split[] = " ";
	char linebuf[512];
	char *result = NULL;
	int mounted = 0;
	char mount_path[128];
	int len;
	len = strlen(mount_point);
	if (!len)
		return EI_FAILURE;
	strcpy(mount_path, mount_point);
	if (mount_path[len - 1] == '/')
		mount_path[len - 1] = '\0';
	
	str_buf = calloc(str_buf_size, 1);
	if (!str_buf) {
		EI_TRACE_LOG(EI_DBG_ERR, "[%s %d] request fail.\n", __func__, __LINE__);
		return EI_FAILURE;
	}
	
	fp = setmntent("/proc/mounts", "r");
	if (fp == NULL) {
		EI_TRACE_LOG(EI_DBG_ERR, "open error mount proc");
		info->total = 0;
		info->free = 0;
		if (str_buf) {
			free(str_buf);
			str_buf = NULL;
		}
		return EI_FAILURE;
	}
	
	memset(&tmp_store, 0, sizeof(tmp_store));
	while (getmntent_r(fp, &tmp_store, str_buf, str_buf_size) != NULL) {
		if (tmp_store.mnt_dir == NULL) {
			EI_TRACE_LOG(EI_DBG_ERR, "mnt_dir is NULL. \n");
			continue;
		}
		if (!strncmp(tmp_store.mnt_dir, mount_path, strlen(mount_path))) {
			mounted = 1;
			break;
		}
	}
	endmntent(fp);
	
	if (str_buf) {
        free(str_buf);
        str_buf = NULL;
	}
	
	if (mounted ==  0) {
		info->total = 0;
		info->free = 0;
		return EI_FAILURE;
	}

	memset(&disk_info, 0, sizeof(struct statfs));
	if (statfs(mount_path, &disk_info) < 0)
		return EI_FAILURE;
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
	return EI_SUCCESS;
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
	if (type == AV_MUXER_TYPE_TS || type == AV_MUXER_TYPE_MP4){
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

EI_S32 DOSS_SHOW_IPPU_START(IPPU_DEV dev, IPPU_CHN ch, EI_U32 width, EI_U32 hight, ROTATION_E rot)
{
    EI_S32 s32Ret;
    IPPU_CHN_ATTR_S ippu_chn_attr;
    IPPU_ROTATION_ATTR_S astIppuChnRot = {0};

    EI_MI_IPPU_GetChnAttr(dev, ch, &ippu_chn_attr);

    ippu_chn_attr.u32Width	= width;
    ippu_chn_attr.u32Height	= hight;
    ippu_chn_attr.u32Align	= 32;
    ippu_chn_attr.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    ippu_chn_attr.stFrameRate.s32SrcFrameRate = SUB_STREAM_FRAMERATE;
    ippu_chn_attr.stFrameRate.s32DstFrameRate = SUB_STREAM_FRAMERATE;
    ippu_chn_attr.u32Depth	= 0;
    
    s32Ret = EI_MI_IPPU_SetChnAttr(dev, ch, &ippu_chn_attr);
    if (s32Ret != EI_SUCCESS)
    {
        EI_TRACE_IPPU(EI_DBG_ERR, "EI_MI_IPPU_SetChnAttr failed with %#x\n", s32Ret);
        return EI_FAILURE;
    }

    s32Ret = EI_MI_IPPU_EnableChn(dev, ch);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "EI_MI_IPPU_EnableChn failed with %#x\n", s32Ret);
        return EI_FAILURE;
    }
    
    if (rot != ROTATION_0) {
        astIppuChnRot.bEnable = EI_TRUE;
        astIppuChnRot.eRotation = rot;
        EI_MI_IPPU_SetChnRotation(dev, ch, &astIppuChnRot);
    }
    return EI_SUCCESS;
}

EI_S32 DOSS_SHOW_IPPU_STOP(IPPU_DEV dev, IPPU_CHN ch)
{
    EI_S32 s32Ret;
    s32Ret = EI_MI_IPPU_DisableChn(dev, ch);
    if (s32Ret != EI_SUCCESS)
    {
        EI_TRACE_IPPU(EI_DBG_ERR, "EI_MI_IPPU_GetChnAttr failed with %#x\n", s32Ret);
        return EI_FAILURE;
    }
    return EI_SUCCESS;
}

static int _bufpool_init(VBUF_CONFIG_S *vbuf_config)
{
    memset(vbuf_config, 0, sizeof(VBUF_CONFIG_S));
    vbuf_config->u32PoolCnt = 3;

    vbuf_config->astFrameInfo[0].enFrameType = MDP_FRAME_TYPE_COMMON;   //??buf?
    vbuf_config->astFrameInfo[0].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[0].stCommFrameInfo.u32Width = 1920;
    vbuf_config->astFrameInfo[0].stCommFrameInfo.u32Height = 1080;
    vbuf_config->astFrameInfo[0].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[0].enRemapMode = VBUF_REMAP_MODE_NOCACHE;      //?cpu??,???cache??
    vbuf_config->astFrameInfo[0].stCommFrameInfo.enCompressMode = COMPRESS_MODE_NONE;   //???????????????
    vbuf_config->astCommPool[0].u32BufCnt = 50;

    vbuf_config->astFrameInfo[1].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[1].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[1].stCommFrameInfo.u32Width = 352;
    vbuf_config->astFrameInfo[1].stCommFrameInfo.u32Height = 288;
    vbuf_config->astFrameInfo[1].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[1].u32BufCnt = 45;
    vbuf_config->astCommPool[1].enRemapMode = VBUF_REMAP_MODE_NOCACHE;

    vbuf_config->astFrameInfo[2].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[2].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[2].stCommFrameInfo.u32Width = 1280;
    vbuf_config->astFrameInfo[2].stCommFrameInfo.u32Height = 720;
    vbuf_config->astFrameInfo[2].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[2].u32BufCnt = 24;
    vbuf_config->astCommPool[2].enRemapMode = VBUF_REMAP_MODE_CACHED;
/*
    vbuf_config->astFrameInfo[3].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[3].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[3].stCommFrameInfo.u32Width = 720;
    vbuf_config->astFrameInfo[3].stCommFrameInfo.u32Height = 1280;
    vbuf_config->astFrameInfo[3].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[3].u32BufCnt = 4;
    vbuf_config->astCommPool[3].enRemapMode = VBUF_REMAP_MODE_CACHED;
*/
    return SAMPLE_COMM_SYS_Init(vbuf_config);
}

static int _bufpool_deinit(VBUF_CONFIG_S *vbuf_config)
{
    return SAMPLE_COMM_SYS_Exit(vbuf_config);
}

static int _start_viss(SAMPLE_VISS_CONFIG_S *viss_config, camera_info_s *camera_info)
{
    int ret = EI_FAILURE;
    SAMPLE_VISS_INFO_S *viss_info;
    CH_FORMAT_S stFormat[VISS_DEV_MAX_CHN_NUM] = {0};
    VISS_CHN_ATTR_S       stVissChnAttr = {0};
    int i;

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
    viss_info->stChnInfo.stChnAttr.u32Depth = 0;
	//viss_info->stChnInfo.stChnAttr.enMirrorFlip = VISS_MIR_FLIP_M_AND_F;
    viss_info->stSnsInfo.enSnsType = SDR_VISS_SNS_DVP;
    viss_info->stChnInfo.stChnAttr.stFrameRate.s32DstFrameRate = SDR_VISS_FRAMERATE;
    viss_info->stChnInfo.stChnAttr.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;

    for (i = 0; i < 4; i++) {
        camera_info[i].stSize.u32Width = 1920;
        camera_info[i].stSize.u32Height = 1080;
        camera_info[i].u32FrameRate = SDR_VISS_FRAMERATE;
    }

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
    viss_info->stChnInfo.enWorkMode = 0;
    viss_info->stChnInfo.stChnAttr.u32Align = 32;
    viss_info->stChnInfo.stChnAttr.u32Depth = 2;
	//viss_info->stChnInfo.stChnAttr.enMirrorFlip = VISS_MIR_FLIP_M_AND_F;
    viss_info->stSnsInfo.enSnsType = SDR_VISS_SNS_MIPI;
    viss_info->stChnInfo.stChnAttr.stFrameRate.s32DstFrameRate = SDR_VISS_FRAMERATE;
    viss_info->stChnInfo.stChnAttr.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;

    SAMPLE_COMM_VISS_StartViss(viss_config);


    EI_MI_VISS_GetSensorChFormat(1, stFormat);
    for (i = 0; i < VISS_DEV_MAX_CHN_NUM; i++) {
        EI_PRINT("stFormat[%d].stSize.u32Width : %d \n", i, stFormat[i].stSize.u32Width);
        EI_PRINT("stFormat[%d].stSize.u32Height : %d \n", i, stFormat[i].stSize.u32Height);
        EI_PRINT("frameRate : %d \n", stFormat[i].u32FrameRate);
    }

    SAMPLE_COMM_VISS_GetChnAttrBySns(viss_config->astVissInfo[1].stSnsInfo.enSnsType, &stVissChnAttr);

    stVissChnAttr.u32Align = 32;
    stVissChnAttr.u32Depth = 0;
	//stVissChnAttr.enMirrorFlip = VISS_MIR_FLIP_M_AND_F;
	

    for (i = 0; i < VISS_DEV_MAX_CHN_NUM; i++) {
        EI_PRINT("w:%d,h:%d,f:%d\n", stFormat[i].stSize.u32Width,
            stFormat[i].stSize.u32Height,
            stFormat[i].u32FrameRate);
        stVissChnAttr.stSize = stFormat[i].stSize;
        stVissChnAttr.stFrameRate.s32SrcFrameRate = stFormat[i].u32FrameRate;
        stVissChnAttr.stFrameRate.s32DstFrameRate = stFormat[i].u32FrameRate;
		
        camera_info[i+4].stSize = stFormat[i].stSize;
        camera_info[i+4].u32FrameRate = stFormat[i].u32FrameRate;

        ret = EI_MI_VISS_SetChnAttr(1, i+4, &stVissChnAttr);
        if (ret != EI_SUCCESS) {
            PRT_VISS_ERR("EI_MI_VISS_SetChnAttr failed with %#x!\n", ret);
            return EI_FAILURE;
        }

        ret = EI_MI_VISS_EnableChn(1, i+4);
        if (ret != EI_SUCCESS) {
            PRT_VISS_INFO("EI_MI_VISS_EnableChn failed with %#x!\n", ret);
            return EI_FAILURE;
        }
    }

    return 0;
}

static int _stop_viss(SAMPLE_VISS_CONFIG_S *viss_config)
{
    int i;
    for (i = 4; i < 8; i++) {
        EI_MI_VISS_DisableChn(1, i);
    }

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
    camera_info_s *camera_info,
    COMMON_FRAME_INFO_S *in_frame_info,
    COMMON_FRAME_INFO_S *out_frame_info)
{
    int i;
    int ret = 0;
    SAMPLE_VISS_INFO_S *viss_info;
    IPPU_DEV_ATTR_S ippu_dev_attr;
    IPPU_CHN_ATTR_S ippu_chn_attr[IPPU_PHY_CHN_MAX_NUM];
    MDP_CHN_S src_viss[ISP_MAX_DEV_NUM], sink_isp[ISP_MAX_DEV_NUM];
	IPPU_ROTATION_ATTR_S astIppuChnRot = {0};
    int dev_num = 0;

    viss_config->astVissInfo[0].stIspInfo.enRunningMode = ISP_MODE_RUNNING_OFFLINE;	//???isp
    viss_config->astVissInfo[0].stIspInfo.stFrc.s32SrcFrameRate =
        SDR_VISS_FRAMERATE; //only support offline
    viss_config->astVissInfo[0].stIspInfo.stFrc.s32DstFrameRate = SUB_STREAM_FRAMERATE;	
    viss_config->astVissInfo[1].stIspInfo.enRunningMode = ISP_MODE_RUNNING_OFFLINE;
    viss_config->astVissInfo[1].stIspInfo.stFrc.s32SrcFrameRate =
        SDR_VISS_FRAMERATE; //only support offline
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

    ippu_chn_attr[2].u32Width	= 1280;
    ippu_chn_attr[2].u32Height	= 720;
    ippu_chn_attr[2].u32Align	= out_frame_info->u32Align;
    ippu_chn_attr[2].enPixelFormat = out_frame_info->enPixelFormat;
    ippu_chn_attr[2].stFrameRate.s32SrcFrameRate = SUB_STREAM_FRAMERATE;
    ippu_chn_attr[2].stFrameRate.s32DstFrameRate = SUB_STREAM_FRAMERATE;
    ippu_chn_attr[2].u32Depth	= 0;

    for (i = 0; i < 8; i++) {
        if (i > 3)
            viss_info = &viss_config->astVissInfo[1];
        else
            viss_info = &viss_config->astVissInfo[0];

        viss_info->stIspInfo.IspDev = i;
        ret = VISS_MASHUP_IPPU_Init(viss_info, &camera_info[i]);
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
		
		#if 1
		if (chn_enabled[1] == EI_TRUE) {
			astIppuChnRot.bEnable = EI_TRUE;
			astIppuChnRot.eRotation = ROTATION_90;
			EI_MI_IPPU_SetChnRotation(viss_info->stIspInfo.IspDev, 1, &astIppuChnRot);
		}
		#endif
		
        dev_num++;
    }

    dev_num = 0;
    for (i = 0; i < 8; i++) {
        src_viss[i].enModId = EI_ID_VISS;
        src_viss[i].s32ChnId = i;
        src_viss[i].s32DevId = i > 3 ? 1 : 0;
        sink_isp[i].enModId = EI_ID_ISP;
        sink_isp[i].s32ChnId = 0;	//????0
        sink_isp[i].s32DevId = i;
        ret = EI_MI_MLINK_Link(&src_viss[i], &sink_isp[i]);		//???????????
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

/* 1ch */
static int _link_1ch_display(VO_LAYER layer, EI_U32 width, EI_U32 hight, VO_CHN_ATTR_S *vo_chn_attr, IPPU_DEV dev, ROTATION_E rot)
{
    int ret = EI_FAILURE;
    MDP_CHN_S src_ippu[ISP_MAX_DEV_NUM], sink_doss[ISP_MAX_DEV_NUM];

    ret = DOSS_SHOW_IPPU_START(dev, 2, width, hight, rot);
    if (ret != EI_SUCCESS) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
        goto exit;
    }

    vo_chn_attr->stRect.s32X = 0;
    vo_chn_attr->stRect.s32Y = 0;
    vo_chn_attr->stRect.u32Width = width;
    vo_chn_attr->stRect.u32Height = hight;
    EI_MI_VO_SetChnAttr(layer, 0, vo_chn_attr);
    EI_MI_VO_EnableChn(layer, 0);

    src_ippu[0].enModId = EI_ID_IPPU;
    src_ippu[0].s32ChnId = 2;
    src_ippu[0].s32DevId = dev;
    sink_doss[0].enModId = EI_ID_DOSS;
    sink_doss[0].s32ChnId = 0;
    sink_doss[0].s32DevId = 0;
    ret = EI_MI_MLINK_Link(&src_ippu[0], &sink_doss[0]);
    if (ret != EI_SUCCESS) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
        goto exit;
    }

exit:
    return ret;
}

static void _unlink_1ch_display(VO_LAYER layer, IPPU_DEV dev)
{
    MDP_CHN_S src_ippu[ISP_MAX_DEV_NUM], sink_doss[ISP_MAX_DEV_NUM];

    src_ippu[0].enModId = EI_ID_IPPU;
    src_ippu[0].s32ChnId = 2;
    src_ippu[0].s32DevId = dev;
    sink_doss[0].enModId = EI_ID_DOSS;
    sink_doss[0].s32ChnId = 0;
    sink_doss[0].s32DevId = 0;
    EI_MI_MLINK_UnLink(&src_ippu[0], &sink_doss[0]);
    EI_MI_VO_DisableChn(layer, 0);

    DOSS_SHOW_IPPU_STOP(dev, 2);
}

/* 4ch */
static int _link_2x2ch_display(VO_LAYER layer, EI_U32 width, EI_U32 hight, VO_CHN_ATTR_S *vo_chn_attr, IPPU_DEV *dev)
{
    int i;
    int ret = EI_FAILURE;
    MDP_CHN_S src_ippu[ISP_MAX_DEV_NUM], sink_doss[ISP_MAX_DEV_NUM];
    //IPPU_DEV dev[4] = {1, 3, 5, 7};
    for (i = 0; i < 4; i++) {
        vo_chn_attr->stRect.s32X = i < 2 ? 0 : width/2;
        vo_chn_attr->stRect.s32Y = (i % 2) * hight/2;
        vo_chn_attr->stRect.u32Width = width/2;
        vo_chn_attr->stRect.u32Height = hight/2;
        EI_MI_VO_SetChnAttr(layer, i, vo_chn_attr);
        EI_MI_VO_SetChnFrameRate(layer, i, SUB_STREAM_FRAMERATE);
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

static void _unlink_2x2ch_display(VO_LAYER layer, IPPU_DEV *dev)
{
    int i;
    MDP_CHN_S src_ippu[ISP_MAX_DEV_NUM], sink_doss[ISP_MAX_DEV_NUM];

    for (i = 0; i < 4; i++) {
        src_ippu[i].enModId = EI_ID_IPPU;
        src_ippu[i].s32ChnId = 1;
        src_ippu[i].s32DevId = dev[i];
        sink_doss[i].enModId = EI_ID_DOSS;
        sink_doss[i].s32ChnId = i;
        sink_doss[i].s32DevId = 0;
        EI_MI_MLINK_UnLink(&src_ippu[i], &sink_doss[i]);
        EI_MI_VO_DisableChn(layer, i);
    }
}

#if 1
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
        EI_MI_VO_SetChnFrameRate(layer, i, SUB_STREAM_FRAMERATE);
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

static void _unlink_4x2ch_display(VO_LAYER layer, IPPU_DEV *dev)
{
    int i;
    MDP_CHN_S src_ippu[ISP_MAX_DEV_NUM], sink_doss[ISP_MAX_DEV_NUM];

    for (i = 0; i < 8; i++) {
        src_ippu[i].enModId = EI_ID_IPPU;
        src_ippu[i].s32ChnId = 1;
        src_ippu[i].s32DevId = dev[i];
        sink_doss[i].enModId = EI_ID_DOSS;
        sink_doss[i].s32ChnId = i;
        sink_doss[i].s32DevId = 0;
        EI_MI_MLINK_UnLink(&src_ippu[i], &sink_doss[i]);
        EI_MI_VO_DisableChn(layer, i);
    }
}
#endif

/* 3x3ch */
static int _link_3x3ch_display(VO_LAYER layer, EI_U32 width, EI_U32 hight, VO_CHN_ATTR_S *vo_chn_attr, IPPU_DEV *dev)
{
    int i;
    int ret = EI_FAILURE;
    MDP_CHN_S src_ippu[ISP_MAX_DEV_NUM], sink_doss[ISP_MAX_DEV_NUM];
    for (i = 0; i < 9; i++) {
        vo_chn_attr->stRect.s32X = (i/3) * width/3;
        vo_chn_attr->stRect.s32Y = (i % 3) * hight/3;
        vo_chn_attr->stRect.u32Width = width/3;
        vo_chn_attr->stRect.u32Height = hight/3;

        EI_MI_VO_SetChnAttr(layer, i, vo_chn_attr);
        EI_MI_VO_EnableChn(layer, i);
        
        if (i == 8) {
            EI_MI_VO_HideChn(layer, i);
            goto exit;
        }
        
        EI_MI_VO_SetChnFrameRate(layer, i, SUB_STREAM_FRAMERATE);
        
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

static void _unlink_3x3ch_display(VO_LAYER layer, IPPU_DEV *dev)
{
    int i;
    MDP_CHN_S src_ippu[ISP_MAX_DEV_NUM], sink_doss[ISP_MAX_DEV_NUM];

    for (i = 0; i < 9; i++) {
        if (i == 8) {
            EI_MI_VO_DisableChn(layer, i);
            continue;
        }
        src_ippu[i].enModId = EI_ID_IPPU;
        src_ippu[i].s32ChnId = 1;
        src_ippu[i].s32DevId = dev[i];
        sink_doss[i].enModId = EI_ID_DOSS;
        sink_doss[i].s32ChnId = i;
        sink_doss[i].s32DevId = 0;
        EI_MI_MLINK_UnLink(&src_ippu[i], &sink_doss[i]);
        EI_MI_VO_DisableChn(layer, i);
    }
}

static int set_doss_layer_partition_mode(VO_LAYER VoLayer, VO_PART_MODE_E enPartMode)
{
    int ret = EI_FAILURE;
    ret = EI_MI_VO_DisableVideoLayer(VoLayer);
    if (ret != EI_SUCCESS) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
    }
    
    ret = EI_MI_VO_SetVideoLayerPartitionMode(VoLayer, enPartMode);
    if (ret != EI_SUCCESS) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
    }
    
    ret = EI_MI_VO_EnableVideoLayer(VoLayer);
    if (ret != EI_SUCCESS) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
    }

    return ret;
}

EI_VOID *_disp_mode_switch(EI_VOID *p)
{
    display_info_s *disp_info;
    VO_VIDEO_LAYER_ATTR_S layer_attr;
    IPPU_DEV ippu_dev[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    IPPU_DEV ippu_dev_2x2_0[4] = {0, 1, 2, 3};
    IPPU_DEV ippu_dev_2x2_1[4] = {4, 5, 6, 7};
    IPPU_DEV ippu_dev_3x3[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    ROTATION_E rot = ROTATION_0;
    
    disp_info = (display_info_s *)p;
    if (!disp_info) {
        EI_TRACE_LOG(EI_DBG_ERR, "err param !\n");
        EI_PRINT("disp_info err param !\n");
        return NULL;
    }
	
	prctl(PR_SET_NAME, "disp_mode_switch");

    int display_switch_flag = disp_info->display_switch_flag;

    display_switch_flag = 10;
    disp_info->bThreadStart = EI_TRUE;
    while (EI_TRUE == disp_info->bThreadStart) {
        if (disp_switch_flag == EI_TRUE) {
            disp_switch_flag = EI_FALSE;
            
            if (display_switch_flag >= 0 && display_switch_flag < 8) {
                _unlink_1ch_display(disp_info->layer, ippu_dev[display_switch_flag]);
            }  else if (display_switch_flag == 8) {
                _unlink_2x2ch_display(disp_info->layer, ippu_dev_2x2_0);
            } else if (display_switch_flag == 9) {
                _unlink_2x2ch_display(disp_info->layer, ippu_dev_2x2_1);
            } else if (display_switch_flag == 10) {
                _unlink_3x3ch_display(disp_info->layer, ippu_dev_3x3);
				//_unlink_4x2ch_display(disp_info->layer, ippu_dev_4x2);
            }
            
            display_switch_flag++;
            if (display_switch_flag > 10) 
                display_switch_flag = 0;
            
            if (display_switch_flag >= 0 && display_switch_flag < 8) {
                if (display_switch_flag == 0) {
                    EI_MI_VO_GetVideoLayerAttr(disp_info->layer, &layer_attr);
                    if (scn_w > scn_h) {
                        layer_attr.stImageSize.u32Width = scn_w;
                        layer_attr.stImageSize.u32Height = scn_h;
                    } else {
                        layer_attr.stImageSize.u32Width = scn_h;
                        layer_attr.stImageSize.u32Height = scn_w;
                    }
                    EI_MI_VO_SetVideoLayerAttr(disp_info->layer, &layer_attr);
                    set_doss_layer_partition_mode(disp_info->layer, VO_PART_MODE_MULTI);
                }

                if (scn_w > scn_h) {
                    _link_1ch_display(disp_info->layer, scn_w, scn_h, disp_info->vo_chn_attr, ippu_dev[display_switch_flag], rot);
                } else {
                    rot = ROTATION_90;
                    _link_1ch_display(disp_info->layer, scn_h, scn_w, disp_info->vo_chn_attr, ippu_dev[display_switch_flag], rot);
                }
            } else if (display_switch_flag == 8) {
                EI_MI_VO_GetVideoLayerAttr(disp_info->layer, &layer_attr);
                layer_attr.stImageSize.u32Width = scn_w;
                layer_attr.stImageSize.u32Height = scn_h;
                EI_MI_VO_SetVideoLayerAttr(disp_info->layer, &layer_attr);

                set_doss_layer_partition_mode(disp_info->layer, VO_PART_MODE_SINGLE);
                _link_2x2ch_display(disp_info->layer, scn_w, scn_h, disp_info->vo_chn_attr, ippu_dev_2x2_0);
            } else if (display_switch_flag == 9) {
                EI_MI_VO_GetVideoLayerAttr(disp_info->layer, &layer_attr);
                layer_attr.stImageSize.u32Width = scn_w;
                layer_attr.stImageSize.u32Height = scn_h;
                EI_MI_VO_SetVideoLayerAttr(disp_info->layer, &layer_attr);
                _link_2x2ch_display(disp_info->layer, scn_w, scn_h, disp_info->vo_chn_attr, ippu_dev_2x2_1);
            } else if (display_switch_flag == 10) {
                _link_3x3ch_display(disp_info->layer, scn_w, scn_h, disp_info->vo_chn_attr, ippu_dev_3x3);
            }
            
            disp_info->display_switch_flag = display_switch_flag;
        }

        usleep(1000 * 1000);
    }

    return NULL;
}

static int _start_disp_mode_switch(display_info_s *disp_info){
    int ret;
    
    disp_info->bThreadStart = EI_TRUE;
    ret = pthread_create(&disp_info->gs_DispPid, NULL, _disp_mode_switch,
            (EI_VOID *)disp_info);
    if (ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));
    return ret;
}


static int _start_disp(VO_DEV dev, VO_LAYER layer,
    COMMON_FRAME_INFO_S *disp_frame_info, display_info_s *disp_info)
{
    int ret = EI_FAILURE;
    VO_VIDEO_LAYER_ATTR_S layer_attr;
    VO_CHN_ATTR_S vo_chn_attr = {0};
    PANEL_TYPE_ATTR_S panel_attr;
    IPPU_DEV ippu_dev_3x3[8] = {0, 1, 2, 3, 4, 5, 6, 7};
	IPPU_DEV ippu_dev_4x2[8] = {0, 1, 2, 3, 4, 5, 6, 7};

    memset(&layer_attr, 0, sizeof(layer_attr));
    memset(&vo_chn_attr, 0, sizeof(vo_chn_attr));
    memset(&panel_attr, 0, sizeof(panel_attr));
    memset(disp_info, 0, sizeof(display_info_s));

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
    layer_attr.stImageSize.u32Width = scn_w;/* disp_frame_info->u32Width; */
    layer_attr.stImageSize.u32Height = scn_h;/* disp_frame_info->u32Height; */
    layer_attr.enPixFormat = disp_frame_info->enPixelFormat;
    layer_attr.stDispRect.s32X = 0;
    layer_attr.stDispRect.s32Y = 0;
    layer_attr.stDispRect.u32Width = scn_w;
    layer_attr.stDispRect.u32Height = scn_h;
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

    _link_3x3ch_display(layer, scn_w, scn_h, &vo_chn_attr, ippu_dev_3x3);

    disp_info->dev = dev;
    disp_info->stDispSize.u32Width = scn_w;
    disp_info->stDispSize.u32Height = scn_h;
    disp_info->layer = layer;
    disp_info->vo_chn_attr = &vo_chn_attr;
    disp_info->display_switch_flag = 10;
    _start_disp_mode_switch(disp_info);

    return ret;
exit1:
    SAMPLE_COMM_DOSS_StopDev(dev);
exit:
    EI_MI_DOSS_Exit();
    return ret;
}


static void _stop_disp(VO_DEV dev, VO_LAYER layer, display_info_s *disp_info)
{
    IPPU_DEV ippu_dev[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    IPPU_DEV ippu_dev_2x2_0[4] = {0, 1, 2, 3};
    IPPU_DEV ippu_dev_2x2_1[4] = {4, 5, 6, 7};
    IPPU_DEV ippu_dev_3x3[8] = {0, 1, 2, 3, 4, 5, 6, 7};
	IPPU_DEV ippu_dev_4x2[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    int display_switch_flag = disp_info->display_switch_flag;
    disp_info->bThreadStart = EI_FALSE;

    if (display_switch_flag >= 0 && display_switch_flag < 8) {
        _unlink_1ch_display(disp_info->layer, ippu_dev[display_switch_flag]);
    }  else if (display_switch_flag == 8) {
        _unlink_2x2ch_display(disp_info->layer, ippu_dev_2x2_0);
    } else if (display_switch_flag == 9) {
        _unlink_2x2ch_display(disp_info->layer, ippu_dev_2x2_1);
    } else if (display_switch_flag == 10) {
        _unlink_3x3ch_display(disp_info->layer, ippu_dev_3x3);
		//_unlink_4x2ch_display(disp_info->layer, ippu_dev_4x2);
    }

    EI_MI_VO_DisableVideoLayer(layer);
    SAMPLE_COMM_DOSS_StopDev(dev);
    EI_MI_DOSS_Exit();
}

typedef struct _region_info_s {
    AXNU_E nn_type;
	EI_BOOL bThreadStart;
    MDP_CHN_S stRgnChn;
    SIZE_S stSize;
    void *nn_ax_out;
    
	pthread_t gs_rgnPid;
} region_info_s;

typedef struct _rectangle_info_s {
    EI_U32 loc_x;
    EI_U32 loc_y;
    EI_U32 width;
    EI_U32 hight;
} rectangle_info_s;

typedef struct _line_info_s {
    EI_U32 loc_x0;
    EI_U32 loc_y0;
    EI_U32 loc_x1;
    EI_U32 loc_y1;
} line_info_s;

#define CAR_RECTANGLEEX_RGN_NUM     6
#define PED_RECTANGLEEX_RGN_NUM     6
#define LANES_0_LINE_RGN_NUM   5
#define LANES_1_LINE_RGN_NUM   5
#define ADAS_RGN_TOTAL_NUM  (CAR_RECTANGLEEX_RGN_NUM + PED_RECTANGLEEX_RGN_NUM + LANES_0_LINE_RGN_NUM + LANES_1_LINE_RGN_NUM)
#define BSD_RGN_TOTAL_NUM   (CAR_RECTANGLEEX_RGN_NUM + PED_RECTANGLEEX_RGN_NUM)
#define FACE_RECTANGLEEX_RGN_NUM     6
#define SMOKES_RECTANGLEEX_RGN_NUM     4
#define PHONES_RECTANGLEEX_RGN_NUM     4
#define DRINKS_RECTANGLEEX_RGN_NUM     4
#define DMS_RGN_TOTAL_NUM   (FACE_RECTANGLEEX_RGN_NUM + SMOKES_RECTANGLEEX_RGN_NUM + PHONES_RECTANGLEEX_RGN_NUM + DRINKS_RECTANGLEEX_RGN_NUM)

static int get_handle_id(){
    static int handle_id = 0;
    
    return handle_id++;
}

static void adas_car_rectangle_reduce_shake(nn_adas_out_t *p_adas_out, nn_adas_cars_t *adas_cars_last)
{
    if (adas_cars_last->size > 0 && p_adas_out->cars.size > 0) {
        for (int last = 0; last < adas_cars_last->size; last++) {
            for (int cur = 0; cur < p_adas_out->cars.size; cur++) {
                int index = 0;
                if (p_adas_out->cars.p[cur].box[0] - 10 < adas_cars_last->p[last].box[0]
                    && p_adas_out->cars.p[cur].box[0] + 10 > adas_cars_last->p[last].box[0])
                    index++;
                if (p_adas_out->cars.p[cur].box[1] - 10 < adas_cars_last->p[last].box[1]
                    && p_adas_out->cars.p[cur].box[1] + 10 > adas_cars_last->p[last].box[1])
                    index++;
                if (p_adas_out->cars.p[cur].box[2] - 10 < adas_cars_last->p[last].box[2]
                    && p_adas_out->cars.p[cur].box[2] + 10 > adas_cars_last->p[last].box[2])
                    index++;
                if (p_adas_out->cars.p[cur].box[3] - 10 < adas_cars_last->p[last].box[3]
                    && p_adas_out->cars.p[cur].box[3] + 10 > adas_cars_last->p[last].box[3])
                    index++;
                if(index == 4) {
                    p_adas_out->cars.p[cur] = adas_cars_last->p[last];
                }
            }
        }
    }

    if (adas_cars_last->p) {
        free(adas_cars_last->p);
        adas_cars_last->p = NULL;
        adas_cars_last->size = 0;
    }
    adas_cars_last->p =
        malloc(p_adas_out->cars.size * sizeof(nn_adas_car_t));
    if (p_adas_out->cars.p) {
        memcpy(adas_cars_last->p, p_adas_out->cars.p,
            p_adas_out->cars.size * sizeof(nn_adas_car_t));
        adas_cars_last->size = p_adas_out->cars.size;
    }
}

static void adas_ped_rectangle_reduce_shake(nn_adas_out_t *p_adas_out, nn_adas_peds_t *adas_peds_last)
{
    if (adas_peds_last->size > 0 && p_adas_out->peds.size > 0) {
        for (int last = 0; last < adas_peds_last->size; last++) {
            for (int cur = 0; cur < p_adas_out->peds.size; cur++) {
                int index = 0;
                if (p_adas_out->peds.p[cur].box[0] - 10 < adas_peds_last->p[last].box[0]
                    && p_adas_out->peds.p[cur].box[0] + 10 > adas_peds_last->p[last].box[0])
                    index++;
                if (p_adas_out->peds.p[cur].box[1] - 10 < adas_peds_last->p[last].box[1]
                    && p_adas_out->peds.p[cur].box[1] + 10 > adas_peds_last->p[last].box[1])
                    index++;
                if (p_adas_out->peds.p[cur].box[2] - 10 < adas_peds_last->p[last].box[2]
                    && p_adas_out->peds.p[cur].box[2] + 10 > adas_peds_last->p[last].box[2])
                    index++;
                if (p_adas_out->peds.p[cur].box[3] - 10 < adas_peds_last->p[last].box[3]
                    && p_adas_out->peds.p[cur].box[3] + 10 > adas_peds_last->p[last].box[3])
                    index++;
                if(index == 4) {
                    p_adas_out->peds.p[cur] = adas_peds_last->p[last];
                }
            }
        }
    }

    if (adas_peds_last->p) {
        free(adas_peds_last->p);
        adas_peds_last->p = NULL;
        adas_peds_last->size = 0;
    }
    adas_peds_last->p =
        malloc(p_adas_out->peds.size * sizeof(nn_adas_ped_t));
    if (p_adas_out->peds.p) {
        memcpy(adas_peds_last->p, p_adas_out->peds.p,
            p_adas_out->peds.size * sizeof(nn_adas_ped_t));
        adas_peds_last->size = p_adas_out->peds.size;
    }
}

EI_VOID *update_adas_rectangle_proc(EI_VOID *p)
{
    EI_U32 i; 
    RGN_ATTR_S stRegion[ADAS_RGN_TOTAL_NUM] = {{0}};
    RGN_HANDLE Handle[ADAS_RGN_TOTAL_NUM];
    RGN_CHN_ATTR_S stRgnChnAttr[ADAS_RGN_TOTAL_NUM] = {{0}};
    rectangle_info_s cars_rectangle[CAR_RECTANGLEEX_RGN_NUM] = {{0}};   
    rectangle_info_s peds_rectangle[PED_RECTANGLEEX_RGN_NUM] = {{0}};   
    line_info_s lane_0_line[LANES_0_LINE_RGN_NUM] = {{0}};   
    line_info_s lane_1_line[LANES_1_LINE_RGN_NUM] = {{0}}; 
    nn_adas_cars_t adas_cars_last = {0};
    nn_adas_peds_t adas_peds_last = {0};
    region_info_s *region_info;
    region_info = (region_info_s *)p;
	
	prctl(PR_SET_NAME, "update_adas_rect");

    for (i = 0; i < CAR_RECTANGLEEX_RGN_NUM + PED_RECTANGLEEX_RGN_NUM; i++)
        stRegion[i].enType = RECTANGLEEX_RGN;

    for (i = CAR_RECTANGLEEX_RGN_NUM + PED_RECTANGLEEX_RGN_NUM; 
         i < ADAS_RGN_TOTAL_NUM; 
         i++)
        stRegion[i].enType = LINEEX_RGN;

    for (i = 0; i < ADAS_RGN_TOTAL_NUM; i++) {
        Handle[i] = get_handle_id();
    }

    while (EI_TRUE == region_info->bThreadStart) {
        if (p_adas_out == NULL) {
            usleep(200*1000);
            continue;
        }

        pthread_mutex_lock(&axframe_info[NN_ADAS].frame_stLock);
        adas_car_rectangle_reduce_shake(p_adas_out, &adas_cars_last);
        adas_ped_rectangle_reduce_shake(p_adas_out, &adas_peds_last);
        
        for (i = 0; i < ADAS_RGN_TOTAL_NUM; i++) {
            stRgnChnAttr[i].bShow = EI_FALSE;
        }

        for (i = 0; i < p_adas_out->cars.size && i < CAR_RECTANGLEEX_RGN_NUM; i++) {
            cars_rectangle[i].loc_x = p_adas_out->cars.p[i].box[0] * region_info->stSize.u32Width / 1280;
            cars_rectangle[i].loc_y = p_adas_out->cars.p[i].box[1] * region_info->stSize.u32Height / 720;
            cars_rectangle[i].width = 
                (p_adas_out->cars.p[i].box[2] - p_adas_out->cars.p[i].box[0]) * region_info->stSize.u32Width / 1280;
            cars_rectangle[i].hight = 
                (p_adas_out->cars.p[i].box[3] - p_adas_out->cars.p[i].box[1]) * region_info->stSize.u32Height / 720;

            stRgnChnAttr[i].bShow = EI_TRUE;
            stRgnChnAttr[i].enType = RECTANGLEEX_RGN;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.u32BorderSize = 4;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.s32X = cars_rectangle[i].loc_x;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.s32Y = cars_rectangle[i].loc_y;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.u32Height = cars_rectangle[i].hight;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.u32Width = cars_rectangle[i].width;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.u32Color = 0xff00ff00;

            EI_MI_RGN_Create(Handle[i], &stRegion[i]);
            EI_MI_RGN_SetChnAttr(Handle[i], &region_info->stRgnChn, &stRgnChnAttr[i]);
            EI_MI_RGN_AddToChn(Handle[i], &region_info->stRgnChn, &stRgnChnAttr[i]);
            
        }

        for (i = CAR_RECTANGLEEX_RGN_NUM; 
             i < (p_adas_out->peds.size + CAR_RECTANGLEEX_RGN_NUM)
                && i < CAR_RECTANGLEEX_RGN_NUM + PED_RECTANGLEEX_RGN_NUM; 
             i++) {
            int ped_index = i - CAR_RECTANGLEEX_RGN_NUM;
            peds_rectangle[ped_index].loc_x = p_adas_out->peds.p[ped_index].box[0] * region_info->stSize.u32Width / 1280;
            peds_rectangle[ped_index].loc_y = p_adas_out->peds.p[ped_index].box[1] * region_info->stSize.u32Height / 720;
            peds_rectangle[ped_index].width = 
                (p_adas_out->peds.p[ped_index].box[2] - p_adas_out->peds.p[ped_index].box[0]) * region_info->stSize.u32Width / 1280;
            peds_rectangle[ped_index].hight = 
                (p_adas_out->peds.p[ped_index].box[3] - p_adas_out->peds.p[ped_index].box[1]) * region_info->stSize.u32Height / 720;

            stRgnChnAttr[i].bShow = EI_TRUE;
            stRgnChnAttr[i].enType = RECTANGLEEX_RGN;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.u32BorderSize = 4;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.s32X = peds_rectangle[ped_index].loc_x;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.s32Y = peds_rectangle[ped_index].loc_y;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.u32Height = peds_rectangle[ped_index].hight;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.u32Width = peds_rectangle[ped_index].width;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.u32Color = 0xffff0000;

            EI_MI_RGN_Create(Handle[i], &stRegion[i]);
            EI_MI_RGN_SetChnAttr(Handle[i], &region_info->stRgnChn, &stRgnChnAttr[i]);
            EI_MI_RGN_AddToChn(Handle[i], &region_info->stRgnChn, &stRgnChnAttr[i]);
        }

        if (p_adas_out->lanes.lanes[0].exist) {
            for (i = CAR_RECTANGLEEX_RGN_NUM + PED_RECTANGLEEX_RGN_NUM; 
                 i < ADAS_RGN_TOTAL_NUM - LANES_1_LINE_RGN_NUM; 
                 i++) {
                int lane_index = i - (CAR_RECTANGLEEX_RGN_NUM + PED_RECTANGLEEX_RGN_NUM);
                lane_0_line[lane_index].loc_x0 = 
                    p_adas_out->lanes.lanes[0].pts[lane_index][0] * region_info->stSize.u32Width / 1280;
                lane_0_line[lane_index].loc_y0 = 
                    p_adas_out->lanes.lanes[0].pts[lane_index][1] * region_info->stSize.u32Height / 720;
                lane_0_line[lane_index].loc_x1 = 
                    p_adas_out->lanes.lanes[0].pts[lane_index + 1][0] * region_info->stSize.u32Width / 1280;
                lane_0_line[lane_index].loc_y1 = 
                    p_adas_out->lanes.lanes[0].pts[lane_index + 1][1] * region_info->stSize.u32Height / 720;

                stRgnChnAttr[i].bShow = EI_TRUE;
                stRgnChnAttr[i].enType = LINEEX_RGN;
                stRgnChnAttr[i].unChnAttr.stLineExChn.stPoints[0].s32X = lane_0_line[lane_index].loc_x0;
                stRgnChnAttr[i].unChnAttr.stLineExChn.stPoints[0].s32Y = lane_0_line[lane_index].loc_y0;
                stRgnChnAttr[i].unChnAttr.stLineExChn.stPoints[1].s32X = lane_0_line[lane_index].loc_x1;
                stRgnChnAttr[i].unChnAttr.stLineExChn.stPoints[1].s32Y = lane_0_line[lane_index].loc_y1;
                stRgnChnAttr[i].unChnAttr.stLineExChn.u32Color = 0xffff0000;
                stRgnChnAttr[i].unChnAttr.stLineExChn.u32LineSize = 8;

                EI_MI_RGN_Create(Handle[i], &stRegion[i]);
                EI_MI_RGN_SetChnAttr(Handle[i], &region_info->stRgnChn, &stRgnChnAttr[i]);
                EI_MI_RGN_AddToChn(Handle[i], &region_info->stRgnChn, &stRgnChnAttr[i]);
            }
        }

        if (p_adas_out->lanes.lanes[1].exist) {
            for (i = ADAS_RGN_TOTAL_NUM - LANES_1_LINE_RGN_NUM; 
                 i < ADAS_RGN_TOTAL_NUM; 
                 i++) {
                int lane_index = i - (ADAS_RGN_TOTAL_NUM - LANES_1_LINE_RGN_NUM);
                lane_1_line[lane_index].loc_x0 = 
                    p_adas_out->lanes.lanes[1].pts[lane_index][0] * region_info->stSize.u32Width / 1280;
                lane_1_line[lane_index].loc_y0 = 
                    p_adas_out->lanes.lanes[1].pts[lane_index][1] * region_info->stSize.u32Height / 720;
                lane_1_line[lane_index].loc_x1 = 
                    p_adas_out->lanes.lanes[1].pts[lane_index + 1][0] * region_info->stSize.u32Width / 1280;
                lane_1_line[lane_index].loc_y1 = 
                    p_adas_out->lanes.lanes[1].pts[lane_index + 1][1] * region_info->stSize.u32Height / 720;

                stRgnChnAttr[i].bShow = EI_TRUE;
                stRgnChnAttr[i].enType = LINEEX_RGN;
                stRgnChnAttr[i].unChnAttr.stLineExChn.stPoints[0].s32X = lane_1_line[lane_index].loc_x0;
                stRgnChnAttr[i].unChnAttr.stLineExChn.stPoints[0].s32Y = lane_1_line[lane_index].loc_y0;
                stRgnChnAttr[i].unChnAttr.stLineExChn.stPoints[1].s32X = lane_1_line[lane_index].loc_x1;
                stRgnChnAttr[i].unChnAttr.stLineExChn.stPoints[1].s32Y = lane_1_line[lane_index].loc_y1;
                stRgnChnAttr[i].unChnAttr.stLineExChn.u32Color = 0xffff0000;
                stRgnChnAttr[i].unChnAttr.stLineExChn.u32LineSize = 8;

                EI_MI_RGN_Create(Handle[i], &stRegion[i]);
                EI_MI_RGN_SetChnAttr(Handle[i], &region_info->stRgnChn, &stRgnChnAttr[i]);
                EI_MI_RGN_AddToChn(Handle[i], &region_info->stRgnChn, &stRgnChnAttr[i]);
            }
        }
		
        pthread_mutex_unlock(&axframe_info[NN_ADAS].frame_stLock);
        
        usleep(50 * 1000);
        for (i = 0; i < ADAS_RGN_TOTAL_NUM; i++) {
            if (stRgnChnAttr[i].bShow == EI_TRUE) {
                EI_MI_RGN_DelFromChn(Handle[i], &region_info->stRgnChn);
                EI_MI_RGN_Destroy(Handle[i]);
            }
        }
    }

    return NULL;
}

static void bsd_car_rectangle_reduce_shake(nn_bsd_out_t *p_bsd_out, nn_bsd_cars_t *bsd_cars_last)
{
    if (bsd_cars_last->size > 0 && p_bsd_out->cars.size > 0) {
        for (int last = 0; last < bsd_cars_last->size; last++) {
            for (int cur = 0; cur < p_bsd_out->cars.size; cur++) {
                int index = 0;
                if (p_bsd_out->cars.p[cur].box[0] - 10 < bsd_cars_last->p[last].box[0]
                    && p_bsd_out->cars.p[cur].box[0] + 10 > bsd_cars_last->p[last].box[0])
                    index++;
                if (p_bsd_out->cars.p[cur].box[1] - 10 < bsd_cars_last->p[last].box[1]
                    && p_bsd_out->cars.p[cur].box[1] + 10 > bsd_cars_last->p[last].box[1])
                    index++;
                if (p_bsd_out->cars.p[cur].box[2] - 10 < bsd_cars_last->p[last].box[2]
                    && p_bsd_out->cars.p[cur].box[2] + 10 > bsd_cars_last->p[last].box[2])
                    index++;
                if (p_bsd_out->cars.p[cur].box[3] - 10 < bsd_cars_last->p[last].box[3]
                    && p_bsd_out->cars.p[cur].box[3] + 10 > bsd_cars_last->p[last].box[3])
                    index++;
                if(index == 4) {
                    p_bsd_out->cars.p[cur] = bsd_cars_last->p[last];
                    //EI_PRINT("rectangle location no change!\n");
                }
            }
        }
    }

    if (bsd_cars_last->p) {
        free(bsd_cars_last->p);
        bsd_cars_last->p = NULL;
        bsd_cars_last->size = 0;
    }
    bsd_cars_last->p =
        malloc(p_bsd_out->cars.size * sizeof(nn_bsd_car_t));
    if (p_bsd_out->cars.p) {
        memcpy(bsd_cars_last->p, p_bsd_out->cars.p,
            p_bsd_out->cars.size * sizeof(nn_bsd_car_t));
        bsd_cars_last->size = p_bsd_out->cars.size;
    }
}

static void bsd_ped_rectangle_reduce_shake(nn_bsd_out_t *p_bsd_out, nn_bsd_peds_t *bsd_peds_last)
{
    if (bsd_peds_last->size > 0 && p_bsd_out->peds.size > 0) {
        for (int last = 0; last < bsd_peds_last->size; last++) {
            for (int cur = 0; cur < p_bsd_out->peds.size; cur++) {
                int index = 0;
                if (p_bsd_out->peds.p[cur].box[0] - 10 < bsd_peds_last->p[last].box[0]
                    && p_bsd_out->peds.p[cur].box[0] + 10 > bsd_peds_last->p[last].box[0])
                    index++;
                if (p_bsd_out->peds.p[cur].box[1] - 10 < bsd_peds_last->p[last].box[1]
                    && p_bsd_out->peds.p[cur].box[1] + 10 > bsd_peds_last->p[last].box[1])
                    index++;
                if (p_bsd_out->peds.p[cur].box[2] - 10 < bsd_peds_last->p[last].box[2]
                    && p_bsd_out->peds.p[cur].box[2] + 10 > bsd_peds_last->p[last].box[2])
                    index++;
                if (p_bsd_out->peds.p[cur].box[3] - 10 < bsd_peds_last->p[last].box[3]
                    && p_bsd_out->peds.p[cur].box[3] + 10 > bsd_peds_last->p[last].box[3])
                    index++;
                if(index == 4) {
                    p_bsd_out->peds.p[cur] = bsd_peds_last->p[last];
                    //EI_PRINT("rectangle location no change!\n");

                }
            }
        }
    }

    if (bsd_peds_last->p) {
        free(bsd_peds_last->p);
        bsd_peds_last->p = NULL;
        bsd_peds_last->size = 0;
    }
    bsd_peds_last->p =
        malloc(p_bsd_out->peds.size * sizeof(nn_bsd_ped_t));
    if (p_bsd_out->peds.p) {
        memcpy(bsd_peds_last->p, p_bsd_out->peds.p,
            p_bsd_out->peds.size * sizeof(nn_bsd_ped_t));
        bsd_peds_last->size = p_bsd_out->peds.size;
    }
}


EI_VOID *update_bsd_rectangle_proc(EI_VOID *p)
{
    EI_U32 i; 
    RGN_ATTR_S stRegion[BSD_RGN_TOTAL_NUM] = {{0}};
    RGN_HANDLE Handle[BSD_RGN_TOTAL_NUM];
    RGN_CHN_ATTR_S stRgnChnAttr[BSD_RGN_TOTAL_NUM] = {{0}};
    rectangle_info_s cars_rectangle[CAR_RECTANGLEEX_RGN_NUM] = {{0}};   
    rectangle_info_s peds_rectangle[PED_RECTANGLEEX_RGN_NUM] = {{0}};   
    nn_bsd_out_t *p_bsd_out;
    nn_bsd_cars_t bsd0_cars_last = {0};
    nn_bsd_cars_t bsd1_cars_last = {0};
    nn_bsd_peds_t bsd0_peds_last = {0};
    nn_bsd_peds_t bsd1_peds_last = {0};
    region_info_s *region_info;
    region_info = (region_info_s *)p;

	prctl(PR_SET_NAME, "update_bsd_rect");

    if (region_info->nn_type == NN_BSD0) {
        p_bsd_out = p_bsd0_out;
    } else {
        p_bsd_out = p_bsd1_out;
    }
    
    for (i = 0; i < BSD_RGN_TOTAL_NUM; i++)
        stRegion[i].enType = RECTANGLEEX_RGN;

    for (i = 0; i < BSD_RGN_TOTAL_NUM; i++) {
        Handle[i] = get_handle_id();
    }

    while (EI_TRUE == region_info->bThreadStart) {
        if (p_bsd_out == NULL) {
            usleep(200*1000);
            continue;
        }

        if (region_info->nn_type == NN_BSD0) {
            pthread_mutex_lock(&axframe_info[NN_BSD0].frame_stLock);
            bsd_car_rectangle_reduce_shake(p_bsd_out, &bsd0_cars_last);
            bsd_ped_rectangle_reduce_shake(p_bsd_out, &bsd0_peds_last);
        } else {
            pthread_mutex_lock(&axframe_info[NN_BSD1].frame_stLock);
            bsd_car_rectangle_reduce_shake(p_bsd_out, &bsd1_cars_last);
            bsd_ped_rectangle_reduce_shake(p_bsd_out, &bsd1_peds_last);
        }
  
        for (i = 0; i < BSD_RGN_TOTAL_NUM; i++) {
            stRgnChnAttr[i].bShow = EI_FALSE;
        }

        for (i = 0; i < p_bsd_out->cars.size && i < CAR_RECTANGLEEX_RGN_NUM; i++) {
            cars_rectangle[i].loc_x = p_bsd_out->cars.p[i].box[0] * region_info->stSize.u32Width / 1280;
            cars_rectangle[i].loc_y = p_bsd_out->cars.p[i].box[1] * region_info->stSize.u32Height / 720;
            cars_rectangle[i].width = 
                (p_bsd_out->cars.p[i].box[2] - p_bsd_out->cars.p[i].box[0]) * region_info->stSize.u32Width / 1280;
            cars_rectangle[i].hight = 
                (p_bsd_out->cars.p[i].box[3] - p_bsd_out->cars.p[i].box[1]) * region_info->stSize.u32Height / 720;

            stRgnChnAttr[i].bShow = EI_TRUE;
            stRgnChnAttr[i].enType = RECTANGLEEX_RGN;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.u32BorderSize = 4;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.s32X = cars_rectangle[i].loc_x;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.s32Y = cars_rectangle[i].loc_y;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.u32Height = cars_rectangle[i].hight;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.u32Width = cars_rectangle[i].width;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.u32Color = 0xff00ff00;

            EI_MI_RGN_Create(Handle[i], &stRegion[i]);
            EI_MI_RGN_SetChnAttr(Handle[i], &region_info->stRgnChn, &stRgnChnAttr[i]);
            EI_MI_RGN_AddToChn(Handle[i], &region_info->stRgnChn, &stRgnChnAttr[i]);
            
        }
        
        for (i = CAR_RECTANGLEEX_RGN_NUM; 
             i < (p_bsd_out->peds.size + CAR_RECTANGLEEX_RGN_NUM)
                && i < CAR_RECTANGLEEX_RGN_NUM + PED_RECTANGLEEX_RGN_NUM; 
             i++) {
            int ped_index = i - CAR_RECTANGLEEX_RGN_NUM;
            peds_rectangle[ped_index].loc_x = p_bsd_out->peds.p[ped_index].box[0] * region_info->stSize.u32Width / 1280;
            peds_rectangle[ped_index].loc_y = p_bsd_out->peds.p[ped_index].box[1] * region_info->stSize.u32Height / 720;
            peds_rectangle[ped_index].width = 
                (p_bsd_out->peds.p[ped_index].box[2] - p_bsd_out->peds.p[ped_index].box[0]) * region_info->stSize.u32Width / 1280;
            peds_rectangle[ped_index].hight = 
                (p_bsd_out->peds.p[ped_index].box[3] - p_bsd_out->peds.p[ped_index].box[1]) * region_info->stSize.u32Height / 720;

            stRgnChnAttr[i].bShow = EI_TRUE;
            stRgnChnAttr[i].enType = RECTANGLEEX_RGN;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.u32BorderSize = 4;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.s32X = peds_rectangle[ped_index].loc_x;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.s32Y = peds_rectangle[ped_index].loc_y;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.u32Height = peds_rectangle[ped_index].hight;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.u32Width = peds_rectangle[ped_index].width;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.u32Color = 0xffff0000;

            EI_MI_RGN_Create(Handle[i], &stRegion[i]);
            EI_MI_RGN_SetChnAttr(Handle[i], &region_info->stRgnChn, &stRgnChnAttr[i]);
            EI_MI_RGN_AddToChn(Handle[i], &region_info->stRgnChn, &stRgnChnAttr[i]);
            
        }
        if (region_info->nn_type == NN_BSD0) {
            pthread_mutex_unlock(&axframe_info[NN_BSD0].frame_stLock);
        } else {
            pthread_mutex_unlock(&axframe_info[NN_BSD1].frame_stLock);
        }
        
        usleep(50 * 1000);
        for (i = 0; i < BSD_RGN_TOTAL_NUM; i++) {
            if (stRgnChnAttr[i].bShow == EI_TRUE) {
                EI_MI_RGN_DelFromChn(Handle[i], &region_info->stRgnChn);
                EI_MI_RGN_Destroy(Handle[i]);
            }
        }
    }

    return NULL;
}

static void dms_face_rectangle_reduce_shake(nn_dms_out_t *p_dms_out, nn_dms_faces_t *dms_faces_last)
{
    if (dms_faces_last->size > 0 && p_dms_out->faces.size > 0) {
        for (int last = 0; last < dms_faces_last->size; last++) {
            for (int cur = 0; cur < p_dms_out->faces.size; cur++) {
                int index = 0;
                if (p_dms_out->faces.p[cur].box[0] - 10 < dms_faces_last->p[last].box[0]
                    && p_dms_out->faces.p[cur].box[0] + 10 > dms_faces_last->p[last].box[0])
                    index++;
                if (p_dms_out->faces.p[cur].box[1] - 10 < dms_faces_last->p[last].box[1]
                    && p_dms_out->faces.p[cur].box[1] + 10 > dms_faces_last->p[last].box[1])
                    index++;
                if (p_dms_out->faces.p[cur].box[2] - 10 < dms_faces_last->p[last].box[2]
                    && p_dms_out->faces.p[cur].box[2] + 10 > dms_faces_last->p[last].box[2])
                    index++;
                if (p_dms_out->faces.p[cur].box[3] - 10 < dms_faces_last->p[last].box[3]
                    && p_dms_out->faces.p[cur].box[3] + 10 > dms_faces_last->p[last].box[3])
                    index++;
                if(index == 4) {
                    p_dms_out->faces.p[cur] = dms_faces_last->p[last];
                    //EI_PRINT("rectangle location no change!\n");
                }
            }
        }
    }

    if (dms_faces_last->p) {
        free(dms_faces_last->p);
        dms_faces_last->p = NULL;
        dms_faces_last->size = 0;
    }
    dms_faces_last->p =
        malloc(p_dms_out->faces.size * sizeof(nn_dms_face_t));
    if (p_dms_out->faces.p) {
        memcpy(dms_faces_last->p, p_dms_out->faces.p,
            p_dms_out->faces.size * sizeof(nn_dms_face_t));
        dms_faces_last->size = p_dms_out->faces.size;
    }
}


EI_VOID *update_dms_rectangle_proc(EI_VOID *p)
{
    EI_U32 i; 
	EI_U32 index = 0;
    RGN_ATTR_S stRegion[DMS_RGN_TOTAL_NUM] = {{0}};
    RGN_HANDLE Handle[DMS_RGN_TOTAL_NUM];
    RGN_CHN_ATTR_S stRgnChnAttr[DMS_RGN_TOTAL_NUM] = {{0}};
    rectangle_info_s face_rectangle[DMS_RGN_TOTAL_NUM] = {{0}};
    nn_dms_faces_t dms_faces_last = {0};
    region_info_s *region_info;
    region_info = (region_info_s *)p;
	
	prctl(PR_SET_NAME, "update_dms_rect");
    
    for (i = 0; i < DMS_RGN_TOTAL_NUM; i++)
        stRegion[i].enType = RECTANGLEEX_RGN;

    for (i = 0; i < DMS_RGN_TOTAL_NUM; i++) {
        Handle[i] = get_handle_id();
    }

    while (EI_TRUE == region_info->bThreadStart) {
        if (p_dms_out == NULL) {
            usleep(200*1000);
            continue;
        }

        pthread_mutex_lock(&axframe_info[NN_DMS].frame_stLock);
        dms_face_rectangle_reduce_shake(p_dms_out, &dms_faces_last);

        for (i = 0; i < DMS_RGN_TOTAL_NUM; i++) {
            stRgnChnAttr[i].bShow = EI_FALSE;
        }

        for (int i = 0; i < p_dms_out->faces.size && i < FACE_RECTANGLEEX_RGN_NUM; i++) {
            face_rectangle[i].loc_x = p_dms_out->faces.p[i].box[0] * region_info->stSize.u32Width / 1280;
            face_rectangle[i].loc_y = p_dms_out->faces.p[i].box[1] * region_info->stSize.u32Height / 720;
            face_rectangle[i].width = 
                (p_dms_out->faces.p[i].box[2] - p_dms_out->faces.p[i].box[0]) * region_info->stSize.u32Width / 1280;
            face_rectangle[i].hight = 
                (p_dms_out->faces.p[i].box[3] - p_dms_out->faces.p[i].box[1]) * region_info->stSize.u32Height / 720;

            stRgnChnAttr[i].bShow = EI_TRUE;
            stRgnChnAttr[i].enType = RECTANGLEEX_RGN;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.u32BorderSize = 4;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.s32X = face_rectangle[i].loc_x;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.s32Y = face_rectangle[i].loc_y;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.u32Height = face_rectangle[i].hight;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.stRect.u32Width = face_rectangle[i].width;
            stRgnChnAttr[i].unChnAttr.stRectangleExChn.u32Color = 0xff00ff00;
			/*
			if (i == p_dms_out->faces.driver_idx)
				stRgnChnAttr[i].unChnAttr.stRectangleExChn.u32Color = 0xff00ff00;
			else
				stRgnChnAttr[i].unChnAttr.stRectangleExChn.u32Color = 0xffffff80;
			*/
            EI_MI_RGN_Create(Handle[i], &stRegion[i]);
            EI_MI_RGN_SetChnAttr(Handle[i], &region_info->stRgnChn, &stRgnChnAttr[i]);
            EI_MI_RGN_AddToChn(Handle[i], &region_info->stRgnChn, &stRgnChnAttr[i]);
        }
		/*
		index = index + p_dms_out->faces.size;
		for (int cnt_drinks = 0; cnt_drinks < p_dms_out->obj_boxes[0].size && cnt_drinks < DRINKS_RECTANGLEEX_RGN_NUM; cnt_drinks++) {
	        int loc_x0, loc_y0, loc_x1, loc_y1;
	        loc_x0 = p_dms_out->obj_boxes[0].p[cnt_drinks].box[0] * region_info->stSize.u32Width / 1280;
	        loc_y0 = p_dms_out->obj_boxes[0].p[cnt_drinks].box[1] * region_info->stSize.u32Height / 720;
	        loc_x1 = p_dms_out->obj_boxes[0].p[cnt_drinks].box[2] * region_info->stSize.u32Width / 1280;
	        loc_y1 = p_dms_out->obj_boxes[0].p[cnt_drinks].box[3] * region_info->stSize.u32Height / 720;
			
			stRgnChnAttr[index+cnt_drinks].bShow = EI_TRUE;
            stRgnChnAttr[index+cnt_drinks].enType = RECTANGLEEX_RGN;
	        stRgnChnAttr[index+cnt_drinks].unChnAttr.stRectangleExChn.stRect.s32X = loc_x0;
			stRgnChnAttr[index+cnt_drinks].unChnAttr.stRectangleExChn.stRect.s32Y = loc_y0;
			stRgnChnAttr[index+cnt_drinks].unChnAttr.stRectangleExChn.stRect.u32Height = loc_y1-loc_y0;
			stRgnChnAttr[index+cnt_drinks].unChnAttr.stRectangleExChn.stRect.u32Width = loc_x1-loc_x0;
			stRgnChnAttr[index+cnt_drinks].unChnAttr.stRectangleExChn.u32BorderSize = 4;
			stRgnChnAttr[index+cnt_drinks].unChnAttr.stRectangleExChn.u32Color = 0xffff0000;
			//EI_MI_RGN_SetChnAttr(Handle[i], &region_info.stRgnChn, &region_info.stRgnChnAttr[index+cnt_drinks]);

			EI_MI_RGN_Create(Handle[index+cnt_drinks], &stRegion[index+cnt_drinks]);
            EI_MI_RGN_SetChnAttr(Handle[index+cnt_drinks], &region_info->stRgnChn, &stRgnChnAttr[index+cnt_drinks]);
            EI_MI_RGN_AddToChn(Handle[index+cnt_drinks], &region_info->stRgnChn, &stRgnChnAttr[index+cnt_drinks]);
	    }
		
		index = index + p_dms_out->obj_boxes[0].size;
	    for (int cnt_phones = 0; cnt_phones < p_dms_out->obj_boxes[1].size && cnt_phones < PHONES_RECTANGLEEX_RGN_NUM; cnt_phones++) {
	        int loc_x0, loc_y0, loc_x1, loc_y1;
	        loc_x0 = p_dms_out->obj_boxes[1].p[cnt_phones].box[0];
	        loc_y0 = p_dms_out->obj_boxes[1].p[cnt_phones].box[1];
	        loc_x1 = p_dms_out->obj_boxes[1].p[cnt_phones].box[2];
	        loc_y1 = p_dms_out->obj_boxes[1].p[cnt_phones].box[3];
			
	        stRgnChnAttr[index+cnt_phones].bShow = EI_TRUE;
            stRgnChnAttr[index+cnt_phones].enType = RECTANGLEEX_RGN;
	        stRgnChnAttr[index+cnt_phones].unChnAttr.stRectangleExChn.stRect.s32X = loc_x0;
			stRgnChnAttr[index+cnt_phones].unChnAttr.stRectangleExChn.stRect.s32Y = loc_y0;
			stRgnChnAttr[index+cnt_phones].unChnAttr.stRectangleExChn.stRect.u32Height = loc_y1-loc_y0;
			stRgnChnAttr[index+cnt_phones].unChnAttr.stRectangleExChn.stRect.u32Width = loc_x1-loc_x0;
			stRgnChnAttr[index+cnt_phones].unChnAttr.stRectangleExChn.u32BorderSize = 4;
			stRgnChnAttr[index+cnt_phones].unChnAttr.stRectangleExChn.u32Color = 0xffff0000;
			//EI_MI_RGN_SetChnAttr(drawframe_info.Handle[0], &drawframe_info.stRgnChn, &drawframe_info.stRgnChnAttr[0]);
			
			EI_MI_RGN_Create(Handle[index+cnt_phones], &stRegion[index+cnt_phones]);
            EI_MI_RGN_SetChnAttr(Handle[index+cnt_phones], &region_info->stRgnChn, &stRgnChnAttr[index+cnt_phones]);
            EI_MI_RGN_AddToChn(Handle[index+cnt_phones], &region_info->stRgnChn, &stRgnChnAttr[index+cnt_phones]);
	    }
		index = index + p_dms_out->obj_boxes[1].size;
	    for (int cnt_smokes = 0; cnt_smokes < p_dms_out->obj_boxes[2].size && cnt_smokes < SMOKES_RECTANGLEEX_RGN_NUM; cnt_smokes++) {
	        int loc_x0, loc_y0, loc_x1, loc_y1;
	        loc_x0 = p_dms_out->obj_boxes[2].p[cnt_smokes].box[0];
	        loc_y0 = p_dms_out->obj_boxes[2].p[cnt_smokes].box[1];
	        loc_x1 = p_dms_out->obj_boxes[2].p[cnt_smokes].box[2];
	        loc_y1 = p_dms_out->obj_boxes[2].p[cnt_smokes].box[3];
			
	        stRgnChnAttr[index+cnt_smokes].bShow = EI_TRUE;
            stRgnChnAttr[index+cnt_smokes].enType = RECTANGLEEX_RGN;
	        stRgnChnAttr[index+cnt_smokes].unChnAttr.stRectangleExChn.stRect.s32X = loc_x0;
			stRgnChnAttr[index+cnt_smokes].unChnAttr.stRectangleExChn.stRect.s32Y = loc_y0;
			stRgnChnAttr[index+cnt_smokes].unChnAttr.stRectangleExChn.stRect.u32Height = loc_y1-loc_y0;
			stRgnChnAttr[index+cnt_smokes].unChnAttr.stRectangleExChn.stRect.u32Width = loc_x1-loc_x0;
			stRgnChnAttr[index+cnt_smokes].unChnAttr.stRectangleExChn.u32BorderSize = 4;
			stRgnChnAttr[index+cnt_smokes].unChnAttr.stRectangleExChn.u32Color = 0xffff0000;
			//EI_MI_RGN_SetChnAttr(drawframe_info.Handle[0], &drawframe_info.stRgnChn, &drawframe_info.stRgnChnAttr[0]);
			
			EI_MI_RGN_Create(Handle[index+cnt_smokes], &stRegion[index+cnt_smokes]);
            EI_MI_RGN_SetChnAttr(Handle[index+cnt_smokes], &region_info->stRgnChn, &stRgnChnAttr[index+cnt_smokes]);
            EI_MI_RGN_AddToChn(Handle[index+cnt_smokes], &region_info->stRgnChn, &stRgnChnAttr[index+cnt_smokes]);
	    }
		*/
        pthread_mutex_unlock(&axframe_info[NN_DMS].frame_stLock);
        
        usleep(50 * 1000);

        for (i = 0; i < DMS_RGN_TOTAL_NUM; i++) {
            if (stRgnChnAttr[i].bShow == EI_TRUE) {
                EI_MI_RGN_DelFromChn(Handle[i], &region_info->stRgnChn);
                EI_MI_RGN_Destroy(Handle[i]);
            }
        }
    }

    return NULL;
}

static int _start_update_dms_rectangle_proc(region_info_s *region_info)
{
    int ret = EI_SUCCESS;

    region_info->bThreadStart = EI_TRUE;

    ret = pthread_create(&region_info->gs_rgnPid, NULL, update_dms_rectangle_proc,
            (EI_VOID *)region_info);
    if (ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));

    return ret;
}

static int _start_update_adas_rectangle_proc(region_info_s *region_info)
{
    int ret = EI_SUCCESS;

    region_info->bThreadStart = EI_TRUE;

    ret = pthread_create(&region_info->gs_rgnPid, NULL, update_adas_rectangle_proc,
            (EI_VOID *)region_info);
    if (ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));

    return ret;
}

static int _start_update_bsd_rectangle_proc(region_info_s *region_info)
{
    int ret = EI_SUCCESS;;

    region_info->bThreadStart = EI_TRUE;

    ret = pthread_create(&region_info->gs_rgnPid, NULL, update_bsd_rectangle_proc,
            (EI_VOID *)region_info);
    if (ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));

    return ret;
}

static int nn_adas_result_cb(int event, void *data)
{
	if (event == NN_ADAS_DET_DONE) {
	    adas_cnt++;
		int adas_afater_time = osal_get_usec();
	    nn_adas_out_t *curr_data = (nn_adas_out_t *)data;
	    //memcpy(p_adas_out, curr_data, sizeof(nn_adas_out_t));
	    //if (adas_cnt % 100 == 0)
	        //EI_PRINT("nn_adas_result_cb event = %d  adas_cnt = %d\n", event, adas_cnt);
		//EI_PRINT("nn_adas_result_cb time0 = %d time1 = %d time = %d\n", adas_afater_time, adas_before_time, adas_afater_time - adas_before_time);
		
	    pthread_mutex_lock(&axframe_info[NN_ADAS].frame_stLock);
	    /* cars data*/
	    if (p_adas_out->cars.p) {
	        free(p_adas_out->cars.p);
	        p_adas_out->cars.p = NULL;
	        p_adas_out->cars.size = 0;
	    }
	    p_adas_out->cars.p =
	        malloc(curr_data->cars.size * sizeof(nn_adas_car_t));
	    if (p_adas_out->cars.p) {
	        memcpy(p_adas_out->cars.p, curr_data->cars.p,
	            curr_data->cars.size * sizeof(nn_adas_car_t));
	        p_adas_out->cars.size = curr_data->cars.size;
	    }

	    /* peds data */
	    if (p_adas_out->peds.p) {
	        free(p_adas_out->peds.p);
	        p_adas_out->peds.p = NULL;
	        p_adas_out->peds.size = 0;
	    }
	    p_adas_out->peds.p = malloc(curr_data->peds.size * sizeof(nn_adas_ped_t));
	    if (p_adas_out->peds.p) {
	        memcpy(p_adas_out->peds.p, curr_data->peds.p,
	            curr_data->peds.size * sizeof(nn_adas_ped_t));
	        p_adas_out->peds.size = curr_data->peds.size;
	    }
	    /* lanes data */
	    memcpy(&p_adas_out->lanes, &curr_data->lanes, sizeof(nn_adas_lanes_t));

	    pthread_mutex_unlock(&axframe_info[NN_ADAS].frame_stLock);
#if 1
	/*
	    if (curr_data->cars.size) {
	        EI_PRINT("nn_adas_result_cb curr_data->cars.size = %d\n", curr_data->cars.size);
	        for (i=0; i<curr_data->cars.size; i++) {
	            EI_PRINT("curr_data->cars.p[%d].status = %d\n", i, curr_data->cars.p[i].status);
	        }
	    }
	    if (curr_data->peds.size) {
	        EI_PRINT("nn_adas_result_cb curr_data->peds.size = %d\n", curr_data->peds.size);
	        for (i=0; i<curr_data->peds.size; i++) {
	            EI_PRINT("curr_data->peds.p[%d].status = %d\n", i, curr_data->peds.p[i].status);
	        }
	        EI_PRINT("box[0] = %d; box[1] = %d; box[2] = %d; box[3] = %d \n",curr_data->peds.p[0].box[0], curr_data->peds.p[0].box[1],
	                       curr_data->peds.p[0].box[2], curr_data->peds.p[0].box[3]);
	        EI_PRINT("p_adas_out box[0] = %d; box[1] = %d; box[2] = %d; box[3] = %d \n",p_adas_out->peds.p[0].box[0], p_adas_out->peds.p[0].box[1],
	                       p_adas_out->peds.p[0].box[2], p_adas_out->peds.p[0].box[3]);
	    }
	    if (curr_data->lanes.lanes[0].exist)
	        EI_PRINT("nn_adas_result_cb curr_data->lanes.lanes[0].exist = %d\n", curr_data->lanes.lanes[0].exist);
	    if (curr_data->lanes.lanes[1].exist)
	        EI_PRINT("nn_adas_result_cb curr_data->lanes.lanes[1].exist = %d\n", curr_data->lanes.lanes[1].exist);
	    if (curr_data->lanes.lanes[0].status)
	        EI_PRINT("nn_adas_result_cb curr_data->lanes.lanes[0].exist = %d\n", curr_data->lanes.lanes[0].status);
	    if (curr_data->lanes.lanes[1].status)
	        EI_PRINT("nn_adas_result_cb curr_data->lanes.lanes[1].exist = %d\n", curr_data->lanes.lanes[1].status);
	        */
#else
	    int i=	0;
	    /* car warning */
	    for (i = 0; i < curr_data->cars.size; i++) {
	        EI_PRINT("car%d[%d %d %d %d] status:%d dist:%f>>", i,
	            curr_data->cars.p[i].box[0], curr_data->cars.p[i].box[1],
	            curr_data->cars.p[i].box[2], curr_data->cars.p[i].box[3],
	            curr_data->cars.p[i].status, curr_data->cars.p[i].dist);
	    }
	    for (i = 0; i < curr_data->peds.size; i++) {
	        EI_PRINT("pedestrian%d[%d %d %d %d] status:%d dist:%f>>", i,
	            curr_data->peds.p[i].box[0], curr_data->peds.p[i].box[1],
	            curr_data->peds.p[i].box[2], curr_data->peds.p[i].box[3],
	            curr_data->peds.p[i].status, curr_data->peds.p[i].dist);
	    }
	    if (curr_data->lanes.lanes[0].status) {
	        EI_PRINT("left lane>> status:%d type:%d [%d %d] [%d %d] [%d %d] [%d %d] [%d %d] [%d %d]",
	            curr_data->lanes.lanes[0].status, curr_data->lanes.lanes[0].type,
	            curr_data->lanes.lanes[0].pts[0][0], curr_data->lanes.lanes[0].pts[0][1],
	            curr_data->lanes.lanes[0].pts[1][0], curr_data->lanes.lanes[0].pts[1][1],
	            curr_data->lanes.lanes[0].pts[2][0], curr_data->lanes.lanes[0].pts[2][1],
	            curr_data->lanes.lanes[0].pts[3][0], curr_data->lanes.lanes[0].pts[3][1],
	            curr_data->lanes.lanes[0].pts[4][0], curr_data->lanes.lanes[0].pts[4][1],
	            curr_data->lanes.lanes[0].pts[5][0], curr_data->lanes.lanes[0].pts[5][1]);
	    }
	    if (curr_data->lanes.lanes[1].status) {
	        EI_PRINT("right lane>> status:%d type:%d [%d %d] [%d %d] [%d %d] [%d %d] [%d %d] [%d %d]",
	            curr_data->lanes.lanes[1].status, curr_data->lanes.lanes[1].type,
	            curr_data->lanes.lanes[1].pts[0][0], curr_data->lanes.lanes[1].pts[0][1],
	            curr_data->lanes.lanes[1].pts[1][0], curr_data->lanes.lanes[1].pts[1][1],
	            curr_data->lanes.lanes[1].pts[2][0], curr_data->lanes.lanes[1].pts[2][1],
	            curr_data->lanes.lanes[1].pts[3][0], curr_data->lanes.lanes[1].pts[3][1],
	            curr_data->lanes.lanes[1].pts[4][0], curr_data->lanes.lanes[1].pts[4][1],
	            curr_data->lanes.lanes[1].pts[5][0], curr_data->lanes.lanes[1].pts[5][1]);
	    }
#endif
		if (adas_cnt % 10 == 0)
			EI_PRINT("nn_adas_result_cb event = %d  adas_cnt = %d\n", event, adas_cnt);
        sem_post(&axframe_info[NN_ADAS].frame_del_sem);
	} else if (event == NN_ADAS_SELF_CALIBRATE) {
		EI_PRINT("NN_ADAS_SELF_CALIBRATE\n");
	}

    return 0;


}


static int nn_bsd0_result_cb(int event, void *data)
{
	if (event == NN_BSD_DET_DONE) {
	    int i;
	    nn_bsd_out_t *curr_data = (nn_bsd_out_t *)data;
	    bsd_cnt++;
	    pthread_mutex_lock(&axframe_info[NN_BSD0].frame_stLock);
	    if (p_bsd0_out->cars.p) {
	        free(p_bsd0_out->cars.p);
	        p_bsd0_out->cars.p = NULL;
	        p_bsd0_out->cars.size = 0;
	    }
	    p_bsd0_out->cars.p =
	        malloc(curr_data->cars.size * sizeof(nn_bsd_car_t));
	    if (p_bsd0_out->cars.p) {
	        memcpy(p_bsd0_out->cars.p, curr_data->cars.p,
	            curr_data->cars.size * sizeof(nn_bsd_car_t));
	        p_bsd0_out->cars.size = curr_data->cars.size;
	    }
	    /* peds data */
	    if (p_bsd0_out->peds.p) {
	        free(p_bsd0_out->peds.p);
	        p_bsd0_out->peds.p = NULL;
	        p_bsd0_out->peds.size = 0;
	    }
	    p_bsd0_out->peds.p = malloc(curr_data->peds.size * sizeof(nn_bsd_ped_t));
	    if (p_bsd0_out->peds.p) {
	        memcpy(p_bsd0_out->peds.p, curr_data->peds.p,
	            curr_data->peds.size * sizeof(nn_bsd_ped_t));
	        p_bsd0_out->peds.size = curr_data->peds.size;
	    }
	    pthread_mutex_unlock(&axframe_info[NN_BSD0].frame_stLock);

	    if (curr_data->cars.size) {
	        //EI_PRINT("bsd0 curr_data->cars.size %d\n", curr_data->cars.size);
	        for (i=0; i<curr_data->cars.size; i++) {
	            //EI_PRINT("bsd0 curr_data->cars.p[%d].status = %d\n", i, curr_data->cars.p[i].status);
	        }
	    }
	    if (curr_data->peds.size) {
	        //EI_PRINT("bsd0 curr_data->peds.size %d\n", curr_data->peds.size);
	        for (i=0; i<curr_data->peds.size; i++) {
	            //EI_PRINT("bsd0 curr_data->peds.p[%d].status = %d\n", i, curr_data->peds.p[i].status);
			}
	    }
		if (bsd_cnt % 10 == 0)
			EI_PRINT("nn_bsd0_result_cb event = %d  bsd_cnt = %d\n", event, bsd_cnt);
        sem_post(&axframe_info[NN_BSD0].frame_del_sem);
	} else if (event == NN_BSD_CALIB_DONE) {
		EI_PRINT("NN_BSD_CALIB_DONE0\n");
	}

    return 0;
}

static int nn_bsd1_result_cb(int event, void *data)
{
	if (event == NN_BSD_DET_DONE) {
	    int i;
	    nn_bsd_out_t *curr_data = (nn_bsd_out_t *)data;
	    bsd1_cnt++;
	    pthread_mutex_lock(&axframe_info[NN_BSD1].frame_stLock);
	    if (p_bsd1_out->cars.p) {
	        free(p_bsd1_out->cars.p);
	        p_bsd1_out->cars.p = NULL;
	        p_bsd1_out->cars.size = 0;
	    }
	    p_bsd1_out->cars.p =
	        malloc(curr_data->cars.size * sizeof(nn_bsd_car_t));
	    if (p_bsd1_out->cars.p) {
	        memcpy(p_bsd1_out->cars.p, curr_data->cars.p,
	            curr_data->cars.size * sizeof(nn_bsd_car_t));
	        p_bsd1_out->cars.size = curr_data->cars.size;
	    }
	    /* peds data */
	    if (p_bsd1_out->peds.p) {
	        free(p_bsd1_out->peds.p);
	        p_bsd1_out->peds.p = NULL;
	        p_bsd1_out->peds.size = 0;
	    }
	    p_bsd1_out->peds.p = malloc(curr_data->peds.size * sizeof(nn_bsd_ped_t));
	    if (p_bsd1_out->peds.p) {
	        memcpy(p_bsd1_out->peds.p, curr_data->peds.p,
	            curr_data->peds.size * sizeof(nn_bsd_ped_t));
	        p_bsd1_out->peds.size = curr_data->peds.size;
	    }
	    pthread_mutex_unlock(&axframe_info[NN_BSD1].frame_stLock);

	    if (curr_data->cars.size) {
	        //EI_PRINT("bsd1 curr_data->cars.size %d\n", curr_data->cars.size);
	        for (i=0; i<curr_data->cars.size; i++) {
	            //EI_PRINT("bsd1 curr_data->cars.p[%d].status = %d\n", i, curr_data->cars.p[i].status);
	        }
	    }
	    if (curr_data->peds.size) {
	        //EI_PRINT("bsd1 curr_data->peds.size %d\n", curr_data->peds.size);
	        for (i=0; i<curr_data->peds.size; i++) {
	            //EI_PRINT("bsd1 curr_data->peds.p[%d].status = %d\n", i, curr_data->peds.p[i].status);
	        }
	    }

		if (bsd1_cnt % 10 == 0)
			EI_PRINT("nn_bsd1_result_cb event = %d  bsd_cnt = %d\n", event, bsd1_cnt);
        sem_post(&axframe_info[NN_BSD1].frame_del_sem);
	} else if (event == NN_BSD_CALIB_DONE) {
		EI_PRINT("NN_BSD_CALIB_DONE1\n");
	}

    return 0;
}


int dms_warning(nn_dms_out_t *data)
{
	nn_dms_face_t *face = NULL;
	nn_dms_state_t *state = NULL;
	int64_t cur_time = osal_get_msec();

	if (data->faces.size > 0) {
		face = &data->faces.p[data->faces.driver_idx];
    	state = &face->face_attr.dms_state;
		if (state->drink) {
			if (warning_info.last_warn_status != LB_WARNING_DMS_DRINK ||
				(cur_time - warning_info.last_warn_time) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_DRINK, NULL, 0);
				warning_info.last_warn_time = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_DRINK;
			}
		} else if (state->call) {
			if (warning_info.last_warn_status != LB_WARNING_DMS_CALL ||
				(cur_time - warning_info.last_warn_time) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_CALL, NULL, 0);
				warning_info.last_warn_time = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_CALL;
			}
		} else if (state->smoke) {
			if (warning_info.last_warn_status != LB_WARNING_DMS_SMOKE ||
				(cur_time - warning_info.last_warn_time) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_SMOKE, NULL, 0);
				warning_info.last_warn_time = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_SMOKE;
			}
		} else if (state->yawn) {
			if (warning_info.last_warn_status != LB_WARNING_DMS_YAWN ||
				(cur_time - warning_info.last_warn_time) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_YAWN, NULL, 0);
				warning_info.last_warn_time = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_YAWN;
			}
		} else if (state->eyeclosed) {
			if (warning_info.last_warn_status != LB_WARNING_DMS_REST ||
				(cur_time - warning_info.last_warn_time) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_REST, NULL, 0);
				warning_info.last_warn_time = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_REST;
			}
		} else if (state->lookaround) {
			if (warning_info.last_warn_status != LB_WARNING_DMS_ATTATION ||
				(cur_time - warning_info.last_warn_time) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_ATTATION, NULL, 0);
				warning_info.last_warn_time = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_ATTATION;
			}
		} else if (state->lookup) {
			if (warning_info.last_warn_status != LB_WARNING_DMS_ATTATION ||
				(cur_time - warning_info.last_warn_time) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_ATTATION, NULL, 0);
				warning_info.last_warn_time = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_ATTATION;
			}
	    } else if (state->lookdown) {
			if (warning_info.last_warn_status != LB_WARNING_DMS_ATTATION ||
				(cur_time - warning_info.last_warn_time) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_ATTATION, NULL, 0);
				warning_info.last_warn_time = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_ATTATION;
			}
	    } else if (face->face_attr.red_resist_glasses) {
			if (warning_info.last_warn_status != LB_WARNING_DMS_INFRARED_BLOCK ||
				(cur_time - warning_info.last_warn_time) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_INFRARED_BLOCK, NULL, 0);
				warning_info.last_warn_time = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_INFRARED_BLOCK;
			}
		}
	} else {
		if (data->cover_state) {
			if ((cur_time - warning_info.last_warn_time) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_CAMERA_COVER, NULL, 0);
				warning_info.last_warn_time = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_CAMERA_COVER;
			} 
		} else {
				if((cur_time - warning_info.last_warn_time) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_NO_FACE, NULL, 0);
					warning_info.last_warn_time = cur_time;
					warning_info.last_warn_status = LB_WARNING_DMS_NO_FACE;
				}
		}
	}
	
	return 0;
}

/**
 * dms_result_cb
 *
 * This function use to deal dms callback results
 *
 * Returns 0
 */
static int dms_result_cb(int event, void *data)
{
	if (event == NN_DMS_DET_DONE) {
	    nn_dms_out_t *out = (nn_dms_out_t*)data;
	    dms_cnt++;
	    //EI_PRINT("dms_result_cb dms_cnt = %d \n", dms_cnt);

	    pthread_mutex_lock(&axframe_info[NN_DMS].frame_stLock);
	    if (p_dms_out->faces.p) {
	        free(p_dms_out->faces.p);
	        p_dms_out->faces.p = NULL;
	        p_dms_out->faces.size = 0;
	    }
	    p_dms_out->faces.p =
	        malloc(out->faces.size * sizeof(nn_dms_face_t));
	    if (p_dms_out->faces.p) {
	        memcpy(p_dms_out->faces.p, out->faces.p,
	            out->faces.size * sizeof(nn_dms_face_t));
	        p_dms_out->faces.size = out->faces.size;
	    }
	    p_dms_out->faces.driver_idx = out->faces.driver_idx;
	    p_dms_out->cover_state = out->cover_state;
	    for (int i = 0; i < 3; i++) {
	        if (p_dms_out->obj_boxes[i].p) {
	            free(p_dms_out->obj_boxes[i].p);
	            p_dms_out->obj_boxes[i].p = NULL;
	            p_dms_out->obj_boxes[i].size = 0;
	        }
	        if (out->obj_boxes[i].size == 0)
	            continue;
	        p_dms_out->obj_boxes[i].p =
	            malloc(sizeof(nn_dmsobj_box_t) * out->obj_boxes[i].size);
	        if (p_dms_out->obj_boxes[i].p) {
	            memcpy(p_dms_out->obj_boxes[i].p, out->obj_boxes[i].p,
	                sizeof(nn_dmsobj_box_t) * out->obj_boxes[i].size);
	            p_dms_out->obj_boxes[i].size = out->obj_boxes[i].size;
	        }
	    }
	    pthread_mutex_unlock(&axframe_info[NN_DMS].frame_stLock);

		dms_warning(out);
	    
	    
		if (dms_cnt % 10 == 0)
			EI_PRINT("dms_result_cb dms_cnt = %d \n", dms_cnt);
        sem_post(&axframe_info[NN_DMS].frame_del_sem);
    } else if (event == NN_DMS_AUTO_CALIBRATE_DONE) {
		EI_PRINT("NN_DMS_DET_DONE\n");
	}

    return 0;
}

int nn_adas_start(VIDEO_FRAME_INFO_S *frame, void *nn_hdl, FILE *fout)
{
    EI_S32 s32Ret;
    static nn_adas_in_t m_adas_in;

    memset(&m_adas_in, 0, sizeof(nn_adas_in_t));
    m_adas_in.img.y_phy = (void *)frame->stVFrame.u64PlanePhyAddr[0];
    m_adas_in.img.y = (void *)frame->stVFrame.ulPlaneVirAddr[0];
    m_adas_in.img.uv_phy = (void *)frame->stVFrame.u64PlanePhyAddr[1];
    m_adas_in.img.uv = (void *)frame->stVFrame.ulPlaneVirAddr[1];
    m_adas_in.img.width = 1280;
    m_adas_in.img.height = 720;
    m_adas_in.img.format = YUV420_SP;
    m_adas_in.gps = 60;
    m_adas_in.carped_en = 1;
    m_adas_in.lane_en = 1;

    if(nn_hdl != NULL){
        s32Ret = nn_adas_cmd(nn_hdl, NN_ADAS_SET_DATA, &m_adas_in);
        if (s32Ret != EI_SUCCESS) {
            EI_PRINT("nn_adas_cmd failed!\n");
        }
    }

#if 0
    EI_PRINT("save yuv buffer sizey %d, stride %d %d %d\n", frame->stVFrame.u32PlaneSizeValid[0], frame->stVFrame.u32PlaneStride[0], frame->stCommFrameInfo.u32Width, frame->stCommFrameInfo.u32Height);
    EI_PRINT("save yuv buffer sizeuv %d, u32PlaneNum %d\n", frame->stVFrame.u32PlaneSizeValid[1], frame->stVFrame.u32PlaneNum);    
    EI_PRINT("save yuv buffer addr %llx %llx %llx\n",frame->stVFrame.u64PlanePhyAddr[0], 
            frame->stVFrame.u64PlanePhyAddr[1], frame->stVFrame.u64PlanePhyAddr[2]);
    EI_PRINT("save yuv buffer virtual addr %x %x %x\n",frame->stVFrame.ulPlaneVirAddr[0], 
            frame->stVFrame.ulPlaneVirAddr[1], frame->stVFrame.ulPlaneVirAddr[2]);
    EI_PRINT("save yuv buffer ID %x\n",frame->u32BufferId);
#endif

#ifdef SAVE_AX_YUV_SP
    for (int i = 0; i < frame->stVFrame.u32PlaneNum; i++){
        if (fout) {
            if (m_adas_in.img.format == YUV420_SP){
                if (i == 0) {
                    fwrite((void *)frame->stVFrame.ulPlaneVirAddr[i], 1,  m_adas_in.img.width*m_adas_in.img.height, fout);
                } else {
                    fwrite((void *)frame->stVFrame.ulPlaneVirAddr[i], 1,  m_adas_in.img.width*m_adas_in.img.height/2, fout);
                }
            } else {
                fwrite((void *)frame->stVFrame.ulPlaneVirAddr[i], 1,  frame->stVFrame.u32PlaneSizeValid[i], fout);
            }
        }
    }
#endif
    return 0;
}

int nn_bsd0_start(VIDEO_FRAME_INFO_S *frame, void *nn_hdl, FILE *fout)
{
    EI_S32 s32Ret;
    static nn_bsd_in_t m_bsd_in;
    
    memset(&m_bsd_in, 0, sizeof(nn_bsd_in_t));
    m_bsd_in.img.y_phy = (void *)frame->stVFrame.u64PlanePhyAddr[0];
    m_bsd_in.img.y = (void *)frame->stVFrame.ulPlaneVirAddr[0];
    m_bsd_in.img.uv_phy = (void *)frame->stVFrame.u64PlanePhyAddr[1];
    m_bsd_in.img.uv = (void *)frame->stVFrame.ulPlaneVirAddr[1];
    m_bsd_in.img.width = 1280;
    m_bsd_in.img.height = 720;
    m_bsd_in.img.format = YUV420_SP;
    m_bsd_in.gps = 60;
    m_bsd_in.warn_param.warn_speed_thresh[0] = 0;
    m_bsd_in.warn_param.warn_speed_thresh[1] = 0;
    m_bsd_in.warn_param.warn_speed_thresh[2] = 0;
    m_bsd_in.warn_param.warn_speed_thresh[3] = 0;
	m_bsd_in.warn_param.gps_thresh[0] = 0;
	m_bsd_in.warn_param.gps_thresh[1] = 0;
	m_bsd_in.warn_param.gps_thresh[2] = 0;
	m_bsd_in.warn_param.gps_thresh[3] = 0;
	m_bsd_in.warn_param.filter_time = 0;

#if 0
    EI_PRINT("save yuv buffer sizey %d, stride %d %d %d\n", frame->stVFrame.u32PlaneSizeValid[0], frame->stVFrame.u32PlaneStride[0], frame->stCommFrameInfo.u32Width, frame->stCommFrameInfo.u32Height);
    EI_PRINT("save yuv buffer sizeuv %d, u32PlaneNum %d\n", frame->stVFrame.u32PlaneSizeValid[1], frame->stVFrame.u32PlaneNum);
    EI_PRINT("save yuv buffer addr %llx %llx %llx\n",frame->stVFrame.u64PlanePhyAddr[0], 
            frame->stVFrame.u64PlanePhyAddr[1], frame->stVFrame.u64PlanePhyAddr[2]);
    EI_PRINT("mmap yuv buffer virtual addr %lx %lx %lx\n",frame->stVFrame.ulPlaneVirAddr[0], 
            frame->stVFrame.ulPlaneVirAddr[1], frame->stVFrame.ulPlaneVirAddr[2]);
#endif

    if(nn_hdl != NULL){
        s32Ret = nn_bsd_cmd(nn_hdl, NN_BSD_SET_DATA, &m_bsd_in);
        if (s32Ret != EI_SUCCESS) {
            EI_PRINT("bsd0 nn_bsd_cmd failed!\n");
        }
    }

#ifdef SAVE_AX_YUV_SP
    for (int i = 0; i < frame->stVFrame.u32PlaneNum; i++)
    {
        if (fout) {
            if (m_bsd_in.img.format == YUV420_SP){
                if (i == 0) {
                    fwrite((void *)frame->stVFrame.ulPlaneVirAddr[i], 1,  m_bsd_in.img.width*m_bsd_in.img.height, fout);
                } else {
                    fwrite((void *)frame->stVFrame.ulPlaneVirAddr[i], 1,  m_bsd_in.img.width*m_bsd_in.img.height/2, fout);
                }
            } else {
                fwrite((void *)frame->stVFrame.ulPlaneVirAddr[i], 1,  frame->stVFrame.u32PlaneSizeValid[i], fout);
            } 
        }
    }
#endif

    return 0;
}

int nn_bsd1_start(VIDEO_FRAME_INFO_S *frame, void *nn_hdl, FILE *fout)
{
    EI_S32 s32Ret;
    static nn_bsd_in_t m_bsd_in;
    
    memset(&m_bsd_in, 0, sizeof(nn_bsd_in_t));
    m_bsd_in.img.y_phy = (void *)frame->stVFrame.u64PlanePhyAddr[0];
    m_bsd_in.img.y = (void *)frame->stVFrame.ulPlaneVirAddr[0];
    m_bsd_in.img.uv_phy = (void *)frame->stVFrame.u64PlanePhyAddr[1];
    m_bsd_in.img.uv = (void *)frame->stVFrame.ulPlaneVirAddr[1];
    m_bsd_in.img.width = 1280;
    m_bsd_in.img.height = 720;
    m_bsd_in.img.format = YUV420_SP;
    m_bsd_in.gps = 60;
    m_bsd_in.warn_param.warn_speed_thresh[0] = 0;
    m_bsd_in.warn_param.warn_speed_thresh[1] = 0;
    m_bsd_in.warn_param.warn_speed_thresh[2] = 0;
    m_bsd_in.warn_param.warn_speed_thresh[3] = 0;
	m_bsd_in.warn_param.gps_thresh[0] = 0;
	m_bsd_in.warn_param.gps_thresh[1] = 0;
	m_bsd_in.warn_param.gps_thresh[2] = 0;
	m_bsd_in.warn_param.gps_thresh[3] = 0;
	m_bsd_in.warn_param.filter_time = 0;
    
#if 0
    EI_PRINT("save yuv buffer sizey %d, stride %d %d %d\n", frame->stVFrame.u32PlaneSizeValid[0], frame->stVFrame.u32PlaneStride[0], frame->stCommFrameInfo.u32Width, frame->stCommFrameInfo.u32Height);
    EI_PRINT("save yuv buffer sizeuv %d, u32PlaneNum %d\n", frame->stVFrame.u32PlaneSizeValid[1], frame->stVFrame.u32PlaneNum);
    EI_PRINT("save yuv buffer addr %llx %llx %llx\n",frame->stVFrame.u64PlanePhyAddr[0], 
            frame->stVFrame.u64PlanePhyAddr[1], frame->stVFrame.u64PlanePhyAddr[2]);

    EI_PRINT("mmap yuv buffer virtual addr %x %x %x\n",frame->stVFrame.ulPlaneVirAddr[0], 
            frame->stVFrame.ulPlaneVirAddr[1], frame->stVFrame.ulPlaneVirAddr[2]);
#endif

    if(nn_hdl != NULL){
        s32Ret = nn_bsd_cmd(nn_hdl, NN_BSD_SET_DATA, &m_bsd_in);
        if (s32Ret != EI_SUCCESS) {
            EI_PRINT("bsd1 nn_bsd_cmd failed!\n");
        }
    }

#ifdef SAVE_AX_YUV_SP
    for (int i = 0; i < frame->stVFrame.u32PlaneNum; i++)
    {
        if (fout) {
            if (m_bsd_in.img.format == YUV420_SP){
                if (i == 0) {
                    fwrite((void *)frame->stVFrame.ulPlaneVirAddr[i], 1,  m_bsd_in.img.width*m_bsd_in.img.height, fout);
                } else {
                    fwrite((void *)frame->stVFrame.ulPlaneVirAddr[i], 1,  m_bsd_in.img.width*m_bsd_in.img.height/2, fout);
                }
            } else {
                    fwrite((void *)frame->stVFrame.ulPlaneVirAddr[i], 1,  frame->stVFrame.u32PlaneSizeValid[i], fout);
            } 
        }
    }
#endif

    return 0;
}


int nn_dms_start(VIDEO_FRAME_INFO_S *frame, void *nn_hdl, FILE *fout)
{
    EI_S32 s32Ret;
    static nn_dms_in_t m_dms_in;
	int match_index = 0;
	float match_score = 0.0;
	int get_feature_flag = 0;

    memset(&m_dms_in, 0, sizeof(nn_dms_in_t));
    m_dms_in.img.y_phy = (void *)frame->stVFrame.u64PlanePhyAddr[0];
    m_dms_in.img.y = (void *)frame->stVFrame.ulPlaneVirAddr[0];
    m_dms_in.img.uv_phy = (void *)frame->stVFrame.u64PlanePhyAddr[1];
    m_dms_in.img.uv = (void *)frame->stVFrame.ulPlaneVirAddr[1];
    m_dms_in.img.width = 1280;
    m_dms_in.img.height = 720;
    m_dms_in.img.format = YUV420_SP;
    m_dms_in.cover_det_en = 1;
    m_dms_in.red_resist_glasses_en = 1;
	facerecg_info.recg_in.img.y_phy = (void *)frame->stVFrame.u64PlanePhyAddr[0];
    facerecg_info.recg_in.img.y = (void *)frame->stVFrame.ulPlaneVirAddr[0];
    facerecg_info.recg_in.img.uv_phy = (void *)frame->stVFrame.u64PlanePhyAddr[1];
    facerecg_info.recg_in.img.uv = (void *)frame->stVFrame.ulPlaneVirAddr[1];
    facerecg_info.recg_in.img.width = facerecg_info.u32Width;
    facerecg_info.recg_in.img.height = facerecg_info.u32Height;
    facerecg_info.recg_in.img.format = YUV420_SP;
	facerecg_info.recg_in.interest_roi[0] = 0;
	facerecg_info.recg_in.interest_roi[1] = 0;
	facerecg_info.recg_in.interest_roi[2] = facerecg_info.recg_in.img.width - 1;
	facerecg_info.recg_in.interest_roi[3] = facerecg_info.recg_in.img.height - 1;

    if(nn_hdl != NULL){
        s32Ret = nn_dms_cmd(nn_hdl, NN_DMS_SET_DATA, &m_dms_in);
        if (s32Ret != EI_SUCCESS) {
            EI_PRINT("nn_dms_cmd failed!\n");
        }
    }
	
	nn_facerecg_detect(facerecg_info.nn_hdl, &facerecg_info.recg_in, &facerecg_info.faces);
	facerecg_info.features.size = facerecg_info.faces.size;
	if (facerecg_info.features.size > 0 && !facerecg_info.features.p) {
		printf(" --- facerecg_info.features.size %d--- \n", facerecg_info.features.size);
		facerecg_info.features.p = (nn_facerecg_feature_t *)malloc(sizeof(nn_facerecg_feature_t)*facerecg_info.features.size);
		facerecg_info.match_features = (nn_facerecg_features_t *)malloc(sizeof(nn_facerecg_features_t));
		memset(facerecg_info.match_features, 0, sizeof(nn_facerecg_features_t));
	}
	for (int i = 0; i<facerecg_info.faces.size; i++) {
		nn_facerecg_match_result_t match_temp;
		printf(" --- get_feature_flag0 %d--- \n", get_feature_flag);
		get_feature_flag = nn_facerecg_get_feature(facerecg_info.nn_hdl, &facerecg_info.recg_in, &facerecg_info.faces.p[i], &facerecg_info.features.p[i]);
		printf(" --- get_feature_flag %d--- \n", get_feature_flag);
		if (get_feature_flag == 0) {
			nn_facerecg_match(facerecg_info.nn_hdl, &facerecg_info.features.p[i], facerecg_info.match_features, &match_temp);
			for (int j=0; j<facerecg_info.match_features->size; j++) {
				if (match_temp.score[j] > match_score) {
					match_score = match_temp.score[j];
					match_index = match_temp.max_index;
				}
				printf("i:%d, match_temp score:%f, max_index:%d %d\n", i, match_temp.score[j], match_temp.max_index, match_index);
			}
		}
	}

 

#if  0
    EI_PRINT("save yuv buffer sizey %d, stride %d %d %d\n", frame->stVFrame.u32PlaneSizeValid[0], frame->stVFrame.u32PlaneStride[0], frame->stCommFrameInfo.u32Width, frame->stCommFrameInfo.u32Height);
    
    EI_PRINT("save yuv buffer addr %llx %llx %llx\n",frame->stVFrame.u64PlanePhyAddr[0], 
                frame->stVFrame.u64PlanePhyAddr[1], frame->stVFrame.u64PlanePhyAddr[2]);
    
    EI_PRINT("save yuv buffer virtual addr %x %x %x\n",frame->stVFrame.ulPlaneVirAddr[0], 
                frame->stVFrame.ulPlaneVirAddr[1], frame->stVFrame.ulPlaneVirAddr[2]);

    EI_PRINT("save yuv buffer ID %x\n",frame->u32BufferId);
#endif


#ifdef SAVE_AX_YUV_SP
        for (int i = 0; i < frame->stVFrame.u32PlaneNum; i++)
        {
            if (fout) {
                if (m_dms_in.img.format == YUV420_SP){
                    if (i == 0) {
                        fwrite((void *)frame->stVFrame.ulPlaneVirAddr[i], 1,  m_dms_in.img.width*m_dms_in.img.height, fout);
                    } else {
                        fwrite((void *)frame->stVFrame.ulPlaneVirAddr[i], 1,  m_dms_in.img.width*m_dms_in.img.height/2, fout);
                    }
                } else {
                    fwrite((void *)frame->stVFrame.ulPlaneVirAddr[i], 1,  frame->stVFrame.u32PlaneSizeValid[i], fout);
                } 
            }
        }
#endif
	if (facerecg_info.features.p) {
		free(facerecg_info.features.p);
		facerecg_info.features.p = NULL;
	}

    return 0;
}

EI_VOID *adas_get_frame_proc(EI_VOID *p)
{
    int ret, i;
    VIDEO_FRAME_INFO_S axFrame = {0};
    axframe_info_cache_s axframe_cache[2] = {{0}};
    EI_U32 cur_frame_id = 0;
    void *nn_hdl = NULL;
    nn_adas_cfg_t m_nn_adas_cfg;
    FILE *fout = NULL;
    axframe_info_s *axframe_info;
    axframe_info = (axframe_info_s *)p;
	
	prctl(PR_SET_NAME, "adas_getframe_proc");

    sem_init(&axframe_info->frame_del_sem, 0, 0);
    pthread_mutex_init(&axframe_info->frame_stLock, NULL);
    
#ifdef SAVE_AX_YUV_SP
    char srcfilename[128];
    sprintf(srcfilename, "/mnt/card/adas_raw_ch%d.yuv", axframe_info->vissExtChn);
    fout = fopen(srcfilename, "wb+");
    if (fout == NULL) {
        EI_TRACE_VDEC(EI_DBG_ERR, "file open error1\n");
    }
#endif

    EI_PRINT("********start_nn_adas_svc()****** \n");
    memset(&m_nn_adas_cfg, 0, sizeof(m_nn_adas_cfg));
    /*init nn_adas, cfg*/
    m_nn_adas_cfg.img_width = 1280;
    m_nn_adas_cfg.img_height = 720;
    m_nn_adas_cfg.interest_box[0] = 0;
    m_nn_adas_cfg.interest_box[1] = 0;
    m_nn_adas_cfg.interest_box[2] = 1280;
    m_nn_adas_cfg.interest_box[3] = 720;
    m_nn_adas_cfg.format = YUV420_SP;
    m_nn_adas_cfg.model_basedir = NN_ADAS_AX_PATH;
    m_nn_adas_cfg.cb_func = nn_adas_result_cb;
    m_nn_adas_cfg.camera_param.vanish_point[0] = 640;
    m_nn_adas_cfg.camera_param.vanish_point[1] = 360;
    m_nn_adas_cfg.camera_param.camera_angle[0] = 80;
    m_nn_adas_cfg.camera_param.camera_angle[1] = 50;
    m_nn_adas_cfg.camera_param.camera_to_ground = 1.2;
    m_nn_adas_cfg.camera_param.car_head_to_camera= 1.5;
    m_nn_adas_cfg.camera_param.car_width= 1.8;
    m_nn_adas_cfg.camera_param.camera_to_middle= 0;
    m_nn_adas_cfg.camera_param.bottom_line = 680;
    m_nn_adas_cfg.ldw_en = 1;//车道偏移和变道报警
	m_nn_adas_cfg.hmw_en = 1;//保持车距报警
	m_nn_adas_cfg.ufcw_en = 0;//低速下前车碰撞报警，reserved备用
	m_nn_adas_cfg.fcw_en = 1;//前车碰撞报警
	m_nn_adas_cfg.pcw_en = 1;//行人碰撞报警
	m_nn_adas_cfg.fmw_en = 0;//前车启动报警，必须有GPS且本车车速为0
	m_nn_adas_cfg.warn_param.hmw_param.min_warn_dist = 0;//最小报警距离，单位米
	m_nn_adas_cfg.warn_param.hmw_param.max_warn_dist = 100;//最大报警距离，单位米
	m_nn_adas_cfg.warn_param.hmw_param.min_warn_gps = 30;//最低报警车速，单位KM/H
	m_nn_adas_cfg.warn_param.hmw_param.max_warn_gps = 200;//最高报警车速，单位KM/H
	m_nn_adas_cfg.warn_param.hmw_param.gps_ttc_thresh[0] = 1.5f;//与前车碰撞小于这个时间报警，单位秒,距离/本车GPS速度，对应WARN_STATUS_HMW_LEVEL1报警状态
	m_nn_adas_cfg.warn_param.hmw_param.gps_ttc_thresh[1] = 0.8f;//与前车碰撞小于这个时间报警，单位秒,距离/本车GPS速度，对应WARN_STATUS_HMW_LEVEL2报警状态

	m_nn_adas_cfg.warn_param.fcw_param.min_warn_dist = 0;
	m_nn_adas_cfg.warn_param.fcw_param.max_warn_dist = 100;
	m_nn_adas_cfg.warn_param.fcw_param.min_warn_gps = 30;
	m_nn_adas_cfg.warn_param.fcw_param.max_warn_gps = 200;
	m_nn_adas_cfg.warn_param.fcw_param.gps_ttc_thresh = 1.5f; //与前车碰撞报警时间，单位秒,距离/本车GPS速度
	m_nn_adas_cfg.warn_param.fcw_param.ttc_thresh = 2.7f; //与前车碰撞报警时间，单位秒,距离/相对车速，以上4个条件同时满足

	m_nn_adas_cfg.warn_param.ufcw_param.min_warn_dist = 0;
	m_nn_adas_cfg.warn_param.ufcw_param.max_warn_dist = 100;
	m_nn_adas_cfg.warn_param.ufcw_param.min_warn_gps = 0;
	m_nn_adas_cfg.warn_param.ufcw_param.max_warn_gps = 30;
	m_nn_adas_cfg.warn_param.ufcw_param.relative_speed_thresh = -3.0f; //与前车碰撞相对车速，前车车速-本车车速小于这个速度

	m_nn_adas_cfg.warn_param.fmw_param.min_static_time = 5.0f;//前车静止的时间需大于多少，单位秒
	m_nn_adas_cfg.warn_param.fmw_param.warn_car_inside_dist = 6.0f;//与前车的距离在多少米内，单位米
	m_nn_adas_cfg.warn_param.fmw_param.warn_dist_leave = 0.6f;//前车启动大于多远报警，单位米

	m_nn_adas_cfg.warn_param.pcw_param.min_warn_dist = 0;
	m_nn_adas_cfg.warn_param.pcw_param.max_warn_dist = 100;
	m_nn_adas_cfg.warn_param.pcw_param.min_warn_gps = 30;
	m_nn_adas_cfg.warn_param.pcw_param.max_warn_gps = 200;
	m_nn_adas_cfg.warn_param.pcw_param.gps_ttc_thresh = 1.5f;
	m_nn_adas_cfg.warn_param.pcw_param.ttc_thresh = 4.0f;

	m_nn_adas_cfg.warn_param.ldw_param.min_warn_gps = 30.0f;//最低报警车速，单位KM/H
	m_nn_adas_cfg.warn_param.ldw_param.ldw_warn_thresh = 1.0f; //在lane_warn_dist距离内持续时间，单位秒
	m_nn_adas_cfg.warn_param.ldw_param.lane_warn_dist = 0.2f;//与车道线距离，车道线内
	m_nn_adas_cfg.warn_param.ldw_param.change_lane_dist = 0.6f; //变道侧车道线离车的中点距离，单位米

    /* open libadas.so*/
    nn_hdl = nn_adas_open(&m_nn_adas_cfg);
    if(nn_hdl == NULL){
        EI_PRINT("nn_adas_open() failed!\n");
        return NULL;
    }

    while (EI_TRUE == axframe_info->bThreadStart) {
        memset(&axFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
        ret = EI_MI_VISS_GetChnFrame(axframe_info->vissDev,
            axframe_info->vissExtChn, &axFrame, 1000);
        //EI_PRINT("axframe_info->nn_type = %d; axframe_info->vissExtChn = %d \n", axframe_info->nn_type, axframe_info->vissExtChn);
        if (ret != EI_SUCCESS) {
            EI_PRINT("vissExtChn %d get frame error\n", axframe_info->vissExtChn);
            usleep(100 * 1000);
            continue;
        }
        
        ret = EI_MI_VBUF_FrameMmap(&axFrame, VBUF_REMAP_MODE_CACHED);
        if (ret != EI_SUCCESS) {
            EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                        &axFrame);
            EI_TRACE_VDEC(EI_DBG_ERR, "axframe Mmap error %x\n", ret);
            continue;
        }
        
        for (i = 0; i < 2; i++) {
            if (axframe_cache[i].axframe_id == 0) {
                cur_frame_id++;
                axframe_cache[i].axframe_id = cur_frame_id;
                memset(&axframe_cache[i].axFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
                //memcpy(&axframe_cache[i].axFrame, &axFrame, sizeof(VIDEO_FRAME_INFO_S));
                axframe_cache[i].axFrame = axFrame;
    
                nn_adas_start(&axframe_cache[i].axFrame, nn_hdl, fout);
                break;
            }
        }
		adas_before_time = osal_get_usec();
        sem_wait(&axframe_info->frame_del_sem);

        if (axframe_cache[0].axframe_id != 0 && axframe_cache[1].axframe_id != 0) {
        
            if (axframe_cache[0].axframe_id < axframe_cache[1].axframe_id){
                //EI_PRINT("Munmap adas_axframe_cache[0].axframe_id = %d \n", axframe_cache[0].axframe_id);
                EI_MI_VBUF_FrameMunmap(&axframe_cache[0].axFrame, VBUF_REMAP_MODE_CACHED);
                axframe_cache[0].axframe_id = 0;
                EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                    &axframe_cache[0].axFrame);
            } else {
                //EI_PRINT("Munmap adas_axframe_cache[1].axframe_id = %d \n", axframe_cache[1].axframe_id);
                EI_MI_VBUF_FrameMunmap(&axframe_cache[1].axFrame, VBUF_REMAP_MODE_CACHED);
                axframe_cache[1].axframe_id = 0;
                EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                    &axframe_cache[1].axFrame);
            }
        }
    }
    nn_adas_close(nn_hdl);

    for (i = 0; i < 2; i++) {
        if (axframe_cache[i].axframe_id != 0) {
            EI_MI_VBUF_FrameMunmap(&axframe_cache[i].axFrame, VBUF_REMAP_MODE_CACHED);
            //EI_PRINT("Munmap adas_axframe_cache[i].axframe_id = %d \n", axframe_cache[i].axframe_id);
            //EI_PRINT("Munmap axframe_info->nn_type = %d\n", axframe_info->nn_type);
            axframe_cache[i].axframe_id = 0;
            EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                    &axframe_cache[i].axFrame);
        }
    }
    
    sem_post(&axframe_info->frame_del_sem);
    sem_destroy(&axframe_info->frame_del_sem);
    pthread_mutex_destroy(&axframe_info->frame_stLock);
    EI_MI_VISS_DisableChn(axframe_info->vissDev, axframe_info->vissExtChn);

#ifdef SAVE_AX_YUV_SP
    if (fout) {
        fclose(fout);
        fout = NULL;
    }
#endif

    return NULL;
}



EI_VOID *bsd0_get_frame_proc(EI_VOID *p)
{
    int ret, i;
    VIDEO_FRAME_INFO_S axFrame = {0};
    void *nn_hdl = NULL;
    EI_U32 cur_frame_id = 0;
    axframe_info_cache_s axframe_cache[2] = {{0}};
    nn_bsd_cfg_t m_nn_bsd_cfg;
    FILE *fout = NULL;
    
    axframe_info_s *axframe_info;
    axframe_info = (axframe_info_s *)p;
	
	prctl(PR_SET_NAME, "bsd0_getframe_proc");

    sem_init(&axframe_info->frame_del_sem, 0, 0);
    pthread_mutex_init(&axframe_info->frame_stLock, NULL);

#ifdef SAVE_AX_YUV_SP
    char srcfilename[128];
    sprintf(srcfilename, "/data/bsd_raw_ch%d.yuv", axframe_info->vissExtChn);
    fout = fopen(srcfilename, "wb+");
    if (fout == NULL) {
        EI_TRACE_VDEC(EI_DBG_ERR, "file open error1\n");
    }
#endif

    EI_PRINT("********start_nn_bsd_svc()****** \n");
    memset(&m_nn_bsd_cfg, 0, sizeof(m_nn_bsd_cfg));
    /*init nn_bsd, cfg*/
    m_nn_bsd_cfg.img_width = 1280; //算法输入图像宽
    m_nn_bsd_cfg.img_height = 720; //算法输入图像高
    m_nn_bsd_cfg.interest_box[0] = 0; //算法检测区域左上角X坐标
    m_nn_bsd_cfg.interest_box[1] = 0; //算法检测区域左上角y坐标
    m_nn_bsd_cfg.interest_box[2] = 1280-1; //算法检测区域右下角x坐标
    m_nn_bsd_cfg.interest_box[3] = 720-1; //算法检测区域右下角y坐标
    m_nn_bsd_cfg.format = YUV420_SP;
    m_nn_bsd_cfg.model_basedir = NN_BSD_AX_PATH;
	m_nn_bsd_cfg.cb_func = nn_bsd0_result_cb;
    m_nn_bsd_cfg.max_track_count = 50; //最大跟踪识别的物体个数。理论上没有上限
    m_nn_bsd_cfg.warn_roi[0][0] = 200; //左边报警区域的左上角x坐标
    m_nn_bsd_cfg.warn_roi[0][1] = 100; //左边报警区域的左上角y坐标
    m_nn_bsd_cfg.warn_roi[1][0] = 400; //左边报警区域的右上角x坐标
    m_nn_bsd_cfg.warn_roi[1][1] = 100; //左边报警区域的右上角y坐标
    m_nn_bsd_cfg.warn_roi[2][0] = 350; //左边报警区域的右下角x坐标
    m_nn_bsd_cfg.warn_roi[2][1] = 500; //左边报警区域的右下角y坐标
    m_nn_bsd_cfg.warn_roi[3][0] = 100; //左边报警区域的左下角x坐标
    m_nn_bsd_cfg.warn_roi[3][1] = 500; //左边报警区域的左下角y坐标
    m_nn_bsd_cfg.warn_roi[4][0] = 880;  //右边报警区域的左上角x坐标
    m_nn_bsd_cfg.warn_roi[4][1] = 100;  //右边报警区域的左上角y坐标
    m_nn_bsd_cfg.warn_roi[5][0] = 1080;  //右边报警区域的右上角x坐标
    m_nn_bsd_cfg.warn_roi[5][1] = 100;  //右边报警区域的右上角y坐标
    m_nn_bsd_cfg.warn_roi[6][0] = 1180;  //右边报警区域的右下角x坐标
    m_nn_bsd_cfg.warn_roi[6][1] = 500;
    m_nn_bsd_cfg.warn_roi[7][0] = 930;
    m_nn_bsd_cfg.warn_roi[7][1] = 500;
    m_nn_bsd_cfg.camera_param.vanish_point[0] = 640;//天际线和车中线交叉点X坐标 单位像素
    m_nn_bsd_cfg.camera_param.vanish_point[1] = 360;//天际线和车中线交叉点Y坐标 单位像素
    m_nn_bsd_cfg.camera_param.camera_angle[0] = 120;//摄像头水平角度
    m_nn_bsd_cfg.camera_param.camera_angle[1] = 80;//摄像头垂直角度
    m_nn_bsd_cfg.camera_param.camera_to_ground = 0.9; //摄像头距离地面的高度
    m_nn_bsd_cfg.camera_param.car_width = 1.8; //车宽，单位，米。汽车1.8，货车2.2
    m_nn_bsd_cfg.fixmode_warn_param.install_mode = 0; //摄像头安装位置，自动校准要求必须是BACK_MODE,且gps速度> 30KM/H
	/* left Or right region bian dao fu zhu ,只有在NN_BSD_BACK_MODE 模式下才起效*/
    m_nn_bsd_cfg.fixmode_warn_param.low_level_warn_gps = 10; //摄像头level0和level1报警速度
    m_nn_bsd_cfg.fixmode_warn_param.high_level_warn_gps = 80; //level3 最小报警速度
    m_nn_bsd_cfg.fixmode_warn_param.ttc_thresh = 4.0; //level3 最小报警碰撞时间
    m_nn_bsd_cfg.fixmode_warn_param.warn_dist_horizon[0] = 1.5f;
	m_nn_bsd_cfg.fixmode_warn_param.warn_dist_horizon[1] = 4.0f;
	m_nn_bsd_cfg.fixmode_warn_param.warn_dist_forward[0] = 0;
	m_nn_bsd_cfg.fixmode_warn_param.warn_dist_forward[1] = 5.0f;
	m_nn_bsd_cfg.fixmode_warn_param.lca_warn_interval[0] = 0;
	m_nn_bsd_cfg.fixmode_warn_param.lca_warn_interval[1] = 10.0f;
	m_nn_bsd_cfg.fixmode_warn_param.lca_warn_interval[2] = 10.0f;
	/* middle region  man qu jian ce ,只有在NN_BSD_BACK_MODE 模式下才起效*/
    m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_dist_forward[0] = 0; //后车和本车最小距离
    m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_dist_forward[1] = 5.0f; //后车和本车最大距离
    m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_dist_horizon[0] = 1.5f; //水平最小距离
    m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_dist_horizon[1] = 4.0f; //水平最大距离
    m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_gps[0] = 0;
	m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_gps[1] = 100;
	m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_interval[0] = 0;
	m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_interval[1] = 10.0f;
	m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_interval[2] = 10.0f;
	//检测阈值,0是车,1是人,2是非机动车，其它索引无效。det_thresh rang 0-1之间,<0.45,内部会转换成0.45, >0.65,内部会转换成0.65。
	m_nn_bsd_cfg.det_thresh[0] = 0.45;//车检测阈值，0.45-0.65之间，阈值低，识别率高，误识别高；阈值高，识别率低，误识别低
	m_nn_bsd_cfg.det_thresh[1] = 0.45;//人检测阈值，0.45-0.65之间，阈值低，识别率高，误识别高；阈值高，识别率低，误识别低
	m_nn_bsd_cfg.det_thresh[2] = 0.40;//非机动车检测阈值，0.45-0.65之间，阈值低，识别率高，误识别高；阈值高，识别率低，误识别低

	m_nn_bsd_cfg.car_det_size_range[0] = 48; //识别车辆的最小像素
    m_nn_bsd_cfg.car_det_size_range[1] = 1280; //识别车辆的最大像素
    m_nn_bsd_cfg.ped_det_size_range[0] = 48; //识别人物的最小像素
    m_nn_bsd_cfg.ped_det_size_range[1] = 1280; //识别人物的最大像素
    m_nn_bsd_cfg.keep_model_memory = 0;//是否保持模型的加载, 如果只开启一路bsd此参数设置为0，
                         //如果开启两路及以上此参数设置为1。这样能节省第二路及以后bsd算法的模型加载时间
    m_nn_bsd_cfg.ax_freq = 500; //ax的频率,单路大于300Mhz,双路不高于600MHz，不建议改
    m_nn_bsd_cfg.resolution = 0;//0的检测距离比较近，1检测距离比较远, 检测距离比较远的消耗的cpu和npu资源比较多
	m_nn_bsd_cfg.calib_param.auto_calib_en = 1;//打开自动校准
	m_nn_bsd_cfg.calib_param.current.camera_to_ground = 0.7;//摄像头离地高度
	m_nn_bsd_cfg.calib_param.ready.camera_to_ground = 0.7;//摄像头离地高度

	m_nn_bsd_cfg.model_type = 3; /*0 - car+person  1-person only 2 - fisheye, 3 - car/person/cycle fisheye or normal camera */
    /* open libbsd.so */
    nn_hdl = nn_bsd_open(&m_nn_bsd_cfg);
    if(nn_hdl == NULL){
        EI_PRINT("nn_bsd_open() failed!");
        return NULL;
    }

    /*int buffer_len = 1280 * 720;*/
    while (EI_TRUE == axframe_info->bThreadStart) {
        memset(&axFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
        ret = EI_MI_VISS_GetChnFrame(axframe_info->vissDev,
            axframe_info->vissExtChn, &axFrame, 1000);
        
		if (ret != EI_SUCCESS) {
            EI_PRINT("vissExtChn %d get frame error\n", axframe_info->vissExtChn);
            usleep(100 * 1000);
            continue;
        }
        
        ret = EI_MI_VBUF_FrameMmap(&axFrame, VBUF_REMAP_MODE_CACHED);
        if (ret != EI_SUCCESS) {
            EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                        &axFrame);
            EI_TRACE_VDEC(EI_DBG_ERR, "axframe Mmap error %x\n", ret);
            continue;
        }
        
        for (i = 0; i < 2; i++) {
            if (axframe_cache[i].axframe_id == 0) {
                cur_frame_id++;
                axframe_cache[i].axframe_id = cur_frame_id;
                memset(&axframe_cache[i].axFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
                //memcpy(&axframe_cache[i].axFrame, &axFrame, sizeof(VIDEO_FRAME_INFO_S));
                axframe_cache[i].axFrame = axFrame;
				
                if (axframe_info->nn_type == NN_BSD0) {
                    nn_bsd0_start(&axframe_cache[i].axFrame, nn_hdl, fout);
                } else if (axframe_info->nn_type == NN_BSD1) {
                    nn_bsd1_start(&axframe_cache[i].axFrame, nn_hdl, fout);
                }
                break;
            }
        }
        sem_wait(&axframe_info->frame_del_sem);

        if (axframe_cache[0].axframe_id != 0 && axframe_cache[1].axframe_id != 0) {

            if (axframe_cache[0].axframe_id < axframe_cache[1].axframe_id){
                EI_MI_VBUF_FrameMunmap(&axframe_cache[0].axFrame, VBUF_REMAP_MODE_CACHED);
                axframe_cache[0].axframe_id = 0;
                EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                    &axframe_cache[0].axFrame);
            } else {
                EI_MI_VBUF_FrameMunmap(&axframe_cache[1].axFrame, VBUF_REMAP_MODE_CACHED);
                axframe_cache[1].axframe_id = 0;
                EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                    &axframe_cache[1].axFrame);
            }
        }
    }
    nn_bsd_close(nn_hdl);

    for (i = 0; i < 2; i++) {
        if (axframe_cache[i].axframe_id != 0) {
            EI_MI_VBUF_FrameMunmap(&axframe_cache[i].axFrame, VBUF_REMAP_MODE_CACHED);
            axframe_cache[i].axframe_id = 0;
            EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                    &axframe_cache[i].axFrame);
        }
    }
    
    sem_post(&axframe_info->frame_del_sem);
    sem_destroy(&axframe_info->frame_del_sem);
    pthread_mutex_destroy(&axframe_info->frame_stLock);
    EI_MI_VISS_DisableChn(axframe_info->vissDev, axframe_info->vissExtChn);

#ifdef SAVE_AX_YUV_SP
    if (fout) {
        fclose(fout);
        fout = NULL;
    }
#endif
    return NULL;

}

EI_VOID *bsd1_get_frame_proc(EI_VOID *p)
{
    int ret, i;
    VIDEO_FRAME_INFO_S axFrame = {0};
    axframe_info_s *axframe_info;
    void *nn_hdl = NULL;
    EI_U32 cur_frame_id = 0;
    axframe_info_cache_s axframe_cache[2] = {{0}};
    nn_bsd_cfg_t m_nn_bsd_cfg;
    FILE *fout = NULL;

    axframe_info = (axframe_info_s *)p;
	
	prctl(PR_SET_NAME, "bsd1_getframe_proc");

    sem_init(&axframe_info->frame_del_sem, 0, 0);
    pthread_mutex_init(&axframe_info->frame_stLock, NULL);
 
#ifdef SAVE_AX_YUV_SP
    char srcfilename[128];
    sprintf(srcfilename, "/data/bsd_raw_ch%d.yuv", axframe_info->vissExtChn);
    fout = fopen(srcfilename, "wb+");
    if (fout == NULL) {
        EI_TRACE_VDEC(EI_DBG_ERR, "file open error1\n");
    }
#endif

    EI_PRINT("********start_nn_bsd_svc()****** \n");
    /*init nn_bsd, cfg*/
    m_nn_bsd_cfg.img_width = 1280; //算法输入图像宽
    m_nn_bsd_cfg.img_height = 720; //算法输入图像高
    m_nn_bsd_cfg.interest_box[0] = 0; //算法检测区域左上角X坐标
    m_nn_bsd_cfg.interest_box[1] = 0; //算法检测区域左上角y坐标
    m_nn_bsd_cfg.interest_box[2] = 1280-1; //算法检测区域右下角x坐标
    m_nn_bsd_cfg.interest_box[3] = 720-1; //算法检测区域右下角y坐标
    m_nn_bsd_cfg.format = YUV420_SP;
    m_nn_bsd_cfg.model_basedir = NN_BSD_AX_PATH;
	m_nn_bsd_cfg.cb_func = nn_bsd1_result_cb;
    m_nn_bsd_cfg.max_track_count = 50; //最大跟踪识别的物体个数。理论上没有上限
    m_nn_bsd_cfg.warn_roi[0][0] = 200; //左边报警区域的左上角x坐标
    m_nn_bsd_cfg.warn_roi[0][1] = 100; //左边报警区域的左上角y坐标
    m_nn_bsd_cfg.warn_roi[1][0] = 400; //左边报警区域的右上角x坐标
    m_nn_bsd_cfg.warn_roi[1][1] = 100; //左边报警区域的右上角y坐标
    m_nn_bsd_cfg.warn_roi[2][0] = 350; //左边报警区域的右下角x坐标
    m_nn_bsd_cfg.warn_roi[2][1] = 500; //左边报警区域的右下角y坐标
    m_nn_bsd_cfg.warn_roi[3][0] = 100; //左边报警区域的左下角x坐标
    m_nn_bsd_cfg.warn_roi[3][1] = 500; //左边报警区域的左下角y坐标
    m_nn_bsd_cfg.warn_roi[4][0] = 880;  //右边报警区域的左上角x坐标
    m_nn_bsd_cfg.warn_roi[4][1] = 100;  //右边报警区域的左上角y坐标
    m_nn_bsd_cfg.warn_roi[5][0] = 1080;  //右边报警区域的右上角x坐标
    m_nn_bsd_cfg.warn_roi[5][1] = 100;  //右边报警区域的右上角y坐标
    m_nn_bsd_cfg.warn_roi[6][0] = 1180;  //右边报警区域的右下角x坐标
    m_nn_bsd_cfg.warn_roi[6][1] = 500;
    m_nn_bsd_cfg.warn_roi[7][0] = 930;
    m_nn_bsd_cfg.warn_roi[7][1] = 500;
    m_nn_bsd_cfg.camera_param.vanish_point[0] = 640;//天际线和车中线交叉点X坐标 单位像素
    m_nn_bsd_cfg.camera_param.vanish_point[1] = 360;//天际线和车中线交叉点Y坐标 单位像素
    m_nn_bsd_cfg.camera_param.camera_angle[0] = 120;//摄像头水平角度
    m_nn_bsd_cfg.camera_param.camera_angle[1] = 80;//摄像头垂直角度
    m_nn_bsd_cfg.camera_param.camera_to_ground = 0.9; //摄像头距离地面的高度
    m_nn_bsd_cfg.camera_param.car_width = 1.8; //车宽，单位，米。汽车1.8，货车2.2
    m_nn_bsd_cfg.fixmode_warn_param.install_mode = 0; //摄像头安装位置，自动校准要求必须是BACK_MODE,且gps速度> 30KM/H
	/* left Or right region bian dao fu zhu ,只有在NN_BSD_BACK_MODE 模式下才起效*/
    m_nn_bsd_cfg.fixmode_warn_param.low_level_warn_gps = 10; //摄像头level0和level1报警速度
    m_nn_bsd_cfg.fixmode_warn_param.high_level_warn_gps = 80; //level3 最小报警速度
    m_nn_bsd_cfg.fixmode_warn_param.ttc_thresh = 4.0; //level3 最小报警碰撞时间
    m_nn_bsd_cfg.fixmode_warn_param.warn_dist_horizon[0] = 1.5f;
	m_nn_bsd_cfg.fixmode_warn_param.warn_dist_horizon[1] = 4.0f;
	m_nn_bsd_cfg.fixmode_warn_param.warn_dist_forward[0] = 0;
	m_nn_bsd_cfg.fixmode_warn_param.warn_dist_forward[1] = 5.0f;
	m_nn_bsd_cfg.fixmode_warn_param.lca_warn_interval[0] = 0;
	m_nn_bsd_cfg.fixmode_warn_param.lca_warn_interval[1] = 10.0f;
	m_nn_bsd_cfg.fixmode_warn_param.lca_warn_interval[2] = 10.0f;
	/* middle region  man qu jian ce ,只有在NN_BSD_BACK_MODE 模式下才起效*/
    m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_dist_forward[0] = 0; //后车和本车最小距离
    m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_dist_forward[1] = 5.0f; //后车和本车最大距离
    m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_dist_horizon[0] = 1.5f; //水平最小距离
    m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_dist_horizon[1] = 4.0f; //水平最大距离
    m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_gps[0] = 0;
	m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_gps[1] = 100;
	m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_interval[0] = 0;
	m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_interval[1] = 10.0f;
	m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_interval[2] = 10.0f;
	//检测阈值,0是车,1是人,2是非机动车，其它索引无效。det_thresh rang 0-1之间,<0.45,内部会转换成0.45, >0.65,内部会转换成0.65。
	m_nn_bsd_cfg.det_thresh[0] = 0.45;//车检测阈值，0.45-0.65之间，阈值低，识别率高，误识别高；阈值高，识别率低，误识别低
	m_nn_bsd_cfg.det_thresh[1] = 0.45;//人检测阈值，0.45-0.65之间，阈值低，识别率高，误识别高；阈值高，识别率低，误识别低
	m_nn_bsd_cfg.det_thresh[2] = 0.40;//非机动车检测阈值，0.45-0.65之间，阈值低，识别率高，误识别高；阈值高，识别率低，误识别低

	m_nn_bsd_cfg.car_det_size_range[0] = 48; //识别车辆的最小像素
    m_nn_bsd_cfg.car_det_size_range[1] = 1280; //识别车辆的最大像素
    m_nn_bsd_cfg.ped_det_size_range[0] = 48; //识别人物的最小像素
    m_nn_bsd_cfg.ped_det_size_range[1] = 1280; //识别人物的最大像素
    m_nn_bsd_cfg.keep_model_memory = 0;//是否保持模型的加载, 如果只开启一路bsd此参数设置为0，
                         //如果开启两路及以上此参数设置为1。这样能节省第二路及以后bsd算法的模型加载时间
    m_nn_bsd_cfg.ax_freq = 500; //ax的频率,单路大于300Mhz,双路不高于600MHz，不建议改
    m_nn_bsd_cfg.resolution = 0;//0的检测距离比较近，1检测距离比较远, 检测距离比较远的消耗的cpu和npu资源比较多
	m_nn_bsd_cfg.calib_param.auto_calib_en = 1;//打开自动校准
	m_nn_bsd_cfg.calib_param.current.camera_to_ground = 0.7;//摄像头离地高度
	m_nn_bsd_cfg.calib_param.ready.camera_to_ground = 0.7;//摄像头离地高度

	m_nn_bsd_cfg.model_type = 3; /*0 - car+person  1-person only 2 - fisheye, 3 - car/person/cycle fisheye or normal camera */

    /* open libbsd.so */
    nn_hdl = nn_bsd_open(&m_nn_bsd_cfg);
    if(nn_hdl == NULL){
        EI_PRINT("nn_bsd_open() failed!");
        return NULL;
    }

    /*int buffer_len = 1280 * 720;*/
    while (EI_TRUE == axframe_info->bThreadStart) {
        memset(&axFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
        ret = EI_MI_VISS_GetChnFrame(axframe_info->vissDev,
            axframe_info->vissExtChn, &axFrame, 1000);
        if (ret != EI_SUCCESS) {
            EI_PRINT("vissExtChn %d get frame error\n", axframe_info->vissExtChn);
            usleep(100 * 1000);
            continue;
        }
        
        ret = EI_MI_VBUF_FrameMmap(&axFrame, VBUF_REMAP_MODE_CACHED);
        if (ret != EI_SUCCESS) {
            EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                        &axFrame);
            EI_TRACE_VDEC(EI_DBG_ERR, "axframe Mmap error %x\n", ret);
            continue;
        }
        
        for (i = 0; i < 2; i++) {
            if (axframe_cache[i].axframe_id == 0) {
                cur_frame_id++;
                axframe_cache[i].axframe_id = cur_frame_id;
                memset(&axframe_cache[i].axFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
                //memcpy(&axframe_cache[i].axFrame, &axFrame, sizeof(VIDEO_FRAME_INFO_S));
                axframe_cache[i].axFrame = axFrame;
				
                if (axframe_info->nn_type == NN_BSD0) {
                    nn_bsd0_start(&axframe_cache[i].axFrame, nn_hdl, fout);
                } else if (axframe_info->nn_type == NN_BSD1) {
                    nn_bsd1_start(&axframe_cache[i].axFrame, nn_hdl, fout);
                }
                break;
            }
        }
        sem_wait(&axframe_info->frame_del_sem);
/*
        usleep(50 * 1000);
        void *test_ptr = malloc(buffer_len);
        void *test_ptr2 = malloc(buffer_len);
        EI_PRINT("log 0 %d\n", cur_frame_id);
        memcpy(test_ptr, (void *)axframe_cache[i].axFrame.stVFrame.u64PlanePhyAddr[0], buffer_len);
        EI_PRINT("log 1 %d\n", cur_frame_id);
        usleep(50 * 1000);
        memcpy(test_ptr2, (void *)axframe_cache[i].axFrame.stVFrame.u64PlanePhyAddr[0], buffer_len);
        EI_PRINT("log 2 %d\n", cur_frame_id);
        free(test_ptr);
        EI_PRINT("log 3 %d\n", cur_frame_id);
        free(test_ptr2);
        EI_PRINT("log 4 %d\n", cur_frame_id);
*/        
        if (axframe_cache[0].axframe_id != 0 && axframe_cache[1].axframe_id != 0) {

            if (axframe_cache[0].axframe_id < axframe_cache[1].axframe_id){
                EI_MI_VBUF_FrameMunmap(&axframe_cache[0].axFrame, VBUF_REMAP_MODE_CACHED);
                axframe_cache[0].axframe_id = 0;
                EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                    &axframe_cache[0].axFrame);
            } else {
                EI_MI_VBUF_FrameMunmap(&axframe_cache[1].axFrame, VBUF_REMAP_MODE_CACHED);
                axframe_cache[1].axframe_id = 0;
                EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                    &axframe_cache[1].axFrame);
            }
        }
    }
    nn_bsd_close(nn_hdl);

    for (i = 0; i < 2; i++) {
        if (axframe_cache[i].axframe_id != 0) {
            EI_MI_VBUF_FrameMunmap(&axframe_cache[i].axFrame, VBUF_REMAP_MODE_CACHED);
            axframe_cache[i].axframe_id = 0;
            EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                    &axframe_cache[i].axFrame);
        }
    }
    
    sem_post(&axframe_info->frame_del_sem);
    sem_destroy(&axframe_info->frame_del_sem);
    pthread_mutex_destroy(&axframe_info->frame_stLock);
    EI_MI_VISS_DisableChn(axframe_info->vissDev, axframe_info->vissExtChn);

#ifdef SAVE_AX_YUV_SP
    if (fout) {
        fclose(fout);
        fout = NULL;
    }
#endif
    return NULL;

}



EI_VOID *dms_get_frame_proc(EI_VOID *p)
{
    int ret, i;
    VIDEO_FRAME_INFO_S axFrame = {0};
    axframe_info_cache_s axframe_cache[2] = {{0}};
    EI_U32 cur_frame_id = 0;
    nn_dms_cfg_t m_nn_dms_cfg;
	nn_facerecg_cfg_t facerecg_cfg;
	nn_facerecg_in_t recg_in;
	nn_facerecg_faces_t faces;
	nn_facerecg_features_t features;
	nn_facerecg_features_t *match_features;
	nn_facerecg_match_result_t result;
	
    FILE *fout = NULL;
    
    axframe_info_s *axframe_info;
    axframe_info = (axframe_info_s *)p;

    
    prctl(PR_SET_NAME, "dms_getframe_proc");

    sem_init(&axframe_info->frame_del_sem, 0, 0);
    pthread_mutex_init(&axframe_info->frame_stLock, NULL);

#ifdef SAVE_AX_YUV_SP
    char srcfilename[128];
    sprintf(srcfilename, "/data/raw_ch%d.yuv", axframe_info->vissExtChn);
    fout = fopen(srcfilename, "wb+");
    if (fout == NULL) {
        EI_TRACE_VDEC(EI_DBG_ERR, "file open error\n");
    }
#endif

    EI_PRINT("********start_nn_dms_svc()****** \n");
    void *nn_hdl = NULL;
	void *nn_hdl_facerecg = NULL;
	
	/* init nn_dms, cfg */
	/*
    m_nn_dms_cfg.img_width = 1280;//axframe_info->u32Width;
    m_nn_dms_cfg.img_height = 720;//axframe_info->u32Height;
    m_nn_dms_cfg.format = YUV420_SP;
    m_nn_dms_cfg.model_basedir = DMS_AX_PATH;
    m_nn_dms_cfg.cb_func = dms_result_cb;
    m_nn_dms_cfg.dms_behave_en = 1;
    m_nn_dms_cfg.headpose_en = 1;
    m_nn_dms_cfg.dms_obj_det_en = 1;
    m_nn_dms_cfg.cover_det_en = 1;
	if (axframe_info->action == DMS_CALIB) {
		m_nn_dms_cfg.warn_cfg.calib_yaw = 0.0;
    	m_nn_dms_cfg.warn_cfg.calib_pitch = 0.0;
		osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_CALIBRATE_START, NULL, 0);
	} else {
		m_nn_dms_cfg.warn_cfg.calib_yaw = dms_get_cali_yaw();
    	m_nn_dms_cfg.warn_cfg.calib_pitch = dms_get_cali_pitch();
	}
    EI_PRINT(" calib_yaw:%f, calib_pitch:%f \n", m_nn_dms_cfg.warn_cfg.calib_yaw, m_nn_dms_cfg.warn_cfg.calib_pitch);
    m_nn_dms_cfg.warn_cfg.lookaround_stat_time = 2000;
    m_nn_dms_cfg.warn_cfg.thresh_lookaround = 30.0f;
    m_nn_dms_cfg.warn_cfg.lookupdown_stat_time = 2000;
    m_nn_dms_cfg.warn_cfg.thresh_lookup = 30.0f;
    m_nn_dms_cfg.warn_cfg.thresh_lookdown = 30.0f;
    m_nn_dms_cfg.red_resist_glasses_en = 1; 
    m_nn_dms_cfg.interest_box[0] = 0;
    m_nn_dms_cfg.interest_box[1] = 0;
    m_nn_dms_cfg.interest_box[2] = 1280 - 1;
    m_nn_dms_cfg.interest_box[3] = 720 - 1;
    EI_PRINT("interest_box x1:%d,y1:%d,x2:%d,y2:%d \n", m_nn_dms_cfg.interest_box[0], m_nn_dms_cfg.interest_box[1],
        m_nn_dms_cfg.interest_box[2], m_nn_dms_cfg.interest_box[3]);
    m_nn_dms_cfg.warn_cfg.drink_thresh = 1000;
    m_nn_dms_cfg.warn_cfg.call_thresh = 1000;
    m_nn_dms_cfg.warn_cfg.smoke_thresh = 1000;
    m_nn_dms_cfg.warn_cfg.yawn_thresh = 1000;
    m_nn_dms_cfg.warn_cfg.eye_close_thresh = 1000;
    m_nn_dms_cfg.warn_cfg.red_resist_glasses_thresh = 1000;
    m_nn_dms_cfg.warn_cfg.cover_thresh = 1000;
	*/
	
	
	m_nn_dms_cfg.img_width = 1280;//axframe_info->u32Width; 
	m_nn_dms_cfg.img_height = 720;//axframe_info->u32Height;
	m_nn_dms_cfg.format = YUV420_SP;
    m_nn_dms_cfg.model_basedir = DMS_AX_PATH;
    m_nn_dms_cfg.cb_func = dms_result_cb;
    m_nn_dms_cfg.dms_behave_en = 1;
	m_nn_dms_cfg.headpose_en = 1; 
	m_nn_dms_cfg.dms_obj_det_en = 1; 
	m_nn_dms_cfg.cover_det_en = 1; 
	m_nn_dms_cfg.red_resist_glasses_en = 1; 
	m_nn_dms_cfg.face_mask_en = 1; 
	m_nn_dms_cfg.auto_calib_en = 0; 
	if (axframe_info->action == DMS_CALIB) {
		m_nn_dms_cfg.warn_cfg.calib_yaw = 0.0;
    	m_nn_dms_cfg.warn_cfg.calib_pitch = 0.0;
		osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_CALIBRATE_START, NULL, 0);
	} else {
		m_nn_dms_cfg.warn_cfg.calib_yaw = dms_get_cali_yaw();
    	m_nn_dms_cfg.warn_cfg.calib_pitch = dms_get_cali_pitch();
	}
    EI_PRINT(" calib_yaw:%f, calib_pitch:%f \n", m_nn_dms_cfg.warn_cfg.calib_yaw, m_nn_dms_cfg.warn_cfg.calib_pitch);
	m_nn_dms_cfg.warn_cfg.distracted_cfg.yaw_thresh[0] = -40.0;
	m_nn_dms_cfg.warn_cfg.distracted_cfg.yaw_thresh[1] = 40.0;
	m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.warn_interval = 5.0;
	m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.count = 1;
	m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.time = 3.0;
	m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.act.ratio = 0.7;
	m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.act.time = 3.0;
	m_nn_dms_cfg.warn_cfg.distracted_cfg.pitch_thresh[0] = -30.0;
	m_nn_dms_cfg.warn_cfg.distracted_cfg.pitch_thresh[1] = 30.0;
	m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.warn_interval = 5.0;
	m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.count = 1;
	m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.time = 3.0;
	m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.act.ratio = 0.7;
	m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.act.time = 3.0;

    m_nn_dms_cfg.warn_cfg.drink_cfg.warn_interval = 5.0;
	m_nn_dms_cfg.warn_cfg.drink_cfg.count = 1;
    m_nn_dms_cfg.warn_cfg.drink_cfg.time = 2.0;
	m_nn_dms_cfg.warn_cfg.drink_cfg.act.ratio = 0.7;
	m_nn_dms_cfg.warn_cfg.drink_cfg.act.time = 2.0;
	m_nn_dms_cfg.warn_cfg.drink_det_thresh = 0.7; //det level, low:0.6;mid:0.7;high:0.8

	m_nn_dms_cfg.warn_cfg.call_cfg.warn_interval = 5.0;
	m_nn_dms_cfg.warn_cfg.call_cfg.count = 1;
    m_nn_dms_cfg.warn_cfg.call_cfg.time = 3.0;
	m_nn_dms_cfg.warn_cfg.call_cfg.act.ratio = 0.7;
	m_nn_dms_cfg.warn_cfg.call_cfg.act.time = 3.0;
	m_nn_dms_cfg.warn_cfg.call_det_thresh = 0.7; //det level, low:0.6;mid:0.7;high:0.8

	m_nn_dms_cfg.warn_cfg.smoke_cfg.warn_interval = 5.0; 
	m_nn_dms_cfg.warn_cfg.smoke_cfg.count = 1;
    m_nn_dms_cfg.warn_cfg.smoke_cfg.time = 2.0;
	m_nn_dms_cfg.warn_cfg.smoke_cfg.act.ratio = 0.7;
	m_nn_dms_cfg.warn_cfg.smoke_cfg.act.time = 2.0;
	m_nn_dms_cfg.warn_cfg.smoke_det_thresh = 0.7; //det level, low:0.6;mid:0.7;high:0.8

	m_nn_dms_cfg.warn_cfg.red_resist_glasses_cfg.warn_interval = 5.0;
	m_nn_dms_cfg.warn_cfg.red_resist_glasses_cfg.count = 1;
    m_nn_dms_cfg.warn_cfg.red_resist_glasses_cfg.time = 5.0;
	m_nn_dms_cfg.warn_cfg.red_resist_glasses_cfg.act.ratio = 0.7;
	m_nn_dms_cfg.warn_cfg.red_resist_glasses_cfg.act.time = 5.0;

	m_nn_dms_cfg.warn_cfg.face_mask_cfg.warn_interval = 5.0;
	m_nn_dms_cfg.warn_cfg.face_mask_cfg.count = 1;
	m_nn_dms_cfg.warn_cfg.face_mask_cfg.time = 5.0;
	m_nn_dms_cfg.warn_cfg.face_mask_cfg.act.ratio = 0.7;
	m_nn_dms_cfg.warn_cfg.face_mask_cfg.act.time = 5.0;

	m_nn_dms_cfg.warn_cfg.cover_cfg.warn_interval = 5.0;
	m_nn_dms_cfg.warn_cfg.cover_cfg.count = 1;
    m_nn_dms_cfg.warn_cfg.cover_cfg.time = 5.0;
	m_nn_dms_cfg.warn_cfg.cover_cfg.act.ratio = 0.7;
	m_nn_dms_cfg.warn_cfg.cover_cfg.act.time = 5.0;

	m_nn_dms_cfg.warn_cfg.driver_leave_cfg.warn_interval = 5.0;
	m_nn_dms_cfg.warn_cfg.driver_leave_cfg.count = 1;
    m_nn_dms_cfg.warn_cfg.driver_leave_cfg.time = 5.0;
	m_nn_dms_cfg.warn_cfg.driver_leave_cfg.act.ratio = 0.7;
	m_nn_dms_cfg.warn_cfg.driver_leave_cfg.act.time = 5.0;
	/* level 2 fragile */
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.warn_interval = 10.0;
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.count = 3; 
    m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.time = 5 * 60;
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.act.ratio = 0.7;
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.act.time = 2.0;
	/* level 4 fragile */
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[0].warn_interval = 10.0;
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[0].count = 3;
    m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[0].time = 2 * 60;
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[0].act.ratio = 0.1;
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[0].act.time = 5.0;
	/* level 6 fragile */
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].warn_interval = 10.0;
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].count = 6;
    m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].time = 1 * 60;
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].act.ratio = 0.1;
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].act.time = 5.0;
	/* level 8 fragile */
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].warn_interval = 10.0;
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].count = 1;
    m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].time = 2;
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].act.ratio = 0.7;
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].act.time = 2.0;
	/* level 10 fragile */
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].warn_interval = 10.0;
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].count = 1;
    m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].time = 4.0;
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].act.ratio = 0.7;
	m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].act.time = 4.0;

	m_nn_dms_cfg.dump_cfg = 1;

    m_nn_dms_cfg.interest_box[0] = 0;
    m_nn_dms_cfg.interest_box[1] = 0;
    m_nn_dms_cfg.interest_box[2] = 1280 - 1;//axframe_info->u32Width - 1;
    m_nn_dms_cfg.interest_box[3] = 720 - 1;//axframe_info->u32Height - 1;
    EI_PRINT("interest_box x1:%d,y1:%d,x2:%d,y2:%d \n", m_nn_dms_cfg.interest_box[0], m_nn_dms_cfg.interest_box[1],
        m_nn_dms_cfg.interest_box[2], m_nn_dms_cfg.interest_box[3]);
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	

    /* open libdms.so*/
    nn_hdl = nn_dms_open(&m_nn_dms_cfg);
    if(nn_hdl == NULL){
        EI_PRINT("nn_dms_open() failed!");
        return NULL;
    }
	memset(&facerecg_info.faces, 0, sizeof(nn_facerecg_faces_t));
	memset(&facerecg_info.facerecg_cfg, 0, sizeof(nn_facerecg_cfg_t));
	facerecg_info.u32Width = 1280;
	facerecg_info.u32Height = 720;
	facerecg_info.facerecg_cfg.fmt = YUV420_SP;
	facerecg_info.facerecg_cfg.img_width = 1280;
	facerecg_info.facerecg_cfg.img_height = 720;
	facerecg_info.facerecg_cfg.model_index = 0;
	facerecg_info.facerecg_cfg.model_basedir = NN_FACERECG_AX_PATH;
	facerecg_info.facerecg_cfg.interest_roi[0] = 0;
	facerecg_info.facerecg_cfg.interest_roi[1] = 0;
	facerecg_info.facerecg_cfg.interest_roi[2] = 1280 - 1;
	facerecg_info.facerecg_cfg.interest_roi[3] = 720 - 1;

	/* open libdms_facerecg.so*/
	facerecg_info.nn_hdl = nn_facerecg_create(&facerecg_info.facerecg_cfg);
	if(facerecg_info.nn_hdl == NULL){
		EI_PRINT("nn_facerecg_create() failed!");
		return -1;
	}
    
    while (EI_TRUE == axframe_info->bThreadStart) {
        memset(&axFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
        ret = EI_MI_VISS_GetChnFrame(axframe_info->vissDev,
            axframe_info->vissExtChn, &axFrame, 1000);
			
        if (ret != EI_SUCCESS) {
            EI_PRINT("vissExtChn %d get frame error\n", axframe_info->vissExtChn);
            usleep(100 * 1000);
            continue;
        }
        
        ret = EI_MI_VBUF_FrameMmap(&axFrame, VBUF_REMAP_MODE_NOCACHE);
        if (ret != EI_SUCCESS) {
            EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                        &axFrame);
            EI_TRACE_VDEC(EI_DBG_ERR, "axframe Mmap error %x\n", ret);
            continue;
        }
        
        for (i = 0; i < 2; i++) {
            if (axframe_cache[i].axframe_id == 0) {
                cur_frame_id++;
                axframe_cache[i].axframe_id = cur_frame_id;
                memset(&axframe_cache[i].axFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
                //memcpy(&axframe_cache[i].axFrame, &axFrame, sizeof(VIDEO_FRAME_INFO_S));
                axframe_cache[i].axFrame = axFrame;
    
                nn_dms_start(&axframe_cache[i].axFrame, nn_hdl, fout);
                break;
            }
        }
        sem_wait(&axframe_info->frame_del_sem);

        if (axframe_cache[0].axframe_id != 0 && axframe_cache[1].axframe_id != 0) {
            if (axframe_cache[0].axframe_id < axframe_cache[1].axframe_id){
                EI_MI_VBUF_FrameMunmap(&axframe_cache[0].axFrame, VBUF_REMAP_MODE_NOCACHE);
                axframe_cache[0].axframe_id = 0;
                EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                    &axframe_cache[0].axFrame);
            } else {
                EI_MI_VBUF_FrameMunmap(&axframe_cache[1].axFrame, VBUF_REMAP_MODE_NOCACHE);
                axframe_cache[1].axframe_id = 0;
                EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                    &axframe_cache[1].axFrame);
            }
        }
    }
    nn_dms_close(nn_hdl);
	if (facerecg_info.nn_hdl)
		nn_facerecg_release(facerecg_info.nn_hdl);
	if (facerecg_info.match_features) {
		free(facerecg_info.match_features);
		facerecg_info.match_features = NULL;
	}

    for (i = 0; i < 2; i++) {
        if (axframe_cache[i].axframe_id != 0) {
            EI_MI_VBUF_FrameMunmap(&axframe_cache[i].axFrame, VBUF_REMAP_MODE_NOCACHE);
            axframe_cache[i].axframe_id = 0;
            EI_MI_VISS_ReleaseChnFrame(axframe_info->vissDev, axframe_info->vissExtChn, 
                    &axframe_cache[i].axFrame);
        }
    }
    
    sem_post(&axframe_info->frame_del_sem);
    sem_destroy(&axframe_info->frame_del_sem);
    pthread_mutex_destroy(&axframe_info->frame_stLock);
    EI_MI_VISS_DisableChn(axframe_info->vissDev, axframe_info->vissExtChn);

#ifdef SAVE_AX_YUV_SP
    if (fout) {
        fclose(fout);
        fout = NULL;
    }
#endif
    return NULL;
}

void warn_tone_play(char *full_path)
{
	char sys_cmd[256];
	if(strlen(full_path)>249) {
		EI_PRINT("path %s too long\n", full_path);
		return;
	}
	memset(sys_cmd, 0x00, sizeof(sys_cmd));
	sprintf(sys_cmd, "aplay %s", full_path);
	system(sys_cmd);
	return;
}

EI_VOID *warning_proc(EI_VOID *p)
{
	int			err = 0;
	tag_warn_msg_t		msg;
    warning_info_s *warning_info = (warning_info_s *)p;
 
    EI_PRINT("get_frame_proc bThreadStart = %d\n", warning_info->bThreadStart);
	prctl(PR_SET_NAME, "warning_proc thread");

	while (warning_info->bThreadStart) {
		err = osal_mq_recv(warning_info->mq_id, &msg, MQ_MSG_LEN, 50);
		if (err) {
			continue;
		}
		EI_PRINT("msg.type = %d\n", msg.type);
		switch (msg.type) {
			case LB_WARNING_DMS_NO_FACE:
				//warn_tone_play("/usr/share/out/res/dms_warning/jiancebudaorenlian.wav");
				break;
			case LB_WARNING_DMS_CALL:
				warn_tone_play("/usr/share/out/res/dms_warning/qingwudadianhua.wav");
				break;
			case LB_WARNING_DMS_DRINK:
				warn_tone_play("/usr/share/out/res/dms_warning/qingwuheshui.wav");
				break;
			case LB_WARNING_DMS_YAWN:
				warn_tone_play("/usr/share/out/res/dms_warning/qingwupilaojiashi.wav");
				break;
			case LB_WARNING_DMS_SMOKE:
				warn_tone_play("/usr/share/out/res/dms_warning/qingwuxiyan.wav");
				break;
			case LB_WARNING_DMS_ATTATION:
				warn_tone_play("/usr/share/out/res/dms_warning/qingzhuanxinjiashi.wav");
				break;
			case LB_WARNING_DMS_REST:
				warn_tone_play("/usr/share/out/res/dms_warning/qingzhuyixiuxi.wav");
				break;
			case LB_WARNING_DMS_CAMERA_COVER:
				warn_tone_play("/usr/share/out/res/dms_warning/qingwuzhedangshexiangtou.wav");
				break;
			case LB_WARNING_DMS_INFRARED_BLOCK:
				warn_tone_play("/usr/share/out/res/dms_warning/qingwupeidaifanhongwaiyanjing.wav");
				break;
			case LB_WARNING_DMS_CALIBRATE_START:
				warn_tone_play("/usr/share/out/res/dms_warning/DMSjiaozhunkaishi.wav");
				break;
			case LB_WARNING_DMS_CALIBRATE_SUCCESS:
				warn_tone_play("/usr/share/out/res/dms_warning/DMSjiaozhunchenggong.wav");
				break;
			case LB_WARNING_DMS_CALIBRATE_FAIL:
				warn_tone_play("/usr/share/out/res/dms_warning/DMSjiaozhunshibai.wav");
				break;
			default:
				break;
		}
	}

    return NULL;
}

static int _start_get_axframe(axframe_info_s *axframe_info, camera_info_s *camera_info)
{
    int i,ret;
    AXNU_E ax[4] = {NN_BSD0, NN_ADAS, NN_BSD1, NN_DMS}; 
    VISS_DEV vissDev[4] = {0, 0, 0, 1};
    VISS_CHN vissPhyChn[4] = {0, 1, 2, 4};
    VISS_CHN vissExtChn[4] = {VISS_MAX_PHY_CHN_NUM, VISS_MAX_PHY_CHN_NUM+1, VISS_MAX_PHY_CHN_NUM+2, VISS_MAX_PHY_CHN_NUM+3};  //16, 17, 18, 19
    static VISS_EXT_CHN_ATTR_S stExtChnAttr[4] = {{0}};
    for (i = 0; i < 4; i++) {
        stExtChnAttr[i].s32BindChn = vissPhyChn[i];
        stExtChnAttr[i].u32Depth = 2;
        stExtChnAttr[i].stSize.u32Width = 1280;
        stExtChnAttr[i].stSize.u32Height = 720;
        stExtChnAttr[i].u32Align = 32;
        stExtChnAttr[i].enPixFormat = PIX_FMT_YUV_SEMIPLANAR_420;
        stExtChnAttr[i].stFrameRate.s32SrcFrameRate = camera_info[vissPhyChn[i]].u32FrameRate;
        stExtChnAttr[i].stFrameRate.s32DstFrameRate = 10;
        stExtChnAttr[i].s32Position = 1;
        ret = EI_MI_VISS_SetExtChnAttr(vissDev[i], vissExtChn[i], &stExtChnAttr[i]);
        if (ret != EI_SUCCESS) {
            EI_PRINT("EI_MI_VISS_SetExtChnAttr failed with %#x!\n", ret);
            return EI_FAILURE;
        }
        ret = EI_MI_VISS_EnableChn(vissDev[i], vissExtChn[i]);
        if (ret != EI_SUCCESS) {
            EI_PRINT("EI_MI_VISS_EnableChn failed with %#x!\n", ret);
            return EI_FAILURE;
        }

        if(NN_ADAS == ax[i]){
            axframe_info[NN_ADAS].nn_type = NN_ADAS;
            axframe_info[NN_ADAS].bThreadStart = EI_TRUE;
            axframe_info[NN_ADAS].vissDev = vissDev[i];
            axframe_info[NN_ADAS].phyChn = vissPhyChn[i];
            axframe_info[NN_ADAS].vissExtChn = vissExtChn[i];
            ret = pthread_create(&axframe_info[NN_ADAS].gs_axframePid, NULL, adas_get_frame_proc,
                    (EI_VOID *)&axframe_info[NN_ADAS]);
			usleep(100000);
        }  else if (NN_BSD0 == ax[i]) {
            axframe_info[NN_BSD0].nn_type = NN_BSD0;
            axframe_info[NN_BSD0].bThreadStart = EI_TRUE;
            axframe_info[NN_BSD0].vissDev = vissDev[i];
            axframe_info[NN_BSD0].phyChn = vissPhyChn[i];
            axframe_info[NN_BSD0].vissExtChn = vissExtChn[i];
            EI_PRINT("NN_BSD0 vissExtChn = %d\n", vissExtChn[i]);
            ret = pthread_create(&axframe_info[NN_BSD0].gs_axframePid, NULL, bsd0_get_frame_proc,
                        (EI_VOID *)&axframe_info[NN_BSD0]);
			usleep(100000);
        } else if (NN_BSD1 == ax[i]) {
            axframe_info[NN_BSD1].nn_type = NN_BSD1;
            axframe_info[NN_BSD1].bThreadStart = EI_TRUE;
            axframe_info[NN_BSD1].vissDev = vissDev[i];
            axframe_info[NN_BSD1].phyChn = vissPhyChn[i];
            axframe_info[NN_BSD1].vissExtChn = vissExtChn[i];
            EI_PRINT("NN_BSD1 vissExtChn = %d\n", vissExtChn[i]);
            ret = pthread_create(&axframe_info[NN_BSD1].gs_axframePid, NULL, bsd1_get_frame_proc,
                        (EI_VOID *)&axframe_info[NN_BSD1]);
			usleep(100000);
        } else if (NN_DMS == ax[i]) {
            axframe_info[NN_DMS].nn_type = NN_DMS;
            axframe_info[NN_DMS].bThreadStart = EI_TRUE;
            axframe_info[NN_DMS].vissDev = vissDev[i];
            axframe_info[NN_DMS].phyChn = vissPhyChn[i];
            axframe_info[NN_DMS].vissExtChn = vissExtChn[i];
            ret = pthread_create(&axframe_info[NN_DMS].gs_axframePid, NULL, dms_get_frame_proc,
                        (EI_VOID *)&axframe_info[NN_DMS]);
			usleep(100000);
        }
        
        if (ret)
            EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));
    }
    EI_PRINT("_start_get_frame success \n");

    return EI_SUCCESS;
}

static void _stop_get_axframe(axframe_info_s *axframe_info, camera_info_s *camera_info)
{
    int i;
    for (i = 0; i < NN_AX_NUM; i++) {
        axframe_info[i].bThreadStart = EI_FALSE;
    }
    
    for (i = 8; i < 12; i++) {
        EI_MI_VISS_DisableChn(1, i);
    }
}

static int create_algorithm_result_cache()
{
    p_adas_out = malloc(sizeof(nn_adas_out_t));
    if (!p_adas_out) {
        EI_PRINT("p_adas_out malloc failed!\n");
        return EI_FAILURE;
    }
    memset(p_adas_out, 0 , sizeof(nn_adas_out_t));

    p_bsd0_out = malloc(sizeof(nn_bsd_out_t));
    if (!p_bsd0_out) {
        EI_PRINT("p_bsd0_out malloc failed!\n");
        return EI_FAILURE;
    }
    memset(p_bsd0_out, 0 , sizeof(nn_bsd_out_t));
    
    p_bsd1_out = malloc(sizeof(nn_bsd_out_t));
    if (!p_bsd1_out) {
        EI_PRINT("p_bsd1_out malloc failed!\n");
        return EI_FAILURE;
    }
    memset(p_bsd1_out, 0 , sizeof(nn_bsd_out_t));

    p_dms_out = malloc(sizeof(nn_dms_out_t));
    if (!p_dms_out) {
        EI_PRINT("p_dms_out malloc failed!\n");
        return EI_FAILURE;
    }
    memset(p_dms_out, 0 , sizeof(nn_dms_out_t));

    return EI_SUCCESS;
}

static void release_algorithm_result_cache()
{
    if (p_adas_out != NULL){
        free(p_adas_out);
        p_adas_out = NULL;
    }

    if (p_bsd0_out != NULL){
        free(p_bsd0_out);
        p_bsd0_out = NULL;
    }

    if (p_bsd1_out != NULL){
        free(p_bsd1_out);
        p_bsd1_out = NULL;
    }

    if (p_dms_out != NULL){
        free(p_dms_out);
        p_dms_out = NULL;
    }
}

static int _start_draw_algorithm_result(axframe_info_s *axframe_info, region_info_s *region_info, camera_info_s *camera_info)
{   
    int i;
    int ret = EI_SUCCESS;



    for (i = 0; i < 4; i++) {
        if (axframe_info[i].bThreadStart == EI_TRUE) {
            if (axframe_info[i].nn_type == NN_ADAS) {
                region_info[NN_ADAS].nn_type = NN_ADAS;
                region_info[NN_ADAS].stRgnChn.enModId = EI_ID_VISS;
                region_info[NN_ADAS].stRgnChn.s32ChnId = axframe_info[i].phyChn;
                region_info[NN_ADAS].stRgnChn.s32DevId = axframe_info[i].vissDev;
                region_info[NN_ADAS].stSize = camera_info[axframe_info[i].phyChn].stSize;
                ret = _start_update_adas_rectangle_proc(&region_info[NN_ADAS]);
                if (EI_SUCCESS != ret)
                    goto exit;
            } else if (axframe_info[i].nn_type == NN_DMS) {
                region_info[NN_DMS].nn_type = NN_DMS;
                region_info[NN_DMS].stRgnChn.enModId = EI_ID_VISS;
                region_info[NN_DMS].stRgnChn.s32ChnId = axframe_info[i].phyChn;
                region_info[NN_DMS].stRgnChn.s32DevId = axframe_info[i].vissDev;
                region_info[NN_DMS].stSize = camera_info[axframe_info[i].phyChn].stSize;
                ret = _start_update_dms_rectangle_proc(&region_info[NN_DMS]);
                if (EI_SUCCESS != ret)
                    goto exit;
            } else if (axframe_info[i].nn_type == NN_BSD0) {
                region_info[NN_BSD0].nn_type = NN_BSD0;
                region_info[NN_BSD0].stRgnChn.enModId = EI_ID_VISS;
                region_info[NN_BSD0].stRgnChn.s32ChnId = axframe_info[i].phyChn;
                region_info[NN_BSD0].stRgnChn.s32DevId = axframe_info[i].vissDev;
                region_info[NN_BSD0].stSize = camera_info[axframe_info[i].phyChn].stSize;
                ret = _start_update_bsd_rectangle_proc(&region_info[NN_BSD0]);
                if (EI_SUCCESS != ret)
                    goto exit;
            } else if (axframe_info[i].nn_type == NN_BSD1) {
                region_info[NN_BSD1].nn_type = NN_BSD1;
                region_info[NN_BSD1].stRgnChn.enModId = EI_ID_VISS;
                region_info[NN_BSD1].stRgnChn.s32ChnId = axframe_info[i].phyChn;
                region_info[NN_BSD1].stRgnChn.s32DevId = axframe_info[i].vissDev;
                region_info[NN_BSD1].stSize = camera_info[axframe_info[i].phyChn].stSize;
                ret = _start_update_bsd_rectangle_proc(&region_info[NN_BSD1]);
                if (EI_SUCCESS != ret)
                    goto exit;
            }
        }
    }
exit:
    return ret;
}

static void _stop_draw_algorithm_result(region_info_s *region_info)
{
    int i;
    for (i = 0; i < NN_MAX; i++) {
        region_info[i].bThreadStart = EI_FAILURE;
    }
}

EI_VOID *snap_proc(EI_VOID *p)
{
    EI_S32 s32Ret;
    VIDEO_FRAME_INFO_S snapFrame = {0};
    VENC_CHN_STATUS_S stStatus;
    VENC_STREAM_S stStream;
    EI_S32 encoded_packet_num = 0;
    AREA_CODE_S stAreaCode[1] = {0};
    camera_info_s camera_info[VISS_MAX_CHN_NUM] = {{0}};
    VC_CHN VeChn;
	int64_t beforeTime, afaterTime;
    snap_info_s *snap_info;
    snap_info = (snap_info_s *)p;
	
	prctl(PR_SET_NAME, "snap_proc");

    usleep(1000 * 1000);

    snap_info->dev = 0;
    snap_info->chn = 0;
    snap_info->width = 1280;
    snap_info->hight = 720;

    memcpy(camera_info, snap_info->camera_info, sizeof(camera_info));
    
    EI_PRINT("******snap proc start******\n");
    while (EI_TRUE == snap_info->bThreadStart) {
        if (snap_flag) {
            snap_info->chn++;
            if (snap_info->chn >= 8) {
                snap_info->chn = 0;
                if (snap_info->width == 1920 && snap_info->hight == 1080) {
                    snap_info->width = 1280;
                    snap_info->hight = 720;
                } else if (snap_info->width == 1280 && snap_info->hight == 720) {
                    snap_info->width = 640;
                    snap_info->hight = 360;
                } else if (snap_info->width == 640 && snap_info->hight == 360) {
                    snap_info->width = 352;
                    snap_info->hight = 288;
                } else if (snap_info->width == 352 && snap_info->hight == 288) {
                    snap_info->width = 1920;
                    snap_info->hight = 1080;
                } 
            }
            snap_info->dev = snap_info->chn > 3 ? 1 : 0;
        } else {
            usleep(200 * 1000);
            continue;
        }
		beforeTime = _osal_get_msec();
		EI_PRINT("beforeTime %ld \n", beforeTime);
        memset(&snapFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
        EI_PRINT("flag 0\n");
        s32Ret = EI_MI_VISS_GetChnFrame(snap_info->dev, snap_info->chn, &snapFrame, 1000);
        EI_PRINT("flag 1\n");
        if (s32Ret != EI_SUCCESS) {
            EI_PRINT("viss get frame error\n");
            usleep(1000 * 1000);
            continue;
        }
        EI_PRINT("flag 2\n");
        snapFrame.stVencFrameInfo.stAreaCodeInfo.u32AreaNum = 1;
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode = &stAreaCode[0];
        EI_PRINT("flag 3\n");
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].stRegion.s32X = 0;
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].stRegion.s32Y = 0;
        EI_PRINT("flag 4\n");
        EI_PRINT("snap_info->chn = %d \n", snap_info->chn);
        EI_PRINT("u32Width = %x\n", camera_info);
        EI_PRINT("u32Width = %d\n", camera_info[snap_info->chn].stSize.u32Width);
        EI_PRINT("u32Height = %d\n", camera_info[snap_info->chn].stSize.u32Height);
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].stRegion.u32Width  = camera_info[snap_info->chn].stSize.u32Width;
        EI_PRINT("flag 5\n");
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].stRegion.u32Height = camera_info[snap_info->chn].stSize.u32Height;
        EI_PRINT("flag 6\n");
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].u32OutWidth = snap_info->width;
        EI_PRINT("flag 7\n");
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].u32OutHeight = snap_info->hight;
        EI_PRINT("flag 8\n");
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].u32AreaCodeInfo = 0;
        snapFrame.stVencFrameInfo.stAreaCodeInfo.pstAreaCode[0].u32Quality = 80;
        EI_PRINT("flag 9\n");

        EI_PRINT("stAreaCode[0].stRegion.u32Width = %d \n", stAreaCode[0].stRegion.u32Width);
        EI_PRINT("stAreaCode[0].stRegion.u32Height = %d \n", stAreaCode[0].stRegion.u32Height);
        if (stAreaCode[0].stRegion.u32Width == 1920 && stAreaCode[0].stRegion.u32Height == 1080) {
            VeChn = snap_info->VeChn[0];
        } else if (stAreaCode[0].stRegion.u32Width == 1280 && stAreaCode[0].stRegion.u32Height == 720) {
            VeChn = snap_info->VeChn[1];
        } else {
            VeChn = snap_info->VeChn[0];
        }
        EI_PRINT("flag 10\n");

        snapFrame.stVencFrameInfo.bEnableScale = EI_TRUE;
        EI_PRINT("flag 11\n");

        s32Ret = EI_MI_VENC_SendFrame(VeChn, &snapFrame, 1000);
        if (s32Ret != EI_SUCCESS) {
            EI_PRINT("send frame error\n");
            EI_MI_VISS_ReleaseChnFrame(snap_info->dev, snap_info->chn, &snapFrame);
            usleep(20*1000);
            continue;
        }

        EI_MI_VISS_ReleaseChnFrame(snap_info->dev, snap_info->chn, &snapFrame);

        snap_flag = EI_FALSE;

        s32Ret = EI_MI_VENC_QueryStatus(VeChn, &stStatus);
        if (s32Ret != EI_SUCCESS) {
            EI_TRACE_VENC(EI_DBG_ERR, "query status chn-%d error : %d\n", snap_info->VeChn, s32Ret);
            break;
        }

        s32Ret = EI_MI_VENC_GetStream(VeChn, &stStream, 1000);

        if (s32Ret == EI_ERR_VENC_NOBUF) {
            EI_TRACE_VENC(EI_DBG_INFO, "No buffer\n");
            usleep(5 * 1000);
            continue;
        } else if (s32Ret != EI_SUCCESS) {
            EI_TRACE_VENC(EI_DBG_ERR, "get venc chn-%d error : %d\n", VeChn, s32Ret);
            break;
        }

    #ifdef SAVE_OUTPUT_STREAM
        {
            FILE *jpegout_file = NULL;
            EI_CHAR jpegFileName[128];
            snprintf(jpegFileName, 128, "/data/snap-chn%d-%dx%d-%d.jpeg", snap_info->chn,
                                    snap_info->width, snap_info->hight, encoded_packet_num);
            EI_TRACE_VENC(EI_DBG_ERR, "jpeg file : %s\n", jpegFileName);
            jpegout_file = fopen(jpegFileName, "wb");
            if (jpegout_file == NULL) {
                EI_MI_VENC_ReleaseStream(VeChn, &stStream);
                EI_TRACE_VENC(EI_DBG_ERR, "open file[%s] failed!\n", jpegFileName);
                continue;
                //return -1;
            } else {
                s32Ret = SAMPLE_COMM_VENC_SaveStream(&stStream, jpegout_file);
                if (s32Ret != EI_SUCCESS) {
                    EI_MI_VENC_ReleaseStream(VeChn, &stStream);
                    EI_TRACE_VENC(EI_DBG_ERR, "save venc chn-%d error : %d\n", VeChn, s32Ret);
                    continue;
                    //break;
                }
                EI_PRINT("\nGenerate %s \n", jpegFileName);
                fclose(jpegout_file);
                jpegout_file = NULL;
            }
        }
    #endif

        s32Ret = EI_MI_VENC_ReleaseStream(VeChn, &stStream);
        if (s32Ret != EI_SUCCESS) {
            EI_TRACE_VENC(EI_DBG_ERR, "release venc chn-%d error : %d\n", VeChn, s32Ret);
            break;
        }
		afaterTime = _osal_get_msec();
		EI_PRINT("afaterTime %ld \n", afaterTime);
		EI_PRINT("snap time: %ld \n", afaterTime - beforeTime);

        encoded_packet_num++;
         
        //usleep(1000 * 1000);
    }
    return NULL;
}

static int _start_snap_proc(COMMON_FRAME_INFO_S *disp_frame_info, snap_info_s *snap_info)
{
    int ret;
    SAMPLE_VENC_CONFIG_S stSnapJVC[2];
    VENC_JPEG_PARAM_S       stJpegParam = {0};
    
	snap_info->bThreadStart = EI_TRUE;

    stSnapJVC[0].enInputFormat   = disp_frame_info->enPixelFormat;
    stSnapJVC[0].u32width        = 1920;//disp_frame_info->u32Width;
    stSnapJVC[0].u32height       = 1080;//disp_frame_info->u32Height;
    stSnapJVC[0].u32bitrate      = 30*1024*1024;

    ret = SAMPLE_COMM_VENC_Start(snap_info->VeChn[0], PT_JPEG,
                                SAMPLE_RC_FIXQP, &stSnapJVC[0], COMPRESS_MODE_NONE, VENC_GOPMODE_DUAL_P);
    if (EI_SUCCESS != ret) {
        EI_PRINT("SAMPLE_COMM_VENC_Start error\n");
        return EI_FAILURE;
    }

    EI_MI_VENC_GetJpegParam(0, &stJpegParam);
    stJpegParam.bEnableAreaCode = 1;
    EI_MI_VENC_SetJpegParam(0, &stJpegParam);

    stSnapJVC[1].enInputFormat   = disp_frame_info->enPixelFormat;
    stSnapJVC[1].u32width        = 1280;//disp_frame_info->u32Width;
    stSnapJVC[1].u32height       = 720;//disp_frame_info->u32Height;
    stSnapJVC[1].u32bitrate      = 10*1024*1024;
    ret = SAMPLE_COMM_VENC_Start(snap_info->VeChn[1], PT_JPEG,
                                SAMPLE_RC_FIXQP, &stSnapJVC[1], COMPRESS_MODE_NONE, VENC_GOPMODE_DUAL_P);
    if (EI_SUCCESS != ret) {
        EI_PRINT("SAMPLE_COMM_VENC_Start error\n");
        return EI_FAILURE;
    }

    EI_MI_VENC_GetJpegParam(1, &stJpegParam);
    stJpegParam.bEnableAreaCode = 1;
    EI_MI_VENC_SetJpegParam(1, &stJpegParam);


    ret = pthread_create(&snap_info->gs_SnapPid, NULL, snap_proc, (EI_VOID *)snap_info);
    if (ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));

    return EI_SUCCESS;
}

static void _stop_snap(snap_info_s *snap_info)
{
    SAMPLE_COMM_VENC_Stop(snap_info->VeChn[0]);
    SAMPLE_COMM_VENC_Stop(snap_info->VeChn[1]);
}


static int _start_main_stream(sdr_sample_venc_para_t *stream_para,
	int num_stream,
	SAMPLE_VISS_CONFIG_S *viss_config,
	camera_info_s *camera_info,
	COMMON_FRAME_INFO_S *pstVissFrameInfo)
{
	int i;
	int ret = EI_SUCCESS;
	SAMPLE_VENC_CONFIG_S venc_config;
	VISS_DEV viss_dev;
	VISS_CHN viss_chn;

    memset(&venc_config, 0, sizeof(venc_config));
    for (i = 0; i < num_stream; i++) {

        venc_config.enInputFormat	 = pstVissFrameInfo->enPixelFormat;
        venc_config.u32width	 = camera_info[i].stSize.u32Width;
        venc_config.u32height	 = camera_info[i].stSize.u32Height;
        venc_config.u32bitrate	 = MAIN_STREAM_BITRATE;
        venc_config.u32srcframerate = camera_info[i].u32FrameRate;  /* SDR_VISS_FRAMERATE; */
        venc_config.u32dstframerate = MAIN_STREAM_FRAMERATE;
        venc_config.u32IdrPeriod = MAIN_STREAM_FRAMERATE;

        ret = SAMPLE_COMM_VENC_Start(stream_para[i].VeChn, PT_H265,
                SAMPLE_RC_CBR, &venc_config,
                COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
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
    SAMPLE_VENC_CONFIG_S venc_config = {0};
    int ret = EI_SUCCESS;

    venc_config.enInputFormat   = ippu_frame_info->enPixelFormat;
    venc_config.u32width	     = ippu_frame_info->u32Width;
    venc_config.u32height	     = ippu_frame_info->u32Height;
    venc_config.u32bitrate      = SUB_STREAM_BITRATE;
    venc_config.u32srcframerate = SUB_STREAM_FRAMERATE;
    venc_config.u32dstframerate = SUB_STREAM_FRAMERATE;
    venc_config.u32IdrPeriod = SUB_STREAM_FRAMERATE;

    for (i = 0; i < num_stream; i++) {
        ret = SAMPLE_COMM_VENC_Start(stream_para[i].VeChn, PT_H265,
                SAMPLE_RC_CBR, &venc_config,
                COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
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

#define IPPU_RGN_TEST_NUM 12
static int _set_watermark(int modId, int chn, int dev, int id)
{
    EI_S32 s32Ret = EI_FAILURE;
    MDP_CHN_S stRgnChn;
    RGN_ATTR_S stRegion[IPPU_RGN_TEST_NUM] = {{0}};
    RGN_HANDLE Handle[IPPU_RGN_TEST_NUM];
    RGN_CHN_ATTR_S stRgnChnAttr[IPPU_RGN_TEST_NUM] = {{0}};
    EI_S32 s32BytesPerPix = 2;
    BITMAP_S stBitmap;

    /* region */
    stRgnChn.enModId = modId;
    stRgnChn.s32ChnId = chn;
    stRgnChn.s32DevId = dev;
#if 1
    stRegion[0].enType = OVERLAYEX_RGN;
    stRegion[0].unAttr.stOverlayEx.enPixelFmt = PIX_FMT_GREY_Y8;
    stRegion[0].unAttr.stOverlayEx.stSize.u32Width = ISP_WATERMARK_W0;
    stRegion[0].unAttr.stOverlayEx.stSize.u32Height = ISP_WATERMARK_H0;
    stRegion[0].unAttr.stOverlayEx.u32CanvasNum = 2;
#else
    stRegion[0].enType = OVERLAYEX_RGN;
    stRegion[0].unAttr.stOverlayEx.enPixelFmt = PIX_FMT_ARGB_1555;
    stRegion[0].unAttr.stOverlayEx.u32BgColor = 0x0000ffff;
    stRegion[0].unAttr.stOverlayEx.stSize.u32Width = 288;
    stRegion[0].unAttr.stOverlayEx.stSize.u32Height = 16;
    stRegion[0].unAttr.stOverlayEx.u32CanvasNum = 2;
#endif

    stRegion[1].enType = RECTANGLEEX_RGN;

    Handle[0] = get_handle_id();

    s32Ret = EI_MI_RGN_Create(Handle[0], &stRegion[0]);

    stRgnChnAttr[0].bShow = EI_TRUE;
    stRgnChnAttr[0].enType = OVERLAYEX_RGN;
    stRgnChnAttr[0].unChnAttr.stOverlayExChn.stPoint.s32X = 30;
    stRgnChnAttr[0].unChnAttr.stOverlayExChn.stPoint.s32Y = 30 + ISP_WATERMARK_H0 * id;
    stRgnChnAttr[0].unChnAttr.stOverlayExChn.u32FgAlpha = 255;
    stRgnChnAttr[0].unChnAttr.stOverlayExChn.stInvertColorAttr.bInvertColorEn = EI_TRUE;

    s32Ret = EI_MI_RGN_AddToChn(Handle[0], &stRgnChn, &stRgnChnAttr[0]);
    if(s32Ret != EI_SUCCESS) {
        EI_PRINT("EI_MI_RGN_AddToChn err \n");
        return EI_FAILURE;
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
#if 1
    stBitmap.pData = malloc(stBitmap.u32Width * stBitmap.u32Height * s32BytesPerPix);
    memcpy(stBitmap.pData, water_mark_260_41, stBitmap.u32Width * stBitmap.u32Height * s32BytesPerPix);
    s32Ret = EI_MI_RGN_SetBitMap(Handle[0], &stBitmap);
    if(s32Ret != EI_SUCCESS) {
        EI_PRINT("EI_MI_RGN_SetBitMap err \n");
        return EI_FAILURE;
    }
    
#else
    stBitmap.pData = malloc(stBitmap.u32Width * stBitmap.u32Height * s32BytesPerPix);
    //osd_chn1-288x16-103.dat
    FILE *fin = fopen("/data/osd_chn1-288x16-103.dat", "rb");
    if (!fin) {
        EI_PRINT("open osd file fai.\n");
        return EI_FAILURE;
    }
    s32Ret = fread(stBitmap.pData, stBitmap.u32Width * stBitmap.u32Height * s32BytesPerPix, 1, fin);
    if (s32Ret != 1) {
        EI_PRINT("Read rgn data fail....ret:%d\n", s32Ret);
        return EI_FAILURE;
    }
    fclose(fin);
    EI_MI_RGN_SetBitMap(0, &stBitmap);
#endif
    
exit2:
    free(stBitmap.pData);
exit1:
    return EI_SUCCESS;
}


int viss_venc_8ch(void)
{
    int ret = EI_FAILURE;
    int i = 0;
    static int count = 0;
    int loop_time;
    SAMPLE_VISS_CONFIG_S viss_config = {0};
    camera_info_s camera_info[VISS_MAX_CHN_NUM] = {{0}};
    VBUF_CONFIG_S vbuf_config = {0};
    sdr_sample_venc_para_t main_stream_para[VISS_MAX_PHY_CHN_NUM];
    sdr_sample_venc_para_t sub_stream_para[ISP_MAX_DEV_NUM];
    EI_BOOL chn_enabled[IPPU_PHY_CHN_MAX_NUM] = {0};
    display_info_s disp_info;
    snap_info_s snap_info;
    //axframe_info_s axframe_info[5] = {{0}};
    region_info_s region_info[8] = {{0}};

	scn_w = scn_h = scn_rate = 0;
    _get_screen_info(&scn_w, &scn_h, &scn_rate);
    if ((scn_w == 0) || (scn_h == 0) || (scn_rate == 0)) {
	    EI_PRINT("%s %d get screen info fail. \n", __func__, __LINE__);
	    goto exit0;
    }
    EI_PRINT("[%s %d] screen:%u %u %u\n", __func__, __LINE__, scn_w, scn_h, scn_rate);

    ret = _bufpool_init(&vbuf_config);
    if (ret) {
        EI_PRINT("%s %d  \n", __func__, __LINE__);
        goto exit0;
    }

    ret = _start_viss(&viss_config, camera_info);
    if (ret) {
        EI_PRINT("%s %d  \n", __func__, __LINE__);
        goto exit1;
    }
	EI_PRINT("0 %s %d  \n", __func__, __LINE__);
    ret = _start_ippu(chn_enabled, MAIN_STREAM_NUMBER, &viss_config, camera_info,
            &vbuf_config.astFrameInfo[0].stCommFrameInfo,
            &vbuf_config.astFrameInfo[1].stCommFrameInfo);
    if (ret) {
        EI_PRINT("%s %d  \n", __func__, __LINE__);
        goto exit2;
    }
	EI_PRINT("%s %d  \n", __func__, __LINE__);
#if 1
    memset(&disp_info, 0, sizeof(display_info_s));
    ret = _start_disp(0, 0, &vbuf_config.astFrameInfo[2].stCommFrameInfo, &disp_info);
    if (ret) {
        EI_TRACE_LOG(EI_DBG_ERR, "start disp failed\n");
        goto exit3;
    }
#endif
	EI_PRINT("2 %s %d  \n", __func__, __LINE__);
#if 1
    ret = create_algorithm_result_cache();
    if (EI_SUCCESS != ret) {
        EI_TRACE_LOG(EI_DBG_ERR, "create algorithm result cache failed\n");
        goto exit3;
    }

    memset(axframe_info, 0, sizeof(axframe_info));
    ret = _start_get_axframe(axframe_info, camera_info);
    if (ret) {
        EI_TRACE_LOG(EI_DBG_ERR, "get axframe failed\n");
        goto exit3;
    }

    ret = _start_draw_algorithm_result(axframe_info, region_info, camera_info);
    if (EI_SUCCESS != ret) {
        EI_TRACE_LOG(EI_DBG_ERR, "get axframe failed\n");
        goto exit3;
    }
#endif


#if 1
    for (i = 0; i < 8; i++) {
        int dev;
        dev = i > 3 ? 1 : 0;
        for (int j = 0; j < 1; j++) {
            ret = _set_watermark(EI_ID_VISS, i, dev, j);
            if (ret != EI_SUCCESS) {
                EI_PRINT("%s %d _set_watermark: chn:%d id:%d err\n", __func__, __LINE__, i, j);
                goto exit3;
            }
        }
    }

#endif
#if 0
	for (i = 0; i < 8; i++) {
        int chn;
        chn = 1;
        for (int j = 0; j < 2; j++) {
            ret = _set_watermark(EI_ID_IPPU, chn, i, j);
            if (ret != EI_SUCCESS) {
                EI_PRINT("%s %d _set_watermark: chn:%d id:%d err\n", __func__, __LINE__, i, j);
                goto exit3;
            }
        }
    }

	for (i = 0; i < 2; i++) {
        int chn;
        chn = i;
        for (int j = 0; j < 2; j++) {
            ret = _set_watermark(EI_ID_JVC, chn, 0, j);
            if (ret != EI_SUCCESS) {
                EI_PRINT("%s %d _set_watermark: chn:%d id:%d err\n", __func__, __LINE__, i, j);
                goto exit3;
            }
        }
    }
#endif

    memset(main_stream_para, 0, sizeof(main_stream_para));
    memset(sub_stream_para, 0, sizeof(sub_stream_para));
    for (i = 0; i < 8; i++) {
        main_stream_para[i].VeChn = i+2;
        main_stream_para[i].bitrate = MAIN_STREAM_BITRATE;
        main_stream_para[i].file_idx = 0;
        main_stream_para[i].muxer_type = MAIN_STREAM_MUXER_TYPE;
        sub_stream_para[i].VeChn = i + 8 + 2;
        sub_stream_para[i].bitrate = SUB_STREAM_BITRATE;
        sub_stream_para[i].file_idx = 0;
        sub_stream_para[i].muxer_type = SUB_STREAM_MUXER_TYPE;
    }

#if 1
    memset(&snap_info, 0, sizeof(snap_info_s));
    snap_info.camera_info = malloc(sizeof(camera_info));
    snap_info.VeChn[0] = 0; 
    snap_info.VeChn[1] = 1;
    memcpy(snap_info.camera_info, camera_info, sizeof(camera_info));
    //snap_info.camera_info = camera_info;
    _start_snap_proc(&vbuf_config.astFrameInfo[0].stCommFrameInfo, &snap_info);
#endif

    while (1) {
        
        _fallocate_stream_file(main_stream_para, MAIN_STREAM_NUMBER);
        _fallocate_stream_file(sub_stream_para, SUB_STREAM_NUMBER);
    
        ret = _start_main_stream(main_stream_para, MAIN_STREAM_NUMBER, &viss_config, camera_info,
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
            if (app_exit_flag)
                goto exit5;
            if (loop_time % 5 == 4)
                EI_PRINT("loop_time %d\n", loop_time);
            EI_TRACE_LOG(EI_DBG_ERR,"loop_time %d\n", loop_time);

        }

        _stop_main_stream(main_stream_para, MAIN_STREAM_NUMBER, &viss_config);
        _stop_sub_stream(sub_stream_para, SUB_STREAM_NUMBER);
    }
    _stop_disp(0, 0, &disp_info);
    _stop_ippu(MAIN_STREAM_NUMBER, chn_enabled);
    _stop_viss(&viss_config);
    ret = _bufpool_deinit(&vbuf_config);
    if (ret)
        EI_PRINT("====_bufpool_deinit failed\n");
    
    return 0;
exit5:
    _stop_main_stream(main_stream_para, MAIN_STREAM_NUMBER, &viss_config);
    _stop_sub_stream(sub_stream_para, SUB_STREAM_NUMBER);
exit4:
    _stop_disp(0, 0, &disp_info);
exit3:
    _stop_ippu(SUB_STREAM_NUMBER, chn_enabled);

    _stop_get_axframe(axframe_info, camera_info);
    release_algorithm_result_cache();
exit2:
    
    _stop_snap(&snap_info);
    EI_PRINT("%s %d  \n", __func__, __LINE__);
    _stop_viss(&viss_config);
exit1:
    EI_PRINT("%s %d  \n", __func__, __LINE__);
    _bufpool_deinit(&vbuf_config);
exit0:
    EI_PRINT("%s %d  \n", __func__, __LINE__);
    return 0;
}

void disp_switch(int signo)
{
    printf("%s %d, signal %d \n", __func__, __LINE__, signo);
    disp_switch_flag = EI_TRUE;
    //app_exit_flag = EI_TRUE;
}

void snap(int signo)
{
    printf("%s %d, signal %d \n", __func__, __LINE__, signo);
    snap_flag = EI_TRUE;
    //disp_switch_flag = EI_TRUE;
}

int main(int argc, char **argv)
{
    int ret = EI_FAILURE;

    signal(SIGINT, disp_switch);
    signal(SIGTSTP, snap);

	warning_info.mq_id = osal_mq_create("/waring_mq", MQ_MSG_NUM, MQ_MSG_LEN);
	if (warning_info.mq_id == 0) {
		EI_PRINT("[%s, %d]Create waring queue failed!\n", __func__, __LINE__);
		goto exit;
	}
	warning_info.bThreadStart = EI_TRUE;
    ret = pthread_create(&(warning_info.g_Pid), NULL, warning_proc,
        (EI_VOID *)&warning_info);
    if (ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));

    ret = viss_venc_8ch();

	warning_info.bThreadStart = EI_FALSE;
	if (warning_info.g_Pid) {
           pthread_join(warning_info.g_Pid, NULL);
	}
	ret = osal_mq_destroy(warning_info.mq_id, "/waring_mq");
	if (ret) {
		EI_PRINT("[%s, %d]waring_mq destory failed!\n", __func__, __LINE__);
	}

exit:
    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
