#define _GNU_SOURCE

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

#include "sample_comm.h"
#include "sdrive_sample_muxer.h"
#include "libuvcg.h"
#include "uvc_i.h"

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

//#define USE_MUXER
#define PATH_ROOT "/mnt/card"
#define SDR_SAMPLE_VENC_TIME 300
#define WRITE_BUFFER_SIZE 1*1024*1024
#define SDR_SAMPLE_FILE_NUM 6

extern uvc_config_t g_cfg;
extern uvc_context_t g_ctx;
int start_flag = 0;
sdr_sample_venc_para_t g_stream_para;

static int64_t _osal_get_msec(void)
{
	struct timespec t;
	t.tv_sec = t.tv_nsec = 0;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	_DBG("t.tv_sec:%ld, t.tv_nsec:%ld \n", t.tv_sec, t.tv_nsec);
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
		_DBG("[%s %d] request fail.\n", __func__, __LINE__);
		return EI_FAILURE;
	}
	
	fp = setmntent("/proc/mounts", "r");
	if (fp == NULL) {
		_DBG("open error mount proc");
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
			_DBG("mnt_dir is NULL. \n");
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
		_DBG("open %s fail\n", path);
		return;
	}
	res = fallocate(fd, FALLOC_FL_KEEP_SIZE, 0, fsize);
	close(fd);

	if (res)
		_DBG("fallocate fail(%i)\n", res);
	else
		_DBG("fallocate %s finish\n", path);
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
		_DBG("open file %s failed!\n", aszFileName);
		return -1;
	}

	sdr_venc_para->buf_offset = 0;
	if (sdr_venc_para->filebuf) {
		_DBG("filebuf is not null(%p)!\n", sdr_venc_para->filebuf);
		return 0;
	}
	sdr_venc_para->filebuf = malloc(WRITE_BUFFER_SIZE);
	if (!sdr_venc_para->filebuf) {
		fclose(sdr_venc_para->flip_out);
		sdr_venc_para->flip_out= NULL;
		_DBG("malloc[%d] failed!\n", WRITE_BUFFER_SIZE);
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
		    _DBG("write fail. ret:%d size:%d\n", ret, size);
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
		_DBG("save stream failed! buf_offset:%d flip_out:%p len:%d addr:%p\n",
		    sdr_venc_para->buf_offset, sdr_venc_para->flip_out, 
		    pstStream->pstPack.u32Len[0], pstStream->pstPack.pu8Addr[0]);
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

static int __get_time_string(char *buf, int size)
{
    time_t timep;
    time(&timep);
    struct tm *p = localtime(&timep);
    snprintf(buf, size-1, "%04d%02d%02d%02d%02d%02d", (1900+p->tm_year),
            (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    return 0;
}

static inline int save_stream_init(sdr_sample_venc_para_t *sdr_venc_para)
{
	int ret = -1;
	char aszFileName[128];
	char *szFilePostfix;
	int type;
	char t_buf[64] = {0};
	static sdcard_info_t info;
    rec_channel_config_t *stream_cfg = &g_cfg.recorder;

    _DBG("\n");
	if (sdr_venc_para == NULL)
		return -1;
	type = sdr_venc_para->muxer_type;

#ifndef USE_MUXER
    type = AV_MUXER_TYPE_RAW;
    sdr_venc_para->muxer_type = type;
#endif
	
	if (type == AV_MUXER_TYPE_RAW) {
	    if (stream_cfg->video_format == PT_H264) {
		    szFilePostfix = ".h264";
	    } else {
		    szFilePostfix = ".h265";
		}
	} else if (type == AV_MUXER_TYPE_TS)
		szFilePostfix = ".ts";
	else if (type == AV_MUXER_TYPE_MP4)
		szFilePostfix = ".mp4";
	else {
		return -1;
	}

	storage_info_get(PATH_ROOT, &info);
	if (info.free < 500 * 1024) {
	    _DBG("Disk space not enough.\n");
		return -1;
	}

    __get_time_string(t_buf, sizeof(t_buf));
	snprintf(aszFileName, 128, PATH_ROOT"/stream_chn%02d-%03d_%s%s",
		sdr_venc_para->VeChn,
		sdr_venc_para->file_idx, t_buf, szFilePostfix);
	if (type == AV_MUXER_TYPE_TS || type == AV_MUXER_TYPE_MP4){
		sdr_venc_para->muxer_hdl = sample_muxer_init(
            type, aszFileName, stream_cfg->width, stream_cfg->height, type, sdr_venc_para->VeChn);
		if (sdr_venc_para->muxer_hdl)
			ret = 0;
	}
    _DBG("aszFileName:%s type:%d\n", aszFileName, type);
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
	VENC_BITRATE_CTRL_S stBitrate = {0};
	char thread_name[16];

	sdr_venc_para = (sdr_sample_venc_para_t *)p;
	if (!sdr_venc_para) {
		_DBG("err param !\n");
		return NULL;
	}

	VencChn = sdr_venc_para->VeChn;

	thread_name[15] = 0;
	snprintf(thread_name, 16, "streamproc%d", VencChn);
	prctl(PR_SET_NAME, thread_name);

	EI_MI_VENC_GetTargetBitrate(VencChn, &stBitrate);

	_DBG("video encoder bitrate:%d\n", stBitrate.u32TargetBitrate);
	while (EI_TRUE == sdr_venc_para->bThreadStart) {
		ret = EI_MI_VENC_GetStream(VencChn, &stStream, 100);
		if (ret == EI_ERR_VENC_NOBUF) {
			_DBG("No buffer\n");
			usleep(5 * 1000);
			continue;
		} else if (ret != EI_SUCCESS) {
			_DBG("get chn-%d error : %d\n", VencChn, ret);
			break;
		}
		if (stStream.pstPack.u32Len[0] == 0) {
			_DBG("%d ch %d pstPack.u32Len:%d-%d, ofset:%d, ret:%x\n",
				__LINE__, VencChn, stStream.pstPack.u32Len[0],
				stStream.pstPack.u32Len[1],
				sdr_venc_para->buf_offset, ret);
		}
		
        //_DBG("sdr_venc_para->init_flag:%d enDataType:%d\n", sdr_venc_para->init_flag, stStream.pstPack.enDataType);
		if (sdr_venc_para->init_flag == 0 && 
            (stStream.pstPack.enDataType == DATA_TYPE_I_FRAME || stStream.pstPack.enDataType == DATA_TYPE_INITPACKET)) {
			save_stream_init(sdr_venc_para);
		}
		if (sdr_venc_para->init_flag) {
			ret = save_stream(sdr_venc_para, &stStream);
			if (ret) {
				_DBG("save stream err!");
				save_stream_deinit(sdr_venc_para);
			}
		}
		encoded_packet++;
		ret = EI_MI_VENC_ReleaseStream(VencChn, &stStream);
		if (ret != EI_SUCCESS) {
			_DBG("release stream chn-%d error : %d\n",
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
        _DBG("errno=%d, reason(%s)\n", errno, strerror(errno));
    return ret;
}

int _stop_get_stream(sdr_sample_venc_para_t *venc_para)
{
    if (venc_para == NULL ||
        venc_para->VeChn < 0 || venc_para->VeChn >= VC_MAX_CHN_NUM) {
        _DBG("%s %d  venc_para:%p:%d\n", __func__, __LINE__,
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

EI_S32 start_rec_stream(void)
{
	int ret = EI_SUCCESS;
	IPPU_CHN PhyChn = 0;
	SAMPLE_VENC_CONFIG_S venc_config;
    IPPU_CHN_ATTR_S astIppuChnAttr[IPPU_PHY_CHN_MAX_NUM] = {{0}};
    IPPU_ROTATION_ATTR_S astIppuChnRot[IPPU_PHY_CHN_MAX_NUM] = {{0}};
	rec_channel_config_t *stream_cfg = &g_cfg.recorder;
	VISS_CHN_ATTR_S stChnAttr = {0};

    SAMPLE_COMM_VISS_GetChnAttrBySns(g_ctx.vissConfig.astVissInfo[0].stSnsInfo.enSnsType, &stChnAttr);

    if (stream_cfg->ippu_chn >= IPPU_PHY_CHN_MAX_NUM) {
        _DBG("Unsupport input ext channel.\n");
        return -1;
    }
    PhyChn = stream_cfg->ippu_chn;

    astIppuChnAttr[PhyChn].u32Width      = stream_cfg->width;
    astIppuChnAttr[PhyChn].u32Height     = stream_cfg->height;
    astIppuChnAttr[PhyChn].u32Align      = 32;
    astIppuChnAttr[PhyChn].enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    astIppuChnAttr[PhyChn].stFrameRate.s32SrcFrameRate = stChnAttr.stFrameRate.s32SrcFrameRate;
    astIppuChnAttr[PhyChn].stFrameRate.s32DstFrameRate = stream_cfg->framerate;
    astIppuChnAttr[PhyChn].u32Depth      = 2;
    
    if ((stream_cfg->rotation == ROTATION_90) || (stream_cfg->rotation == ROTATION_180) || (stream_cfg->rotation == ROTATION_270)) {
        astIppuChnRot[PhyChn].bEnable = EI_TRUE;
        astIppuChnRot[PhyChn].eRotation = stream_cfg->rotation;
        EI_MI_IPPU_SetChnRotation(stream_cfg->ippu_dev, PhyChn, &astIppuChnRot[PhyChn]);
    }
    ret = EI_MI_IPPU_SetChnAttr(stream_cfg->ippu_dev, stream_cfg->ippu_chn, &astIppuChnAttr[PhyChn]);
    if (ret != EI_SUCCESS) {
        _DBG("EI_MI_IPPU_SetChnAttr failed with %#x\n", ret);
        return -1;
    }
    
    ret = EI_MI_IPPU_EnableChn(stream_cfg->ippu_dev, stream_cfg->ippu_chn);
    if (ret != EI_SUCCESS) {
        _DBG("EI_MI_IPPU_EnableChn failed with %#x\n", ret);
        return -1;
    }


    memset(&venc_config, 0, sizeof(venc_config));
    venc_config.enInputFormat    = PIX_FMT_YUV_SEMIPLANAR_420;
    venc_config.u32width     = stream_cfg->width;
    venc_config.u32height    = stream_cfg->height;
    venc_config.u32bitrate   = stream_cfg->bitrate;
    venc_config.u32srcframerate = stChnAttr.stFrameRate.s32SrcFrameRate;  /* SDR_VISS_FRAMERATE; */
    venc_config.u32dstframerate = stream_cfg->framerate;
    venc_config.u32IdrPeriod = stream_cfg->framerate * 2;
    
    ret = SAMPLE_COMM_VENC_Start(stream_cfg->venc_chn, stream_cfg->video_format,
            SAMPLE_RC_CBR, &venc_config,
            COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
    if (EI_SUCCESS != ret) {
        _DBG("VENC_Start %d fail\n", stream_cfg->venc_chn);
        return -1;
    }
    
    ret = SAMPLE_COMM_IPPU_Link_VPU(stream_cfg->ippu_dev, stream_cfg->ippu_chn, stream_cfg->venc_chn);
    if (EI_SUCCESS != ret) {
        _DBG("ippu link vpu failed %d-%d-%d \n", stream_cfg->ippu_dev, stream_cfg->ippu_chn, stream_cfg->venc_chn);
        return -1;
    }
    
    g_stream_para.muxer_type = AV_MUXER_TYPE_RAW;
    g_stream_para.bitrate = venc_config.u32bitrate;
    g_stream_para.VeChn = stream_cfg->venc_chn;
    ret = _start_get_stream(&g_stream_para);
    if (EI_SUCCESS != ret) {
        _DBG("_start_get_stream failed %d-%d-%d \n",  stream_cfg->ippu_dev, stream_cfg->ippu_chn, stream_cfg->venc_chn);
        SAMPLE_COMM_VISS_UnLink_VPU(stream_cfg->ippu_dev, stream_cfg->ippu_chn, stream_cfg->venc_chn);
        SAMPLE_COMM_VENC_Stop(g_stream_para.VeChn);
        return -1;
    }
    start_flag = 1;
    
    return 0;
}

EI_S32 stop_rec_stream(void)
{
	rec_channel_config_t *stream_cfg = &g_cfg.recorder;

    if (!start_flag) {
        _DBG("Recorder not start.\n");
        return -1;
    }
    _stop_get_stream(&g_stream_para);
    SAMPLE_COMM_VISS_UnLink_VPU(stream_cfg->ippu_dev, stream_cfg->ippu_chn, g_stream_para.VeChn);
    SAMPLE_COMM_VENC_Stop(g_stream_para.VeChn);
    EI_MI_IPPU_DisableChn(stream_cfg->ippu_dev, stream_cfg->ippu_chn);
    save_stream_deinit(&g_stream_para);
    start_flag = 0;

    return 0;
}

