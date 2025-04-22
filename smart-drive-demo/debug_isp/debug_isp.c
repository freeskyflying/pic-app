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
#include <sys/stat.h>
#include <sys/types.h>
#include "sample_comm.h"

#define EI_TRACE_LOG(level, fmt, args...)\
do{ \
    EI_TRACE(EI_ID_LOG, level, fmt, ##args);\
} while (0)


#define MDA_FMT		"[M][%s:%d] "

#define _DBG(fmt, ...) \
    do { \
        struct timespec t; \
        clock_gettime(CLOCK_MONOTONIC, &t); \
        fprintf(stderr, "[%7ld.%03ld]"MDA_FMT fmt "\n", \
            t.tv_sec, t.tv_nsec/1000000L,  \
            __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)
typedef struct __sample_venc_para_t {
    EI_BOOL bThreadStart;
    VC_CHN VeChn;
    int bitrate;
    int file_idx;
    int pt_type;
    int muxer_type;
    int init_flag;

    FILE *flip_out;
    char *filebuf;
    int buf_offset;
    char filename[64];
    int len;

    void *muxer_hdl;
    pthread_t gs_VencPid;

    EI_BOOL GetFrmFlag;
    pthread_t gs_GetFrmPid;
} sdr_sample_venc_para_t;

#define TEST_SRC_WIDTH 1920
#define TEST_SRC_HEIGHT 1080

#define MAIN_STREAM_BITRATE 8 * 1024 * 1024
#define MAIN_STREAM_FRAMERATE 25 /* */
#define MAIN_STREAM_MUXER_TYPE AV_MUXER_TYPE_TS
#define DISP_ENC_CHANNEL   (0) 

#define VO_DEVICE  OTM1289A_MIPI_720_1280_60FPS

#define PATH_ROOT "/mnt/card/"
VISS_ROTATION_ATTR_S stChnRotAttr = {0};
IPPU_ROTATION_ATTR_S pstChnRot = {0};
IPPU_CHN_ATTR_S pstChnAttr = {0};


static EI_U32 scn_w, scn_h, scn_rate;

static void write_nv21_data(VIDEO_FRAME_INFO_S *info, FILE *file)
{
    if (!file) {
        //EI_TRACE_VENC(EI_DBG_ERR, "open %s failed\n", name);
        _DBG("file == NULL");
        return;
    }
    #if 1
    /* 这样写入数据画面会不太正常[720P的帧数据不是32对齐的] */
    for (int i=0; i<info->stVFrame.u32PlaneNum; i++) {
        int n= fwrite(info->stVFrame.ulPlaneVirAddr[i],
            info->stVFrame.u32PlaneSizeValid[i],
            1, file);
        if (n != 1) {
            //EI_TRACE_VENC(EI_DBG_ERR, "fwrite %s plane %d size %d failed\n", name, i,  n);
            _DBG("fwrite error, i = %d, n = %d", i, n);
        }
    }
    #else
    //_DBG("u32PlaneNum = %d, Size = %d x %d", info->stVFrame.u32PlaneNum, 
    //    info->stCommFrameInfo.u32Width, info->stCommFrameInfo.u32Height);
    fwrite((char *)info->stVFrame.ulPlaneVirAddr[0],
        info->stCommFrameInfo.u32Width * info->stCommFrameInfo.u32Height, 1, file);
    fwrite((char *)info->stVFrame.ulPlaneVirAddr[1],
        info->stCommFrameInfo.u32Width * info->stCommFrameInfo.u32Height >> 1, 1, file);
    #endif

}
FILE *  open_yuv_file(char * filename)
{
    FILE *file = fopen(filename, "wb");
    _DBG("Open %s File, %p", filename, file);
    return file;
}
int save_yuv_data1(VIDEO_FRAME_INFO_S * frameinfo, FILE * fp)
{
    if (frameinfo == NULL || fp == NULL)
    {
        return -1;
    }
    int ret = EI_MI_VBUF_FrameMmap(frameinfo, VBUF_REMAP_MODE_NOCACHE);
    if (ret != EI_SUCCESS) 
    {
        _DBG("wb vbuf mmap fail\n");
        return -1;
    }
    
    write_nv21_data(frameinfo, fp);
    EI_MI_VBUF_FrameMunmap(frameinfo, VBUF_REMAP_MODE_NOCACHE);
    
    return 0;
}

int save_yuv_data2(VENC_STREAM_S *pstStream, FILE *fp)
{
    int size = 0;
	int ret = 0;
    if (pstStream == NULL || fp == NULL)
    {
        return -1;
    }

    if (pstStream->pstPack.u32Len[0] >  0) {
        ret = fwrite((char *)pstStream->pstPack.pu8Addr[0], 1, pstStream->pstPack.u32Len[0], fp);
		if (ret < 0)
			return ret;
        size += pstStream->pstPack.u32Len[0];
    }
    if (pstStream->pstPack.u32Len[1] >  0) {
        ret = fwrite((char *)pstStream->pstPack.pu8Addr[1], 1, pstStream->pstPack.u32Len[1], fp);
		if (ret < 0)
			return ret;
        size += pstStream->pstPack.u32Len[1];
    }
    	return size;
}

int close_yuv_file(FILE ** fp)
{
    _DBG("Close YUV File");
    if (*fp) {
        fclose(*fp);
    }
    *fp = NULL;
    return 0;
}

int64_t _get_msec(void)
{
	struct timespec t;
	t.tv_sec = t.tv_nsec = 0;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	return (int64_t)(t.tv_sec)*1000LL + t.tv_nsec/1000000LL;
}

typedef struct {
	unsigned int total;
	unsigned int free;
} storage_info_t;

static void _storage_info_get(const char *mount_point, storage_info_t *info)
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
	if (mount_path[len-1] == '/')
		mount_path[len-1] = '\0';
	fp = fopen("/proc/mounts", "r");
	if (fp == NULL) {
		_DBG("open error mount proc %s", strerror(errno));
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


EI_VOID *_get_venc_stream_proc(EI_VOID *p)
{
    sdr_sample_venc_para_t *sdr_venc_para;
    VC_CHN VencChn;
    VENC_STREAM_S stStream;
    int ret, encoded_packet = 0;
    char thread_name[16];
    FILE *dbg_fp = NULL;
    unsigned int total_size = 0;
    int64_t time1, time2;
	ISP_FRAME_INFO_S stIspFrame;

    sdr_venc_para = (sdr_sample_venc_para_t *)p;
    if (!sdr_venc_para) {
        EI_TRACE_LOG(EI_DBG_ERR, "err param !\n");
        return NULL;
    }

    VencChn = sdr_venc_para->VeChn;

    thread_name[15] = 0;
    snprintf(thread_name, 16, "streamproc%d", VencChn);
    prctl(PR_SET_NAME, thread_name);
    time2 = time1 = _get_msec();

    while (EI_TRUE == sdr_venc_para->bThreadStart) {
        memset(&stStream, 0, sizeof(VENC_STREAM_S));
        ret = EI_MI_VENC_GetStream(VencChn, &stStream, 300);
        if (ret == EI_ERR_VENC_NOBUF) {
            EI_TRACE_LOG(EI_DBG_INFO, "No buffer\n");
            if (VencChn == DISP_ENC_CHANNEL)
            {
               _DBG("[EI_MI_VENC_GetStream] VencChn = %d, NO Buffer \n", VencChn);
            }
            usleep(5 * 1000);
            continue;
        } else if (ret != EI_SUCCESS) {
            EI_TRACE_LOG(EI_DBG_ERR, "get chn-%d error : %d\n", VencChn, ret);
            _DBG("[EI_MI_VENC_GetStream] get chn-%d error : %d \n", VencChn, ret);
            break;
        }
		EI_MI_ISP_GetFrameInfo(0, &stIspFrame);
		//EI_PRINT("stIspFrame.s32BV: %d\n", stIspFrame.s32BV);
        if (stStream.pstPack.u32Len[0] == 0) {
            EI_PRINT("%d ch %d pstPack.u32Len:%d-%d, ofset:%d, ret:%x\n",
                __LINE__, VencChn, stStream.pstPack.u32Len[0],
                stStream.pstPack.u32Len[1],
                sdr_venc_para->buf_offset, ret);
            _DBG("%d ch %d pstPack.u32Len:%d-%d, ofset:%d, ret:%x\n",
                __LINE__, VencChn, stStream.pstPack.u32Len[0],
                stStream.pstPack.u32Len[1],
                sdr_venc_para->buf_offset, ret);
        }
        if (sdr_venc_para->init_flag == 0 
            && ((stStream.pstPack.enDataType == DATA_TYPE_I_FRAME) || (stStream.pstPack.enDataType == DATA_TYPE_INITPACKET))) {
                char path[128] = {0};                
                char *mount_point = PATH_ROOT;
                storage_info_t store_info;
                EI_U64 u64CurPTS;

               /* memset(&store_info, 0, sizeof(store_info));
                _storage_info_get(mount_point, &store_info);
                _DBG("disk space: total:%d free:%d\n", store_info.total, store_info.free);
                 if (store_info.free < (512 * 1024)) {
                    _DBG("%s free space too small.\n");
                    continue;
                } */
                EI_MI_MBASE_GetCurPTS(&u64CurPTS);

                sprintf(path, "%s/%llu.h265", PATH_ROOT, u64CurPTS);
                dbg_fp = open_yuv_file(path);
                if (dbg_fp) {
                    time1 = _get_msec();
                    sdr_venc_para->init_flag = 1;                    
                    _DBG("open %s success. dbg_fp:%p\n", path, dbg_fp);
                } else {
                    _DBG("open %s fail. dbg_fp:%p\n", path, dbg_fp);
                }
        }
       // _DBG("total_size:%u %u", total_size, (MAIN_STREAM_BITRATE * 1 * 60 / 8));
       time2 = _get_msec();
        if (dbg_fp && ((time2 - time1) <= (1 * 20 * 1000))) {
            ret = save_yuv_data2(&stStream, dbg_fp);
			if (ret < 0) {
				sdr_venc_para->bThreadStart = EI_FALSE;
			} else {
				total_size += ret;
			}
        } else if (dbg_fp) {
            _DBG("close %p.\n", dbg_fp);
            close_yuv_file(&dbg_fp);
            system("sync");
            sdr_venc_para->init_flag = 0;
            total_size = 0;
        }
        encoded_packet++;
        ret = EI_MI_VENC_ReleaseStream(VencChn, &stStream);
        if (ret != EI_SUCCESS) {
            EI_TRACE_LOG(EI_DBG_ERR, "release stream chn-%d error : %d\n",
                VencChn, ret);
            break;
        }        
    }
    close_yuv_file(&dbg_fp);
 
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

EI_VOID *_get_vo_frame_proc(EI_VOID *p)
{
    sdr_sample_venc_para_t *sdr_venc_para;
    VC_CHN VencChn;
    int ret;
    char thread_name[16];

    sdr_venc_para = (sdr_sample_venc_para_t *)p;
    if (!sdr_venc_para) {
        EI_TRACE_LOG(EI_DBG_ERR, "err param !\n");
        return NULL;
    }

    VencChn = sdr_venc_para->VeChn;

    thread_name[15] = 0;
    snprintf(thread_name, 16, "getframe%d", VencChn);
    prctl(PR_SET_NAME, thread_name);

#define SAVE_WBC_YUV 1  /* 保存yuv, 就不编码了 */
#if SAVE_WBC_YUV
    FILE * fp = NULL;
    int frame_num = 0;
    if (VencChn == DISP_ENC_CHANNEL)
    {
        char wbc_file_name[128]=  {0};
        snprintf(wbc_file_name, sizeof(wbc_file_name), "%s/wb.yuv", PATH_ROOT);
        fp =  open_yuv_file(wbc_file_name);
        _DBG("wbc_file_name = %s", wbc_file_name);
    }
#endif    
    while (EI_TRUE == sdr_venc_para->GetFrmFlag) {
        if (VencChn == DISP_ENC_CHANNEL)
        {
            VO_WBC wbc = 0;
            VIDEO_FRAME_INFO_S stVideoFrameInfo = {0};

            ret = EI_MI_VO_GetWBCFrame(wbc, (VIDEO_FRAME_INFO_S *)&stVideoFrameInfo.stVFrame, 100);
    	    if (ret) {
    	    	EI_TRACE_VENC(EI_DBG_ERR, "s32Ret[%d]", ret);
                _DBG("[EI_MI_VO_GetWBCFrame]wbc = %d, ret = 0x%X", wbc, ret);
    	        continue;
    	    }

#if SAVE_WBC_YUV
            frame_num++;
            if (frame_num <= 50)
            {
               save_yuv_data1(&stVideoFrameInfo, fp);
            }
            else
            {
                if (fp != NULL)
                {
                    close_yuv_file(&fp);
                    fp = NULL;
                }
            }
#endif
        _DBG("Size = %d x %d", stVideoFrameInfo.stCommFrameInfo.u32Width, stVideoFrameInfo.stCommFrameInfo.u32Height);
		ret = EI_MI_VENC_SendFrame(VencChn, &stVideoFrameInfo, 100);
		if (ret != EI_SUCCESS)
		{
			ret = EI_MI_VO_ReleaseWBCFrame(wbc, &stVideoFrameInfo);
			_DBG("[EI_MI_VENC_SendFrame] send frame error");
			usleep(20*1000);
			continue;
		}

		ret = EI_MI_VO_ReleaseWBCFrame(wbc, &stVideoFrameInfo);
        if (ret) {
            EI_TRACE_VENC(EI_DBG_ERR, "s32Ret[%d]", ret);
            _DBG("[EI_MI_VO_ReleaseWBCFrame] ret = %d", ret);
            //break;
        }
        #if 0
        VENC_CHN_STATUS_S stStatus;
        memset(&stStatus, 0, sizeof(stStatus));
		ret = EI_MI_VENC_QueryStatus(VencChn, &stStatus);
		if (ret != EI_SUCCESS)
		{
			_DBG("[EI_MI_VENC_QueryStatus] query status chn-%d error : %d", VencChn, ret);
			usleep(20*1000);
			continue;
		}
	    #endif
        }    
    }
#if SAVE_WBC_YUV    
    if (fp != NULL)
    {
        close_yuv_file(&fp);
        fp = NULL;
    }
#endif
    return NULL;
}


int _start_get_frame(sdr_sample_venc_para_t *venc_para)
{
    int ret;
    if (venc_para == NULL || venc_para->VeChn < 0 ||
            venc_para->VeChn >= VC_MAX_CHN_NUM)
        return EI_FAILURE;

    venc_para->GetFrmFlag = EI_TRUE;

    ret = pthread_create(&venc_para->gs_GetFrmPid, 0, _get_vo_frame_proc,
            (EI_VOID *)venc_para);
    if (ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));
    return ret;
}

int _stop_get_frame(sdr_sample_venc_para_t *venc_para)
{
    if (venc_para == NULL ||
        venc_para->VeChn < 0 || venc_para->VeChn >= VC_MAX_CHN_NUM) {
        EI_PRINT("%s %d  venc_para:%p:%d\n", __func__, __LINE__,
            venc_para, venc_para->VeChn);
        return EI_FAILURE;
    }
    venc_para->GetFrmFlag = EI_FALSE;
    if (venc_para->gs_GetFrmPid) {
        pthread_join(venc_para->gs_GetFrmPid, 0);
        venc_para->gs_GetFrmPid = 0;
    }
    return EI_SUCCESS;
}


static int _bufpool_init(VBUF_CONFIG_S *vbuf_config)
{
    memset(vbuf_config, 0, sizeof(VBUF_CONFIG_S));

    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].stCommFrameInfo.u32Width = 1920; 
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].stCommFrameInfo.u32Height = 1080;    
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[vbuf_config->u32PoolCnt].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].stCommFrameInfo.enCompressMode = COMPRESS_MODE_NONE;
    vbuf_config->astCommPool[vbuf_config->u32PoolCnt].u32BufCnt = 10;
    vbuf_config->u32PoolCnt++;
#if 0
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].enFrameType = MDP_FRAME_TYPE_DOSS;//MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].stCommFrameInfo.u32Width = 1280;//1920; 
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].stCommFrameInfo.u32Height = 720;//1080;    
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[vbuf_config->u32PoolCnt].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].stCommFrameInfo.enCompressMode = COMPRESS_MODE_NONE;
    vbuf_config->astCommPool[vbuf_config->u32PoolCnt].u32BufCnt = 10;
    vbuf_config->u32PoolCnt++;
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].enFrameType = MDP_FRAME_TYPE_COMMON;
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].stCommFrameInfo.u32Align = 32;
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].stCommFrameInfo.u32Width = 352;
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].stCommFrameInfo.u32Height = 288;
    vbuf_config->astFrameInfo[vbuf_config->u32PoolCnt].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    vbuf_config->astCommPool[vbuf_config->u32PoolCnt].u32BufCnt = 10;
    vbuf_config->astCommPool[vbuf_config->u32PoolCnt].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    vbuf_config->u32PoolCnt++;
