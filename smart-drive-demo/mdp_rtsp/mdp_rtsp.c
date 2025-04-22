/*
 *------------------------------------------------------------------------------
 * @File      :    mdp_gesture.c
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
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <errno.h>
#include <sys/prctl.h>
#include "dirent.h"
#include <pthread.h>
#include "semaphore.h"
#include "sample_comm.h"
#include "osal.h"
#include "gesture_interface.h"
#include "frame_queue.h"
#include "live_server.h"
#include "live_video.h"


#define EI_TRACE_LOG(level, fmt, args...)\
do{ \
    EI_TRACE(EI_ID_LOG, level, fmt, ##args);\
} while (0)

#define NN_HANDDET_MODEL_PATH	 "/usr/share/ax/"
#define NN_GESTURE_MODEL_PATH	 "/usr/share/ax/"
#define PATH_ROOT "/data/" /* save file root path */
#define MQ_MSG_NUM 128 /* message queue number */
#define MQ_MSG_LEN 128 /* every message max lenght */
#define ALARM_INTERVAL 5000 /* same warning message interval time unit:ms */
#define SDR_SAMPLE_VENC_TIME 120 /* venc time interval unit:s */
#define SDR_SAMPLE_VENC_BITRATE 2000000 /* venc file bitrate unit:bit/s */
#define SDR_VISS_FRAMERATE 25
#define FULLNAME_LEN_MAX 128 /* file len max, full name, include path */
#define RGN_ARRAY_NUM 2  /* rgn  array 0:rect 1:line */
#define VENC_RGN_WM_SOURCE_NUM 12  /* watermark rgn source num*/
#define VENC_RGN_WM_NUM 18  /* watermark rgn num */
#define RGN_RECT_NUM 16 /* rect max num */
#define RGN_LINE_NUM 16 /* line max num */
#define AL_SEND_FRAMERATE 15 /* send al frame rate */
#define AL_PRO_FRAMERATE_STATISTIC /* al static frame rate */
#define SCN_WIDTH 1280
#define SCN_HEIGHT 720
/* #define SAVE_JPG_DEC_YUV_SP */ /* save yuv data after jpg dec */
/* #define SAVE_AX_YUV_SP */ /* save yuv frame send to al module */
/* #define SAVE_DRAW_YUV_SP */ /* save yuv frame send to disp module, not use */
/* #define SAVE_DRAW_AX_H265 */ /* if open this macro, save h265 send to disp module,
include draw rect; else save h265 send to al module, not include  draw rect */
/* #define SAVE_WATERMARK_SOURCE */ /* save bmp data,not include bmp header,bmp data must positive direction*/
#define WATERMARK_X 957
#define WATERMARK_Y 5
#define WATERMARK_W 17
#define WATERMARK_H 28
#define WATERMARK_XIEGANG_INDEX 10
#define WATERMARK_MAOHAO_INDEX 11

#define GESTURE_TYPE_OTHER    0
#define GESTURE_TYPE_OK       1
#define GESTURE_TYPE_PALM     2
#define GESTURE_TYPE_UP       3
#define GESTURE_TYPE_DOWN     4
#define GESTURE_TYPE_LEFT     5
#define GESTURE_TYPE_RIGHT    6
#define GESTURE_TYPE_HEART    7 //not use
#define GESTURE_TYPE_QUIET    8 //not use
#define GESTURE_TYPE_PINCH    9
#define RTSP_SUPPORT

typedef enum _WARNING_MSG_TYPE_ {
	LB_WARNING_BEGIN = 0xF000,
	LB_WARNING_GESTURE_TYPE_OTHER,
	LB_WARNING_GESTURE_TYPE_OK,
	LB_WARNING_GESTURE_TYPE_PALM,
	LB_WARNING_GESTURE_TYPE_UP,
	LB_WARNING_GESTURE_TYPE_DOWN,
	LB_WARNING_GESTURE_TYPE_LEFT,
	LB_WARNING_GESTURE_TYPE_RIGHT,
	LB_WARNING_GESTURE_TYPE_HEART,
	LB_WARNING_GESTURE_TYPE_QUIET,
	LB_WARNING_GESTURE_TYPE_PINCH,
	LB_WARNING_USERDEF,
	LB_WARNING_END,
} WARNING_MSG_TYPE;

typedef enum _WM_MOD_TYPE_ {
	MOD_PREVIEW,
	MOD_VENC,
} WM_MOD_TYPE;

char watermark_filename[VENC_RGN_WM_SOURCE_NUM][64] = {
	"/usr/share/out/res/wm_source/num0.bmp",
	"/usr/share/out/res/wm_source/num1.bmp",
	"/usr/share/out/res/wm_source/num2.bmp",
	"/usr/share/out/res/wm_source/num3.bmp",
	"/usr/share/out/res/wm_source/num4.bmp",
	"/usr/share/out/res/wm_source/num5.bmp",
	"/usr/share/out/res/wm_source/num6.bmp",
	"/usr/share/out/res/wm_source/num7.bmp",
	"/usr/share/out/res/wm_source/num8.bmp",
	"/usr/share/out/res/wm_source/num9.bmp",
	"/usr/share/out/res/wm_source/xiegang.bmp",
	"/usr/share/out/res/wm_source/maohao.bmp"
};
typedef struct _axframe_info_s {
    EI_BOOL bThreadStart;
    VISS_CHN phyChn;
    VISS_DEV dev;
    VISS_CHN chn;
    RECT_S stFrameSize;
    unsigned int frameRate;
    EI_U32 u32Width;
    EI_U32 u32Height;
    pthread_t gs_framePid;
	pthread_mutex_t p_det_mutex;
	ezax_boxes_t *p_det_info;
	void *handdet_hdl;
	void *gesture_hdl;
	FILE *ax_fout;
} axframe_info_s;

typedef struct _drawframe_info_s {
    EI_BOOL bThreadStart;
    VISS_CHN phyChn;
    VISS_DEV dev;
    VISS_CHN chn;
	VO_DEV VoDev;
	VO_LAYER VoLayer;
	VO_CHN VoChn;
    RECT_S stFrameSize;
	RGN_ATTR_S stRegion[3];/* 0:OVERLAYEX_RGN;1:RECTANGLEEX_ARRAY_RGN;2:LINEEX_ARRAY_RGN */
    RGN_CHN_ATTR_S stRgnChnAttr[RGN_ARRAY_NUM+VENC_RGN_WM_NUM];
    RGN_HANDLE Handle[RGN_ARRAY_NUM+VENC_RGN_WM_NUM];/* 0~VENC_RGN_WM_NUM: OVERLAYEX_RGN;VENC_RGN_WM_NUM:RECTANGLEEX_ARRAY_RGN;VENC_RGN_WM_NUM+1:LINEEX_ARRAY_RGN*/
    RECTANGLEEX_ARRAY_SUB_ATTRS stRectArray[RGN_RECT_NUM];
    LINEEX_ARRAY_SUB_ATTRS stLineArray[RGN_LINE_NUM];
	MDP_CHN_S stRgnChn[2];/* 0:ai rect;1:time watermark */
	char *wm_data[VENC_RGN_WM_SOURCE_NUM];
	int last_draw_num;
    EI_U32 u32Width;
    EI_U32 u32Height;
	EI_U32 u32scnWidth;
	EI_U32 u32scnHeight;
    unsigned int frameRate;
    pthread_t gs_framePid;
	FILE *draw_fout;
} drawframe_info_s;

typedef struct _warning_info_s {
    EI_BOOL bThreadStart;
    int mq_id;
	int64_t last_warn_time[LB_WARNING_END-LB_WARNING_BEGIN-1];
	int last_warn_status;
    pthread_t g_Pid;
} warning_info_s;

typedef struct tag_warn_msg {
	int32_t		type;
	int32_t		len;
	char		data[120];
} tag_warn_msg_t;

typedef struct __sample_venc_para_t {
    EI_BOOL bThreadStart;
	VISS_DEV dev;
    VC_CHN VeChn;
	VISS_CHN chn;
	VISS_CHN phyChn;
    int bitrate;
    int file_idx;
    int muxer_type;
    int init_flag;
	int exit_flag;
    FILE *flip_out;
    char aszFileName[128];
	char szFilePostfix[32];
    int buf_offset;
	RGN_ATTR_S stRegion[2]; /* 0:OVERLAYEX_RGN;1: reserved */
	RGN_CHN_ATTR_S stRgnChnAttr[VENC_RGN_WM_NUM];
	RGN_HANDLE Handle[VENC_RGN_WM_NUM];
	MDP_CHN_S stRgnChn;
	char *wm_data[VENC_RGN_WM_SOURCE_NUM];
    void *muxer_hdl;
    pthread_t gs_VencPid;
} sdr_sample_venc_para_t;

