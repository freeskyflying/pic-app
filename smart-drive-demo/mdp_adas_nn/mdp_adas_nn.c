/*
 *------------------------------------------------------------------------------
 * @File      :    mdp_adas.c
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
#include "nn_adas_api.h"
#include "mdp_adas_config.h"

#define NN_ADAS_PATH	 "/usr/share/ax/adas"
#define COUNT_CALIBRATE_NUM 20
#define PATH_ROOT "/data/" /* save file root path */
#define MQ_MSG_NUM 128 /* message queue number */
#define MQ_MSG_LEN 128 /* every message max lenght */
#define ALARM_INTERVAL 5000 /* same warning message interval time unit:ms */
#define CALI_FAIL_ALARM_INTERVAL 20000 /* same warning message interval time unit:ms */
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
#define VBUF_POOL_MAX_NUM 4
#define VBUF_BUFFER_MAX_NUM 64

typedef enum _WARNING_MSG_TYPE_ {
	LB_WARNING_BEGIN = 0xF000,
	LB_WARNING_ADAS_LAUNCH,//前车启动
	LB_WARNING_ADAS_DISTANCE,//保持车距
	LB_WARNING_ADAS_COLLIDE,//注意碰撞
	LB_WARNING_ADAS_PRESSURE,//车道偏移
	LB_WARNING_ADAS_PEDS,//小心行人
	LB_WARNING_ADAS_CHANGLANE,//车辆变道
	LB_WARNING_ADAS_CALIBRATE_START,//校准开始，not used, 用于手动校准
	LB_WARNING_ADAS_CALIBRATE_SUCCESS,//校准成功, not used, 用于手动校准
	LB_WARNING_ADAS_CALIBRATE_FAIL,//校准失败，not used, 用于手动校准
	LB_WARNING_USERDEF,//自定义系统消息
	LB_WARNING_END,
} WARNING_MSG_TYPE;

typedef enum _adas_cali_flag_{
	ADAS_CALI_START = 1,
	ADAS_CALI_END,
	ADAS_CALI_MAX,
} adas_cali_flag_e;

typedef enum _ADAS_ACTION_ {
	ADAS_RECOGNITION,
	ADAS_CALIB,
	ADAS_ACTION_END,
} ADAS_ACTION;

typedef enum _ADAS_LINE_ {
	LEFT_LANE_LINE,
	RIGHT_LANE_LINE,
	VANISH_Y_LINE,
	VANISH_X_LINE,
	BOTTOM_Y_LINE,
} ADAS_LINE;

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

typedef struct _camera_info_s{
	VISS_DEV dev;
	SNS_TYPE_E sns;
	EI_U32 chn_num;
	VISS_CHN chn;
	VISS_OUT_PATH_E viss_out_path;
	ROTATION_E rot;
	EI_BOOL mirror[VISS_MAX_CHN_NUM];
	EI_BOOL flip[VISS_MAX_CHN_NUM];
	SIZE_S stSize[VISS_MAX_CHN_NUM];
	EI_U32 u32FrameRate[VISS_MAX_CHN_NUM];
	CH_FORMAT_S stFormat[VISS_DEV_MAX_CHN_NUM];
} camera_info_s;

typedef struct _axframe_info_s {
	ADAS_ACTION action;
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
    sem_t frame_del_sem;
	nn_adas_cfg_t m_nn_adas_cfg;
	nn_adas_out_t *p_det_info;
	void *nn_hdl;
	FILE *ax_fout;
} axframe_info_s;