#endif



    return SAMPLE_COMM_SYS_Init(vbuf_config);
}

static int _bufpool_deinit(VBUF_CONFIG_S *vbuf_config)
{
    _DBG("vbuf_config->u32PoolCnt = %d", vbuf_config->u32PoolCnt);
    return SAMPLE_COMM_SYS_Exit(vbuf_config);
}

static int _start_disp_dev(VO_DEV dev)
{
    int ret = EI_FAILURE;
    PANEL_TYPE_ATTR_S panel_attr;
        
    memset(&panel_attr, 0, sizeof(panel_attr));
    
    /*display begin*/
    EI_MI_DOSS_Init();
#if 0
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
#else
	ret = EI_MI_VO_Enable(dev);
    if (ret != EI_SUCCESS) {
        EI_TRACE_DOSS(EI_DBG_ERR, "EI_MI_VO_Enable() failed with %#x!\n", ret);
        return ret;
    }
#endif
    return 0;
    
exit:
    EI_MI_DOSS_Exit();
    return ret;    
}

static void _stop_disp_dev(VO_DEV dev)
{    
    SAMPLE_COMM_DOSS_StopDev(dev);
    EI_MI_DOSS_Exit();
}

static int _start_disp(VO_LAYER layer, int src_w, int src_h)
{
    int ret = EI_FAILURE;
    VO_VIDEO_LAYER_ATTR_S layer_attr;
    VO_CHN_ATTR_S vo_chn_attr;
    MDP_CHN_S stSrcChn = {0}, stSinkChn = {0};
    VO_CHN_ATTR_S stVoChnAttr = {0};

    memset(&layer_attr, 0, sizeof(layer_attr));
    memset(&vo_chn_attr, 0, sizeof(vo_chn_attr));


    layer_attr.u32Align = 32;
    layer_attr.stImageSize.u32Width = src_w;
    layer_attr.stImageSize.u32Height = src_h;
    layer_attr.enPixFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    layer_attr.stDispRect.s32X = 0;
    layer_attr.stDispRect.s32Y = 0;
    layer_attr.stDispRect.u32Width = scn_w;
    layer_attr.stDispRect.u32Height = scn_h;
    ret = EI_MI_VO_SetVideoLayerAttr(layer, &layer_attr);
    if (ret != EI_SUCCESS) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
        _DBG("[EI_MI_VO_SetVideoLayerAttr] ret = 0x%X\n", ret);
        goto exit1;
    }

    ret = EI_MI_VO_SetDisplayBufLen(layer, 3);
    if (ret != EI_SUCCESS) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
        _DBG("[EI_MI_VO_SetDisplayBufLen] ret = 0x%X\n", ret);
        goto exit1;
    }

    ret = EI_MI_VO_SetVideoLayerPartitionMode(layer, VO_PART_MODE_MULTI);
    if (ret != EI_SUCCESS) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
        _DBG("[EI_MI_VO_SetVideoLayerPartitionMode] ret = 0x%X\n", ret);
        goto exit1;
    }

    ret = EI_MI_VO_EnableVideoLayer(layer);
    if (ret != EI_SUCCESS) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
        _DBG("[EI_MI_VO_EnableVideoLayer] ret = 0x%X\n", ret);
        goto exit1;
    }
    stVoChnAttr.stRect.s32X = 0;
    stVoChnAttr.stRect.s32Y = 0;
    stVoChnAttr.stRect.u32Width = src_w;
    stVoChnAttr.stRect.u32Height = src_h;

    EI_MI_VO_SetChnAttr(0, 0, &stVoChnAttr);
    EI_MI_VO_EnableChn(0, 0);
    /* EI_MI_VO_SetChnRotation(0, 0, ROTATION_90); */

    stSrcChn.enModId = EI_ID_IPPU;
    stSrcChn.s32ChnId = 0;
    stSrcChn.s32DevId = 0;
    stSinkChn.enModId = EI_ID_DOSS;
    stSinkChn.s32ChnId = 0;
    stSinkChn.s32DevId = 0;
    EI_MI_MLINK_Link(&stSrcChn, &stSinkChn);

    return ret;