typedef struct {
    unsigned int total;
    unsigned int free;
} sdcard_info_t;

static axframe_info_s axframe_info;
static drawframe_info_s drawframe_info;
static warning_info_s warning_info;
sdr_sample_venc_para_t stream_para[ISP_MAX_DEV_NUM] = {{0}};

void preview_help(void)
{
    printf("usage: mdp_gesture \n");
    printf("such as for gesture recognition: mdp_gesture\n");
    printf("such as for gesture recognition: mdp_gesture -d 0 -c 0 -r 90\n");

    printf("arguments:\n");
    printf("-d    select input device, Dev:0~1, default: 0\n");
    printf("-c    select chn id, support:0~3, default: 0\n");
    printf("-r    select rotation, 0/90/180/270, default: 90\n");
	printf("-s    sensor or rx macro value, such as 200800, default: 100301\n");
    printf("-h    help\n");
}

EI_S32 rgn_start(void)
{
	EI_S32 s32Ret = EI_FAILURE;

	drawframe_info.stRgnChn[1].enModId = EI_ID_VISS;
	drawframe_info.stRgnChn[1].s32DevId = drawframe_info.dev;
	drawframe_info.stRgnChn[1].s32ChnId = drawframe_info.chn;
	drawframe_info.Handle[VENC_RGN_WM_NUM] = VENC_RGN_WM_NUM;
    drawframe_info.stRegion[1].enType = RECTANGLEEX_ARRAY_RGN;
    drawframe_info.stRegion[1].unAttr.u32RectExArrayMaxNum = RGN_RECT_NUM;
	
    s32Ret = EI_MI_RGN_Create(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRegion[1]);
    if(s32Ret) {
        EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_Create \n");
        goto exit;
    }
	drawframe_info.Handle[VENC_RGN_WM_NUM+1] = VENC_RGN_WM_NUM+1;
    drawframe_info.stRegion[2].enType = LINEEX_ARRAY_RGN;
    drawframe_info.stRegion[2].unAttr.u32LineExArrayMaxNum = RGN_LINE_NUM;
    s32Ret = EI_MI_RGN_Create(drawframe_info.Handle[VENC_RGN_WM_NUM+1], &drawframe_info.stRegion[2]);
    if(s32Ret) {
        EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_Create \n");
        goto exit;
    }
	drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].bShow = EI_TRUE;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].enType = RECTANGLEEX_ARRAY_RGN;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].unChnAttr.stRectangleExArrayChn.u32MaxRectNum = RGN_RECT_NUM;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].unChnAttr.stRectangleExArrayChn.u32ValidRectNum = RGN_RECT_NUM;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].unChnAttr.stRectangleExArrayChn.pstRectExArraySubAttr = drawframe_info.stRectArray;
    s32Ret = EI_MI_RGN_AddToChn(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
	if(s32Ret) {
        EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_AddToChn \n");
    }
	drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1].bShow = EI_TRUE;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1].enType = LINEEX_ARRAY_RGN;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1].unChnAttr.stLineExArrayChn.u32MaxLineNum = RGN_LINE_NUM;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1].unChnAttr.stLineExArrayChn.u32ValidLineNum = RGN_LINE_NUM;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1].unChnAttr.stLineExArrayChn.pstLineExArraySubAttr = drawframe_info.stLineArray;
    s32Ret = EI_MI_RGN_AddToChn(drawframe_info.Handle[VENC_RGN_WM_NUM+1], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1]);
	if(s32Ret) {
        EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_AddToChn \n");
    }

	return EI_SUCCESS;
exit:
	return s32Ret;
}

void rgn_stop(void)
{
	int i;

	for (i = 0;i < RGN_ARRAY_NUM; i++) {
		EI_MI_RGN_DelFromChn(drawframe_info.Handle[VENC_RGN_WM_NUM+i], &drawframe_info.stRgnChn[1]);
		EI_MI_RGN_Destroy(drawframe_info.Handle[VENC_RGN_WM_NUM+i]);
	}
}

static void gesture_draw_location_rgn(VIDEO_FRAME_INFO_S *drawFrame, ezax_boxes_t *p_det_info)
{
	EI_S32 s32Ret = EI_FAILURE;
	if (p_det_info == NULL) {
		EI_PRINT("line:%d NULL\n", __LINE__);
		return;
	}
	if (p_det_info->num > RGN_RECT_NUM) {
		EI_PRINT("line:%d %d num over!\n", __LINE__, p_det_info->num);
		return;
	}

	if (drawframe_info.last_draw_num > p_det_info->num) {
		for (int i = drawframe_info.last_draw_num-1; i >= p_det_info->num; i--) {
			drawframe_info.stRectArray[i].isShow = EI_FALSE;
			s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM],
				&drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
			if(s32Ret) {
        		EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_SetChnAttr \n");
    		}
		}
	}
	for (int i = 0; i < p_det_info->num; i++) {
		drawframe_info.stRectArray[i].stRectAttr.stRect.s32X = p_det_info->pRect[i].x0;
		drawframe_info.stRectArray[i].stRectAttr.stRect.s32Y = p_det_info->pRect[i].y0;
		drawframe_info.stRectArray[i].stRectAttr.stRect.u32Height = p_det_info->pRect[i].y1-p_det_info->pRect[i].y0;
		drawframe_info.stRectArray[i].stRectAttr.stRect.u32Width = p_det_info->pRect[i].x1-p_det_info->pRect[i].x0;
		drawframe_info.stRectArray[i].stRectAttr.u32BorderSize = 4;
		drawframe_info.stRectArray[i].stRectAttr.u32Color = 0xffff0000;
		drawframe_info.stRectArray[i].isShow = EI_TRUE;
		drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].bShow = EI_TRUE;
		s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
		if(s32Ret) {
        	EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_SetChnAttr \n");
    	}
	}
	drawframe_info.last_draw_num = p_det_info->num;
}