typedef struct _drawframe_info_s {
    EI_BOOL bThreadStart;
	EI_BOOL is_disp;
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
	MDP_CHN_S stRgnChn[2];/* 0:time watermark 1:rect or line;*/
	char *wm_data[VENC_RGN_WM_SOURCE_NUM];
	int last_draw_rect_num;
	int last_draw_line0_num;
	int last_draw_line1_num;
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

typedef struct _adas_calibrate_para_ {
	float vanish_point_x;
	float vanish_point_y;
	float bottom_line_y;
	adas_cali_flag_e cali_flag;
	ADAS_LINE adas_calibline;
	unsigned int adas_calibstep;
	int auto_calib_flag;
} adas_calibrate_para_t;

typedef struct eiVBUF_MAP_S {
    VBUF_BUFFER u32BufID;
	VIDEO_FRAME_INFO_S stVfrmInfo;
} VBUF_MAP_S;

typedef struct _pool_info_t_ {
	VBUF_BUFFER Buffer[VBUF_BUFFER_MAX_NUM];
	VBUF_POOL Pool;
    VBUF_REMAP_MODE_E enRemapMode;
    EI_U32 u32BufNum;
    VBUF_MAP_S *pstBufMap;
} pool_info_t;

static axframe_info_s axframe_info;
static drawframe_info_s drawframe_info;
static warning_info_s warning_info;
sdr_sample_venc_para_t stream_para[ISP_MAX_DEV_NUM] = {{0}};
static adas_calibrate_para_t m_adas_cali;
static pool_info_t pool_info[VBUF_POOL_MAX_NUM] = {0};

void preview_help(void)
{
    OSAL_LOGW("usage: mdp_adas_nn \n");
    OSAL_LOGW("such as for adas recognition: mdp_adas_nn\n");
    OSAL_LOGW("such as for adas recognition: mdp_adas_nn -d 0 -c 0 -r 90\n");
	OSAL_LOGW("such as for calib: mdp_adas_nn -d 0 -c 0 -r 90 -a 1\n");

    OSAL_LOGW("arguments:\n");
    OSAL_LOGW("-d    select input device, Dev:0~2, 0:dvp,1:mipi0, 2:mipi1, default: 0, means dvp interface\n");
    OSAL_LOGW("-c    select chn id, support:0~3, default: 0\n");
    OSAL_LOGW("-r    select rotation, 0/90/180/270, default: 90\n");
	OSAL_LOGW("-s    sensor or rx macro value, see ei_comm_camera.h, such as 200800, default: 100301\n");
	OSAL_LOGW("-a    action 0: adas recognition 1: adas calib, default: 0\n");
	OSAL_LOGW("-p    disp enable or diable, 0: disable 1: disp enable, default: 1\n");
    OSAL_LOGW("-h    help\n");
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
        OSAL_LOGE("EI_MI_RGN_Create \n");
        goto exit;
    }
	drawframe_info.Handle[VENC_RGN_WM_NUM+1] = VENC_RGN_WM_NUM+1;
    drawframe_info.stRegion[2].enType = LINEEX_ARRAY_RGN;
    drawframe_info.stRegion[2].unAttr.u32LineExArrayMaxNum = RGN_LINE_NUM;
    s32Ret = EI_MI_RGN_Create(drawframe_info.Handle[VENC_RGN_WM_NUM+1], &drawframe_info.stRegion[2]);
    if(s32Ret) {
        OSAL_LOGE("EI_MI_RGN_Create \n");
        goto exit;
    }
	drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].bShow = EI_TRUE;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].enType = RECTANGLEEX_ARRAY_RGN;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].unChnAttr.stRectangleExArrayChn.u32MaxRectNum = RGN_RECT_NUM;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].unChnAttr.stRectangleExArrayChn.u32ValidRectNum = RGN_RECT_NUM;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].unChnAttr.stRectangleExArrayChn.pstRectExArraySubAttr = drawframe_info.stRectArray;
    s32Ret = EI_MI_RGN_AddToChn(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
	if(s32Ret) {
        OSAL_LOGE("EI_MI_RGN_AddToChn \n");
    }
	drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1].bShow = EI_TRUE;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1].enType = LINEEX_ARRAY_RGN;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1].unChnAttr.stLineExArrayChn.u32MaxLineNum = RGN_LINE_NUM;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1].unChnAttr.stLineExArrayChn.u32ValidLineNum = RGN_LINE_NUM;
    drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1].unChnAttr.stLineExArrayChn.pstLineExArraySubAttr = drawframe_info.stLineArray;
    s32Ret = EI_MI_RGN_AddToChn(drawframe_info.Handle[VENC_RGN_WM_NUM+1], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1]);
	if(s32Ret) {
        OSAL_LOGE("EI_MI_RGN_AddToChn \n");
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

int adas_update_para(float vanish_point_x, float vanish_point_y, float bottom_line_y)
{
	OSAL_LOGE("********adas_update()******");
	int ret = -1;
	if (axframe_info.nn_hdl == NULL) {
		return ret;
	}
	axframe_info.m_nn_adas_cfg.camera_param.vanish_point[0] = vanish_point_x;
	axframe_info.m_nn_adas_cfg.camera_param.vanish_point[1] = vanish_point_y;
	axframe_info.m_nn_adas_cfg.camera_param.bottom_line = bottom_line_y;
	ret = nn_adas_cmd(axframe_info.nn_hdl, NN_ADAS_SET_PARAM, &axframe_info.m_nn_adas_cfg);
	if( ret == -1) {
		OSAL_LOGE("NN_ADAS_SET_PARAM, failed failed!!");
	}
	return ret;
}

static void adas_draw_location_rgn(VIDEO_FRAME_INFO_S *drawFrame, nn_adas_out_t *p_det_info)
{
	int draw_rect_num, draw_line0_num, draw_line1_num;
	int index = 0;
	int loc_x0, loc_y0, loc_x1, loc_y1;
	EI_S32 s32Ret = EI_FAILURE;

	if (axframe_info.action == ADAS_CALIB) {
		for (int i = 2; i < 5; i++) {
			drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1].bShow = EI_TRUE;
			drawframe_info.stLineArray[i].isShow = EI_TRUE;
			if (i == VANISH_Y_LINE) {
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32X = 0;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32Y = m_adas_cali.vanish_point_y;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32X = drawframe_info.u32scnWidth;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32Y = m_adas_cali.vanish_point_y;
		        drawframe_info.stLineArray[i].stLineAttr.u32Color = 0xffff0000;
			} else if (i == VANISH_X_LINE) {
				drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32X = m_adas_cali.vanish_point_x;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32Y = 0;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32X = m_adas_cali.vanish_point_x;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32Y = drawframe_info.u32scnHeight;
		        drawframe_info.stLineArray[i].stLineAttr.u32Color = 0xff00ff00;
			} else if (i == BOTTOM_Y_LINE) {
				drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32X = 0;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32Y = m_adas_cali.bottom_line_y;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32X = drawframe_info.u32scnWidth;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32Y = m_adas_cali.bottom_line_y;
		        drawframe_info.stLineArray[i].stLineAttr.u32Color = 0xff8000ff;
			}
			if (i==m_adas_cali.adas_calibline)
				drawframe_info.stLineArray[i].stLineAttr.u32LineSize = 8;
			else
	        	drawframe_info.stLineArray[i].stLineAttr.u32LineSize = 4;
			s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM+1], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1]);
			if(s32Ret) {
				OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
	    	}
    	}
	} else {
		draw_rect_num = p_det_info->cars.size + p_det_info->peds.size;
		draw_line0_num = p_det_info->lanes.lanes[0].exist;
		draw_line1_num = p_det_info->lanes.lanes[1].exist;
		if (draw_rect_num > RGN_RECT_NUM || (draw_line0_num+draw_line1_num) > RGN_LINE_NUM)
			return;

		if (drawframe_info.last_draw_rect_num > draw_rect_num) {
			for (int i = drawframe_info.last_draw_rect_num-1; i >= draw_rect_num; i--) {
				drawframe_info.stRectArray[i].isShow = EI_FALSE;
				s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM],
					&drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
				if(s32Ret) {
					OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
	    		}
			}
		}
		if (drawframe_info.last_draw_line0_num > draw_line0_num) {
				drawframe_info.stLineArray[0].isShow = EI_FALSE;
				s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM+1],
					&drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1]);
				if(s32Ret) {
					OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
	    		}
		}
		if (drawframe_info.last_draw_line1_num > draw_line1_num) {
				drawframe_info.stLineArray[1].isShow = EI_FALSE;
				s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM+1],
					&drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1]);
				if(s32Ret) {
					OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
	    		}
		}
	    if (p_det_info->cars.size) {
			for (int cnt_cars = 0; cnt_cars < p_det_info->cars.size; cnt_cars++) {
		        loc_x0 = p_det_info->cars.p[cnt_cars].box[0];
		        loc_y0 = p_det_info->cars.p[cnt_cars].box[1];
		        loc_x1 = p_det_info->cars.p[cnt_cars].box[2];
		        loc_y1 = p_det_info->cars.p[cnt_cars].box[3];
		        drawframe_info.stRectArray[cnt_cars].stRectAttr.stRect.s32X = loc_x0;
				drawframe_info.stRectArray[cnt_cars].stRectAttr.stRect.s32Y = loc_y0;
				drawframe_info.stRectArray[cnt_cars].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
				drawframe_info.stRectArray[cnt_cars].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
				drawframe_info.stRectArray[cnt_cars].stRectAttr.u32BorderSize = 4;
				if (p_det_info->cars.p[cnt_cars].region <= 1)
					drawframe_info.stRectArray[cnt_cars].stRectAttr.u32Color = 0xffffff00;
				else
					drawframe_info.stRectArray[cnt_cars].stRectAttr.u32Color = 0xff00ff00;
				drawframe_info.stRectArray[cnt_cars].isShow = EI_TRUE;
				drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].bShow = EI_TRUE;
				s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
				if(s32Ret) {
					OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
	    		}
			}
	    }
		index = index + p_det_info->cars.size;
	    for (int cnt_peds = 0; cnt_peds < p_det_info->peds.size; cnt_peds++) {
	        loc_x0 = p_det_info->peds.p[cnt_peds].box[0];
	        loc_y0 = p_det_info->peds.p[cnt_peds].box[1];
	        loc_x1 = p_det_info->peds.p[cnt_peds].box[2];
	        loc_y1 = p_det_info->peds.p[cnt_peds].box[3];
	        drawframe_info.stRectArray[index+cnt_peds].stRectAttr.stRect.s32X = loc_x0;
			drawframe_info.stRectArray[index+cnt_peds].stRectAttr.stRect.s32Y = loc_y0;
			drawframe_info.stRectArray[index+cnt_peds].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
			drawframe_info.stRectArray[index+cnt_peds].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
			drawframe_info.stRectArray[index+cnt_peds].stRectAttr.u32BorderSize = 4;
			drawframe_info.stRectArray[index+cnt_peds].stRectAttr.u32Color = 0xffff0000;
			drawframe_info.stRectArray[index+cnt_peds].isShow = EI_TRUE;
			drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].bShow = EI_TRUE;
			s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
			if(s32Ret) {
				OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
	    	}
	    }
		drawframe_info.last_draw_rect_num = draw_rect_num;
	    for (int i = 0; i < 2; i++) {
	        if (p_det_info->lanes.lanes[i].exist) {
				drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1].bShow = EI_TRUE;
				drawframe_info.stLineArray[i].isShow = EI_TRUE;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32X = p_det_info->lanes.lanes[i].pts[0][0];
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32Y = p_det_info->lanes.lanes[i].pts[0][1];
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32X = p_det_info->lanes.lanes[i].pts[5][0];
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32Y = p_det_info->lanes.lanes[i].pts[5][1];
		        drawframe_info.stLineArray[i].stLineAttr.u32Color = 0xffffff80;
		        drawframe_info.stLineArray[i].stLineAttr.u32LineSize = 4;
				s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM+1], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1]);
				if(s32Ret) {
					OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
	    		}
	        }
    	}
		drawframe_info.last_draw_line0_num = draw_line0_num;
		drawframe_info.last_draw_line1_num = draw_line1_num;
		for (int i = 2; i < 5; i++) {
			drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1].bShow = EI_TRUE;
			drawframe_info.stLineArray[i].isShow = EI_TRUE;
			if (i == VANISH_Y_LINE) {
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32X = 0;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32Y = m_adas_cali.vanish_point_y;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32X = drawframe_info.u32scnWidth;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32Y = m_adas_cali.vanish_point_y;
		        drawframe_info.stLineArray[i].stLineAttr.u32Color = 0xffff0000;
			} else if (i == VANISH_X_LINE) {
				drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32X = m_adas_cali.vanish_point_x;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32Y = 0;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32X = m_adas_cali.vanish_point_x;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32Y = drawframe_info.u32scnHeight;
		        drawframe_info.stLineArray[i].stLineAttr.u32Color = 0xff00ff00;
			} else if (i == BOTTOM_Y_LINE) {
				drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32X = 0;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32Y = m_adas_cali.bottom_line_y;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32X = drawframe_info.u32scnWidth;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32Y = m_adas_cali.bottom_line_y;
		        drawframe_info.stLineArray[i].stLineAttr.u32Color = 0xff8000ff;
			}
	        drawframe_info.stLineArray[i].stLineAttr.u32LineSize = 4;
		}
	}
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
			OSAL_LOGE("EI_MI_RGN_Create fail\n");
	        goto exit1;
	    }
	    stRgnChnAttr[i].bShow = EI_TRUE;
	    stRgnChnAttr[i].enType = OVERLAYEX_RGN;
	    stRgnChnAttr[i].unChnAttr.stOverlayExChn.stPoint.s32X = WATERMARK_X;
		stRgnChnAttr[i].unChnAttr.stOverlayExChn.stPoint.s32Y = WATERMARK_Y;
	    stRgnChnAttr[i].unChnAttr.stOverlayExChn.u32FgAlpha = 255;
	    s32Ret = EI_MI_RGN_AddToChn(Handle[i], stRgnChn, &stRgnChnAttr[i]);
	    if (s32Ret) {
			OSAL_LOGE("EI_MI_RGN_AddToChn fail\n");
	        goto exit1;
	    }
	}
	size = stRegion[0].unAttr.stOverlayEx.stSize.u32Width * stRegion[0].unAttr.stOverlayEx.stSize.u32Height * 4;
	for (int i = 0; i < VENC_RGN_WM_SOURCE_NUM; i++) {
		if (mod == MOD_VENC) {
			stream_para[0].wm_data[i] = malloc(size);
			if (!stream_para[0].wm_data[i]) {
				OSAL_LOGE("malloc fail\n");
				return -1;
			}
			fd = fopen(source_name[i], "rb");
			if (!fd) {
				OSAL_LOGE("fopen fail\n");
				return -1;
			}
			fseek(fd, 54, SEEK_SET);
			len = fread(stream_para[0].wm_data[i], 1, size, fd);
			if(len!=size) {
				OSAL_LOGE("fread size fail\n");
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
				OSAL_LOGE("malloc fail\n");
				return -1;
			}
			fd = fopen(source_name[i], "rb");
			if (!fd) {
				OSAL_LOGE("fopen fail\n");
				return -1;
			}
			fseek(fd, 54, SEEK_SET);
			len = fread(drawframe_info.wm_data[i], 1, size, fd);
			if(len!=size) {
				OSAL_LOGE("fread size fail\n");
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
			OSAL_LOGE("fopen fail\n");
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
	        OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
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

int adas_warning(nn_adas_out_t *data)
{
	int64_t cur_time = osal_get_msec();
	int   status, index;

	if (axframe_info.action == ADAS_CALIB) {
		return 0;
	}

	if (data->cars.size > 0) {
		for (int cnt_cars = 0; cnt_cars < data->cars.size; cnt_cars++) {
			status = data->cars.p[cnt_cars].status;
			OSAL_LOGE("cnt_cars %d cars status:%d\n", cnt_cars, status);
			if (status == WARN_STATUS_FCW) {
				index = LB_WARNING_ADAS_COLLIDE-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_ADAS_COLLIDE, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_ADAS_COLLIDE;
				}
 			} else if (status >= WARN_STATUS_HMW_LEVEL1 && status <= WARN_STATUS_HMW_LEVEL4) {
				index = LB_WARNING_ADAS_DISTANCE-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_ADAS_DISTANCE, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_ADAS_DISTANCE;
				}
			} else if (status == WARN_STATUS_FMW) {
				index = LB_WARNING_ADAS_LAUNCH-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_ADAS_LAUNCH, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_ADAS_LAUNCH;
				}
			}
		}
	}
	if (data->peds.size > 0) {
		for (int cnt_peds = 0; cnt_peds < data->peds.size; cnt_peds++) {
			status = data->peds.p[cnt_peds].status;
			OSAL_LOGE("cnt_peds %d peds status:%d\n", cnt_peds, status);
			if (status) {
				index = LB_WARNING_ADAS_PEDS-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_ADAS_PEDS, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_ADAS_PEDS;
				}
			}
		}
	}

	for (int cnt_lanes = 0; cnt_lanes < 2; cnt_lanes++) {
		status = data->lanes.lanes[cnt_lanes].status;
		OSAL_LOGE("cnt_lanes %d lanes status:%d\n", cnt_lanes, status);
		if (status == WARN_STATUS_PRESS_LANE) {
			index = LB_WARNING_ADAS_PRESSURE-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_ADAS_PRESSURE, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_ADAS_PRESSURE;
			}
		}
		if (status == WARN_STATUS_CHANGING_LANE) {
			index = LB_WARNING_ADAS_CHANGLANE-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_ADAS_CHANGLANE, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_ADAS_CHANGLANE;
			}
		}
	}

	return 0;
}