exit1:
    return -1;
}

static void _stop_disp(VO_LAYER layer)
{
    int ret = -1;
    MDP_CHN_S stSrcChn = {0}, stSinkChn = {0};

    stSrcChn.enModId = EI_ID_IPPU;
    stSrcChn.s32ChnId = 0;
    stSrcChn.s32DevId = 0;
    stSinkChn.enModId = EI_ID_DOSS;
    stSinkChn.s32ChnId = 0;
    stSinkChn.s32DevId = 0;
    EI_MI_MLINK_UnLink(&stSrcChn, &stSinkChn);
    ret = EI_MI_VO_DisableChn(layer, 0);
    if (ret != EI_SUCCESS) {
        _DBG("[EI_MI_VO_DisableChn]sret = 0x%X\n", ret);
    }
    ret = EI_MI_VO_DisableVideoLayer(layer);
    if (ret != EI_SUCCESS) {
        _DBG("[EI_MI_VO_DisableVideoLayer] ret = 0x%X\n",  ret);
    }
}

static EI_S32 _start_mfake_src(int src_w, int src_h)
{
    MFAKE_SRC_CHN_ATTR_S stMfakeAttr = {0};

    EI_MI_MFAKE_Init();

    stMfakeAttr.s32Chn = 0;
    stMfakeAttr.u32Align = 32;
    stMfakeAttr.u32Width = src_w;//TEST_SRC_WIDTH;
    stMfakeAttr.u32Height = src_h;//TEST_SRC_HEIGHT;
    stMfakeAttr.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    EI_MI_MFAKE_SetSrcChnAttr(0, &stMfakeAttr);
    EI_MI_MFAKE_EnableSrcChn(0);
    
    return EI_SUCCESS;
}