static int watermark_init(EI_U32 handleId_start, EI_U32 handleId_num, char source_name[][64], WM_MOD_TYPE mod)
{
    EI_S32 s32Ret = EI_FAILURE;
	FILE *fd = NULL;
	int size;
	int len;
	RGN_ATTR_S *stRegion;
	RGN_CHN_ATTR_S *stRgnChnAttr;
	RGN_HANDLE *Handle;
	MDP_CHN_S *stRgnChn;

	if (mod == MOD_VENC) {
		stRegion = &stream_para[0].stRegion[0];
		stRgnChnAttr = &stream_para[0].stRgnChnAttr[0];
		Handle = &stream_para[0].Handle[0];
		stRgnChn = &stream_para[0].stRgnChn;
		stRgnChn->enModId = EI_ID_VPU;
		stRgnChn->s32DevId = stream_para[0].dev;
		stRgnChn->s32ChnId = stream_para[0].VeChn;
	} else if (mod == MOD_PREVIEW) {
		stRegion = &drawframe_info.stRegion[0];
		stRgnChnAttr = &drawframe_info.stRgnChnAttr[0];
		Handle = &drawframe_info.Handle[0];
		stRgnChn = &drawframe_info.stRgnChn[0];
		stRgnChn->enModId = EI_ID_VISS;
		stRgnChn->s32DevId = drawframe_info.dev;
		stRgnChn->s32ChnId = drawframe_info.chn;
	}

    stRegion[0].enType = OVERLAYEX_RGN;
    stRegion[0].unAttr.stOverlayEx.enPixelFmt = PIX_FMT_ARGB_8888;
    stRegion[0].unAttr.stOverlayEx.stSize.u32Width = WATERMARK_W;
    stRegion[0].unAttr.stOverlayEx.stSize.u32Height = WATERMARK_H;
    stRegion[0].unAttr.stOverlayEx.u32CanvasNum = 2;
	stRegion[0].unAttr.stOverlayEx.u32BgColor = 0xff00ff00;
	for (int i = 0; i < handleId_num; i++) {
	    Handle[i] = handleId_start+i;
	    s32Ret = EI_MI_RGN_Create(Handle[i], stRegion);
	    if (s32Ret) {
			EI_PRINT("EI_MI_RGN_Create fail\n");
	        goto exit1;
	    }
	    stRgnChnAttr[i].bShow = EI_TRUE;
	    stRgnChnAttr[i].enType = OVERLAYEX_RGN;
	    stRgnChnAttr[i].unChnAttr.stOverlayExChn.stPoint.s32X = WATERMARK_X;
		stRgnChnAttr[i].unChnAttr.stOverlayExChn.stPoint.s32Y = WATERMARK_Y;
	    stRgnChnAttr[i].unChnAttr.stOverlayExChn.u32FgAlpha = 255;
	    s32Ret = EI_MI_RGN_AddToChn(Handle[i], stRgnChn, &stRgnChnAttr[i]);
	    if (s32Ret) {
			EI_PRINT("EI_MI_RGN_AddToChn fail\n");
	        goto exit1;
	    }
	}
	size = stRegion[0].unAttr.stOverlayEx.stSize.u32Width * stRegion[0].unAttr.stOverlayEx.stSize.u32Height * 4;
	for (int i = 0; i < VENC_RGN_WM_SOURCE_NUM; i++) {
		if (mod == MOD_VENC) {
			stream_para[0].wm_data[i] = malloc(size);
			if (!stream_para[0].wm_data[i]) {
				EI_PRINT("malloc fail\n");
				return -1;
			}
			fd = fopen(source_name[i], "rb");
			if (!fd) {
				EI_PRINT("fopen fail\n");
				return -1;
			}
			fseek(fd, 54, SEEK_SET);
			len = fread(stream_para[0].wm_data[i], 1, size, fd);
			if(len!=size) {
				EI_PRINT("fread size fail\n");
				fclose(fd);
				return -1;
			}
			fclose(fd);
			for(int j = 0; j < len/4; j++) {
				if (stream_para[0].wm_data[i][4*j+0] < 0x80 || stream_para[0].wm_data[i][4*j+1] < 0x80
					|| stream_para[0].wm_data[i][4*j+2] < 0x80) {
					stream_para[0].wm_data[i][4*j+3] = 0xff;
				}
			}
		} else if (mod == MOD_PREVIEW) {
			drawframe_info.wm_data[i] = malloc(size);
			if (!drawframe_info.wm_data[i]) {
				EI_PRINT("malloc fail\n");
				return -1;
			}
			fd = fopen(source_name[i], "rb");
			if (!fd) {
				EI_PRINT("fopen fail\n");
				return -1;
			}
			fseek(fd, 54, SEEK_SET);
			len = fread(drawframe_info.wm_data[i], 1, size, fd);
			if(len!=size) {
				EI_PRINT("fread size fail\n");
				fclose(fd);
				return -1;
			}
			fclose(fd);
			for(int j = 0; j < len/4; j++) {
				if (drawframe_info.wm_data[i][4*j+0] < 0x80 || drawframe_info.wm_data[i][4*j+1] < 0x80
					|| drawframe_info.wm_data[i][4*j+2] < 0x80) {
					drawframe_info.wm_data[i][4*j+3] = 0xff;
				}
			}
		}
#ifdef SAVE_WATERMARK_SOURCE
		char srcfilename[128];
		memset(srcfilename, 0x00, sizeof(srcfilename));
		sprintf(srcfilename, "%s/watermark%d.bin", PATH_ROOT, i);
		fd = fopen(srcfilename, "wb+");
		if (!fd) {
			EI_PRINT("fopen fail\n");
			return -1;
		}
		if (mod == MOD_VENC)
			fwrite((void *)stream_para[0].wm_data[i], 1, len, fd);
		else if (mod == MOD_PREVIEW)
			fwrite((void *)drawframe_info.wm_data[i], 1, len, fd);
		fclose(fd);
#endif
	}
exit1:
	return s32Ret;
}
static int watermark_update(struct tm *p_tm, EI_U32 x, EI_U32 y, WM_MOD_TYPE mod)
{
    EI_S32 s32Ret = EI_FAILURE;
    BITMAP_S stBitmap;
	int index;
	OSAL_LOGD("watermark_update\n");

	RGN_ATTR_S *stRegion;
	RGN_CHN_ATTR_S *stRgnChnAttr;
	RGN_HANDLE *Handle;
	MDP_CHN_S *stRgnChn;
	char *wm_data[VENC_RGN_WM_SOURCE_NUM];
	if (mod == MOD_VENC) {
		stRegion = &stream_para[0].stRegion[0];
		stRgnChnAttr = &stream_para[0].stRgnChnAttr[0];
		Handle = &stream_para[0].Handle[0];
		stRgnChn = &stream_para[0].stRgnChn;
		for (int i = 0; i < VENC_RGN_WM_SOURCE_NUM; i++) {
			wm_data[i] = &stream_para[0].wm_data[i][0];
		}
	} else if (mod == MOD_PREVIEW) {
		stRegion = &drawframe_info.stRegion[0];
		stRgnChnAttr = &drawframe_info.stRgnChnAttr[0];
		Handle = &drawframe_info.Handle[0];
		stRgnChn = &drawframe_info.stRgnChn[0];
		for (int i = 0; i < VENC_RGN_WM_SOURCE_NUM; i++) {
			wm_data[i] = &drawframe_info.wm_data[i][0];
		}
	}
    stBitmap.enPixelFormat = stRegion[0].unAttr.stOverlayEx.enPixelFmt;
    stBitmap.u32Width = stRegion[0].unAttr.stOverlayEx.stSize.u32Width;
    stBitmap.u32Height = stRegion[0].unAttr.stOverlayEx.stSize.u32Height;
	index = ((p_tm->tm_year + 1900) / 1000) % 10;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[0], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = ((p_tm->tm_year + 1900) / 100) % 10;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[1], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = ((p_tm->tm_year + 1900) / 10) % 10;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[2], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = (p_tm->tm_year + 1900) % 10;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[3], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = WATERMARK_XIEGANG_INDEX;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[4], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = ((p_tm->tm_mon + 1) / 10) % 10;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[5], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = (p_tm->tm_mon + 1) % 10;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[6], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = WATERMARK_XIEGANG_INDEX;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[7], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = (p_tm->tm_mday / 10) % 10;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[8], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = p_tm->tm_mday % 10;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[9], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = (p_tm->tm_hour / 10) % 10;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[10], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = p_tm->tm_hour % 10;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[11], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = WATERMARK_MAOHAO_INDEX;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[12], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = (p_tm->tm_min / 10) % 10;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[13], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = p_tm->tm_min % 10;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[14], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = WATERMARK_MAOHAO_INDEX;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[15], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = (p_tm->tm_sec / 10) % 10;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[16], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	index = p_tm->tm_sec % 10;
	stBitmap.pData = wm_data[index];
	s32Ret = EI_MI_RGN_SetBitMap(Handle[17], &stBitmap);
    if (s32Ret) {
        goto exit2;
    }
	for (int i = 0; i < VENC_RGN_WM_NUM; i++) {
		stRgnChnAttr[i].bShow = EI_TRUE;
		if (i < 10)
			stRgnChnAttr[i].unChnAttr.stOverlayExChn.stPoint.s32X = x + WATERMARK_W*i;
		else
			stRgnChnAttr[i].unChnAttr.stOverlayExChn.stPoint.s32X = x + WATERMARK_W*(i+1);/* reserve a space between day to hour */
		stRgnChnAttr[i].unChnAttr.stOverlayExChn.stPoint.s32Y = y;
		s32Ret = EI_MI_RGN_SetChnAttr(Handle[i], stRgnChn, &stRgnChnAttr[i]);
		if(s32Ret) {
	        EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_SetChnAttr \n");
	    }
	}
 exit2:
    return s32Ret;
}