static int nn_adas_result_cb(int event, void *data)
{
	if (event == NN_ADAS_DET_DONE) {
		nn_adas_out_t *curr_data = (nn_adas_out_t *)data;
    	int i;
		pthread_mutex_lock(&axframe_info.p_det_mutex);
	    /* cars data */
	    if (axframe_info.p_det_info->cars.p) {
	        free(axframe_info.p_det_info->cars.p);
	        axframe_info.p_det_info->cars.p = NULL;
	        axframe_info.p_det_info->cars.size = 0;
	    }
	    axframe_info.p_det_info->cars.p =
	        malloc(curr_data->cars.size * sizeof(nn_adas_car_t));
	    if (axframe_info.p_det_info->cars.p) {
	        memcpy(axframe_info.p_det_info->cars.p, curr_data->cars.p,
	            curr_data->cars.size * sizeof(nn_adas_car_t));
	        axframe_info.p_det_info->cars.size = curr_data->cars.size;
	    }

	    /* peds data */
	    if (axframe_info.p_det_info->peds.p) {
	        free(axframe_info.p_det_info->peds.p);
	        axframe_info.p_det_info->peds.p = NULL;
	        axframe_info.p_det_info->peds.size = 0;
	    }
	    axframe_info.p_det_info->peds.p = malloc(curr_data->peds.size * sizeof(nn_adas_ped_t));
	    if (axframe_info.p_det_info->peds.p) {
	        memcpy(axframe_info.p_det_info->peds.p, curr_data->peds.p,
	            curr_data->peds.size * sizeof(nn_adas_ped_t));
	        axframe_info.p_det_info->peds.size = curr_data->peds.size;
	    }
	    /* lanes data */
	    memcpy(&axframe_info.p_det_info->lanes, &curr_data->lanes, sizeof(nn_adas_lanes_t));
		pthread_mutex_unlock(&axframe_info.p_det_mutex);
		if (axframe_info.action != ADAS_CALIB) {
		    if (curr_data->cars.size) {
		        OSAL_LOGE("nn_adas_result_cb curr_data->cars.size = %d\n", curr_data->cars.size);
		        for (i=0; i<curr_data->cars.size; i++) {
		            OSAL_LOGE("curr_data->cars.p[%d].status = %d\n", i, curr_data->cars.p[i].status);
		        }
		    }
		    if (curr_data->peds.size) {
		        OSAL_LOGE("nn_adas_result_cb curr_data->peds.size = %d\n", curr_data->peds.size);
		        for (i=0; i<curr_data->peds.size; i++) {
		            OSAL_LOGE("curr_data->peds.p[%d].status = %d\n", i, curr_data->peds.p[i].status);
		        }
		    }
		    if (curr_data->lanes.lanes[0].exist)
		        OSAL_LOGE("nn_adas_result_cb curr_data->lanes.lanes[0].exist = %d\n", curr_data->lanes.lanes[0].exist);
		    if (curr_data->lanes.lanes[1].exist)
		        OSAL_LOGE("nn_adas_result_cb curr_data->lanes.lanes[1].exist = %d\n", curr_data->lanes.lanes[1].exist);
		    if (curr_data->lanes.lanes[0].status)
		        OSAL_LOGE("nn_adas_result_cb curr_data->lanes.lanes[0].exist = %d\n", curr_data->lanes.lanes[0].status);
		    if (curr_data->lanes.lanes[1].status)
		        OSAL_LOGE("nn_adas_result_cb curr_data->lanes.lanes[1].exist = %d\n", curr_data->lanes.lanes[1].status);
		}
		adas_warning(curr_data);
		sem_post(&axframe_info.frame_del_sem);
    } else if (event == NN_ADAS_SELF_CALIBRATE) {
		OSAL_LOGE("NN_ADAS_SELF_CALIBRATE\n");
		nn_adas_cfg_t *curr_data = (nn_adas_cfg_t *)data;
		OSAL_LOGE("curr_data->camera_param.vanish_point: %d %d %d %f\n",
			curr_data->camera_param.vanish_point[0], curr_data->camera_param.vanish_point[1],
			curr_data->camera_param.bottom_line, curr_data->camera_param.camera_to_middle);
		adas_save_cali_parm(curr_data->camera_param.vanish_point[0], curr_data->camera_param.vanish_point[1], curr_data->camera_param.bottom_line);
		if (!m_adas_cali.auto_calib_flag) { /*only warning for first time */
			osal_mq_send(warning_info.mq_id, LB_WARNING_ADAS_CALIBRATE_SUCCESS, NULL, 0);
		}
		m_adas_cali.auto_calib_flag = 1;
		/* save calibrate param used for next power on */
		al_para_save();
		al_para_exit();
	}
    
    return 0;


}