static EI_S32 _stop_mfake_src(void)
{
    EI_S32 s32Ret;
    s32Ret = EI_MI_MFAKE_DisableSrcChn(0);
    return s32Ret;
}

static EI_S32 _start_isp_src(SAMPLE_VISS_CONFIG_S *pstVissConfig, int VissDev, SNS_TYPE_E enSnsType, int src_w, int src_h)
{
    ISP_DEV Ispdev = 0;
    IPPU_CHN IppuCh = 0;
    ROTATION_E rot = ROTATION_90;
    VISS_PIC_TYPE_E stPicType;
    IPPU_DEV_ATTR_S stIppuDevAttr = {0};
    IPPU_CHN_ATTR_S astIppuChnAttr[IPPU_PHY_CHN_MAX_NUM] = {{0}};
    EI_S32 s32Ret = EI_FAILURE;
    EI_BOOL abChnEnable[IPPU_PHY_CHN_MAX_NUM] = {0};


    pstVissConfig->astVissInfo[0].stDevInfo.VissDev = VissDev; /*0: DVP, 1: MIPI*/
    pstVissConfig->astVissInfo[0].stSnsInfo.enSnsType = enSnsType;
    pstVissConfig->astVissInfo[0].stChnInfo.aVissChn[0] = 0;
    pstVissConfig->astVissInfo[0].stDevInfo.aBindPhyChn[0] = 0;
    pstVissConfig->astVissInfo[0].stIspInfo.IspDev = Ispdev;
    pstVissConfig->astVissInfo[0].stIspInfo.enRunningMode = ISP_MODE_RUNNING_ONLINE;
    pstVissConfig->astVissInfo[0].stDevInfo.enOutPath = VISS_OUT_PATH_PIXEL;
    pstVissConfig->astVissInfo[0].stChnInfo.enWorkMode = VISS_WORK_MODE_1Chn;
    pstVissConfig->s32WorkingVissNum = 1;

    SAMPLE_COMM_VISS_GetPicTypeBySensor(pstVissConfig->astVissInfo[0].stSnsInfo.enSnsType, &stPicType);

    /* isp crop setting */
    if(enSnsType >= 200700 && enSnsType < 200800) {
        pstVissConfig->astVissInfo[0].stIspInfo.stCrop.s32X = (stPicType.stSize.u32Width - 3840) / 2;
        pstVissConfig->astVissInfo[0].stIspInfo.stCrop.s32Y = (stPicType.stSize.u32Height - 2160) / 2;
        pstVissConfig->astVissInfo[0].stIspInfo.stCrop.u32Width = 3840;
        pstVissConfig->astVissInfo[0].stIspInfo.stCrop.u32Height = 2160;
    }

    s32Ret = SAMPLE_COMM_ISP_Start(&pstVissConfig->astVissInfo[0]);
    if (s32Ret) {
        return -1;
    }
	
    s32Ret = SAMPLE_COMM_VISS_StartViss(pstVissConfig);
    if (s32Ret) {
        SAMPLE_COMM_ISP_Stop(Ispdev);
        return -1;
    }
	/* stChnRotAttr.bEnable = EI_TRUE;
    stChnRotAttr.enRotation = ROTATION_90;
	stChnRotAttr.eMirror = MIRDIRDIR_VER; */
    s32Ret = EI_MI_VISS_SetChnRotAttr(VissDev, 0, &stChnRotAttr);
	EI_PRINT("EI_MI_VISS_SetChnRotAttr s32Ret: %d %d\n", s32Ret, __LINE__);
    stIppuDevAttr.s32IppuDev = pstVissConfig->astVissInfo[0].stIspInfo.IspDev;
    if(enSnsType >= 200700 && enSnsType < 200800) {
        stIppuDevAttr.u32InputWidth     = 3840;
        stIppuDevAttr.u32InputHeight    = 2160;
    } else {
        stIppuDevAttr.u32InputWidth     = stPicType.stSize.u32Width;
        stIppuDevAttr.u32InputHeight    = stPicType.stSize.u32Height;
    }
    stIppuDevAttr.u32DataSrc        = pstVissConfig->astVissInfo[0].stDevInfo.VissDev;
    stIppuDevAttr.enRunningMode     = IPPU_MODE_RUNNING_ONLINE;
	//pstChnRot.bEnable = EI_TRUE;
	//pstChnRot.eRotation = ROTATION_90;
	//pstChnRot.s32Chn = 0;
	//pstChnRot.eMirror = MIRDIRDIR_VER;
	//s32Ret = EI_MI_IPPU_SetChnRotation(Ispdev, 0, &pstChnRot);

    {
        EI_S32 PhyChn, ExtChn;
  
        PhyChn = 0;
        abChnEnable[PhyChn] = EI_TRUE;
        astIppuChnAttr[PhyChn].u32Width      = src_w;
        astIppuChnAttr[PhyChn].u32Height     = src_h;
        astIppuChnAttr[PhyChn].u32Align      = 32;
        astIppuChnAttr[PhyChn].enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
        astIppuChnAttr[PhyChn].stFrameRate.s32SrcFrameRate = 30;
        astIppuChnAttr[PhyChn].stFrameRate.s32DstFrameRate = 30;
        astIppuChnAttr[PhyChn].u32Depth      = 0;
		astIppuChnAttr[PhyChn].bMirror = EI_TRUE;
		astIppuChnAttr[PhyChn].bFlip = EI_TRUE;
      
        PhyChn = 1;
        abChnEnable[PhyChn] = EI_FALSE;
        astIppuChnAttr[PhyChn].u32Width      = 1280;
        astIppuChnAttr[PhyChn].u32Height     = 720;
        astIppuChnAttr[PhyChn].u32Align      = 32;
        astIppuChnAttr[PhyChn].enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
        astIppuChnAttr[PhyChn].stFrameRate.s32SrcFrameRate = 30;
        astIppuChnAttr[PhyChn].stFrameRate.s32DstFrameRate = 30;
        astIppuChnAttr[PhyChn].u32Depth      = 0;
    }	
    s32Ret = SAMPLE_COMM_IPPU_Start(Ispdev, abChnEnable, &stIppuDevAttr, astIppuChnAttr);
    if (s32Ret) {
        SAMPLE_COMM_VISS_StopViss(pstVissConfig);
        SAMPLE_COMM_ISP_Stop(Ispdev);
        return -1;
    }

	//pstChnRot.bEnable = EI_TRUE;
	//pstChnRot.eRotation = ROTATION_90;
	//pstChnRot.s32Chn = 0;
	//pstChnRot.eMirror = MIRDIRDIR_VER;
	//s32Ret = EI_MI_IPPU_SetChnRotation(Ispdev, 0, &pstChnRot);
	//EI_PRINT("EI_MI_IPPU_SetChnRotation s32Ret: %d %d\n", s32Ret, __LINE__);
	//s32Ret = EI_MI_IPPU_GetChnAttr(Ispdev, 0, &pstChnAttr);
	//EI_PRINT("EI_MI_IPPU_GetChnAttr s32Ret: %d %d %d %d\n", s32Ret, __LINE__, pstChnAttr->bFlip,pstChnAttr->bMirror);
	//s32Ret = EI_MI_IPPU_SetChnAttr(Ispdev, 0, &pstChnAttr);
	//EI_PRINT("EI_MI_IPPU_SetChnAttr s32Ret: %d %d\n", s32Ret, __LINE__);
	
    
    return EI_SUCCESS;
}