static int watermark_exit(EI_U32 handleId_num, WM_MOD_TYPE mod)
{
	RGN_HANDLE *Handle;
	MDP_CHN_S *stRgnChn;
	char *wm_data[VENC_RGN_WM_SOURCE_NUM];

	if (mod == MOD_VENC) {
		Handle = &stream_para[0].Handle[0];
		stRgnChn = &stream_para[0].stRgnChn;
		for (int i = 0; i < VENC_RGN_WM_SOURCE_NUM; i++) {
			wm_data[i] = &stream_para[0].wm_data[i][0];
		}
	} else if (mod == MOD_PREVIEW) {
		Handle = &drawframe_info.Handle[0];
		stRgnChn = &drawframe_info.stRgnChn[0];
		for (int i = 0; i < VENC_RGN_WM_SOURCE_NUM; i++) {
			wm_data[i] = &drawframe_info.wm_data[i][0];
		}
	}
	for (int i = 0; i < handleId_num; i++) {
		EI_MI_RGN_DelFromChn(Handle[i], stRgnChn);
		EI_MI_RGN_Destroy(Handle[i]);
	}
	for (int i = 0; i < VENC_RGN_WM_SOURCE_NUM; i++) {
		free(wm_data[i]);
	}

    return EI_SUCCESS;
}
int gesture_warning(ezax_boxes_t *p_det_info)
{
	int64_t cur_time = osal_get_msec();
	uint32_t index;
	if (p_det_info == NULL) {
		EI_PRINT("line:%d NULL\n", __LINE__);
	}
	if (p_det_info->num == 0) {
		OSAL_LOGD("line:%d NULL\n", __LINE__);
		return -1;
	}
	for (int i=0; i<p_det_info->num; i++) {
		//EI_PRINT("%d warn_status:%d\n", i, p_det_info->pRect[i].c);
		if (p_det_info->pRect[i].c == GESTURE_TYPE_OK) {
			index = LB_WARNING_GESTURE_TYPE_OK-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_GESTURE_TYPE_OK, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_GESTURE_TYPE_OK;
			}
		} 
		if (p_det_info->pRect[i].c == GESTURE_TYPE_PALM) {
			index = LB_WARNING_GESTURE_TYPE_PALM-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_GESTURE_TYPE_PALM, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_GESTURE_TYPE_PALM;
			}
		}
		if (p_det_info->pRect[i].c == GESTURE_TYPE_UP) {
			index = LB_WARNING_GESTURE_TYPE_UP-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_GESTURE_TYPE_UP, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_GESTURE_TYPE_UP;
			}
		}
		if (p_det_info->pRect[i].c == GESTURE_TYPE_DOWN) {
			index = LB_WARNING_GESTURE_TYPE_DOWN-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_GESTURE_TYPE_DOWN, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_GESTURE_TYPE_DOWN;
			}

		}
		if (p_det_info->pRect[i].c == GESTURE_TYPE_LEFT) {
			index = LB_WARNING_GESTURE_TYPE_LEFT-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_GESTURE_TYPE_LEFT, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_GESTURE_TYPE_LEFT;
			}

		}
		if (p_det_info->pRect[i].c == GESTURE_TYPE_RIGHT) {
			index = LB_WARNING_GESTURE_TYPE_RIGHT-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_GESTURE_TYPE_RIGHT, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_GESTURE_TYPE_RIGHT;
			}

		}
		if (p_det_info->pRect[i].c == GESTURE_TYPE_HEART) {
			index = LB_WARNING_GESTURE_TYPE_HEART-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_GESTURE_TYPE_HEART, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_GESTURE_TYPE_HEART;
			}

		}
		if (p_det_info->pRect[i].c == GESTURE_TYPE_QUIET) {
			index = LB_WARNING_GESTURE_TYPE_QUIET-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_GESTURE_TYPE_QUIET, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_GESTURE_TYPE_QUIET;
			}
		}
		if (p_det_info->pRect[i].c == GESTURE_TYPE_PINCH) {
			index = LB_WARNING_GESTURE_TYPE_PINCH-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_GESTURE_TYPE_PINCH, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_GESTURE_TYPE_PINCH;
			}
		}
	}

	return 0;
}

int nn_handdet_gesture_process(VIDEO_FRAME_INFO_S *frame, axframe_info_s *axframe_info)
{
    EI_S32 s32Ret;
	ezax_img_t img = {0};
	static ezax_boxes_t roi = {0};
	static ezax_rt_t rect[32] = {0};

    s32Ret = EI_MI_VBUF_FrameMmap(frame, VBUF_REMAP_MODE_CACHED);
    if (s32Ret != EI_SUCCESS)
    {
        EI_TRACE_VDEC(EI_DBG_ERR, "save yuv buffer error %x\n", s32Ret);
        return -1;
    }

	img.img_handle.fmt = EZAX_YUV420_SP;
	img.img_handle.w = axframe_info->u32Width;
	img.img_handle.h = axframe_info->u32Height;
	img.img_handle.stride = axframe_info->u32Width;
	img.img_handle.c = 3;
	img.nUID = 0;

	img.img_handle.pVir    = (void *)frame->stVFrame.ulPlaneVirAddr[0];
	img.img_handle.pPhy    = frame->stVFrame.u64PlanePhyAddr[0];
	img.img_handle.pVir_UV = (void *)frame->stVFrame.ulPlaneVirAddr[1];
	img.img_handle.pPhy_UV = frame->stVFrame.u64PlanePhyAddr[1];

	rect[0].x0 = 0;
	rect[0].y0 = 0;
	rect[0].x1 = axframe_info->u32Width - 1;
	rect[0].y1 = axframe_info->u32Height - 1;

	roi.num = 1;
	roi.pRect = rect;

	pthread_mutex_lock(&axframe_info->p_det_mutex);
	nna_handdet_process(axframe_info->handdet_hdl, &img, &roi, &axframe_info->p_det_info);
	if (axframe_info->p_det_info != NULL) {
		nna_gesture_process(axframe_info->gesture_hdl, &img, axframe_info->p_det_info);
	}
	pthread_mutex_unlock(&axframe_info->p_det_mutex);

#ifdef SAVE_AX_YUV_SP
    EI_S32 i;
#if 0
    EI_PRINT("save yuv buffer sizey %d, stride %d %d %d\n", frame->stVFrame.u32PlaneSizeValid[0], frame->stVFrame.u32PlaneStride[0], frame->stCommFrameInfo.u32Width, frame->stCommFrameInfo.u32Height);
    EI_PRINT("save yuv buffer sizeuv %d, u32PlaneNum %d\n", frame->stVFrame.u32PlaneSizeValid[1], frame->stVFrame.u32PlaneNum);

    EI_PRINT("save yuv buffer addr %llx %llx %llx\n",frame->stVFrame.u64PlanePhyAddr[0],
            frame->stVFrame.u64PlanePhyAddr[1], frame->stVFrame.u64PlanePhyAddr[2]);

    EI_PRINT("save yuv buffer virtual addr %x %x %x\n",frame->stVFrame.ulPlaneVirAddr[0],
            frame->stVFrame.ulPlaneVirAddr[1], frame->stVFrame.ulPlaneVirAddr[2]);

    EI_PRINT("save yuv buffer ID %x\n",frame->u32BufferId);
#endif
    for (i = 0; i < frame->stVFrame.u32PlaneNum; i++)
    {
        if (axframe_info.ax_fout) {
			if (img.img_handle.fmt == YUV420_SP) {
                if (i == 0) {
                    fwrite((void *)frame->stVFrame.ulPlaneVirAddr[i],
                        1,  img.img_handle.w *
                        img.img_handle.h, axframe_info.ax_fout);
                } else {
                    fwrite((void *)frame->stVFrame.ulPlaneVirAddr[i],
                        1,  img.img_handle.w *
                        img.img_handle.h/2, axframe_info.ax_fout);
                }
            }
        }
    }
#endif
    EI_MI_VBUF_FrameMunmap(frame, VBUF_REMAP_MODE_CACHED);

    return 0;
}