int nn_adas_start(VIDEO_FRAME_INFO_S *frame, void *nn_hdl)
{
    EI_S32 s32Ret;
    static nn_adas_in_t m_adas_in;
	int pool_idx = 0;
	for (pool_idx = 0; pool_idx < VBUF_POOL_MAX_NUM; pool_idx++) {
		if (frame->u32PoolId == pool_info[pool_idx].Pool) {
			break;
		}
	}
	if (pool_idx >= VBUF_POOL_MAX_NUM)
		return 0;
	/* 获取保存的映射虚拟地址 */
    m_adas_in.img.y_phy = (void *)frame->stVFrame.u64PlanePhyAddr[0];
    m_adas_in.img.y = (void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[0];
    m_adas_in.img.uv_phy = (void *)frame->stVFrame.u64PlanePhyAddr[1];
    m_adas_in.img.uv = (void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[1];
    m_adas_in.img.width = 1280;
    m_adas_in.img.height = 720;
    m_adas_in.img.format = YUV420_SP;
    m_adas_in.gps = 60;
    m_adas_in.carped_en = 1;
    m_adas_in.lane_en = 1;
	m_adas_in.cb_param = NULL;

    if (nn_hdl != NULL) {
        s32Ret = nn_adas_cmd(nn_hdl, NN_ADAS_SET_DATA, &m_adas_in);
		if (s32Ret < 0) {
			OSAL_LOGE("NN_ADAS_SET_DATA failed %d!\n", s32Ret);
		}
    }

#ifdef SAVE_AX_YUV_SP
    EI_S32 i;
#if 0
    OSAL_LOGE("save yuv buffer sizey %d, stride %d %d %d\n", frame->stVFrame.u32PlaneSizeValid[0], frame->stVFrame.u32PlaneStride[0], frame->stCommFrameInfo.u32Width, frame->stCommFrameInfo.u32Height);
    OSAL_LOGE("save yuv buffer sizeuv %d, u32PlaneNum %d\n", frame->stVFrame.u32PlaneSizeValid[1], frame->stVFrame.u32PlaneNum);

    OSAL_LOGE("save yuv buffer addr %llx %llx %llx\n",frame->stVFrame.u64PlanePhyAddr[0],
            frame->stVFrame.u64PlanePhyAddr[1], frame->stVFrame.u64PlanePhyAddr[2]);

    OSAL_LOGE("save yuv buffer virtual addr %x %x %x\n",frame->stVFrame.ulPlaneVirAddr[0],
            frame->stVFrame.ulPlaneVirAddr[1], frame->stVFrame.ulPlaneVirAddr[2]);

    OSAL_LOGE("save yuv buffer ID %x\n",frame->u32BufferId);
#endif
    for (i = 0; i < frame->stVFrame.u32PlaneNum; i++)
    {
        if (axframe_info.ax_fout) {
			if (m_adas_in.img.format == YUV420_SP) {
                if (i == 0) {
                    fwrite((void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[i], 1,  m_adas_in.img.width*m_adas_in.img.height, axframe_info.ax_fout);
                } else {
                    fwrite((void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[i], 1,  m_adas_in.img.width*m_adas_in.img.height/2, axframe_info.ax_fout);
                }
            }
        }
    }
#endif
    sem_wait(&axframe_info.frame_del_sem);

    return 0;
}

EI_VOID *get_axframe_proc(EI_VOID *p)
{
    int ret;
    VIDEO_FRAME_INFO_S axFrame = {0};
    axframe_info_s *axframe_info;
    void *nn_hdl = NULL;
	char adaslib_ver[64]={0};
#if 0
	nn_adas_cfg_t nn_adas_config_test = {0};
#endif
    axframe_info = (axframe_info_s *)p;
    OSAL_LOGE("get_axframe_proc bThreadStart = %d\n", axframe_info->bThreadStart);
#ifdef SAVE_AX_YUV_SP
    char srcfilename[FULLNAME_LEN_MAX];
    sprintf(srcfilename, "%s/adas_raw_ch%d.yuv", PATH_ROOT, axframe_info->chn);
    axframe_info->ax_fout = fopen(srcfilename, "wb+");
    if (axframe_info->ax_fout == NULL) {
        OSAL_LOGE("file open error1\n");
    }
#endif

#ifdef AL_PRO_FRAMERATE_STATISTIC
	int64_t time1 = 0LL, time2 = 0LL, time_GetChnFrame1 = 0LL, time_GetChnFrame2 = 0LL,
	time_AlProFrame1 = 0LL, time_AlProFrame2 = 0LL;
	int64_t frame_cnt = 0;
#endif
    OSAL_LOGE("********start_nn_adas******\n");
    memset(&axframe_info->m_nn_adas_cfg, 0, sizeof(axframe_info->m_nn_adas_cfg));
    /*init nn_adas, cfg*/
    axframe_info->m_nn_adas_cfg.img_width = axframe_info->u32Width; //算法输入图像宽
    axframe_info->m_nn_adas_cfg.img_height = axframe_info->u32Height; //算法输入图像高
    axframe_info->m_nn_adas_cfg.interest_box[0] = 0; //算法检测区域左上角X坐标
    axframe_info->m_nn_adas_cfg.interest_box[1] = 0; //算法检测区域左上角y坐标
    axframe_info->m_nn_adas_cfg.interest_box[2] = axframe_info->u32Width-1; //算法检测区域右下角x坐标
    axframe_info->m_nn_adas_cfg.interest_box[3] = axframe_info->u32Height-1; //算法检测区域右下角y坐标
    axframe_info->m_nn_adas_cfg.format = YUV420_SP;
    axframe_info->m_nn_adas_cfg.model_basedir = NN_ADAS_PATH;
    axframe_info->m_nn_adas_cfg.cb_func = nn_adas_result_cb;
    axframe_info->m_nn_adas_cfg.camera_param.vanish_point[0] = adas_get_vanish_point_x(); //天际线和车中线交叉点X坐标 单位像素
    axframe_info->m_nn_adas_cfg.camera_param.vanish_point[1] = adas_get_vanish_point_y(); //天际线和车中线交叉点Y坐标 单位像素
    axframe_info->m_nn_adas_cfg.camera_param.bottom_line = adas_get_bottom_line_y(); //引擎盖线
    axframe_info->m_nn_adas_cfg.camera_param.camera_angle[0] = 80; //摄像头水平角度
    axframe_info->m_nn_adas_cfg.camera_param.camera_angle[1] = 50; //摄像头垂直角度
    axframe_info->m_nn_adas_cfg.camera_param.camera_to_ground = 1.5; //与地面距离，单位米
    axframe_info->m_nn_adas_cfg.camera_param.car_head_to_camera= 1.5; //与车头距离 单位米
    axframe_info->m_nn_adas_cfg.camera_param.car_width= 1.8; //车宽，单位米
    axframe_info->m_nn_adas_cfg.camera_param.camera_to_middle= 0; //与车中线距离，单位米，建议设备安装在车的中间，左负右正
	axframe_info->m_nn_adas_cfg.calib_param.auto_calib_en = 1;//自动校准使能
	axframe_info->m_nn_adas_cfg.calib_param.calib_finish = 1;//设置为1，在自动校准前也会按默认参数报警, 设置为0，则在自动校准前不报警
 
	axframe_info->m_nn_adas_cfg.ldw_en = 1;//车道偏移和变道报警
	axframe_info->m_nn_adas_cfg.hmw_en = 1;//保持车距报警
	axframe_info->m_nn_adas_cfg.ufcw_en = 0;//低速下前车碰撞报警，reserved备用
	axframe_info->m_nn_adas_cfg.fcw_en = 1;//前车碰撞报警
	axframe_info->m_nn_adas_cfg.pcw_en = 1;//行人碰撞报警
	axframe_info->m_nn_adas_cfg.fmw_en = 1;//前车启动报警，必须有GPS，且本车车速为0
	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.min_warn_dist = 0;//最小报警距离，单位米
	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.max_warn_dist = 100;//最大报警距离，单位米
	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.min_warn_gps = 30;//最低报警车速，单位KM/H
	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.max_warn_gps = 200;//最高报警车速，单位KM/H
	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.gps_ttc_thresh[0] = 1.2f;//与前车碰撞小于这个时间报警(假设前车不动)，单位秒,距离/本车GPS速度，对应WARN_STATUS_HMW_LEVEL1报警状态
	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.gps_ttc_thresh[1] = 0.6f;//与前车碰撞小于这个时间报警(假设前车不动)，单位秒,距离/本车GPS速度，对应WARN_STATUS_HMW_LEVEL2报警状态

	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.min_warn_dist = 0;
	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.max_warn_dist = 100;
	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.min_warn_gps = 30;
	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.max_warn_gps = 200;
	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.gps_ttc_thresh = 2.0f; //与前车碰撞报警时间(假设前车不动)，单位秒,距离/本车GPS速度
	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.ttc_thresh = 2.5f; //与前车碰撞报警时间，单位秒,距离/相对车速，以上4个条件同时满足

	axframe_info->m_nn_adas_cfg.warn_param.ufcw_param.min_warn_dist = 0;
	axframe_info->m_nn_adas_cfg.warn_param.ufcw_param.max_warn_dist = 100;
	axframe_info->m_nn_adas_cfg.warn_param.ufcw_param.min_warn_gps = 0;
	axframe_info->m_nn_adas_cfg.warn_param.ufcw_param.max_warn_gps = 30;
	axframe_info->m_nn_adas_cfg.warn_param.ufcw_param.relative_speed_thresh = -3.0f; //与前车碰撞相对车速，前车车速-本车车速小于这个速度

	axframe_info->m_nn_adas_cfg.warn_param.fmw_param.min_static_time = 5.0f;//前车静止的时间需大于多少，单位秒
	axframe_info->m_nn_adas_cfg.warn_param.fmw_param.warn_car_inside_dist = 6.0f;//与前车的距离在多少米内，单位米
	axframe_info->m_nn_adas_cfg.warn_param.fmw_param.warn_dist_leave = 0.6f;//前车启动大于多远后报警，单位米

	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.min_warn_dist = 0;
	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.max_warn_dist = 100;
	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.min_warn_gps = 30;
	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.max_warn_gps = 200;
	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.gps_ttc_thresh = 1.5f;
	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.ttc_thresh = 5.0f;

	axframe_info->m_nn_adas_cfg.warn_param.ldw_param.min_warn_gps = 30.0f;//最低报警车速，单位KM/H
	axframe_info->m_nn_adas_cfg.warn_param.ldw_param.ldw_warn_thresh = 0.5f; //在lane_warn_dist距离内持续时间，单位秒
	axframe_info->m_nn_adas_cfg.warn_param.ldw_param.lane_warn_dist = 0.0f;//与车道线距离，车道线内
	axframe_info->m_nn_adas_cfg.warn_param.ldw_param.change_lane_dist = 0.6f; //变道侧车道线离车的中点距离，单位米

    /* open libadas.so*/
    nn_hdl = nn_adas_open(&axframe_info->m_nn_adas_cfg);
    if (nn_hdl == NULL) {
        OSAL_LOGE("nn_adas_open() failed!");
        return NULL;
    }
	axframe_info->nn_hdl = nn_hdl;
#ifdef AL_PRO_FRAMERATE_STATISTIC
	time1 = osal_get_msec();
#endif
	for (int i= 0; i < LB_WARNING_END-LB_WARNING_BEGIN-1; i++) {
		warning_info.last_warn_time[i] = osal_get_msec();
	}
    while (EI_TRUE == axframe_info->bThreadStart) {
        memset(&axFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time_GetChnFrame1 = osal_get_msec();
#endif
        ret = EI_MI_VISS_GetChnFrame(axframe_info->dev,
            axframe_info->chn, &axFrame, 1000);
        if (ret != EI_SUCCESS) {
            OSAL_LOGE("chn %d get frame error 0x%x\n", axframe_info->chn, ret);
            usleep(100 * 1000);
            continue;
        }
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time_GetChnFrame2 = osal_get_msec();
#endif
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time_AlProFrame1 = osal_get_msec();
#endif
        nn_adas_start(&axFrame, nn_hdl);
#ifdef AL_PRO_FRAMERATE_STATISTIC
	time_AlProFrame2 = osal_get_msec();
	OSAL_LOGE("time_GetChnFrame %lld ms, time_AlProFrame %lld ms\n",
		time_GetChnFrame2-time_GetChnFrame1, time_AlProFrame2-time_AlProFrame1);
#endif
#ifdef AL_PRO_FRAMERATE_STATISTIC
		frame_cnt++;
		time2 = osal_get_msec();
		if ((time2 - time1) >= 3000) {
			OSAL_LOGE("algo process %lld fps\n", (frame_cnt * 1000) / (time2 - time1));
			time1 = time2;
			frame_cnt  = 0;
		}
#endif
		nn_adas_cmd(nn_hdl, NN_ADAS_GET_VERSION, adaslib_ver);
		OSAL_LOGE("adaslib_ver %s\n", adaslib_ver);
	#if 0
		if (axframe_info->m_nn_adas_cfg.warn_param.fcw_param.max_warn_gps != 100) {
			axframe_info->m_nn_adas_cfg.warn_param.fcw_param.max_warn_gps = 100;
			nn_adas_cmd(nn_hdl, NN_ADAS_SET_PARAM, &axframe_info->m_nn_adas_cfg);
			memset(&nn_adas_config_test,0,sizeof(nn_adas_cfg_t));
			nn_adas_cmd(nn_hdl, NN_ADAS_GET_PARAM, &nn_adas_config_test);
			OSAL_LOGE("get max_warn_gps %d %f\n", nn_adas_config_test.warn_param.fcw_param.max_warn_gps, nn_adas_config_test.warn_param.fcw_param.gps_ttc_thresh);
		}
	#endif
        EI_MI_VISS_ReleaseChnFrame(axframe_info->dev, axframe_info->chn, &axFrame);
    }
    EI_MI_VISS_DisableChn(axframe_info->dev, axframe_info->chn);
	OSAL_LOGE("axframe_info->dev %d %d\n", axframe_info->dev, axframe_info->chn);
	OSAL_LOGE("nn_adas_close 0\n");
    nn_adas_close(nn_hdl);
	OSAL_LOGE("nn_adas_close 1\n");
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
    OSAL_LOGE("get_frame_proc bThreadStart = %d\n", drawframe_info->bThreadStart);

    sprintf(srcfilename, "%s/adas_draw_raw_ch%d.yuv", PATH_ROOT, drawframe_info->chn);
    OSAL_LOGE("********get_drawframe_proc******\n");
#ifdef SAVE_DRAW_YUV_SP
    drawframe_info->draw_fout = fopen(srcfilename, "wb+");
    if (drawframe_info->draw_fout == NULL) {
        OSAL_LOGE("file open error1\n");
    }
#endif
    while (EI_TRUE == drawframe_info->bThreadStart) {
        memset(&drawFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
        ret = EI_MI_VISS_GetChnFrame(drawframe_info->dev,
            drawframe_info->chn, &drawFrame, 1000);
        if (ret != EI_SUCCESS) {
            OSAL_LOGE("chn %d get frame error 0x%x\n", drawframe_info->chn, ret);
            usleep(100 * 1000);
            continue;
        }
		pthread_mutex_lock(&axframe_info.p_det_mutex);
		adas_draw_location_rgn(&drawFrame, axframe_info.p_det_info);
		pthread_mutex_unlock(&axframe_info.p_det_mutex);
		if (drawframe_info->is_disp == EI_TRUE) {
			drawFrame.enFrameType = MDP_FRAME_TYPE_DOSS;
			EI_MI_VO_SendFrame(0, 0, &drawFrame, 100);
		}
#ifdef SAVE_DRAW_AX_H265
		drawFrame.enFrameType = MDP_FRAME_TYPE_VENC;
		EI_MI_VENC_SendFrame(stream_para[0].VeChn, &drawFrame, 100);
#endif
        EI_MI_VISS_ReleaseChnFrame(drawframe_info->dev,
            drawframe_info->chn, &drawFrame);
    }
    EI_MI_VISS_DisableChn(drawframe_info->dev, drawframe_info->chn);
	OSAL_LOGE("drawframe_info->dev %d %d\n", drawframe_info->dev, drawframe_info->chn);
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
		OSAL_LOGE("path %s too long\n", full_path);
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
 
    OSAL_LOGE("get_frame_proc bThreadStart = %d\n", warning_info->bThreadStart);
	prctl(PR_SET_NAME, "warning_proc thread");

	while (warning_info->bThreadStart) {
		err = osal_mq_recv(warning_info->mq_id, &msg, MQ_MSG_LEN, 50);
		if (err) {
			continue;
		}
		OSAL_LOGE("msg.type = %d\n", msg.type);
		switch (msg.type) {
			case LB_WARNING_ADAS_LAUNCH:
				warn_tone_play("/usr/share/out/res/adas_warning/alarm_launch.wav");
				break;
			case LB_WARNING_ADAS_DISTANCE:
				warn_tone_play("/usr/share/out/res/adas_warning/alarm_distance.wav");
				break;
			case LB_WARNING_ADAS_COLLIDE:
				warn_tone_play("/usr/share/out/res/adas_warning/alarm_collide.wav");
				break;
			case LB_WARNING_ADAS_PRESSURE:
				warn_tone_play("/usr/share/out/res/adas_warning/alarm_pressure.wav");
				break;
			case LB_WARNING_ADAS_PEDS:
				warn_tone_play("/usr/share/out/res/adas_warning/xiaoxinxingren.wav");
				break;
			case LB_WARNING_ADAS_CHANGLANE:
				warn_tone_play("/usr/share/out/res/adas_warning/cheliangbiandao.wav");
				break;
			case LB_WARNING_ADAS_CALIBRATE_START:
				warn_tone_play("/usr/share/out/res/adas_warning/jiaozhunkaishi.wav");
				break;
			case LB_WARNING_ADAS_CALIBRATE_SUCCESS:
				warn_tone_play("/usr/share/out/res/adas_warning/jiaozhunchenggong.wav");
				break;
			case LB_WARNING_ADAS_CALIBRATE_FAIL:
				warn_tone_play("/usr/share/out/res/adas_warning/jiaozhunshibai.wav");
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
        OSAL_LOGE("EI_MI_VISS_SetExtChnAttr failed with %#x!\n", ret);
        return EI_FAILURE;
    }
    ret = EI_MI_VISS_EnableChn(axframe_info->dev, axframe_info->chn);
    if (ret != EI_SUCCESS) {
        OSAL_LOGE("EI_MI_VISS_EnableChn failed with %#x!\n", ret);
        return EI_FAILURE;
    }
    axframe_info->bThreadStart = EI_TRUE;
    ret = pthread_create(&(axframe_info->gs_framePid), NULL, get_axframe_proc,
        (EI_VOID *)axframe_info);
    if (ret)
        OSAL_LOGE("errno=%d, reason(%s)\n", errno, strerror(errno));
    OSAL_LOGE("start_get_axframe success %#x! phyChn:%d chn :%d dev:%d\n", ret, axframe_info->phyChn, axframe_info->chn, axframe_info->dev);

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
        OSAL_LOGE("EI_MI_VISS_SetExtChnAttr failed with %#x!\n", ret);
        return EI_FAILURE;
    }
    ret = EI_MI_VISS_EnableChn(drawframe_info->dev, drawframe_info->chn);
    if (ret != EI_SUCCESS) {
        OSAL_LOGE("EI_MI_VISS_EnableChn failed with %#x!\n", ret);
        return EI_FAILURE;
    }

    drawframe_info->bThreadStart = EI_TRUE;
    ret = pthread_create(&(drawframe_info->gs_framePid), NULL, get_drawframe_proc,
        (EI_VOID *)drawframe_info);
    if (ret)
        OSAL_LOGE("errno=%d, reason(%s)\n", errno, strerror(errno));
    OSAL_LOGE("start_get_drawframe success %#x! phyChn:%d chn :%d dev:%d\n", ret, drawframe_info->phyChn, drawframe_info->chn, drawframe_info->dev);

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
        OSAL_LOGE("open error mount proc");
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
		OSAL_LOGE("%s already exist\n", path);
        return -1;
    }
    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        OSAL_LOGE("open %s fail\n", path);
        return -2;
    }
    ret = fallocate(fd, FALLOC_FL_KEEP_SIZE, 0, fsize);
    close(fd);

    if (ret)
        OSAL_LOGE("fallocate fail(%i)\n", ret);
	else
        OSAL_LOGI("fallocate %s finish\n", path);

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
		OSAL_LOGE("Open dir error... %s\n", path);
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
	OSAL_LOGE("venc_para->aszFileName:%s\n", sdr_venc_param->aszFileName);
	if (info.free < 500 * 1024) {
    	ret = _get_first_file(PATH_ROOT, first_fullfilename, szFilePostfix);
		OSAL_LOGE("first_fullfilename:%s\n", first_fullfilename);
		if (ret < 0) {
			OSAL_LOGE("maybe first_fullfilename:%s is not exist\n", first_fullfilename);
			unlink(first_fullfilename);
		}
		/* rename for reduce fragment */
		ret = rename(first_fullfilename, sdr_venc_param->aszFileName);
		if (ret < 0) {
			OSAL_LOGE("maybe first_fullfilename:%s is not exist\n", first_fullfilename);
			unlink(first_fullfilename);
		}	
	} else {
		size = sdr_venc_param->bitrate * SDR_SAMPLE_VENC_TIME / 8 + 16 * 1024 *1024;
    	ret = do_fallocate(sdr_venc_param->aszFileName, size);
	}

	return ret;
}

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
        OSAL_LOGE("err param !\n");
        return NULL;
    }

    VencChn = sdr_venc_para->VeChn;

    thread_name[15] = 0;
    snprintf(thread_name, 16, "streamproc%d", VencChn);
    prctl(PR_SET_NAME, thread_name);
	ret = get_stream_file_name(sdr_venc_para, ".h265", sizeof(".h265"));
	while (ret < 0) {
		OSAL_LOGE("get_stream_file_name : %d\n", ret);
		usleep(1000*1000);
		ret = get_stream_file_name(sdr_venc_para, ".h265", sizeof(".h265"));
	}

	sdr_venc_para->flip_out = fopen(sdr_venc_para->aszFileName, "wb");
	if (!sdr_venc_para->flip_out) {
		OSAL_LOGE("creat file failed\n");
	}
    while (EI_TRUE == sdr_venc_para->bThreadStart) {
    	 ret = EI_MI_VENC_QueryStatus(VencChn, &stStatus);
         if (ret != EI_SUCCESS) {
              OSAL_LOGE( "query status chn-%d error : %d\n", VencChn, ret);
              break;
         }
        ret = EI_MI_VENC_GetStream(VencChn, &stStream, 1000);
        if (ret == EI_ERR_VENC_NOBUF) {
            OSAL_LOGI("No buffer\n");
            usleep(5 * 1000);
            continue;
        } else if (ret != EI_SUCCESS) {
            OSAL_LOGE("get chn-%d error : %d\n", VencChn, ret);
            break;
        }
		now = time(NULL);
		p_tm = localtime(&now);
		watermark_update(p_tm, WATERMARK_X, WATERMARK_Y, MOD_VENC);
		if (drawframe_info.is_disp == EI_TRUE) {
			watermark_update(p_tm, WATERMARK_X, WATERMARK_Y, MOD_PREVIEW);
		}
        if (stStream.pstPack.u32Len[0] == 0) {
            OSAL_LOGE("%d ch %d pstPack.u32Len:%d-%d, ofset:%d, ret:%x\n",
                __LINE__, VencChn, stStream.pstPack.u32Len[0],
                stStream.pstPack.u32Len[1],
                sdr_venc_para->buf_offset, ret);
        }
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
			OSAL_LOGE( "release stream chn-%d error : %d\n", VencChn, ret);
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
        OSAL_LOGE("errno=%d, reason(%s)\n", errno, strerror(errno));
    return ret;
}
int _stop_get_stream(sdr_sample_venc_para_t *venc_para)
{
    if (venc_para == NULL ||
        venc_para->VeChn < 0 || venc_para->VeChn >= VC_MAX_CHN_NUM) {
        OSAL_LOGE("%s %d  venc_para:%p:%d\n", __func__, __LINE__,
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

    OSAL_LOGW("%s %d, signal %d\n", __func__, __LINE__, signo);
	OSAL_LOGW("stop_get_frame end start!\n");
}
static int _viss_chn_detect(SAMPLE_VISS_CONFIG_S *viss_config, camera_info_s *camera_info)
{
    SAMPLE_VISS_INFO_S *viss_info = NULL;
    EI_S32 i = 0, j = 0;

    memset(viss_config, 0, sizeof(SAMPLE_VISS_CONFIG_S));
    viss_info = viss_config->astVissInfo;

    viss_config->s32WorkingVissNum = 1;
    viss_info = &viss_config->astVissInfo[0];
    viss_info->stDevInfo.VissDev = camera_info[0].dev; /*0: DVP, 1: MIPI0, 2: MIPI1*/
    viss_info->stDevInfo.aBindPhyChn[0] = 0;
    viss_info->stDevInfo.aBindPhyChn[1] = 1;
    viss_info->stDevInfo.aBindPhyChn[2] = 2;
    viss_info->stDevInfo.aBindPhyChn[3] = 3;
    viss_info->stDevInfo.enOutPath = VISS_OUT_PATH_DMA;
    viss_info->stChnInfo.aVissChn[0] = 0;
    viss_info->stChnInfo.aVissChn[1] = 1;
    viss_info->stChnInfo.aVissChn[2] = 2;
    viss_info->stChnInfo.aVissChn[3] = 3;
    viss_info->stChnInfo.enWorkMode = 0;
    viss_info->stChnInfo.stChnAttr.u32Align = 32;
    viss_info->stSnsInfo.enSnsType = camera_info[0].sns;

    viss_info->stSnsInfo.enSnsInitMode = COMMON_AUTOCFG;
    SAMPLE_COMM_VISS_StartDev(&viss_config->astVissInfo[i]);
    EI_MI_VISS_GetSensorChFormat(viss_config->astVissInfo[i].stDevInfo.VissDev, camera_info[0].stFormat);
    for (j = 0; j < camera_info[0].chn_num; j++) {
        OSAL_LOGW("viss dev:%d ch%d res:%dx%d %d\n", viss_config->astVissInfo[i].stDevInfo.VissDev, j,
            camera_info[0].stFormat[j].stSize.u32Width, camera_info[0].stFormat[j].stSize.u32Height, camera_info[0].stFormat[j].u32FrameRate);
		camera_info[0].stSize[j].u32Width = camera_info[0].stFormat[j].stSize.u32Width;
		camera_info[0].stSize[j].u32Height = camera_info[0].stFormat[j].stSize.u32Height;
		camera_info[0].u32FrameRate[j] = camera_info[0].stFormat[j].u32FrameRate;
    }
    SAMPLE_COMM_VISS_StopDev(&viss_config->astVissInfo[i]);

    return 0;
}

static int show_1ch(VISS_DEV	dev, VISS_CHN chn, ROTATION_E rot, EI_BOOL mirror, EI_BOOL flip,
    unsigned int rect_x, unsigned int rect_y, unsigned int width, unsigned int hight,
    SNS_TYPE_E sns)
{
	static VBUF_CONFIG_S stVbConfig;
	static SAMPLE_VISS_CONFIG_S stVissConfig;
	VISS_CHN_ATTR_S stVissChnAttr = {0};
	VISS_DEV_ATTR_S stVissDevAttr = {0};
#ifndef SAVE_DRAW_AX_H265
	static VISS_EXT_CHN_ATTR_S stExtChnAttr;
#endif
    EI_S32 s32Ret = EI_FAILURE;
    VO_VIDEO_LAYER_ATTR_S stVoLayerAttr = {0};
    VO_CHN_ATTR_S stVoChnAttr = {0};
	SAMPLE_VENC_CONFIG_S venc_config = {0};
	int loop_time;
	int key = 0;
	VISS_PIC_TYPE_E stPicType = {0};
	VBUF_POOL_CONFIG_S stPoolCfg = {0};
	camera_info_s camera_info[VISS_MAX_DEV_NUM] = {{0}};
	camera_info[0].dev = dev;
	camera_info[0].sns = sns;
	camera_info[0].chn_num = 4;
	camera_info[0].chn = chn;
	camera_info[0].viss_out_path = VISS_OUT_PATH_DMA;

	SAMPLE_COMM_VISS_GetPicTypeBySensor(sns, &stPicType);
	OSAL_LOGW("stPicType.stSize.u32Width = %d \n", stPicType.stSize.u32Width);
	OSAL_LOGW("stPicType.stSize.u32Height = %d \n", stPicType.stSize.u32Height);
	OSAL_LOGW("stPicType.u32FrameRate = %d \n", stPicType.u32FrameRate);
	if (stPicType.stSize.u32Width && stPicType.stSize.u32Height) {
		if (!stPicType.u32FrameRate)
			stPicType.u32FrameRate = 25; //default
		for (EI_S32 i = 0; i < camera_info[0].chn_num; i++) {
			camera_info[0].stSize[i].u32Width = stPicType.stSize.u32Width;
			camera_info[0].stSize[i].u32Height = stPicType.stSize.u32Height;
			camera_info[0].u32FrameRate[i] = stPicType.u32FrameRate;
		}
	} else { /* mashup type */
		_viss_chn_detect(&stVissConfig, camera_info);
	}
	
	stVbConfig.astFrameInfo[0].enFrameType = MDP_FRAME_TYPE_COMMON;
    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Align = 32;
    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Width = camera_info[0].stSize[chn].u32Width;
    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Height = camera_info[0].stSize[chn].u32Height;
    stVbConfig.astFrameInfo[0].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stVbConfig.astCommPool[0].u32BufCnt = 16;
    stVbConfig.astCommPool[0].enRemapMode = VBUF_REMAP_MODE_NOCACHE;

	stVbConfig.astFrameInfo[1].enFrameType = MDP_FRAME_TYPE_COMMON;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Align = 32;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width = 1280;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height = 720;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stVbConfig.astCommPool[1].u32BufCnt = 32;
    stVbConfig.astCommPool[1].enRemapMode = VBUF_REMAP_MODE_CACHED;

	stVbConfig.u32PoolCnt = 2;

	EI_MI_MBASE_Init();
    EI_MI_VBUF_Init();
    EI_MI_MLINK_Init();
	if (stVbConfig.u32PoolCnt > VBUF_POOL_MAX_NUM) {
		OSAL_LOGE("VBUF MAX ERROE %s %d %d\n", __FILE__, __LINE__, stVbConfig.u32PoolCnt);
		return -1;
	}
	for (int u32Idx = 0; u32Idx < stVbConfig.u32PoolCnt; ++u32Idx) {
	    stPoolCfg.u32BufSize = EI_MI_VBUF_GetPicBufferSize(&stVbConfig.astFrameInfo[u32Idx]);
	    stPoolCfg.u32BufCnt = stVbConfig.astCommPool[u32Idx].u32BufCnt;
	    stPoolCfg.enRemapMode = stVbConfig.astCommPool[u32Idx].enRemapMode;
	    stPoolCfg.enVbufUid = VBUF_UID_COMMON;
	    strcpy(stPoolCfg.acPoolName, "common");
    	pool_info[u32Idx].Pool = EI_MI_VBUF_CreatePool(&stPoolCfg);
    	s32Ret = EI_MI_VBUF_SetFrameInfo(pool_info[u32Idx].Pool, &stVbConfig.astFrameInfo[u32Idx]);
		if (s32Ret) {
		        OSAL_LOGE("%s %d\n", __FILE__, __LINE__);
		}
		pool_info[u32Idx].u32BufNum = stVbConfig.astCommPool[u32Idx].u32BufCnt;
		pool_info[u32Idx].enRemapMode = stVbConfig.astCommPool[u32Idx].enRemapMode;
		pool_info[u32Idx].pstBufMap = malloc(sizeof(VBUF_MAP_S) * pool_info[u32Idx].u32BufNum);
		/* 这里在初始化的时候保存映射的虚拟地址，是避免在算法使用的时候映射会占用 cpu 资源 */
		for (int i = 0; i < stVbConfig.astCommPool[u32Idx].u32BufCnt; i++) {
            pool_info[u32Idx].Buffer[i] = EI_MI_VBUF_GetBuffer(pool_info[u32Idx].Pool, 10*1000);
            s32Ret = EI_MI_VBUF_GetFrameInfo(pool_info[u32Idx].Buffer[i], &stVbConfig.astFrameInfo[u32Idx]);
			if (s32Ret) {
		        OSAL_LOGE("%s %d\n", __FILE__, __LINE__);
			}
            s32Ret = EI_MI_VBUF_FrameMmap(&stVbConfig.astFrameInfo[u32Idx], stVbConfig.astCommPool[u32Idx].enRemapMode);
			if (s32Ret) {
		        OSAL_LOGE("%s %d\n", __FILE__, __LINE__);
			}
			pool_info[u32Idx].pstBufMap[stVbConfig.astFrameInfo[u32Idx].u32Idx].u32BufID = pool_info[u32Idx].Buffer[i];
			pool_info[u32Idx].pstBufMap[stVbConfig.astFrameInfo[u32Idx].u32Idx].stVfrmInfo = stVbConfig.astFrameInfo[u32Idx];
#if 0
			for (int j = 0; j < FRAME_MAX_PLANE; j++) {
        		pool_info[u32Idx].pstBufMap[stVbConfig.astFrameInfo[u32Idx].u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[j] = stVbConfig.astFrameInfo[u32Idx].stVFrame.ulPlaneVirAddr[j];
    		}

			OSAL_LOGE("u32Idx %d %s %d\n", stVbConfig.astFrameInfo[u32Idx].u32Idx, __FILE__, __LINE__);
		    OSAL_LOGE("Plane Size: 0x%08x, 0x%08x, PhyAddr: 0x%llx, 0x%llx, MmapVirAddr: 0x%08lx, 0x%08lx\n",
	            stVbConfig.astFrameInfo[u32Idx].stVFrame.u32PlaneSize[0],
	            stVbConfig.astFrameInfo[u32Idx].stVFrame.u32PlaneSize[1],
	            stVbConfig.astFrameInfo[u32Idx].stVFrame.u64PlanePhyAddr[0],
	            stVbConfig.astFrameInfo[u32Idx].stVFrame.u64PlanePhyAddr[1],
	            stVbConfig.astFrameInfo[u32Idx].stVFrame.ulPlaneVirAddr[0],
	            stVbConfig.astFrameInfo[u32Idx].stVFrame.ulPlaneVirAddr[1]);
#endif
        }
		
        for (int i = 0; i < stVbConfig.astCommPool[u32Idx].u32BufCnt; i++) {
            s32Ret = EI_MI_VBUF_PutBuffer(pool_info[u32Idx].Buffer[i]);
			if (s32Ret) {
		        OSAL_LOGE("%s %d\n", __FILE__, __LINE__);
				goto exit0;
			}
        }
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
	if (camera_info[0].sns == TP2815_MIPI_ADAPTIVE_MASHUP_4CH_YUV)
		stVissConfig.astVissInfo[0].stChnInfo.enWorkMode = 0;
	else
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
    if (s32Ret) {
        OSAL_LOGE(" comm viss start error\n");
    }
	if (camera_info[0].sns == TP2815_MIPI_ADAPTIVE_MASHUP_4CH_YUV) {
		SAMPLE_COMM_VISS_GetDevAttrBySns(camera_info[0].sns, &stVissDevAttr);
		SAMPLE_COMM_VISS_GetChnAttrBySns(camera_info[0].sns, &stVissChnAttr);
		stVissChnAttr.u32Align = 32;
		stVissChnAttr.u32Depth = 2;
		for (EI_S32 i = 0; i < camera_info[0].chn_num; i++) {
			if (camera_info[0].stSize[i].u32Width==0 || camera_info[0].stSize[i].u32Height==0 ||
				camera_info[0].u32FrameRate[i]==0) {
				OSAL_LOGW("stFormat[%d].stSize.u32Width : %d \n", i, camera_info[0].stFormat[i].stSize.u32Width);
				OSAL_LOGW("stFormat[%d].stSize.u32Height : %d \n", i, camera_info[0].stFormat[i].stSize.u32Height);
				OSAL_LOGW("frameRate : %d \n", camera_info[0].stFormat[i].u32FrameRate);
				camera_info[0].stSize[i].u32Width = camera_info[0].stFormat[i].stSize.u32Width;
				camera_info[0].stSize[i].u32Height = camera_info[0].stFormat[i].stSize.u32Height;
				camera_info[0].u32FrameRate[i] = camera_info[0].stFormat[i].u32FrameRate;
			}
			if (camera_info[0].stFormat[i].stSize.u32Width && camera_info[0].stFormat[i].stSize.u32Height && camera_info[0].stFormat[i].u32FrameRate) {
				stVissChnAttr.stSize = camera_info[0].stFormat[i].stSize;
		        stVissChnAttr.stFrameRate.s32SrcFrameRate = camera_info[0].stFormat[i].u32FrameRate;
		        stVissChnAttr.stFrameRate.s32DstFrameRate = camera_info[0].stFormat[i].u32FrameRate;
				if (i==camera_info[0].chn) {
			        s32Ret = EI_MI_VISS_SetChnAttr(camera_info[0].dev, i, &stVissChnAttr);
			        if (s32Ret != EI_SUCCESS) {
			            OSAL_LOGE("EI_MI_VISS_SetChnAttr failed with %#x!\n", s32Ret);
			            return EI_FAILURE;
			        }

			        s32Ret = EI_MI_VISS_EnableChn(camera_info[0].dev, i);
			        if (s32Ret != EI_SUCCESS) {
			            OSAL_LOGE("EI_MI_VISS_EnableChn failed with %#x!\n", s32Ret);
			            return EI_FAILURE;
			        }
				}
			}
		}
	}
	if (drawframe_info.is_disp == EI_TRUE) {
	    /* display begin */
	    EI_MI_DOSS_Init();

		drawframe_info.VoDev = 0;
		drawframe_info.VoLayer = 0;
		drawframe_info.VoChn = 0;
		drawframe_info.u32scnWidth = SCN_WIDTH;
		drawframe_info.u32scnHeight = SCN_HEIGHT;
		s32Ret = EI_MI_VO_Enable(drawframe_info.VoDev);
	    if (s32Ret != EI_SUCCESS) {
	        OSAL_LOGE("EI_MI_VO_Enable() failed with %#x!\n", s32Ret);
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
	        OSAL_LOGE("\n");
	        goto exit1;
	    }

	    s32Ret = EI_MI_VO_SetDisplayBufLen(drawframe_info.VoLayer, 3);
	    if (s32Ret != EI_SUCCESS) {
	        OSAL_LOGE("\n");
	        goto exit1;
	    }

	    s32Ret = EI_MI_VO_SetVideoLayerPartitionMode(drawframe_info.VoLayer, VO_PART_MODE_MULTI);
	    if (s32Ret != EI_SUCCESS) {
	        OSAL_LOGE("\n");
	        goto exit1;
	    }

	    s32Ret = EI_MI_VO_SetVideoLayerPriority(drawframe_info.VoLayer, 0);
	    if (s32Ret != EI_SUCCESS) {
			OSAL_LOGE("\n");
			goto exit1;
	    }

	    s32Ret = EI_MI_VO_EnableVideoLayer(drawframe_info.VoLayer);
	    if (s32Ret != EI_SUCCESS) {
	        OSAL_LOGE("\n");
	        goto exit1;
	    }

	    stVoChnAttr.stRect.s32X = 0;
	    stVoChnAttr.stRect.s32Y = 0;
	    stVoChnAttr.stRect.u32Width = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
	    stVoChnAttr.stRect.u32Height = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;

	    EI_MI_VO_SetChnAttr(drawframe_info.VoLayer, drawframe_info.VoChn, &stVoChnAttr);
	    EI_MI_VO_EnableChn(drawframe_info.VoLayer, drawframe_info.VoChn);
	    EI_MI_VO_SetChnRotation(drawframe_info.VoLayer, drawframe_info.VoChn, rot);
	}
	pthread_mutex_init(&axframe_info.p_det_mutex, NULL);
    axframe_info.p_det_info = malloc(sizeof(nn_adas_out_t));
    if (!axframe_info.p_det_info) {
		OSAL_LOGW("axframe_info.p_det_info malloc failed!\n");
		goto exit1;
    }
	if (axframe_info.action == ADAS_CALIB) {
		m_adas_cali.cali_flag = ADAS_CALI_START;
		m_adas_cali.vanish_point_x = adas_get_vanish_point_x();
		m_adas_cali.vanish_point_y = adas_get_vanish_point_y();
		m_adas_cali.bottom_line_y = adas_get_bottom_line_y();
		m_adas_cali.adas_calibline = VANISH_Y_LINE;
		m_adas_cali.adas_calibstep = 5;
		osal_mq_send(warning_info.mq_id, LB_WARNING_ADAS_CALIBRATE_START, NULL, 0);
		OSAL_LOGE("m_adas_cali.vanish_point_x %f %f %f\n", m_adas_cali.vanish_point_x, m_adas_cali.vanish_point_y, m_adas_cali.bottom_line_y);
	}
    memset(axframe_info.p_det_info, 0x00, sizeof(nn_adas_out_t));
    axframe_info.dev = dev;
    axframe_info.chn = VISS_MAX_PHY_CHN_NUM;
    axframe_info.phyChn = chn;
    axframe_info.frameRate = SDR_VISS_FRAMERATE;
    axframe_info.u32Width = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
    axframe_info.u32Height = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
	axframe_info.ax_fout = NULL;
    sem_init(&axframe_info.frame_del_sem, 0, 0);

    start_get_axframe(&axframe_info);
    drawframe_info.dev = dev;
    drawframe_info.chn = VISS_MAX_PHY_CHN_NUM+1;
    drawframe_info.phyChn = chn;
    drawframe_info.frameRate = SDR_VISS_FRAMERATE;
    drawframe_info.u32Width = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
    drawframe_info.u32Height = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
	drawframe_info.draw_fout = NULL;
	drawframe_info.last_draw_rect_num = RGN_RECT_NUM;
	drawframe_info.last_draw_line0_num = RGN_LINE_NUM;
	drawframe_info.last_draw_line1_num = RGN_LINE_NUM;
	s32Ret = rgn_start();
	if (s32Ret == EI_FAILURE) {
		 OSAL_LOGE("rgn_start failed with %#x!\n", s32Ret);
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
	if (drawframe_info.is_disp == EI_TRUE) {
		watermark_init(VENC_RGN_WM_NUM+RGN_ARRAY_NUM, VENC_RGN_WM_NUM, watermark_filename, MOD_PREVIEW);
	}
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
	while(axframe_info.action == ADAS_CALIB && stream_para[0].exit_flag != EI_TRUE){
		key = getchar();
		OSAL_LOGE("get key 0x%x!\n", key);
        switch (key)
        {
			case 0x2b:/* +  */
				if (m_adas_cali.adas_calibline == VANISH_X_LINE) {
					m_adas_cali.vanish_point_x+=m_adas_cali.adas_calibstep;
					if (m_adas_cali.vanish_point_x > drawframe_info.u32scnWidth)
						m_adas_cali.vanish_point_x = drawframe_info.u32scnWidth;
				} else if (m_adas_cali.adas_calibline == VANISH_Y_LINE) {
					m_adas_cali.vanish_point_y+=m_adas_cali.adas_calibstep;
					if (m_adas_cali.vanish_point_y > m_adas_cali.bottom_line_y)
						m_adas_cali.vanish_point_y = m_adas_cali.bottom_line_y;
				} else if (m_adas_cali.adas_calibline == BOTTOM_Y_LINE) {
					m_adas_cali.bottom_line_y+=m_adas_cali.adas_calibstep;
					if (m_adas_cali.bottom_line_y > drawframe_info.u32scnHeight)
						m_adas_cali.bottom_line_y = drawframe_info.u32scnHeight;
				} else {
					OSAL_LOGE("err adas_calibline %d!\n", m_adas_cali.adas_calibline);
				}
				break;
			case 0x2d:/* -  */
				if (m_adas_cali.adas_calibline == VANISH_X_LINE) {
					m_adas_cali.vanish_point_x-=m_adas_cali.adas_calibstep;
					if (m_adas_cali.vanish_point_x < 0)
						m_adas_cali.vanish_point_x = 0;
				} else if (m_adas_cali.adas_calibline == VANISH_Y_LINE) {
					m_adas_cali.vanish_point_y-=m_adas_cali.adas_calibstep;
					if (m_adas_cali.vanish_point_y < 0)
							m_adas_cali.vanish_point_y = 0;
				} else if (m_adas_cali.adas_calibline == BOTTOM_Y_LINE) {
					m_adas_cali.bottom_line_y-=m_adas_cali.adas_calibstep;
					if (m_adas_cali.bottom_line_y < m_adas_cali.vanish_point_y)
						m_adas_cali.bottom_line_y = m_adas_cali.vanish_point_y;
				} else {
					OSAL_LOGE("err adas_calibline %d!\n", m_adas_cali.adas_calibline);
				}
				break;
			case 0x20:/* Space */
				m_adas_cali.adas_calibline++;
				if (m_adas_cali.adas_calibline > BOTTOM_Y_LINE)
					m_adas_cali.adas_calibline = VANISH_Y_LINE;
				break;
			case 'p':
	        case 'P':
	            system("cat /proc/mdp/*");
	        	break;
			case 'q':
        	case 'Q':/* quit */
				axframe_info.action = ADAS_RECOGNITION;
				adas_save_cali_parm(m_adas_cali.vanish_point_x, m_adas_cali.vanish_point_y, m_adas_cali.bottom_line_y);
				adas_update_para(m_adas_cali.vanish_point_x, m_adas_cali.vanish_point_y, m_adas_cali.bottom_line_y);
				osal_mq_send(warning_info.mq_id, LB_WARNING_ADAS_CALIBRATE_SUCCESS, NULL, 0);
				/* save calibrate param */
				al_para_save();
				al_para_exit();
				break;
			default:
          		break;
		}

	}
	OSAL_LOGW("mdp start rec\n");
	while (stream_para[0].exit_flag != EI_TRUE) {
	    s32Ret = SAMPLE_COMM_VENC_Start(stream_para[0].VeChn, PT_H265,
	                SAMPLE_RC_CBR, &venc_config,
	                COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
	    if (EI_SUCCESS != s32Ret) {
	        OSAL_LOGE("%s %d %d ret:%d \n", __func__, __LINE__, stream_para[0].VeChn, s32Ret);
			system("cat /proc/mdp/*");
			while(1);
	    }
#ifndef SAVE_DRAW_AX_H265
	    s32Ret = EI_MI_VISS_SetExtChnAttr(dev, stream_para[0].chn, &stExtChnAttr);
	    if (s32Ret != EI_SUCCESS) {
	        OSAL_LOGE("EI_MI_VISS_SetExtChnAttr failed with %#x!\n", s32Ret);
	        return EI_FAILURE;
	    }
	    s32Ret = EI_MI_VISS_EnableChn(dev, stream_para[0].chn);
	    if (s32Ret != EI_SUCCESS) {
	        OSAL_LOGE("EI_MI_VISS_EnableChn failed with %#x!\n", s32Ret);
			SAMPLE_COMM_VENC_Stop(stream_para[0].VeChn);
	        return EI_FAILURE;
	    }
	    s32Ret = SAMPLE_COMM_VISS_Link_VPU(dev, stream_para[0].chn, stream_para[0].VeChn);
	    if (EI_SUCCESS != s32Ret) {
	        OSAL_LOGE("viss link vpu failed %d-%d-%d \n",
	            0, 0, stream_para[0].VeChn);
			system("cat /proc/mdp/*");
			while(1);
	        SAMPLE_COMM_VENC_Stop(stream_para[0].VeChn);
			EI_MI_VISS_DisableChn(dev, stream_para[0].chn);
			return EI_FAILURE;
	    }
#endif
	    s32Ret = _start_get_stream(&stream_para[0]);
	    if (EI_SUCCESS != s32Ret) {
	        OSAL_LOGE("_StartGetStream failed %d \n", stream_para[0].VeChn);
#ifndef SAVE_DRAW_AX_H265
	        SAMPLE_COMM_VISS_UnLink_VPU(dev, stream_para[0].chn, stream_para[0].VeChn);
#endif
	        SAMPLE_COMM_VENC_Stop(stream_para[0].VeChn);
	    }
	    loop_time = SDR_SAMPLE_VENC_TIME;
	    while (loop_time-- && stream_para[0].exit_flag != EI_TRUE) {
	        usleep(1000 * 1000);
	        if (loop_time % 5 == 4)
	            OSAL_LOGW("loop_time %d\n", loop_time);
	    }
	    stream_para[0].bThreadStart = EI_FALSE;
	    _stop_get_stream(&stream_para[0]);
#ifndef SAVE_DRAW_AX_H265
	    SAMPLE_COMM_VISS_UnLink_VPU(dev, stream_para[0].chn, stream_para[0].VeChn);
		EI_MI_VISS_DisableChn(dev, stream_para[0].chn);
#endif
		SAMPLE_COMM_VENC_Stop(stream_para[0].VeChn);
	}
	OSAL_LOGW("mdp quit rec\n");
	OSAL_LOGW("drawframe wait exit!\n");
    drawframe_info.bThreadStart = EI_FALSE;
    if (drawframe_info.gs_framePid)
           pthread_join(drawframe_info.gs_framePid, NULL);
	OSAL_LOGW("adas wait exit0!\n");
    axframe_info.bThreadStart = EI_FALSE;
    if (axframe_info.gs_framePid)
           pthread_join(axframe_info.gs_framePid, NULL);
	OSAL_LOGW("adas wait exit1!\n");
    sem_destroy(&axframe_info.frame_del_sem);
	OSAL_LOGW("adas wait exit3!\n");
	pthread_mutex_destroy(&axframe_info.p_det_mutex);
	if (drawframe_info.is_disp == EI_TRUE) {
		watermark_exit(VENC_RGN_WM_NUM, MOD_PREVIEW);
	}
	watermark_exit(VENC_RGN_WM_NUM, MOD_VENC);
	rgn_stop();
    OSAL_LOGW("adas start exit!\n");
exit1:
	stVissConfig.astVissInfo[0].stChnInfo.enWorkMode = camera_info[0].chn_num;//add for mashup 4ch to disable viss ch
    SAMPLE_COMM_VISS_StopViss(&stVissConfig);
	if (drawframe_info.is_disp == EI_TRUE) {
	    EI_MI_VO_DisableChn(drawframe_info.VoLayer, drawframe_info.VoChn);
	    EI_MI_VO_DisableVideoLayer(drawframe_info.VoLayer);
	    SAMPLE_COMM_DOSS_StopDev(drawframe_info.VoDev);
	    EI_MI_VO_CloseFd();
	    EI_MI_DOSS_Exit();
	}
exit0:
	/* 这里在退出的时候反映射保存的虚拟地址 */
	for (int u32Idx = 0; u32Idx < stVbConfig.u32PoolCnt; ++u32Idx) {
		for (int i = 0; i < stVbConfig.astCommPool[u32Idx].u32BufCnt; i++) {
            EI_MI_VBUF_FrameMunmap(&pool_info[u32Idx].pstBufMap[i].stVfrmInfo, pool_info[u32Idx].enRemapMode);
        }
		free(pool_info[u32Idx].pstBufMap);
    	pool_info[u32Idx].pstBufMap = NULL;
		EI_MI_VBUF_DestroyPool(pool_info[u32Idx].Pool);
	}
    EI_MI_MLINK_Exit();
    EI_MI_VBUF_Exit();
    EI_MI_MBASE_Exit();
    if (axframe_info.p_det_info) {
        free(axframe_info.p_det_info);
        axframe_info.p_det_info = NULL;
    }
    OSAL_LOGW("show_1ch end exit!\n");

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

	OSAL_LOGW("[%d] mdp_adas_nn start version1.0_202309081400\n", __LINE__);
	drawframe_info.is_disp = EI_TRUE;
    while ((c = getopt(argc, argv, "d:c:r:s:a:p:h:")) != -1) {
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
		case 'a':
			axframe_info.action = (unsigned int)strtol(argv[optind - 1], NULL, 10);
 			break;
		case 'p':
			drawframe_info.is_disp = (unsigned int)strtol(argv[optind - 1], NULL, 10);
			break;
        default:
            preview_help();
            goto EXIT;
        }
    }
	/* open calibrate param file */
	al_para_init();
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
		OSAL_LOGE("[%s, %d]Create waring queue failed!\n", __func__, __LINE__);
		goto EXIT;
	}
	warning_info.bThreadStart = EI_TRUE;
    s32Ret = pthread_create(&(warning_info.g_Pid), NULL, warning_proc,
        (EI_VOID *)&warning_info);
    if (s32Ret)
        OSAL_LOGE("errno=%d, reason(%s)\n", errno, strerror(errno));
    s32Ret = show_1ch(dev, chn, rot, mirror, flip, rect_x, rect_y, width, hight, sns);
	warning_info.bThreadStart = EI_FALSE;
	if (warning_info.g_Pid) {
           pthread_join(warning_info.g_Pid, NULL);
	}
	s32Ret = osal_mq_destroy(warning_info.mq_id, "/waring_mq");
	if (s32Ret) {
		OSAL_LOGE("[%s, %d]waring_mq destory failed!\n", __func__, __LINE__);
	}
	/* save calibrate param */
	al_para_save();
	al_para_exit();
	OSAL_LOGW("[%d] mdp_adas_nn EXIT\n", __LINE__);
	exit(EXIT_SUCCESS);
EXIT:
    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