static EI_S32 _stop_isp_src(SAMPLE_VISS_CONFIG_S *pstVissConfig)
{
    EI_S32 s32Ret = EI_SUCCESS;
    EI_BOOL abChnEnable[IPPU_PHY_CHN_MAX_NUM] = {0};

    abChnEnable[0] = EI_TRUE;

    SAMPLE_COMM_IPPU_Stop(0, abChnEnable);
    SAMPLE_COMM_VISS_StopViss(pstVissConfig);
    SAMPLE_COMM_ISP_Stop(0);
    return s32Ret;
}


static int _start_vo_wbc()
{
    int ret = -1;

    VO_WBC wbc = 0; 
    VO_WBC_SOURCE_S wbc_src;
    VO_WBC_MODE_E stWbcmode = VO_WBC_MODE_NORMAL;
    EI_U32 u32Depth = 3;

    wbc_src.enSourceType = VO_WBC_SOURCE_DEV;
    wbc_src.u32SourceId = 0;
    
    ret = EI_MI_VO_SetWBCSource(wbc, &wbc_src);
    if (ret != EI_SUCCESS)
    {
        _DBG("EI_MI_VO_SetWBCSource failed. ret = 0x%X", ret);
        return -1;
    }

    VO_WBC_ATTR_S wbc_attr;
    memset(&wbc_attr, 0, sizeof(VO_WBC_ATTR_S));
    wbc_attr.stTargetSize.u32Width = scn_w;
    wbc_attr.stTargetSize.u32Height = scn_h;
    wbc_attr.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    wbc_attr.u32FrameRate = 25;
//    wbc_attr.u32Align = 32;
    //wbc_attr.enDynamicRange = DYNAMIC_RANGE_SDR8;
    //wbc_attr.enCompressMode = COMPRESS_MODE_NONE;
    ret = EI_MI_VO_SetWBCAttr(wbc, &wbc_attr);
    if (ret != EI_SUCCESS)
    {
        _DBG("EI_MI_VO_SetWBCAttr failed. ret = 0x%X", ret);
        return -1;
    }

    ret = EI_MI_VO_GetWBCMode(wbc, &stWbcmode);
    if (ret) {
        _DBG("EI_MI_VO_GetWBCMode err[0x%x]", ret);
        return ret;
    }
     
    /* set wbc mode */
    stWbcmode = VO_WBC_MODE_NORMAL;
    ret = EI_MI_VO_SetWBCMode(wbc, stWbcmode);
    if (ret) {
        _DBG("EI_MI_VO_SetWBCMode err[0x%x]", ret);
        return ret;
    }

    ret = EI_MI_VO_EnableWBC(wbc);    
    if (ret != EI_SUCCESS)
    {
        _DBG("EI_MI_VO_EnableWBC failed. ret = 0x%X", ret);
        return -1;
    }
     
    /* set wbc depth */
    ret = EI_MI_VO_SetWBCDepth(wbc, u32Depth);
    if (ret) {
    	_DBG("err[0x%x]", ret);
        return ret;
    }
    return 0;
}