EI_VOID *get_axframe_proc(EI_VOID *p)
{
    int ret;
    VIDEO_FRAME_INFO_S axFrame = {0};
    axframe_info_s *axframe_info;
	ezax_handdet_cfg_t handdet_cfg;
	ezax_gesture_cfg_t gesture_cfg;

    axframe_info = (axframe_info_s *)p;
    EI_PRINT("get_axframe_proc bThreadStart = %d\n", axframe_info->bThreadStart);
#ifdef SAVE_AX_YUV_SP
    char srcfilename[FULLNAME_LEN_MAX];
	sprintf(srcfilename, "%s/gesture_raw_ch%d.yuv", PATH_ROOT, axframe_info->chn);
    axframe_info->ax_fout = fopen(srcfilename, "wb+");
    if (axframe_info->ax_fout == NULL) {
        EI_TRACE_VDEC(EI_DBG_ERR, "file open error1\n");
    }
#endif

#ifdef AL_PRO_FRAMERATE_STATISTIC
	int64_t time1 = 0LL, time2 = 0LL;
	int64_t frame_cnt = 0;
#endif
    EI_PRINT("********start_gesture******\n");
    memset(&handdet_cfg, 0, sizeof(ezax_handdet_cfg_t));
	memset(&gesture_cfg, 0, sizeof(ezax_gesture_cfg_t));
    /* init gesture, cfg */
	handdet_cfg.width= axframe_info->u32Width; //Ëã·¨ÊäÈëÍ¼Ïñ¿í
	handdet_cfg.height= axframe_info->u32Height; //Ëã·¨ÊäÈëÍ¼Ïñ¸ß
	handdet_cfg.model_rootpath = NN_HANDDET_MODEL_PATH;
	handdet_cfg.wh_ratio = axframe_info->u32Width/axframe_info->u32Height;
	handdet_cfg.conf_thresh = 0.85;

	axframe_info->handdet_hdl = nna_handdet_open(&handdet_cfg);

	gesture_cfg.width= axframe_info->u32Width; //Ëã·¨ÊäÈëÍ¼Ïñ¿í
	gesture_cfg.height= axframe_info->u32Height; //Ëã·¨ÊäÈëÍ¼Ïñ¸ß
	gesture_cfg.model_rootpath = NN_GESTURE_MODEL_PATH;
	gesture_cfg.thresh[0] = 0.7f;
	gesture_cfg.thresh[1] = 0.88f;
	gesture_cfg.thresh[2] = 0.92f;
	gesture_cfg.thresh[3] = 0.80f;
	gesture_cfg.thresh[4] = 0.9f;
	gesture_cfg.thresh[5] = 0.9f;
	gesture_cfg.thresh[6] = 0.85f;
	gesture_cfg.thresh[7] = 0.9f;
	gesture_cfg.thresh[8] = 0.9f;
	gesture_cfg.thresh[9] = 0.92f;

	axframe_info->gesture_hdl= nna_gesture_open(&gesture_cfg);
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time1 = osal_get_msec();
#endif
	for (int i= 0; i < LB_WARNING_END-LB_WARNING_BEGIN-1; i++) {
		warning_info.last_warn_time[i] = osal_get_msec();
	}
    while (EI_TRUE == axframe_info->bThreadStart) {
        memset(&axFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
        ret = EI_MI_VISS_GetChnFrame(axframe_info->dev,
            axframe_info->chn, &axFrame, 1000);
        if (ret != EI_SUCCESS) {
            EI_PRINT("chn %d get frame error 0x%x\n", axframe_info->chn, ret);
            usleep(100 * 1000);
            continue;
        }

        nn_handdet_gesture_process(&axFrame, axframe_info);
		gesture_warning(axframe_info->p_det_info);
#ifdef AL_PRO_FRAMERATE_STATISTIC
		frame_cnt++;
		time2 = osal_get_msec();
		if ((time2 - time1) >= 3000) {
			//EI_PRINT("algo process %lld fps\n", (frame_cnt * 1000) / (time2 - time1));
			time1 = time2;
			frame_cnt  = 0;
		}
#endif
        EI_MI_VISS_ReleaseChnFrame(axframe_info->dev,
            axframe_info->chn, &axFrame);
    }
    EI_MI_VISS_DisableChn(axframe_info->dev, axframe_info->chn);
	nna_handdet_close(axframe_info->handdet_hdl);
	nna_gesture_close(axframe_info->gesture_hdl);
#ifdef SAVE_AX_YUV_SP
    if (axframe_info->ax_fout) {
        fclose(axframe_info->ax_fout);
        axframe_info->ax_fout = NULL;
    }
#endif
    return NULL;
}
EI_VOID *get_drawframe_proc(EI_VOID *p)
{
    int ret;
    VIDEO_FRAME_INFO_S drawFrame = {0};
    drawframe_info_s *drawframe_info;

    char srcfilename[128];
    drawframe_info = (drawframe_info_s *)p;
    EI_PRINT("get_frame_proc bThreadStart = %d\n", drawframe_info->bThreadStart);

    sprintf(srcfilename, "%s/gesture_draw_raw_ch%d.yuv", PATH_ROOT, drawframe_info->chn);
    EI_PRINT("********get_drawframe_proc******\n");
#ifdef SAVE_DRAW_YUV_SP
    drawframe_info->draw_fout = fopen(srcfilename, "wb+");
    if (drawframe_info->draw_fout == NULL) {
        EI_TRACE_VDEC(EI_DBG_ERR, "file open error1\n");
    }
#endif
    while (EI_TRUE == drawframe_info->bThreadStart) {
        memset(&drawFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
        ret = EI_MI_VISS_GetChnFrame(drawframe_info->dev,
            drawframe_info->chn, &drawFrame, 1000);
        if (ret != EI_SUCCESS) {
            EI_PRINT("chn %d get frame error 0x%x\n", drawframe_info->chn, ret);
            usleep(100 * 1000);
            continue;
        }
		pthread_mutex_lock(&axframe_info.p_det_mutex);
		gesture_draw_location_rgn(&drawFrame, axframe_info.p_det_info);
		pthread_mutex_unlock(&axframe_info.p_det_mutex);
        drawFrame.enFrameType = MDP_FRAME_TYPE_DOSS;
        EI_MI_VO_SendFrame(0, 0, &drawFrame, 100);
#ifdef SAVE_DRAW_AX_H265
		drawFrame.enFrameType = MDP_FRAME_TYPE_VENC;
		EI_MI_VENC_SendFrame(stream_para[0].VeChn, &drawFrame, 100);
#endif
        EI_MI_VISS_ReleaseChnFrame(drawframe_info->dev,
            drawframe_info->chn, &drawFrame);
    }
    EI_MI_VISS_DisableChn(drawframe_info->dev, drawframe_info->chn);
#ifdef SAVE_DRAW_YUV_SP
    if (drawframe_info->draw_fout) {
        fclose(drawframe_info->draw_fout);
        drawframe_info->draw_fout = NULL;
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
			case LB_WARNING_GESTURE_TYPE_OK:
				warn_tone_play("/usr/share/out/res/gesture_warning/OK.wav");
				break;
			case LB_WARNING_GESTURE_TYPE_PALM:
				warn_tone_play("/usr/share/out/res/gesture_warning/shouzhang.wav");
				break;
			case LB_WARNING_GESTURE_TYPE_UP:
				warn_tone_play("/usr/share/out/res/gesture_warning/muzhixiangshang.wav");
				break;
			case LB_WARNING_GESTURE_TYPE_DOWN:
				warn_tone_play("/usr/share/out/res/gesture_warning/muzhixiangxia.wav");
				break;
			case LB_WARNING_GESTURE_TYPE_LEFT:
				warn_tone_play("/usr/share/out/res/gesture_warning/muzhixiangzuo.wav");
				break;
			case LB_WARNING_GESTURE_TYPE_RIGHT:
				warn_tone_play("/usr/share/out/res/gesture_warning/muzhixiangyou.wav");
				break;
			case LB_WARNING_GESTURE_TYPE_HEART:
				warn_tone_play("/usr/share/out/res/gesture_warning/bixin.wav");
				break;
			case LB_WARNING_GESTURE_TYPE_QUIET:
				warn_tone_play("/usr/share/out/res/gesture_warning/xu.wav");
				break;
			case LB_WARNING_GESTURE_TYPE_PINCH:
				warn_tone_play("/usr/share/out/res/gesture_warning/shuba.wav");
				break;
			default:
				break;
		}
	}

    return NULL;
}

static int start_get_axframe(axframe_info_s *axframe_info)
{
    int ret;
    static VISS_EXT_CHN_ATTR_S stExtChnAttr;

    stExtChnAttr.s32BindChn = axframe_info->phyChn;
    stExtChnAttr.u32Depth = 1;
    stExtChnAttr.stSize.u32Width = axframe_info->u32Width;
    stExtChnAttr.stSize.u32Height = axframe_info->u32Height;
    stExtChnAttr.u32Align = 32;
    stExtChnAttr.enPixFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stExtChnAttr.stFrameRate.s32SrcFrameRate = axframe_info->frameRate;
    stExtChnAttr.stFrameRate.s32DstFrameRate = AL_SEND_FRAMERATE;
    stExtChnAttr.s32Position = 1;

    ret = EI_MI_VISS_SetExtChnAttr(axframe_info->dev, axframe_info->chn, &stExtChnAttr);
    if (ret != EI_SUCCESS) {
        EI_PRINT("EI_MI_VISS_SetExtChnAttr failed with %#x!\n", ret);
        return EI_FAILURE;
    }
    ret = EI_MI_VISS_EnableChn(axframe_info->dev, axframe_info->chn);
    if (ret != EI_SUCCESS) {
        EI_PRINT("EI_MI_VISS_EnableChn failed with %#x!\n", ret);
        return EI_FAILURE;
    }

    axframe_info->bThreadStart = EI_TRUE;
    ret = pthread_create(&(axframe_info->gs_framePid), NULL, get_axframe_proc,
        (EI_VOID *)axframe_info);
    if (ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));
    EI_PRINT("start_get_axframe success %#x! phyChn:%d chn :%d dev:%d\n", ret, axframe_info->phyChn, axframe_info->chn, axframe_info->dev);

    return EI_SUCCESS;
}
static int start_get_drawframe(drawframe_info_s *drawframe_info)
{
    int ret;
    static VISS_EXT_CHN_ATTR_S stExtChnAttr;

    stExtChnAttr.s32BindChn = drawframe_info->phyChn;
    stExtChnAttr.u32Depth = 1;
    stExtChnAttr.stSize.u32Width = drawframe_info->u32Width;
    stExtChnAttr.stSize.u32Height = drawframe_info->u32Height;
    stExtChnAttr.u32Align = 32;
    stExtChnAttr.enPixFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stExtChnAttr.stFrameRate.s32SrcFrameRate = drawframe_info->frameRate;
    stExtChnAttr.stFrameRate.s32DstFrameRate = drawframe_info->frameRate;
    stExtChnAttr.s32Position = 1;

    ret = EI_MI_VISS_SetExtChnAttr(drawframe_info->dev, drawframe_info->chn, &stExtChnAttr);
    if (ret != EI_SUCCESS) {
        EI_PRINT("EI_MI_VISS_SetExtChnAttr failed with %#x!\n", ret);
        return EI_FAILURE;
    }
    ret = EI_MI_VISS_EnableChn(drawframe_info->dev, drawframe_info->chn);
    if (ret != EI_SUCCESS) {
        EI_PRINT("EI_MI_VISS_EnableChn failed with %#x!\n", ret);
        return EI_FAILURE;
    }

    drawframe_info->bThreadStart = EI_TRUE;
    ret = pthread_create(&(drawframe_info->gs_framePid), NULL, get_drawframe_proc,
        (EI_VOID *)drawframe_info);
    if (ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));
    EI_PRINT("start_get_drawframe success %#x! phyChn:%d chn :%d dev:%d\n", ret, drawframe_info->phyChn, drawframe_info->chn, drawframe_info->dev);

    return EI_SUCCESS;
}

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

int do_fallocate(const char *path, unsigned long fsize)
{
    int ret;
    int fd;

    fd = open(path, O_RDWR, 0666);
    if (fd >= 0) {
        close(fd);
		EI_TRACE_LOG(EI_DBG_ERR, "%s already exist\n", path);
        return -1;
    }
    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        EI_TRACE_LOG(EI_DBG_ERR, "open %s fail\n", path);
        return -2;
    }
    ret = fallocate(fd, FALLOC_FL_KEEP_SIZE, 0, fsize);
    close(fd);

    if (ret)
        EI_TRACE_LOG(EI_DBG_ERR, "fallocate fail(%i)\n", ret);
	else
        EI_TRACE_LOG(EI_DBG_INFO, "fallocate %s finish\n", path);

	return ret;
}
static int _get_first_file(char *path, char *first_fullname, char *szFilePostfix)
{
	DIR *dir = NULL;
	struct dirent *ptr = NULL;
	char first_filename[FULLNAME_LEN_MAX];
	int len;

	dir = opendir(path);
	if (dir == NULL) {
		EI_PRINT("Open dir error... %s\n", path);
		return -1;
	}
	memset(first_filename, 0x00, sizeof(first_filename));
	memset(first_fullname, 0x00, FULLNAME_LEN_MAX);
	while ((ptr = readdir(dir)) != NULL) {
		if (ptr->d_type == 8 && strstr(ptr->d_name, szFilePostfix) != NULL) {
			if (!strlen(first_filename)
				|| strncmp(first_filename, ptr->d_name, sizeof(ptr->d_name)) >= 0) {
				strcpy(first_filename, ptr->d_name);
				continue;
			}
		}
	}
	closedir(dir);
	if (!strlen(first_filename))
		return -2;
	strncpy(first_fullname, path, FULLNAME_LEN_MAX - 1);
	len = strlen(first_fullname);
	strncpy(first_fullname + len, first_filename, FULLNAME_LEN_MAX - len - 1);

	return 0;

}
int get_stream_file_name(sdr_sample_venc_para_t *sdr_venc_param, char *szFilePostfix, int len)
{
	int ret = 0;
	static sdcard_info_t info;
    int size;
	struct tm *p_tm; /* time variable */
	time_t now;
	now = time(NULL);
	p_tm = localtime(&now);
	char first_fullfilename[FULLNAME_LEN_MAX];

    if (sdr_venc_param == NULL)
        return -1;
    storage_info_get(PATH_ROOT, &info);
	memset(sdr_venc_param->szFilePostfix, 0x00, sizeof(sdr_venc_param->szFilePostfix));
	memset(sdr_venc_param->aszFileName, 0x00, sizeof(sdr_venc_param->aszFileName));
	memcpy(sdr_venc_param->szFilePostfix, szFilePostfix, len);
    sprintf(sdr_venc_param->aszFileName,
		"%s/stream_chn%02d-%02d%02d%02d%02d%02d%02d%s", PATH_ROOT, sdr_venc_param->phyChn,
		p_tm->tm_year + 1900, p_tm->tm_mon + 1, p_tm->tm_mday,
		p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec, sdr_venc_param->szFilePostfix);
	EI_PRINT("venc_para->aszFileName:%s\n", sdr_venc_param->aszFileName);
	if (info.free < 500 * 1024) {
    	ret = _get_first_file(PATH_ROOT, first_fullfilename, szFilePostfix);
		EI_PRINT("first_fullfilename:%s\n", first_fullfilename);
		if (ret < 0) {
			EI_PRINT("maybe first_fullfilename:%s is not exist\n", first_fullfilename);
			unlink(first_fullfilename);
		}
		/* rename for reduce fragment */
		ret = rename(first_fullfilename, sdr_venc_param->aszFileName);
		if (ret < 0) {
			EI_PRINT("maybe first_fullfilename:%s is not exist\n", first_fullfilename);
			unlink(first_fullfilename);
		}	
	} else {
		size = sdr_venc_param->bitrate * SDR_SAMPLE_VENC_TIME / 8 + 16 * 1024 *1024;
    	ret = do_fallocate(sdr_venc_param->aszFileName, size);
	}

	return ret;
}
#ifdef RTSP_SUPPORT
static int RTSP_SERVER_Media_Buf_Handle(dword dwFrameQueueNum, VENC_STREAM_S *stream)
{
	int ret = EI_SUCCESS;

    frame_dsc_s FrameDsc = {0};
    frame_dsc_s FrameDscLeft = {0};

    FrameDsc.pbyData = stream->pstPack.pu8Addr[0];
    FrameDsc.dwDataLen = stream->pstPack.u32Len[0];
	
    
	ret = live_video_set_data_to_buf(dwFrameQueueNum, &FrameDsc);
	if (ret != EI_SUCCESS) {
		EI_PRINT("line-%d live_video_set_data_to_buf %d \n", __LINE__, ret);
		return EI_FAILURE;
	}
    if (stream->pstPack.pu8Addr[1]  && stream->pstPack.u32Len[1] > 0){
        FrameDscLeft.pbyData = stream->pstPack.pu8Addr[1];
        FrameDscLeft.dwDataLen = stream->pstPack.u32Len[1];
		EI_PRINT("stream->pstPack.pu8Addr[1] exist ï¼\n");
		ret = live_video_set_data_to_buf(dwFrameQueueNum, &FrameDscLeft);
		if (ret != EI_SUCCESS) {
			EI_PRINT("line-%d frame_queue_push_node error \n", __LINE__);
			return EI_FAILURE;
		}
    }
	return EI_SUCCESS;
}
#endif