static int _stop_vo_wbc()
{
    int ret = -1;
    VO_WBC wbc = 0; 

    ret = EI_MI_VO_DisableWBC(wbc);
    if (ret != EI_SUCCESS) {
        _DBG("EI_MI_VO_DisableWBC failed. ret = 0x%X", ret);
        return -1;
    }
    
    return 0;
}

static int _start_disp_stream(sdr_sample_venc_para_t *stream_para)
{
    int i = 0;
    SAMPLE_VENC_CONFIG_S venc_config;
    int ret = EI_SUCCESS;
    MDP_CHN_S stSrcChn;
    MDP_CHN_S stSinkChn;

    stSrcChn.enModId = EI_ID_IPPU;
    stSrcChn.s32ChnId = 0;
    stSrcChn.s32DevId = 0;
    stSinkChn.enModId = EI_ID_VPU;
    stSinkChn.s32ChnId = stream_para->VeChn;
    stSinkChn.s32DevId = 0;
    EI_MI_MLINK_Link(&stSrcChn, &stSinkChn);

    memset(&venc_config, 0, sizeof(SAMPLE_VENC_CONFIG_S));
    venc_config.enInputFormat   = PIX_FMT_YUV_SEMIPLANAR_420;
	if (pstChnRot.eRotation == ROTATION_90||pstChnRot.eRotation == ROTATION_270) {
		venc_config.u32width         = TEST_SRC_HEIGHT;//scn_w;
    	venc_config.u32height        = TEST_SRC_WIDTH;//scn_h;
	} else {
    	venc_config.u32width         = TEST_SRC_WIDTH;//scn_w;
    	venc_config.u32height        = TEST_SRC_HEIGHT;//scn_h;
	}
    venc_config.u32bitrate      = stream_para->bitrate;
    venc_config.u32srcframerate = MAIN_STREAM_FRAMERATE;
    venc_config.u32dstframerate = MAIN_STREAM_FRAMERATE;  
    ret = SAMPLE_COMM_VENC_Start(stream_para->VeChn, stream_para->pt_type, /* PT_H265,*/
            SAMPLE_RC_CBR, &venc_config,
            COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
    if (EI_SUCCESS != ret) {
        EI_PRINT("%s %d %d ret:%d \n", __func__, __LINE__, stream_para[i].VeChn, ret);
        return -1;
    }

    //stream_para[i].muxer_type = SUB_STREAM_MUXER_TYPE;
    stream_para[0].muxer_type = stream_para->muxer_type;
    stream_para[0].bitrate = venc_config.u32bitrate;
    
    //snprintf(stream_para[i].filename, 63, "%s/main_%02d.264", HB_PATH_ROOT, stream_para[i].VeChn);
    //_DBG("filename = %s", stream_para[i].filename);
    ret = _start_get_stream(&stream_para[0]);
    if (EI_SUCCESS != ret) {
        _DBG("_StartGetStream failed, Venc_Chn = %d \n", stream_para[0].VeChn);
        //SAMPLE_COMM_VISS_UnLink_VPU(i, IPPU_CHN_MAIN, stream_para[i].VeChn);
        SAMPLE_COMM_VENC_Stop(stream_para[i].VeChn);
        return -1;
    }

    return ret;
}

static int _stop_disp_stream(sdr_sample_venc_para_t *stream_para)
{
    int i = 0;

    MDP_CHN_S stSrcChn = {0}, stSinkChn = {0};

    stSrcChn.enModId = EI_ID_IPPU;
    stSrcChn.s32ChnId = 0;
    stSrcChn.s32DevId = 0;
    stSinkChn.enModId = EI_ID_VPU;
    stSinkChn.s32ChnId = stream_para->VeChn;
    stSinkChn.s32DevId = 0;
    EI_MI_MLINK_UnLink(&stSrcChn, &stSinkChn);

    stream_para[i].bThreadStart = EI_FALSE;    
    _stop_get_stream(&stream_para[i]);
    //SAMPLE_COMM_IPPU_UnLink_VPU(i, IPPU_CHN_MAIN, stream_para[i].VeChn);
    SAMPLE_COMM_VENC_Stop(stream_para[i].VeChn);
    
    return 0;
}

static int _get_screen_info(EI_U32 *scn_w, EI_U32 *scn_h, EI_U32 *scn_rate)
{
	return SAMPLE_COMM_DOSS_GetWH(VO_DEVICE, VO_OUTPUT_USER, scn_w, scn_h, scn_rate);
}

int debug_isp(void)
{
    int ret = EI_FAILURE;
    VBUF_CONFIG_S vbuf_config = {0};
    sdr_sample_venc_para_t disp_stream_para;
    int is_start_disp_stream = 0;
    SAMPLE_VISS_CONFIG_S stVissConfig = {0};
    int VissDev = 2;
    SNS_TYPE_E enSnsType = GC2053_MIPI_1920_1080_30FPS_RAW10;
    _DBG("");
    scn_w = scn_h = scn_rate = 0;
    //_get_screen_info(&scn_w, &scn_h, &scn_rate);
    scn_w = 480;
	scn_h = 480;
	scn_rate = 60;
    _DBG("");
    if ((scn_w == 0) || (scn_h == 0) || (scn_rate == 0)) {
	    EI_PRINT("%s %d get screen info fail. ", __func__, __LINE__);
	    goto EXIT;
    }
    _DBG("");
    ret = _bufpool_init(&vbuf_config);
    if (ret) {
        EI_PRINT("%s %d  \n", __func__, __LINE__);
        goto EXIT;
    }

    _DBG("start isp");
    ret = _start_isp_src(&stVissConfig, VissDev, enSnsType, TEST_SRC_WIDTH, TEST_SRC_HEIGHT);
    if (ret) {
        EI_PRINT("%s %d  \n", __func__, __LINE__);
        goto EXIT;
    }
#if 0
    _DBG("start disp");
    _start_disp_dev(0);
    ret = _start_disp(0, TEST_SRC_WIDTH, TEST_SRC_HEIGHT);
    if (ret) {
        EI_TRACE_LOG(EI_DBG_ERR, "\n");
        goto EXIT;
    }
#endif
    memset(&disp_stream_para, 0, sizeof(sdr_sample_venc_para_t));
    disp_stream_para.VeChn = DISP_ENC_CHANNEL;
    disp_stream_para.bitrate = MAIN_STREAM_BITRATE;
    disp_stream_para.file_idx = 0;
    disp_stream_para.muxer_type = 0;
    disp_stream_para.pt_type = PT_H265;
    _start_disp_stream(&disp_stream_para);
    is_start_disp_stream = 1;
    
    int key = 0;
    //while(1) sleep(1);
    while (1) {        
        key = getchar();
        switch (key)
        {
        case 'q':
        case 'Q':
            _DBG("Exit~~~\n\n");
            goto EXIT;
        break;            

        case 'd':
        case 'D':
            if (is_start_disp_stream == 0)
            {
                _DBG("Start Disp Stream");
                _start_disp_stream(&disp_stream_para);
                is_start_disp_stream = 1;
            }
            else
            {
                _DBG("Stop Disp Stream!");
                _stop_disp_stream(&disp_stream_para);
                is_start_disp_stream = 0;                
            }
            break;
        case 'p':
        case 'P':
            system("cat /proc/mdp/*");
        break;
        default:
            break;
        }
    }
    _DBG("");
EXIT:

    if (is_start_disp_stream)
    {
        _stop_disp_stream(&disp_stream_para);
    }
    
    
    _DBG("stop disp");
    _stop_disp(0);
    _stop_disp_dev(0);
    _stop_isp_src(&stVissConfig);
    ret = _bufpool_deinit(&vbuf_config);
    if (ret)
        EI_PRINT("====_bufpool_deinit failed\n");

    return 0;

}

int main(int argc, char **argv)
{
    int ret = EI_FAILURE;
    char *mount_point = PATH_ROOT;
    storage_info_t store_info;
    

#if 0
    do {
        sleep(1);
        //ret = system("mount /dev/mmcblk0p1 /mnt/card/");
        //ret = system("mount -o iocharset=utf8,codepage=936 /dev/sda /mnt/card");
        //if (ret == 0)
        //  break;
        memset(&store_info, 0, sizeof(store_info));
        _storage_info_get(mount_point, &store_info);
        _DBG("disk space: total:%d free:%d\n", store_info.total, store_info.free);
    } while(store_info.free <= 0);
    _DBG("");
    if (store_info.free < (1024 * 1024)) {
        _DBG("free space too small. %d\n", store_info.free);
        return -1;
    }
#endif
    _DBG("");
    ret = debug_isp();

    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