EI_VOID *_get_venc_stream_proc(EI_VOID *p)
{
    sdr_sample_venc_para_t *sdr_venc_para;
    VC_CHN VencChn;
    VENC_STREAM_S stStream;
    int ret;
    char thread_name[16];
    VENC_CHN_STATUS_S stStatus;
	struct tm *p_tm; /* time variable */
	time_t now;

    sdr_venc_para = (sdr_sample_venc_para_t *)p;
    if (!sdr_venc_para) {
        EI_TRACE_LOG(EI_DBG_ERR, "err param !\n");
        return NULL;
    }

    VencChn = sdr_venc_para->VeChn;

    thread_name[15] = 0;
    snprintf(thread_name, 16, "streamproc%d", VencChn);
    prctl(PR_SET_NAME, thread_name);
	ret = get_stream_file_name(sdr_venc_para, ".h265", sizeof(".h265"));
	while (ret < 0) {
		EI_TRACE_LOG(EI_DBG_ERR, "get_stream_file_name : %d\n", ret);
		usleep(1000*1000);
		ret = get_stream_file_name(sdr_venc_para, ".h265", sizeof(".h265"));
	}

	sdr_venc_para->flip_out = fopen(sdr_venc_para->aszFileName, "wb");
	if (!sdr_venc_para->flip_out) {
		EI_TRACE_LOG(EI_DBG_ERR, "creat file faile : %d\n");
	}
    while (EI_TRUE == sdr_venc_para->bThreadStart) {
    	 ret = EI_MI_VENC_QueryStatus(VencChn, &stStatus);
         if (ret != EI_SUCCESS) {
              EI_TRACE_VENC(EI_DBG_ERR, "query status chn-%d error : %d\n", VencChn, ret);
              break;
         }
        ret = EI_MI_VENC_GetStream(VencChn, &stStream, 1000);
        if (ret == EI_ERR_VENC_NOBUF) {
            EI_TRACE_LOG(EI_DBG_INFO, "No buffer\n");
            usleep(5 * 1000);
            continue;
        } else if (ret != EI_SUCCESS) {
            EI_TRACE_LOG(EI_DBG_ERR, "get chn-%d error : %d\n", VencChn, ret);
            break;
        }
		now = time(NULL);
		p_tm = localtime(&now);
		watermark_update(p_tm, WATERMARK_X, WATERMARK_Y, MOD_VENC);
		watermark_update(p_tm, WATERMARK_X, WATERMARK_Y, MOD_PREVIEW);
        if (stStream.pstPack.u32Len[0] == 0) {
            EI_PRINT("%d ch %d pstPack.u32Len:%d-%d, ofset:%d, ret:%x\n",
                __LINE__, VencChn, stStream.pstPack.u32Len[0],
                stStream.pstPack.u32Len[1],
                sdr_venc_para->buf_offset, ret);
        }
#ifdef RTSP_SUPPORT
		if (frame_queue_counter(0) < FRAME_QUEUE_SIZE) {
			RTSP_SERVER_Media_Buf_Handle(0, &stStream);
		 } else {
			EI_PRINT("line-%d frame_queue_counter(0) %d \n", __LINE__, frame_queue_counter(0));
		 }
#endif
		if (stStream.pstPack.u32Len[0] > 0) {
            fwrite(stStream.pstPack.pu8Addr[0], 1, stStream.pstPack.u32Len[0],
				sdr_venc_para->flip_out);
        }
        if (stStream.pstPack.u32Len[1] > 0) {
            fwrite(stStream.pstPack.pu8Addr[1], 1, stStream.pstPack.u32Len[1],
				sdr_venc_para->flip_out);
        }
		ret = EI_MI_VENC_ReleaseStream(VencChn, &stStream);
		if (ret != EI_SUCCESS) {
			EI_TRACE_VENC(EI_DBG_ERR, "release stream chn-%d error : %d\n", VencChn, ret);
			break;
		}
    }
   
	if (sdr_venc_para->flip_out) {
		fclose(sdr_venc_para->flip_out);
		sdr_venc_para->flip_out = NULL;
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

void stop_get_frame(int signo)
{
	stream_para[0].exit_flag = EI_TRUE;

    printf("%s %d, signal %d\n", __func__, __LINE__, signo);
   	printf("stop_get_frame end start!\n");
}


static int show_1ch(VISS_DEV	dev, VISS_CHN chn, ROTATION_E rot, EI_BOOL mirror, EI_BOOL flip,
    unsigned int rect_x, unsigned int rect_y, unsigned int width, unsigned int hight,
    SNS_TYPE_E sns)
{
	static VBUF_CONFIG_S stVbConfig;
	static SAMPLE_VISS_CONFIG_S stVissConfig;
#ifndef SAVE_DRAW_AX_H265
	static VISS_EXT_CHN_ATTR_S stExtChnAttr;
#endif
    EI_S32 s32Ret = EI_FAILURE;
    VO_VIDEO_LAYER_ATTR_S stVoLayerAttr = {0};
    VO_CHN_ATTR_S stVoChnAttr = {0};
	SAMPLE_VENC_CONFIG_S venc_config = {0};
	int loop_time;

    if (((sns >= TP9930_DVP_1920_1080_25FPS_1CH_YUV) &&
    	(sns <= TP9930_DVP_1920_1080_XXFPS_4CH_YUV)) ||
    	((sns >= TP2815_MIPI_1920_1080_25FPS_4CH_YUV) &&
    	(sns <= TP2815_MIPI_1920_1080_30FPS_4CH_YUV))) {
	    stVbConfig.astFrameInfo[0].enFrameType = MDP_FRAME_TYPE_COMMON;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Align = 32;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Width = 1920;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Height = 1080;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
	    stVbConfig.astCommPool[0].u32BufCnt = 40;
	    stVbConfig.astCommPool[0].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    } else {
	    stVbConfig.astFrameInfo[0].enFrameType = MDP_FRAME_TYPE_COMMON;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Align = 32;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Width = 1280;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Height = 720;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
	    stVbConfig.astCommPool[0].u32BufCnt = 40;
	    stVbConfig.astCommPool[0].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    }
    stVbConfig.astFrameInfo[1].enFrameType = MDP_FRAME_TYPE_COMMON;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Align = 32;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width = 1280;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height = 720;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stVbConfig.astCommPool[1].u32BufCnt = 48;
    stVbConfig.astCommPool[1].enRemapMode = VBUF_REMAP_MODE_CACHED;
    stVbConfig.u32PoolCnt = 2;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConfig);
    if (s32Ret) {
        goto exit0;
    }

    stVissConfig.astVissInfo[0].stDevInfo.VissDev = dev; /*0: DVP, 1: MIPI*/
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
    stVissConfig.astVissInfo[0].stSnsInfo.enSnsType = sns;
    stVissConfig.astVissInfo[0].stChnInfo.stChnAttr.stFrameRate.s32DstFrameRate =
		SDR_VISS_FRAMERATE;

    stVissConfig.astVissInfo[0].stChnInfo.stChnAttr.u32Depth = 2;
    stVissConfig.s32WorkingVissNum = 1;

    s32Ret = SAMPLE_COMM_VISS_StartViss(&stVissConfig);
    EI_TRACE_VENC(EI_DBG_ERR," comm viss start test\n");
    if (s32Ret) {
        EI_TRACE_VENC(EI_DBG_ERR," comm viss start error\n");
    }
    /* display begin */
    EI_MI_DOSS_Init();

	drawframe_info.VoDev = 0;
	drawframe_info.VoLayer = 0;
	drawframe_info.VoChn = 0;
	drawframe_info.u32scnWidth = SCN_WIDTH;
	drawframe_info.u32scnHeight = SCN_HEIGHT;
	s32Ret = EI_MI_VO_Enable(drawframe_info.VoDev);
    if (s32Ret != EI_SUCCESS) {
        EI_PRINT("EI_MI_VO_Enable() failed with %#x!\n", s32Ret);
        goto exit1;
    }

    stVoLayerAttr.u32Align = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Align;
    stVoLayerAttr.stImageSize.u32Width = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
    stVoLayerAttr.stImageSize.u32Height = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
    stVoLayerAttr.enPixFormat = stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat;
    stVoLayerAttr.stDispRect.s32X = 0;
    stVoLayerAttr.stDispRect.s32Y = 0;
    stVoLayerAttr.stDispRect.u32Width = width;
    stVoLayerAttr.stDispRect.u32Height = hight;
    s32Ret = EI_MI_VO_SetVideoLayerAttr(drawframe_info.VoLayer, &stVoLayerAttr);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit1;
    }

    s32Ret = EI_MI_VO_SetDisplayBufLen(drawframe_info.VoLayer, 3);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit1;
    }

    s32Ret = EI_MI_VO_SetVideoLayerPartitionMode(drawframe_info.VoLayer, VO_PART_MODE_MULTI);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit1;
    }

    s32Ret = EI_MI_VO_SetVideoLayerPriority(drawframe_info.VoLayer, 0);
    if (s32Ret != EI_SUCCESS) {
		EI_TRACE_LOG(EI_DBG_ERR, "\n");
		goto exit1;
    }

    s32Ret = EI_MI_VO_EnableVideoLayer(drawframe_info.VoLayer);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit1;
    }

    stVoChnAttr.stRect.s32X = 0;
    stVoChnAttr.stRect.s32Y = 0;
    stVoChnAttr.stRect.u32Width = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
    stVoChnAttr.stRect.u32Height = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;

    EI_MI_VO_SetChnAttr(drawframe_info.VoLayer, drawframe_info.VoChn, &stVoChnAttr);
    EI_MI_VO_EnableChn(drawframe_info.VoLayer, drawframe_info.VoChn);
    EI_MI_VO_SetChnRotation(drawframe_info.VoLayer, drawframe_info.VoChn, rot);
	pthread_mutex_init(&axframe_info.p_det_mutex, NULL);
	axframe_info.p_det_info = NULL;
    axframe_info.dev = dev;
    axframe_info.chn = VISS_MAX_PHY_CHN_NUM;
    axframe_info.phyChn = chn;
    axframe_info.frameRate = SDR_VISS_FRAMERATE;
    axframe_info.u32Width = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
    axframe_info.u32Height = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
	axframe_info.ax_fout = NULL;

    start_get_axframe(&axframe_info);
    drawframe_info.dev = dev;
    drawframe_info.chn = VISS_MAX_PHY_CHN_NUM+1;
    drawframe_info.phyChn = chn;
    drawframe_info.frameRate = SDR_VISS_FRAMERATE;
    drawframe_info.u32Width = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
    drawframe_info.u32Height = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
	drawframe_info.draw_fout = NULL;
	drawframe_info.last_draw_num = RGN_RECT_NUM;
	s32Ret = rgn_start();
	if (s32Ret == EI_FAILURE) {
		 EI_PRINT("rgn_start failed with %#x!\n", s32Ret);
	}
    start_get_drawframe(&drawframe_info);
    venc_config.enInputFormat   = stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat;
    venc_config.u32width	    = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
    venc_config.u32height	    = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
    venc_config.u32bitrate      = SDR_SAMPLE_VENC_BITRATE;
    venc_config.u32srcframerate = SDR_VISS_FRAMERATE;
    venc_config.u32dstframerate = SDR_VISS_FRAMERATE;
	memset(stream_para, 0, sizeof(stream_para));
	stream_para[0].dev = dev;
	stream_para[0].VeChn = 0;
	stream_para[0].chn = VISS_MAX_PHY_CHN_NUM+2;
	stream_para[0].bitrate = SDR_SAMPLE_VENC_BITRATE;
	stream_para[0].exit_flag = EI_FALSE;
	stream_para[0].phyChn = chn;
	watermark_init(0, VENC_RGN_WM_NUM, watermark_filename, MOD_VENC);
	watermark_init(VENC_RGN_WM_NUM+RGN_ARRAY_NUM, VENC_RGN_WM_NUM, watermark_filename, MOD_PREVIEW);
#ifndef SAVE_DRAW_AX_H265
	stExtChnAttr.s32BindChn = chn;
    stExtChnAttr.u32Depth = 1;
    stExtChnAttr.stSize.u32Width = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
    stExtChnAttr.stSize.u32Height = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
    stExtChnAttr.u32Align = 32;
    stExtChnAttr.enPixFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stExtChnAttr.stFrameRate.s32SrcFrameRate = SDR_VISS_FRAMERATE;
    stExtChnAttr.stFrameRate.s32DstFrameRate = SDR_VISS_FRAMERATE;
    stExtChnAttr.s32Position = 1;
#endif
	EI_PRINT("mdp start rec\n");
#ifdef RTSP_SUPPORT
	frame_queue_init_default();
#endif
#ifdef RTSP_SUPPORT
	set_rtsp_work(1);
#endif
	while (stream_para[0].exit_flag != EI_TRUE) {
	    s32Ret = SAMPLE_COMM_VENC_Start(stream_para[0].VeChn, PT_H265,
	                SAMPLE_RC_CBR, &venc_config,
	                COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
	    if (EI_SUCCESS != s32Ret) {
	        EI_PRINT("%s %d %d ret:%d \n", __func__, __LINE__, stream_para[0].VeChn, s32Ret);
	    }
#ifndef SAVE_DRAW_AX_H265
	    s32Ret = EI_MI_VISS_SetExtChnAttr(dev, stream_para[0].chn, &stExtChnAttr);
	    if (s32Ret != EI_SUCCESS) {
	        EI_PRINT("EI_MI_VISS_SetExtChnAttr failed with %#x!\n", s32Ret);
	        return EI_FAILURE;
	    }
	    s32Ret = EI_MI_VISS_EnableChn(dev, stream_para[0].chn);
	    if (s32Ret != EI_SUCCESS) {
	        EI_PRINT("EI_MI_VISS_EnableChn failed with %#x!\n", s32Ret);
			SAMPLE_COMM_VENC_Stop(stream_para[0].VeChn);
	        return EI_FAILURE;
	    }
	    s32Ret = SAMPLE_COMM_VISS_Link_VPU(dev, stream_para[0].chn, stream_para[0].VeChn);
	    if (EI_SUCCESS != s32Ret) {
	        EI_PRINT("viss link vpu failed %d-%d-%d \n",
	            0, 0, stream_para[0].VeChn);
	        SAMPLE_COMM_VENC_Stop(stream_para[0].VeChn);
			EI_MI_VISS_DisableChn(dev, stream_para[0].chn);
			return EI_FAILURE;
	    }
#endif
	    s32Ret = _start_get_stream(&stream_para[0]);
	    if (EI_SUCCESS != s32Ret) {
	        EI_PRINT("_StartGetStream failed %d \n", stream_para[0].VeChn);
#ifndef SAVE_DRAW_AX_H265
	        SAMPLE_COMM_VISS_UnLink_VPU(dev, stream_para[0].chn, stream_para[0].VeChn);
#endif
	        SAMPLE_COMM_VENC_Stop(stream_para[0].VeChn);
	    }

	    loop_time = SDR_SAMPLE_VENC_TIME;
	    while (loop_time-- && stream_para[0].exit_flag != EI_TRUE) {
	        usleep(1000 * 1000);
	        if (loop_time % 5 == 4)
	            EI_PRINT("loop_time %d\n", loop_time);
	        //EI_TRACE_LOG(EI_DBG_ERR,"loop_time %d\n", loop_time);
	    }
	    stream_para[0].bThreadStart = EI_FALSE;
	    _stop_get_stream(&stream_para[0]);
#ifndef SAVE_DRAW_AX_H265
	    SAMPLE_COMM_VISS_UnLink_VPU(dev, stream_para[0].chn, stream_para[0].VeChn);
		EI_MI_VISS_DisableChn(dev, stream_para[0].chn);
#endif
		SAMPLE_COMM_VENC_Stop(stream_para[0].VeChn);
	}
#ifdef RTSP_SUPPORT
	set_rtsp_work(0);
#endif
	EI_PRINT("mdp quit rec\n");
	printf("gesture wait exit!\n");
    axframe_info.bThreadStart = EI_FALSE;
    if (axframe_info.gs_framePid)
           pthread_join(axframe_info.gs_framePid, NULL);
    drawframe_info.bThreadStart = EI_FALSE;
    if (drawframe_info.gs_framePid)
		pthread_join(drawframe_info.gs_framePid, NULL);
	pthread_mutex_destroy(&axframe_info.p_det_mutex);
	watermark_exit(VENC_RGN_WM_NUM, MOD_PREVIEW);
	watermark_exit(VENC_RGN_WM_NUM, MOD_VENC);
	rgn_stop();
    printf("gesture start exit!\n");
exit1:
    SAMPLE_COMM_VISS_StopViss(&stVissConfig);
    EI_MI_VO_DisableChn(drawframe_info.VoLayer, drawframe_info.VoChn);
    EI_MI_VO_DisableVideoLayer(drawframe_info.VoLayer);
    SAMPLE_COMM_DOSS_StopDev(drawframe_info.VoDev);
    EI_MI_VO_CloseFd();
    EI_MI_DOSS_Exit();
exit0:
    SAMPLE_COMM_SYS_Exit(&stVbConfig);
    printf("show_1ch end exit!\n");

    return s32Ret;

}


int main(int argc, char **argv)
{
    EI_S32 s32Ret = EI_FAILURE;
    VISS_DEV dev = 0;
    VISS_CHN chn = 0;
    SNS_TYPE_E sns = TP9930_DVP_1920_1080_25FPS_4CH_YUV;
    int rot = 90;
    EI_BOOL flip = EI_FALSE;
    EI_BOOL mirror = EI_FALSE;
    unsigned int rect_x = 0;
    unsigned int rect_y = 0;
    unsigned int width = 720;
    unsigned int hight = 1280;
    int c;

	EI_PRINT("[%d] mdp_rtsp start v1.0rc1_202302161000\n", __LINE__);
	memset(&axframe_info, 0x00, sizeof(axframe_info));
	memset(&drawframe_info, 0x00, sizeof(drawframe_info));
    while ((c = getopt(argc, argv, "d:c:r:s:h:")) != -1) {
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
		case 's':
			sns = (unsigned int)strtol(argv[optind - 1], NULL, 10);
 			break;
        default:
            preview_help();
            goto EXIT;
        }
    }
    signal(SIGINT, stop_get_frame);
    signal(SIGTERM, stop_get_frame);
    if (rot == 90) {
        rot = ROTATION_90;
    } else if (rot == 180) {
        rot = ROTATION_180;
    } else if (rot == 270) {
        rot = ROTATION_270;
    }
	warning_info.mq_id = osal_mq_create("/waring_mq", MQ_MSG_NUM, MQ_MSG_LEN);
	if (warning_info.mq_id == 0) {
		EI_PRINT("[%s, %d]Create waring queue failed!\n", __func__, __LINE__);
		goto EXIT;
	}

	warning_info.bThreadStart = EI_TRUE;
    s32Ret = pthread_create(&(warning_info.g_Pid), NULL, warning_proc,
        (EI_VOID *)&warning_info);
    if (s32Ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));
    s32Ret = show_1ch(dev, chn, rot, mirror, flip, rect_x, rect_y, width, hight, sns);
	warning_info.bThreadStart = EI_FALSE;
	if (warning_info.g_Pid) {
           pthread_join(warning_info.g_Pid, NULL);
	}
	s32Ret = osal_mq_destroy(warning_info.mq_id, "/waring_mq");
	if (s32Ret) {
		EI_PRINT("[%s, %d]waring_mq destory failed!\n", __func__, __LINE__);
	}
	EI_PRINT("[%d] mdp_gesture EXIT\n", __LINE__);
	exit(EXIT_SUCCESS);
EXIT:
    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
