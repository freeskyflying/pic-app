/*
 *------------------------------------------------------------------------------
 * @File      :    mdp_bsd_nn.c
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
#include "nn_bsd_api.h"
#include "mdp_bsd_config.h"
#include <math.h>


#define EI_TRACE_LOG(level, fmt, args...)\
do{ \
    EI_TRACE(EI_ID_LOG, level, fmt, ##args);\
} while (0)
#define NN_BSD_AX_PATH	 "/usr/share/ax/bsd"
#define COUNT_CALIBRATE_NUM 20
#define PATH_ROOT "/mnt/card" /* save file root path */
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
#define AL_SEND_FRAMERATE 20 /* send al frame rate, every lane need 10-15fps */
#define AL_PRO_FRAMERATE_STATISTIC /* al static frame rate */
#define SCN_WIDTH 1280
#define SCN_HEIGHT 720
#define SAVE_AX_YUV_SP /* save yuv frame send to al module */
/* #define SAVE_DRAW_YUV_SP */  /* save yuv frame send to disp module, not use */
#define SAVE_DRAW_AX_H265 /* if open this macro, save h265 send to disp module,
include draw rect; else save h265 send to al module, not include  draw rect */
/* #define SAVE_WATERMARK_SOURCE */ /* save bmp data,not include bmp header,bmp data must positive direction*/
/* #define BSD_CUSTOM_WAR_REGION */ /* if open this macro,  bsd warning region is a semicircle */
#define WATERMARK_X 957
#define WATERMARK_Y 5
#define WATERMARK_W 17
#define WATERMARK_H 28
#define WATERMARK_XIEGANG_INDEX 10
#define WATERMARK_MAOHAO_INDEX 11
#define BSD_LANE 2
#define VBUF_POOL_MAX_NUM 4
#define VBUF_BUFFER_MAX_NUM 64

typedef enum _WARNING_MSG_TYPE_ {
	LB_WARNING_BEGIN = 0xF000,
	LB_WARNING_BSD_LEFT,
	LB_WARNING_BSD_RIGHT,
	LB_WARNING_BSD_PEDS,
	LB_WARNING_BSD_CYCLES,
	LB_WARNING_BSD_CALIBRATE_START,
	LB_WARNING_BSD_CALIBRATE_SUCCESS,
	LB_WARNING_BSD_CALIBRATE_FAIL,
	LB_WARNING_BSD_LEVEL0,
	LB_WARNING_BSD_LEVEL1,
	LB_WARNING_BSD_LEVEL2,
	LB_WARNING_USERDEF,
	LB_WARNING_END,
} WARNING_MSG_TYPE;

typedef enum _bsd_cali_flag_{
	BSD_CALI_START = 1,
	BSD_CALI_END,
	BSD_CALI_MAX,
} bsd_cali_flag_e;

typedef enum _BSD_ACTION_ {
	BSD_RECOGNITION,
	BSD_CALIB,
	BSD_ACTION_END,
} BSD_ACTION;
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
	BSD_ACTION action;
    EI_BOOL bThreadStart;
    VISS_CHN phyChn;
    VISS_DEV dev;
    VISS_CHN chn;
	int lane_idx;
    RECT_S stFrameSize;
    unsigned int frameRate;
    EI_U32 u32Width;
    EI_U32 u32Height;
    pthread_t gs_framePid;
	pthread_mutex_t p_det_mutex;
    sem_t frame_del_sem;
	nn_bsd_cfg_t m_nn_bsd_cfg;
	nn_bsd_out_t *p_det_info;
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
	int last_draw_line_num;
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

typedef struct _bsd_calibrate_para_ {
	float vanish_point_x;
	float vanish_point_y;
	float bottom_line_y;
	bsd_cali_flag_e cali_flag;
} bsd_calibrate_para_t;

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

static axframe_info_s axframe_info[BSD_LANE];
static drawframe_info_s drawframe_info;
static warning_info_s warning_info;
sdr_sample_venc_para_t stream_para[ISP_MAX_DEV_NUM] = {{0}};
static bsd_calibrate_para_t m_bsd_cali;
static pool_info_t pool_info[VBUF_POOL_MAX_NUM] = {0};

void preview_help(void)
{
    printf("usage: mdp_bsd_nn \n");
    printf("such as for bsd recognition: mdp_bsd_nn\n");
    printf("such as for bsd recognition: mdp_bsd_nn -d 0 -c 0 -r 90\n");
	printf("such as for calib: mdp_bsd_nn -d 0 -c 0 -r 90 -a 1\n");

    printf("arguments:\n");
    printf("-d    select input device, Dev:0~2, default: 1, means mipi 0 interface\n");
    printf("-c    select chn id, support:0~3, default: 0\n");
    printf("-r    select rotation, 0/90/180/270, default: 0\n");
	printf("-s    sensor or rx macro value, such as 200800, default: 201601\n");
	printf("-n    sensor channel num, default: 1\n");
	printf("-o    viss_out_path mode, default: 1, VISS_OUT_PATH_PIXEL\n");
	printf("-m    reserved for mirror, not use, default: 0\n");
	printf("-f    reserved for flip, not use, default: 0\n");
	printf("-a    action 0: bsd recognition 1: bsd calib, default: 0\n");
	printf("-p    disp enable or diable, 0: disable 1: disp enable, default: 1\n");
    printf("-h    help\n");
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
EI_S32 rgn_start(void)
{
	EI_S32 s32Ret = EI_FAILURE;

	drawframe_info.stRgnChn[1].enModId = EI_ID_IPPU;
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

int bsd_update_para(float vanish_point_x, float vanish_point_y)
{
	EI_PRINT("********bsd_update()******");
	int ret = -1;
	for (int lane_idx = 0; lane_idx < BSD_LANE; lane_idx++) {
		if (axframe_info[lane_idx].nn_hdl == NULL) {
			return ret;
		}
		axframe_info[lane_idx].m_nn_bsd_cfg.camera_param.vanish_point[0] = vanish_point_x;
		axframe_info[lane_idx].m_nn_bsd_cfg.camera_param.vanish_point[1] = vanish_point_y;
		ret = nn_bsd_cmd(axframe_info[lane_idx].nn_hdl, NN_BSD_SET_PARAM, &axframe_info[lane_idx].m_nn_bsd_cfg);
		if( ret == -1) {
			EI_PRINT("NN_BSD_SET_PARAM, failed failed!!");
		}
	}
	return ret;
}

static void bsd_cali_fun(nn_bsd_out_t *out)
{
	if (m_bsd_cali.cali_flag == BSD_CALI_START) {	
		m_bsd_cali.cali_flag = BSD_CALI_END;
		bsd_save_cali_parm(m_bsd_cali.vanish_point_x, m_bsd_cali.vanish_point_y);
		bsd_update_para(m_bsd_cali.vanish_point_x, m_bsd_cali.vanish_point_y);
		osal_mq_send(warning_info.mq_id, LB_WARNING_BSD_CALIBRATE_SUCCESS, NULL, 0);
				
	}
}

static void bsd_draw_location_rgn(VIDEO_FRAME_INFO_S *drawFrame, nn_bsd_out_t *p_det_info)
{
#ifdef SAVE_DRAW_AX_H265
	int draw_rect_num = 0; 
	int	draw_line_num = 0;
	int index = 0;
	int loc_x0, loc_y0, loc_x1, loc_y1;
	EI_S32 s32Ret = EI_FAILURE;
#endif
	if (axframe_info[0].action == BSD_CALIB) {
		bsd_cali_fun(p_det_info);
	}
#ifdef SAVE_DRAW_AX_H265
	else {
		draw_rect_num = p_det_info->cars.size + p_det_info->peds.size+p_det_info->cycles.size;
		if (draw_rect_num > RGN_RECT_NUM || draw_line_num > RGN_LINE_NUM)
			return;

		if (drawframe_info.last_draw_rect_num > draw_rect_num) {
			for (int i = drawframe_info.last_draw_rect_num-1; i >= draw_rect_num; i--) {
				drawframe_info.stRectArray[i].isShow = EI_FALSE;
				s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM],
					&drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
				if(s32Ret) {
					EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_SetChnAttr \n");
	    		}
			}
		}
	    if (p_det_info->cars.size) {
			for (int cnt_cars = 0; cnt_cars < p_det_info->cars.size; cnt_cars++) {
		        loc_x0 = p_det_info->cars.p[cnt_cars].box_smooth[0];
		        loc_y0 = p_det_info->cars.p[cnt_cars].box_smooth[1];
		        loc_x1 = p_det_info->cars.p[cnt_cars].box_smooth[2];
		        loc_y1 = p_det_info->cars.p[cnt_cars].box_smooth[3];
		        drawframe_info.stRectArray[cnt_cars].stRectAttr.stRect.s32X = loc_x0;
				drawframe_info.stRectArray[cnt_cars].stRectAttr.stRect.s32Y = loc_y0;
				drawframe_info.stRectArray[cnt_cars].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
				drawframe_info.stRectArray[cnt_cars].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
				drawframe_info.stRectArray[cnt_cars].stRectAttr.u32BorderSize = 4;
				drawframe_info.stRectArray[cnt_cars].stRectAttr.u32Color = 0xff00ff00;
				drawframe_info.stRectArray[cnt_cars].isShow = EI_TRUE;
				drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].bShow = EI_TRUE;
				s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
				if(s32Ret) {
					EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_SetChnAttr \n");
	    		}
			}
	    }
		index = index + p_det_info->cars.size;
	    for (int cnt_peds = 0; cnt_peds < p_det_info->peds.size; cnt_peds++) {
	        loc_x0 = p_det_info->peds.p[cnt_peds].box_smooth[0];
	        loc_y0 = p_det_info->peds.p[cnt_peds].box_smooth[1];
	        loc_x1 = p_det_info->peds.p[cnt_peds].box_smooth[2];
	        loc_y1 = p_det_info->peds.p[cnt_peds].box_smooth[3];
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
				EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_SetChnAttr \n");
	    	}
	    }
		index = index + p_det_info->peds.size;
	    for (int cnt_cycles = 0; cnt_cycles < p_det_info->cycles.size; cnt_cycles++) {
	        loc_x0 = p_det_info->cycles.p[cnt_cycles].box_smooth[0];
	        loc_y0 = p_det_info->cycles.p[cnt_cycles].box_smooth[1];
	        loc_x1 = p_det_info->cycles.p[cnt_cycles].box_smooth[2];
	        loc_y1 = p_det_info->cycles.p[cnt_cycles].box_smooth[3];
	        drawframe_info.stRectArray[index+cnt_cycles].stRectAttr.stRect.s32X = loc_x0;
			drawframe_info.stRectArray[index+cnt_cycles].stRectAttr.stRect.s32Y = loc_y0;
			drawframe_info.stRectArray[index+cnt_cycles].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
			drawframe_info.stRectArray[index+cnt_cycles].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
			drawframe_info.stRectArray[index+cnt_cycles].stRectAttr.u32BorderSize = 4;
			drawframe_info.stRectArray[index+cnt_cycles].stRectAttr.u32Color = 0xff0000ff;
			drawframe_info.stRectArray[index+cnt_cycles].isShow = EI_TRUE;
			drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].bShow = EI_TRUE;
			s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
			if(s32Ret) {
				EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_SetChnAttr \n");
			}
	    }
		drawframe_info.last_draw_rect_num = draw_rect_num;
#ifndef BSD_CUSTOM_WAR_REGION
		if (drawframe_info.is_disp == EI_TRUE) {
			for (int i = 0; i < 8; i++) {
				drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1].bShow = EI_TRUE;
				drawframe_info.stLineArray[i].isShow = EI_TRUE;
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32X = axframe_info[0].m_nn_bsd_cfg.warn_roi[i][0];
		        drawframe_info.stLineArray[i].stLineAttr.stPoints[0].s32Y = axframe_info[0].m_nn_bsd_cfg.warn_roi[i][1];
				if (i==7) {
					drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32X = axframe_info[0].m_nn_bsd_cfg.warn_roi[4][0];
					drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32Y = axframe_info[0].m_nn_bsd_cfg.warn_roi[4][1];
				} else if (i==3){
					drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32X = axframe_info[0].m_nn_bsd_cfg.warn_roi[0][0];
					drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32Y = axframe_info[0].m_nn_bsd_cfg.warn_roi[0][1];
				} else {
					drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32X = axframe_info[0].m_nn_bsd_cfg.warn_roi[i+1][0];
					drawframe_info.stLineArray[i].stLineAttr.stPoints[1].s32Y = axframe_info[0].m_nn_bsd_cfg.warn_roi[i+1][1];
				}
		        drawframe_info.stLineArray[i].stLineAttr.u32Color = 0xffff0000;
		        drawframe_info.stLineArray[i].stLineAttr.u32LineSize = 4;
				s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM+1], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM+1]);
				if(s32Ret) {
					EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_SetChnAttr \n");
				}
	    	}
		}
#endif
	}
#endif
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
		stRgnChn->enModId = EI_ID_IPPU;
		stRgnChn->s32DevId = stream_para[0].dev;
		stRgnChn->s32ChnId = stream_para[0].chn;
	} else if (mod == MOD_PREVIEW) {
		stRegion = &drawframe_info.stRegion[0];
		stRgnChnAttr = &drawframe_info.stRgnChnAttr[0];
		Handle = &drawframe_info.Handle[0];
		stRgnChn = &drawframe_info.stRgnChn[0];
		stRgnChn->enModId = EI_ID_IPPU;
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

int bsd_warning(nn_bsd_out_t *data)
{
	int64_t cur_time = osal_get_msec();
	int   status, index;

	if (data->cars.size > 0) {
		for (int cnt_cars = 0; cnt_cars < data->cars.size; cnt_cars++) {
			status = data->cars.p[cnt_cars].status;
			EI_PRINT("cnt_cars %d cars status:%d\n", cnt_cars, status);
			if (status == 1) {
				index = LB_WARNING_BSD_LEFT-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_BSD_LEFT, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_BSD_LEFT;
				}
 			} else if (status == 2) {
				index = LB_WARNING_BSD_RIGHT-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_BSD_RIGHT, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_BSD_RIGHT;
				}
			}
		}
	}
	if (data->peds.size > 0) {
		for (int cnt_peds = 0; cnt_peds < data->peds.size; cnt_peds++) {
			status = data->peds.p[cnt_peds].status;
			EI_PRINT("cnt_peds %d peds status:%d\n", cnt_peds, status);
			if (status) {
				index = LB_WARNING_BSD_PEDS-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_BSD_PEDS, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_BSD_PEDS;
				}
			}
		}
	}

	if (data->cycles.size > 0) {
		for (int cnt_cycles = 0; cnt_cycles < data->cycles.size; cnt_cycles++) {
			status = data->cycles.p[cnt_cycles].status;
			EI_PRINT("cnt_cycles %d cycles status:%d\n", cnt_cycles, status);
			if (status) {
				index = LB_WARNING_BSD_CYCLES-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_BSD_CYCLES, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_BSD_CYCLES;
				}
			}
		}
	}

	return 0;
}

int bsd_warning_custom(nn_bsd_out_t *data) //本接口针对报警区域是半圆形
{
	int64_t cur_time = osal_get_msec();
	int   index;
	float r = 0;
	float war_r_level[3] = {SCN_HEIGHT/10, SCN_HEIGHT*2/10, SCN_HEIGHT*3/10};
	
	if (data->cars.size > 0) {
		for (int cnt_cars = 0; cnt_cars < data->cars.size; cnt_cars++) {
			/*计算车框矩形框底部中点到图像底部中点的像素距离,勾股定理，求X^2+Y^2的平方根*/
			r = sqrt(pow((SCN_HEIGHT-data->cars.p[cnt_cars].box[3]), 2)+pow((SCN_WIDTH/2-(data->cars.p[cnt_cars].box[0]+data->cars.p[cnt_cars].box[3])/2), 2));
			EI_PRINT("cnt_cars %d r:%f\n", cnt_cars, r);
			if (r<war_r_level[0]) {
				index = LB_WARNING_BSD_LEVEL0-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_BSD_LEVEL0, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_BSD_LEVEL0;
				}
 			} else if (r<war_r_level[1]) {
				index = LB_WARNING_BSD_LEVEL1-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_BSD_LEVEL1, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_BSD_LEVEL1;
				}
			} else if (r<war_r_level[2]) {
				index = LB_WARNING_BSD_LEVEL2-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_BSD_LEVEL2, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_BSD_LEVEL2;
				}
			}
		}
	}
	if (data->peds.size > 0) {
		for (int cnt_peds = 0; cnt_peds < data->peds.size; cnt_peds++) {
			/*计算行人矩形框底部中点到图像中点的像素距离,勾股定理，求X^2+Y^2的平方根*/
			r = sqrt(pow((SCN_HEIGHT-data->peds.p[cnt_peds].box[3]), 2)+pow((SCN_WIDTH/2-(data->peds.p[cnt_peds].box[0]+data->peds.p[cnt_peds].box[3])/2), 2));
			EI_PRINT("cnt_peds %d r:%f\n", cnt_peds, r);
			if (r<war_r_level[0]) {
				index = LB_WARNING_BSD_LEVEL0-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_BSD_LEVEL0, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_BSD_LEVEL0;
				}
			} else if (r<war_r_level[1]) {
				index = LB_WARNING_BSD_LEVEL1-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_BSD_LEVEL1, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_BSD_LEVEL1;
				}
			} else if (r<war_r_level[2]) {
				index = LB_WARNING_BSD_LEVEL2-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_BSD_LEVEL2, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_BSD_LEVEL2;
				}
			}
		}
	}

	return 0;
}


static int nn_bsd_result_cb0(int event, void *data)
{
	if (event == NN_BSD_DET_DONE) {
	    nn_bsd_out_t *curr_data = (nn_bsd_out_t *)data;
		pthread_mutex_lock(&axframe_info[0].p_det_mutex);
	    /* cars data*/
	    if (axframe_info[0].p_det_info->cars.p) {
	        free(axframe_info[0].p_det_info->cars.p);
	        axframe_info[0].p_det_info->cars.p = NULL;
	        axframe_info[0].p_det_info->cars.size = 0;
	    }
	    axframe_info[0].p_det_info->cars.p =
	        malloc(curr_data->cars.size * sizeof(nn_bsd_car_t));
	    if (axframe_info[0].p_det_info->cars.p) {
	        memcpy(axframe_info[0].p_det_info->cars.p, curr_data->cars.p,
	            curr_data->cars.size * sizeof(nn_bsd_car_t));
	        axframe_info[0].p_det_info->cars.size = curr_data->cars.size;
	    }

	    /* peds data */
	    if (axframe_info[0].p_det_info->peds.p) {
	        free(axframe_info[0].p_det_info->peds.p);
	        axframe_info[0].p_det_info->peds.p = NULL;
	        axframe_info[0].p_det_info->peds.size = 0;
	    }
	    axframe_info[0].p_det_info->peds.p = malloc(curr_data->peds.size * sizeof(nn_bsd_ped_t));
	    if (axframe_info[0].p_det_info->peds.p) {
	        memcpy(axframe_info[0].p_det_info->peds.p, curr_data->peds.p,
	            curr_data->peds.size * sizeof(nn_bsd_ped_t));
	        axframe_info[0].p_det_info->peds.size = curr_data->peds.size;
	    }
		/* cycles data */
	    if (axframe_info[0].p_det_info->cycles.p) {
	        free(axframe_info[0].p_det_info->cycles.p);
	        axframe_info[0].p_det_info->cycles.p = NULL;
	        axframe_info[0].p_det_info->cycles.size = 0;
	    }
	    axframe_info[0].p_det_info->cycles.p = malloc(curr_data->cycles.size * sizeof(nn_bsd_cycle_t));
	    if (axframe_info[0].p_det_info->cycles.p) {
	        memcpy(axframe_info[0].p_det_info->cycles.p, curr_data->cycles.p,
	            curr_data->cycles.size * sizeof(nn_bsd_cycle_t));
	        axframe_info[0].p_det_info->cycles.size = curr_data->cycles.size;
	    }

		pthread_mutex_unlock(&axframe_info[0].p_det_mutex);
	    if (curr_data->cars.size) {
	        EI_PRINT("nn_bsd_result_cb0 curr_data->cars.size = %d\n", curr_data->cars.size);
#ifndef BSD_CUSTOM_WAR_REGION
	        for (int i=0; i<curr_data->cars.size; i++) {
	            EI_PRINT("curr_data->cars.p[%d].status = %d\n", i, curr_data->cars.p[i].status);
	        }
#endif
	    }
	    if (curr_data->peds.size) {
	        EI_PRINT("nn_bsd_result_cb0 curr_data->peds.size = %d\n", curr_data->peds.size);
#ifndef BSD_CUSTOM_WAR_REGION
	        for (int i=0; i<curr_data->peds.size; i++) {
	            EI_PRINT("curr_data->peds.p[%d].status = %d\n", i, curr_data->peds.p[i].status);
	        }
#endif
	    }
		if (curr_data->cycles.size) {
	        EI_PRINT("nn_bsd_result_cb0 curr_data->cycles.size = %d\n", curr_data->cycles.size);
#ifndef BSD_CUSTOM_WAR_REGION
	        for (int i=0; i<curr_data->cycles.size; i++) {
	            EI_PRINT("curr_data->cycles.p[%d].status = %d\n", i, curr_data->cycles.p[i].status);
	        }
#endif
	    }
#ifndef BSD_CUSTOM_WAR_REGION
		bsd_warning(curr_data);
#else
		bsd_warning_custom(curr_data);
#endif
	    sem_post(&axframe_info[0].frame_del_sem);
	} else if (event == NN_BSD_CALIB_DONE) {
		nn_bsd_cfg_t *curr_data = (nn_bsd_cfg_t *)data;
		EI_PRINT("NN_BSD_CALIB_DONE0 \n");
		EI_PRINT("match_count = %d, %f\n", curr_data->calib_param.current.match_count, curr_data->calib_param.current.camera_to_ground);
		EI_PRINT("current x_dist_scale");
		for (int i=0; i<sizeof(curr_data->calib_param.current.x_dist_scale)/sizeof(curr_data->calib_param.current.x_dist_scale[0]); i++)
			EI_PRINT(" %f", curr_data->calib_param.current.x_dist_scale[i]);
		EI_PRINT("\n");
		EI_PRINT("match_count = %d, %f\n", curr_data->calib_param.ready.match_count, curr_data->calib_param.ready.camera_to_ground);
		EI_PRINT("ready x_dist_scale");
		for (int i=0; i<sizeof(curr_data->calib_param.ready.x_dist_scale)/sizeof(curr_data->calib_param.ready.x_dist_scale[0]); i++)
			EI_PRINT(" %f", curr_data->calib_param.ready.x_dist_scale[i]);
		EI_PRINT("\n");
		EI_PRINT("vanish_point = %d, %d\n", curr_data->camera_param.vanish_point[0], curr_data->camera_param.vanish_point[1]);
		bsd_save_cali_parm(curr_data->camera_param.vanish_point[0], curr_data->camera_param.vanish_point[1]);
		osal_mq_send(warning_info.mq_id, LB_WARNING_BSD_CALIBRATE_SUCCESS, NULL, 0);
	}
    return 0;
}
static int nn_bsd_result_cb1(int event, void *data)
{
	if (event == NN_BSD_DET_DONE) {
	    nn_bsd_out_t *curr_data = (nn_bsd_out_t *)data;
		pthread_mutex_lock(&axframe_info[1].p_det_mutex);
	    /* cars data*/
	    if (axframe_info[1].p_det_info->cars.p) {
	        free(axframe_info[1].p_det_info->cars.p);
	        axframe_info[1].p_det_info->cars.p = NULL;
	        axframe_info[1].p_det_info->cars.size = 0;
	    }
	    axframe_info[1].p_det_info->cars.p =
	        malloc(curr_data->cars.size * sizeof(nn_bsd_car_t));
	    if (axframe_info[1].p_det_info->cars.p) {
	        memcpy(axframe_info[1].p_det_info->cars.p, curr_data->cars.p,
	            curr_data->cars.size * sizeof(nn_bsd_car_t));
	        axframe_info[1].p_det_info->cars.size = curr_data->cars.size;
	    }

	    /* peds data */
	    if (axframe_info[1].p_det_info->peds.p) {
	        free(axframe_info[1].p_det_info->peds.p);
	        axframe_info[1].p_det_info->peds.p = NULL;
	        axframe_info[1].p_det_info->peds.size = 0;
	    }
	    axframe_info[1].p_det_info->peds.p = malloc(curr_data->peds.size * sizeof(nn_bsd_ped_t));
	    if (axframe_info[1].p_det_info->peds.p) {
	        memcpy(axframe_info[1].p_det_info->peds.p, curr_data->peds.p,
	            curr_data->peds.size * sizeof(nn_bsd_ped_t));
	        axframe_info[1].p_det_info->peds.size = curr_data->peds.size;
	    }
		/* cycles data */
	    if (axframe_info[1].p_det_info->cycles.p) {
	        free(axframe_info[1].p_det_info->cycles.p);
	        axframe_info[1].p_det_info->cycles.p = NULL;
	        axframe_info[1].p_det_info->cycles.size = 0;
	    }
	    axframe_info[1].p_det_info->cycles.p = malloc(curr_data->cycles.size * sizeof(nn_bsd_cycle_t));
	    if (axframe_info[1].p_det_info->cycles.p) {
	        memcpy(axframe_info[1].p_det_info->cycles.p, curr_data->cycles.p,
	            curr_data->cycles.size * sizeof(nn_bsd_cycle_t));
	        axframe_info[1].p_det_info->cycles.size = curr_data->cycles.size;
	    }

		pthread_mutex_unlock(&axframe_info[1].p_det_mutex);
	    if (curr_data->cars.size) {
	        EI_PRINT("nn_bsd_result_cb1 curr_data->cars.size = %d\n", curr_data->cars.size);
#ifndef BSD_CUSTOM_WAR_REGION
	        for (int i=0; i<curr_data->cars.size; i++) {
	            EI_PRINT("curr_data->cars.p[%d].status = %d\n", i, curr_data->cars.p[i].status);
	        }
#endif
	    }
	    if (curr_data->peds.size) {
	        EI_PRINT("nn_bsd_result_cb1 curr_data->peds.size = %d\n", curr_data->peds.size);
#ifndef BSD_CUSTOM_WAR_REGION
	        for (int i=0; i<curr_data->peds.size; i++) {
	            EI_PRINT("curr_data->peds.p[%d].status = %d\n", i, curr_data->peds.p[i].status);
	        }
#endif
	    }
		if (curr_data->cycles.size) {
	        EI_PRINT("nn_bsd_result_cb1 curr_data->cycles.size = %d\n", curr_data->cycles.size);
#ifndef BSD_CUSTOM_WAR_REGION
	        for (int i=0; i<curr_data->cycles.size; i++) {
	            EI_PRINT("curr_data->cycles.p[%d].status = %d\n", i, curr_data->cycles.p[i].status);
	        }
#endif
	    }
#ifndef BSD_CUSTOM_WAR_REGION
		bsd_warning(curr_data);
#else
		bsd_warning_custom(curr_data);
#endif
	    sem_post(&axframe_info[1].frame_del_sem);
	} else if (event == NN_BSD_CALIB_DONE) {
		nn_bsd_cfg_t *curr_data = (nn_bsd_cfg_t *)data;
		EI_PRINT("NN_BSD_CALIB_DONE1 \n");
		EI_PRINT("match_count = %d, %f\n", curr_data->calib_param.current.match_count, curr_data->calib_param.current.camera_to_ground);
		EI_PRINT("current x_dist_scale");
		for (int i=0; i<sizeof(curr_data->calib_param.current.x_dist_scale)/sizeof(curr_data->calib_param.current.x_dist_scale[0]); i++)
			EI_PRINT(" %f", curr_data->calib_param.current.x_dist_scale[i]);
		EI_PRINT("\n");
		EI_PRINT("match_count = %d, %f\n", curr_data->calib_param.ready.match_count, curr_data->calib_param.ready.camera_to_ground);
		EI_PRINT("ready x_dist_scale");
		for (int i=0; i<sizeof(curr_data->calib_param.ready.x_dist_scale)/sizeof(curr_data->calib_param.ready.x_dist_scale[0]); i++)
			EI_PRINT(" %f", curr_data->calib_param.ready.x_dist_scale[i]);
		EI_PRINT("\n");
		EI_PRINT("vanish_point = %d, %d\n", curr_data->camera_param.vanish_point[0], curr_data->camera_param.vanish_point[1]);
		bsd_save_cali_parm(curr_data->camera_param.vanish_point[0], curr_data->camera_param.vanish_point[1]);
		osal_mq_send(warning_info.mq_id, LB_WARNING_BSD_CALIBRATE_SUCCESS, NULL, 0);
	}
    return 0;


}

int nn_bsd_start(VIDEO_FRAME_INFO_S *frame, void *nn_hdl)
{
    EI_S32 s32Ret;
    static nn_bsd_in_t m_bsd_in;
	int pool_idx = 0;
	int lane_idx = 0;

	/* 获取保存的映射虚拟地址 */
	for (pool_idx = 0; pool_idx < VBUF_POOL_MAX_NUM; pool_idx++) {
		if (frame->u32PoolId == pool_info[pool_idx].Pool) {
			break;
		}
	}
	if (pool_idx >= VBUF_POOL_MAX_NUM)
		return 0;

    m_bsd_in.img.y_phy = (void *)frame->stVFrame.u64PlanePhyAddr[0];
    m_bsd_in.img.y = (void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[0];
    m_bsd_in.img.uv_phy = (void *)frame->stVFrame.u64PlanePhyAddr[1];
    m_bsd_in.img.uv = (void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[1];
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
    if (nn_hdl != NULL) {
        s32Ret = nn_bsd_cmd(nn_hdl, NN_BSD_SET_DATA, &m_bsd_in);
		if (s32Ret < 0) {
			EI_PRINT("NN_BSD_SET_DATA failed %d!\n", s32Ret);
		}
	}
	for (lane_idx = 0; lane_idx < BSD_LANE; lane_idx++) {
		if (axframe_info[lane_idx].nn_hdl == nn_hdl) {
			break;
		}	
	}
#ifdef SAVE_AX_YUV_SP
    EI_S32 i;
#if 0
    EI_PRINT("save yuv buffer sizey %d, stride %d %d %d\n", frame->stVFrame.u32PlaneSizeValid[0], frame->stVFrame.u32PlaneStride[0], frame->stCommFrameInfo.u32Width, frame->stCommFrameInfo.u32Height);
    EI_PRINT("save yuv buffer sizeuv %d, u32PlaneNum %d\n", frame->stVFrame.u32PlaneSizeValid[1], frame->stVFrame.u32PlaneNum);

    EI_PRINT("save yuv buffer addr %llx %llx %llx\n",frame->stVFrame.u64PlanePhyAddr[0],
            frame->stVFrame.u64PlanePhyAddr[1], frame->stVFrame.u64PlanePhyAddr[2]);

    EI_PRINT("save yuv buffer virtual addr %x %x %x\n",pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[0],
            pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[1]);

    EI_PRINT("save yuv buffer ID %x\n",frame->u32BufferId);
#endif

    for (i = 0; i < frame->stVFrame.u32PlaneNum; i++)
    {
        if (axframe_info[lane_idx].ax_fout) {
            if (m_bsd_in.img.format == YUV420_SP) {
                if (i == 0) {
                    fwrite((void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[i], 1,  m_bsd_in.img.width*m_bsd_in.img.height, axframe_info[lane_idx].ax_fout);
                } else {
                    fwrite((void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[i], 1,  m_bsd_in.img.width*m_bsd_in.img.height/2, axframe_info[lane_idx].ax_fout);
                }
            }
        }
    }
#endif
    sem_wait(&axframe_info[lane_idx].frame_del_sem);

    return 0;
}

EI_VOID *get_axframe_proc(EI_VOID *p)
{
    int ret;
    VIDEO_FRAME_INFO_S axFrame = {0};
    axframe_info_s *axframe_info;
    void *nn_hdl = NULL;
	char bsdlib_ver[64]={0};

    axframe_info = (axframe_info_s *)p;
    EI_PRINT("get_axframe_proc bThreadStart = %d\n", axframe_info->bThreadStart);


#ifdef SAVE_AX_YUV_SP
    char srcfilename[FULLNAME_LEN_MAX];
    sprintf(srcfilename, "%s/bsd_raw_ch%dlane%d.yuv", PATH_ROOT, axframe_info->chn, axframe_info->lane_idx);
    axframe_info->ax_fout = fopen(srcfilename, "wb+");
    if (axframe_info->ax_fout == NULL) {
        EI_TRACE_VDEC(EI_DBG_ERR, "file open error1\n");
    }
#endif

#ifdef AL_PRO_FRAMERATE_STATISTIC
	int64_t time1 = 0LL, time2 = 0LL, time_GetChnFrame1 = 0LL, time_GetChnFrame2 = 0LL,
	time_AlProFrame1 = 0LL, time_AlProFrame2 = 0LL;
	int64_t frame_cnt = 0;
#endif
    EI_PRINT("********start_nn_bsd******");
    memset(&axframe_info->m_nn_bsd_cfg, 0, sizeof(axframe_info->m_nn_bsd_cfg));
    /*init nn_bsd, cfg*/
    axframe_info->m_nn_bsd_cfg.img_width = 1280; //算法输入图像宽
    axframe_info->m_nn_bsd_cfg.img_height = 720; //算法输入图像高
    axframe_info->m_nn_bsd_cfg.interest_box[0] = 0; //算法检测区域左上角X坐标
    axframe_info->m_nn_bsd_cfg.interest_box[1] = 0; //算法检测区域左上角y坐标
    axframe_info->m_nn_bsd_cfg.interest_box[2] = 1280-1; //算法检测区域右下角x坐标
    axframe_info->m_nn_bsd_cfg.interest_box[3] = 720-1; //算法检测区域右下角y坐标
    axframe_info->m_nn_bsd_cfg.format = YUV420_SP;
    axframe_info->m_nn_bsd_cfg.model_basedir = NN_BSD_AX_PATH;
	if (axframe_info->lane_idx == 0)
		axframe_info->m_nn_bsd_cfg.cb_func = nn_bsd_result_cb0;
	else
		axframe_info->m_nn_bsd_cfg.cb_func = nn_bsd_result_cb1;
    axframe_info->m_nn_bsd_cfg.max_track_count = 50; //最大跟踪识别的物体个数。理论上没有上限
    axframe_info->m_nn_bsd_cfg.warn_roi[0][0] = 200; //左边报警区域的左上角x坐标
    axframe_info->m_nn_bsd_cfg.warn_roi[0][1] = 100; //左边报警区域的左上角y坐标
    axframe_info->m_nn_bsd_cfg.warn_roi[1][0] = 400; //左边报警区域的右上角x坐标
    axframe_info->m_nn_bsd_cfg.warn_roi[1][1] = 100; //左边报警区域的右上角y坐标
    axframe_info->m_nn_bsd_cfg.warn_roi[2][0] = 350; //左边报警区域的右下角x坐标
    axframe_info->m_nn_bsd_cfg.warn_roi[2][1] = 500; //左边报警区域的右下角y坐标
    axframe_info->m_nn_bsd_cfg.warn_roi[3][0] = 100; //左边报警区域的左下角x坐标
    axframe_info->m_nn_bsd_cfg.warn_roi[3][1] = 500; //左边报警区域的左下角y坐标
    axframe_info->m_nn_bsd_cfg.warn_roi[4][0] = 880;  //右边报警区域的左上角x坐标
    axframe_info->m_nn_bsd_cfg.warn_roi[4][1] = 100;  //右边报警区域的左上角y坐标
    axframe_info->m_nn_bsd_cfg.warn_roi[5][0] = 1080;  //右边报警区域的右上角x坐标
    axframe_info->m_nn_bsd_cfg.warn_roi[5][1] = 100;  //右边报警区域的右上角y坐标
    axframe_info->m_nn_bsd_cfg.warn_roi[6][0] = 1180;  //右边报警区域的右下角x坐标
    axframe_info->m_nn_bsd_cfg.warn_roi[6][1] = 500;
    axframe_info->m_nn_bsd_cfg.warn_roi[7][0] = 930;
    axframe_info->m_nn_bsd_cfg.warn_roi[7][1] = 500;
    axframe_info->m_nn_bsd_cfg.camera_param.vanish_point[0] = 640;//天际线和车中线交叉点X坐标 单位像素
    axframe_info->m_nn_bsd_cfg.camera_param.vanish_point[1] = 360;//天际线和车中线交叉点Y坐标 单位像素
    axframe_info->m_nn_bsd_cfg.camera_param.camera_angle[0] = 120;//摄像头水平角度
    axframe_info->m_nn_bsd_cfg.camera_param.camera_angle[1] = 80;//摄像头垂直角度
    axframe_info->m_nn_bsd_cfg.camera_param.camera_to_ground = 0.9; //摄像头距离地面的高度
    axframe_info->m_nn_bsd_cfg.camera_param.car_width = 1.8; //车宽，单位，米。汽车1.8，货车2.2
    axframe_info->m_nn_bsd_cfg.fixmode_warn_param.install_mode = 0; //摄像头安装位置，自动校准要求必须是BACK_MODE,且gps速度> 30KM/H
	/* left Or right region bian dao fu zhu ,只有在NN_BSD_BACK_MODE 模式下才起效*/
    axframe_info->m_nn_bsd_cfg.fixmode_warn_param.low_level_warn_gps = 10; //摄像头level0和level1报警速度
    axframe_info->m_nn_bsd_cfg.fixmode_warn_param.high_level_warn_gps = 80; //level3 最小报警速度
    axframe_info->m_nn_bsd_cfg.fixmode_warn_param.ttc_thresh = 4.0; //level3 最小报警碰撞时间
    axframe_info->m_nn_bsd_cfg.fixmode_warn_param.warn_dist_horizon[0] = 1.5f;
	axframe_info->m_nn_bsd_cfg.fixmode_warn_param.warn_dist_horizon[1] = 4.0f;
	axframe_info->m_nn_bsd_cfg.fixmode_warn_param.warn_dist_forward[0] = 0;
	axframe_info->m_nn_bsd_cfg.fixmode_warn_param.warn_dist_forward[1] = 5.0f;
	axframe_info->m_nn_bsd_cfg.fixmode_warn_param.lca_warn_interval[0] = 0;
	axframe_info->m_nn_bsd_cfg.fixmode_warn_param.lca_warn_interval[1] = 10.0f;
	axframe_info->m_nn_bsd_cfg.fixmode_warn_param.lca_warn_interval[2] = 10.0f;
	/* middle region  man qu jian ce ,只有在NN_BSD_BACK_MODE 模式下才起效*/
    axframe_info->m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_dist_forward[0] = 0; //后车和本车最小距离
    axframe_info->m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_dist_forward[1] = 5.0f; //后车和本车最大距离
    axframe_info->m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_dist_horizon[0] = 1.5f; //水平最小距离
    axframe_info->m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_dist_horizon[1] = 4.0f; //水平最大距离
    axframe_info->m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_gps[0] = 0;
	axframe_info->m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_gps[1] = 100;
	axframe_info->m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_interval[0] = 0;
	axframe_info->m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_interval[1] = 10.0f;
	axframe_info->m_nn_bsd_cfg.fixmode_warn_param.bsd_warn_interval[2] = 10.0f;
	//检测阈值,0是车,1是人,2是非机动车，其它索引无效。det_thresh rang 0-1之间,<0.45,内部会转换成0.45, >0.65,内部会转换成0.65。
	axframe_info->m_nn_bsd_cfg.det_thresh[0] = 0.45;//车检测阈值，0.45-0.65之间，阈值低，识别率高，误识别高；阈值高，识别率低，误识别低
	axframe_info->m_nn_bsd_cfg.det_thresh[1] = 0.45;//人检测阈值，0.45-0.65之间，阈值低，识别率高，误识别高；阈值高，识别率低，误识别低
	axframe_info->m_nn_bsd_cfg.det_thresh[2] = 0.40;//非机动车检测阈值，0.45-0.65之间，阈值低，识别率高，误识别高；阈值高，识别率低，误识别低

	axframe_info->m_nn_bsd_cfg.car_det_size_range[0] = 48; //识别车辆的最小像素
    axframe_info->m_nn_bsd_cfg.car_det_size_range[1] = 1280; //识别车辆的最大像素
    axframe_info->m_nn_bsd_cfg.ped_det_size_range[0] = 48; //识别人物的最小像素
    axframe_info->m_nn_bsd_cfg.ped_det_size_range[1] = 1280; //识别人物的最大像素
    axframe_info->m_nn_bsd_cfg.keep_model_memory = 0;//是否保持模型的加载, 如果只开启一路bsd此参数设置为0，
                         //如果开启两路及以上此参数设置为1。这样能节省第二路及以后bsd算法的模型加载时间
    axframe_info->m_nn_bsd_cfg.ax_freq = 500; //ax的频率,单路大于300Mhz,双路不高于600MHz，不建议改
    axframe_info->m_nn_bsd_cfg.resolution = 0;//0的检测距离比较近，1检测距离比较远, 检测距离比较远的消耗的cpu和npu资源比较多
	axframe_info->m_nn_bsd_cfg.calib_param.auto_calib_en = 1;//打开自动校准
	axframe_info->m_nn_bsd_cfg.calib_param.current.camera_to_ground = 0.7;//摄像头离地高度
	axframe_info->m_nn_bsd_cfg.calib_param.ready.camera_to_ground = 0.7;//摄像头离地高度

	axframe_info->m_nn_bsd_cfg.model_type = 3; /*0 - car+person  1-person only 2 - fisheye, 3 - car/person/cycle fisheye or normal camera */

    /* open libbsd.so */
    nn_hdl = nn_bsd_open(&axframe_info->m_nn_bsd_cfg);
    if (nn_hdl == NULL) {
        EI_PRINT("nn_bsd_open() failed!");
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
        ret = EI_MI_IPPU_GetChnFrame(axframe_info->dev,
            axframe_info->chn, &axFrame, 1000);
        if (ret != EI_SUCCESS) {
            EI_PRINT("chn %d get frame error 0x%x\n", axframe_info->chn, ret);
            usleep(100 * 1000);
            continue;
        }
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time_GetChnFrame2 = osal_get_msec();
#endif
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time_AlProFrame1 = osal_get_msec();
#endif
        nn_bsd_start(&axFrame, nn_hdl);
#ifdef AL_PRO_FRAMERATE_STATISTIC
	time_AlProFrame2 = osal_get_msec();
	EI_PRINT("time_GetChnFrame %lld ms, time_AlProFrame %lld ms\n",
		time_GetChnFrame2-time_GetChnFrame1, time_AlProFrame2-time_AlProFrame1);
#endif
#ifdef AL_PRO_FRAMERATE_STATISTIC
		frame_cnt++;
		time2 = osal_get_msec();
		if ((time2 - time1) >= 3000) {
			EI_PRINT("algo process %lld fps\n", (frame_cnt * 1000) / (time2 - time1));
			time1 = time2;
			frame_cnt  = 0;
		}
#endif
		nn_bsd_cmd(nn_hdl, NN_BSD_GET_VERSION, bsdlib_ver);
        EI_MI_IPPU_ReleaseChnFrame(axframe_info->dev, axframe_info->chn, &axFrame);
    }
	EI_PRINT("nn_bsd_close 0 \n");
    nn_bsd_close(nn_hdl);
	EI_PRINT("nn_bsd_close 1\n");
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

    sprintf(srcfilename, "%s/bsd_draw_raw_ch%d.yuv", PATH_ROOT, drawframe_info->chn);
    EI_PRINT("********get_drawframe_proc******\n");
#ifdef SAVE_DRAW_YUV_SP
    drawframe_info->draw_fout = fopen(srcfilename, "wb+");
    if (drawframe_info->draw_fout == NULL) {
        EI_TRACE_VDEC(EI_DBG_ERR, "file open error1\n");
    }
#endif
    while (EI_TRUE == drawframe_info->bThreadStart) {
        memset(&drawFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
        ret = EI_MI_IPPU_GetChnFrame(drawframe_info->dev,
            drawframe_info->chn, &drawFrame, 1000);
        if (ret != EI_SUCCESS) {
            EI_PRINT("chn %d get frame error 0x%x\n", drawframe_info->chn, ret);
            usleep(100 * 1000);
            continue;
        }
#ifdef SAVE_DRAW_YUV_SP
		int pool_idx = 0;
		/* 获取保存的映射虚拟地址 */
		for (pool_idx = 0; pool_idx < VBUF_POOL_MAX_NUM; pool_idx++) {
			if (frame->u32PoolId == pool_info[pool_idx].Pool) {
				break;
			}
		}
	if (pool_idx >= VBUF_POOL_MAX_NUM)
		return 0;
		if (drawframe_info->draw_fout) {
			EI_PRINT("drawFrame.stCommFrameInfo.u32Width = %d %d\n", drawFrame.stCommFrameInfo.u32Width, drawFrame.stCommFrameInfo.u32Height);
			fwrite((void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[0], 1, drawFrame.stCommFrameInfo.u32Width*drawFrame.stCommFrameInfo.u32Height, drawframe_info->draw_fout);
			fwrite((void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[1], 1, drawFrame.stCommFrameInfo.u32Width*drawFrame.stCommFrameInfo.u32Height/2, drawframe_info->draw_fout);
		}
#endif
		pthread_mutex_lock(&axframe_info[0].p_det_mutex);
		bsd_draw_location_rgn(&drawFrame, axframe_info[0].p_det_info);
		pthread_mutex_unlock(&axframe_info[0].p_det_mutex);
		if (drawframe_info->is_disp == EI_TRUE) {
			drawFrame.enFrameType = MDP_FRAME_TYPE_DOSS;
			EI_MI_VO_SendFrame(0, 0, &drawFrame, 100);
		}

        EI_MI_IPPU_ReleaseChnFrame(drawframe_info->dev,
            drawframe_info->chn, &drawFrame);
    }
    EI_MI_IPPU_DisableChn(drawframe_info->dev, drawframe_info->chn);
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
		EI_PRINT("msg.type = %x\n", msg.type);
		switch (msg.type) {
			case LB_WARNING_BSD_LEFT:
				warn_tone_play("/usr/share/out/res/bsd_warning/alarm_left.wav");
				break;
			case LB_WARNING_BSD_RIGHT:
				warn_tone_play("/usr/share/out/res/bsd_warning/alarm_right.wav");
				break;
			case LB_WARNING_BSD_PEDS:
				warn_tone_play("/usr/share/out/res/bsd_warning/xiaoxinxingren.wav");
				break;
			case LB_WARNING_BSD_CYCLES:
				warn_tone_play("/usr/share/out/res/bsd_warning/zhuyifeijidongche.wav");
				break;
			case LB_WARNING_BSD_CALIBRATE_START:
				warn_tone_play("/usr/share/out/res/bsd_warning/jiaozhunkaishi.wav");
				break;
			case LB_WARNING_BSD_CALIBRATE_SUCCESS:
				warn_tone_play("/usr/share/out/res/bsd_warning/jiaozhunchenggong.wav");
				break;
			case LB_WARNING_BSD_CALIBRATE_FAIL:
				warn_tone_play("/usr/share/out/res/bsd_warning/jiaozhunshibai.wav");
				break;
			case LB_WARNING_BSD_LEVEL0:
				break;
			case LB_WARNING_BSD_LEVEL1:
				break;
			case LB_WARNING_BSD_LEVEL2:
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

    axframe_info->bThreadStart = EI_TRUE;

    ret = pthread_create(&(axframe_info->gs_framePid), NULL, get_axframe_proc,
        (EI_VOID *)axframe_info);
    if (ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));
    EI_PRINT("start_get_axframe success %#x! phyChn:%d ippu chn :%d dev:%d\n", ret, axframe_info->phyChn, axframe_info->chn, axframe_info->dev);

    return EI_SUCCESS;
}
static int start_get_drawframe(drawframe_info_s *drawframe_info)
{
    int ret;

    drawframe_info->bThreadStart = EI_TRUE;
    ret = pthread_create(&(drawframe_info->gs_framePid), NULL, get_drawframe_proc,
        (EI_VOID *)drawframe_info);
    if (ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));
    EI_PRINT("start_get_drawframe success %#x! phyChn:%d ippu chn :%d dev:%d\n", ret, drawframe_info->phyChn, drawframe_info->chn, drawframe_info->dev);

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
		if (drawframe_info.is_disp == EI_TRUE) {
			watermark_update(p_tm, WATERMARK_X, WATERMARK_Y, MOD_PREVIEW);
		}
        if (stStream.pstPack.u32Len[0] == 0) {
            EI_PRINT("%d ch %d pstPack.u32Len:%d-%d, ofset:%d, ret:%x\n",
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
    SNS_TYPE_E sns, unsigned int ch_num, VISS_OUT_PATH_E viss_out_path)
{
	static VBUF_CONFIG_S stVbConfig;
	static SAMPLE_VISS_CONFIG_S stVissConfig;
	EI_BOOL abChnEnable[IPPU_PHY_CHN_MAX_NUM] = {0};
    IPPU_DEV_ATTR_S stIppuDevAttr = {0};
    IPPU_CHN_ATTR_S astIppuChnAttr[IPPU_PHY_CHN_MAX_NUM] = {{0}};
    EI_S32 s32Ret = EI_FAILURE;

    VO_VIDEO_LAYER_ATTR_S stVoLayerAttr = {0};
    VO_CHN_ATTR_S stVoChnAttr = {0};
	SAMPLE_VENC_CONFIG_S venc_config = {0};
	int loop_time;
	VISS_PIC_TYPE_E stPicType = {0};
	MDP_CHN_S stSrcViss[4], stSinkIsp[4];
	VBUF_POOL_CONFIG_S stPoolCfg = {0};
	EI_PRINT("[%s, %d]\n", __func__, __LINE__);
	SAMPLE_COMM_VISS_GetPicTypeBySensor(sns, &stPicType);
	EI_PRINT("stPicType.stSize.u32Width = %d \n", stPicType.stSize.u32Width);
	EI_PRINT("stPicType.stSize.u32Height = %d \n", stPicType.stSize.u32Height);

	stVbConfig.astFrameInfo[0].enFrameType = MDP_FRAME_TYPE_COMMON;
    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Align = 32;
    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Width = stPicType.stSize.u32Width;
    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Height = stPicType.stSize.u32Height;
    stVbConfig.astFrameInfo[0].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stVbConfig.astCommPool[0].u32BufCnt = 12*BSD_LANE;
    stVbConfig.astCommPool[0].enRemapMode = VBUF_REMAP_MODE_NOCACHE;

    stVbConfig.astFrameInfo[1].enFrameType = MDP_FRAME_TYPE_COMMON;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Align = 32;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width = 1280;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height = 720;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stVbConfig.astCommPool[1].u32BufCnt = 24*BSD_LANE;
    stVbConfig.astCommPool[1].enRemapMode = VBUF_REMAP_MODE_CACHED;

	stVbConfig.u32PoolCnt = 2;
	EI_PRINT("[%s, %d]\n", __func__, __LINE__);
	EI_MI_MBASE_Init();
    EI_MI_VBUF_Init();
    EI_MI_MLINK_Init();
	if (stVbConfig.u32PoolCnt > VBUF_POOL_MAX_NUM) {
		EI_TRACE_VENC(EI_DBG_ERR,"VBUF MAX ERROE %s %d %d\n", __FILE__, __LINE__, stVbConfig.u32PoolCnt);
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
		        EI_TRACE_VENC(EI_DBG_ERR,"%s %d\n", __FILE__, __LINE__);
		}
		pool_info[u32Idx].u32BufNum = stVbConfig.astCommPool[u32Idx].u32BufCnt;
		pool_info[u32Idx].enRemapMode = stVbConfig.astCommPool[u32Idx].enRemapMode;
		pool_info[u32Idx].pstBufMap = malloc(sizeof(VBUF_MAP_S) * pool_info[u32Idx].u32BufNum);
		/* 这里在初始化的时候保存映射的虚拟地址，是避免在算法使用的时候映射会占用 cpu 资源 */
		for (int i = 0; i < stVbConfig.astCommPool[u32Idx].u32BufCnt; i++) {
            pool_info[u32Idx].Buffer[i] = EI_MI_VBUF_GetBuffer(pool_info[u32Idx].Pool, 10*1000);
            s32Ret = EI_MI_VBUF_GetFrameInfo(pool_info[u32Idx].Buffer[i], &stVbConfig.astFrameInfo[u32Idx]);
			if (s32Ret) {
		        EI_TRACE_VENC(EI_DBG_ERR,"%s %d\n", __FILE__, __LINE__);
			}
            s32Ret = EI_MI_VBUF_FrameMmap(&stVbConfig.astFrameInfo[u32Idx], stVbConfig.astCommPool[u32Idx].enRemapMode);
			if (s32Ret) {
		        EI_TRACE_VENC(EI_DBG_ERR,"%s %d\n", __FILE__, __LINE__);
			}
			pool_info[u32Idx].pstBufMap[stVbConfig.astFrameInfo[u32Idx].u32Idx].u32BufID = pool_info[u32Idx].Buffer[i];
			pool_info[u32Idx].pstBufMap[stVbConfig.astFrameInfo[u32Idx].u32Idx].stVfrmInfo = stVbConfig.astFrameInfo[u32Idx];
#if 0
			for (int j = 0; j < FRAME_MAX_PLANE; j++) {
				pool_info[u32Idx].pstBufMap[stVbConfig.astFrameInfo[u32Idx].u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[j] = stVbConfig.astFrameInfo[u32Idx].stVFrame.ulPlaneVirAddr[j];
			}

			EI_PRINT("u32Idx %d %s %d\n", stVbConfig.astFrameInfo[u32Idx].u32Idx, __FILE__, __LINE__);
		    EI_PRINT("Plane Size: 0x%08x, 0x%08x, PhyAddr: 0x%llx, 0x%llx, MmapVirAddr: 0x%08lx, 0x%08lx\n",
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
		        EI_TRACE_VENC(EI_DBG_ERR,"%s %d\n", __FILE__, __LINE__);
				goto exit0;
			}
        }
	}
	stVissConfig.astVissInfo[0].stDevInfo.VissDev = dev; /*0: DVP, 1: MIPI0, 2:MIPI1*/
	for (int i=0; i<ch_num; i++)
		stVissConfig.astVissInfo[0].stDevInfo.aBindPhyChn[i] = i;
    stVissConfig.astVissInfo[0].stDevInfo.enOutPath = viss_out_path;
	for (int i=0; i<ch_num; i++)
		stVissConfig.astVissInfo[0].stChnInfo.aVissChn[i] = i;
    stVissConfig.astVissInfo[0].stChnInfo.enWorkMode = ch_num;
    stVissConfig.astVissInfo[0].stIspInfo.IspDev = 0;
	if (viss_out_path == VISS_OUT_PATH_PIXEL)
		stVissConfig.astVissInfo[0].stIspInfo.enRunningMode = ISP_MODE_RUNNING_ONLINE;
	else
		stVissConfig.astVissInfo[0].stIspInfo.enRunningMode = ISP_MODE_RUNNING_OFFLINE;
    stVissConfig.astVissInfo[0].stChnInfo.stChnAttr.u32Align = 32;
    stVissConfig.astVissInfo[0].stSnsInfo.enSnsType = sns;
    stVissConfig.astVissInfo[0].stChnInfo.stChnAttr.stFrameRate.s32DstFrameRate =
		SDR_VISS_FRAMERATE;

	s32Ret = SAMPLE_COMM_ISP_Start(&stVissConfig.astVissInfo[0]);
    if (s32Ret) {
		EI_PRINT("SAMPLE_COMM_ISP_Start failed\n");
        goto exit1;
    }
	EI_PRINT("[%s, %d]\n", __func__, __LINE__);
	stIppuDevAttr.s32IppuDev = stVissConfig.astVissInfo[0].stIspInfo.IspDev;
	stIppuDevAttr.u32InputWidth     = stPicType.stSize.u32Width;
	stIppuDevAttr.u32InputHeight    = stPicType.stSize.u32Height;
    stIppuDevAttr.u32DataSrc        = stVissConfig.astVissInfo[0].stDevInfo.VissDev;
    stIppuDevAttr.enRunningMode     = IPPU_MODE_RUNNING_ONLINE;
	stIppuDevAttr.stFrameRate.s32SrcFrameRate = 25;
	stIppuDevAttr.stFrameRate.s32DstFrameRate = 25;

	abChnEnable[0] = EI_FALSE;
	astIppuChnAttr[0].u32Width      = stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Width;
	astIppuChnAttr[0].u32Height     = stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Height;
	astIppuChnAttr[0].u32Align      = stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Align;
	astIppuChnAttr[0].enPixelFormat = stVbConfig.astFrameInfo[0].stCommFrameInfo.enPixelFormat;
	astIppuChnAttr[0].stFrameRate.s32SrcFrameRate = 25;
	astIppuChnAttr[0].stFrameRate.s32DstFrameRate = 25;
	astIppuChnAttr[0].u32Depth      = 0;

	abChnEnable[1] = EI_TRUE;
	astIppuChnAttr[1].u32Width      = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
	astIppuChnAttr[1].u32Height     = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
	astIppuChnAttr[1].u32Align      = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Align;
	astIppuChnAttr[1].enPixelFormat = stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat;
	astIppuChnAttr[1].stFrameRate.s32SrcFrameRate = 25;
	astIppuChnAttr[1].stFrameRate.s32DstFrameRate = 25;
	astIppuChnAttr[1].u32Depth      = 2;

	abChnEnable[2] = EI_TRUE;
	astIppuChnAttr[2].u32Width      = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
	astIppuChnAttr[2].u32Height     = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
	astIppuChnAttr[2].u32Align      = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Align;
	astIppuChnAttr[2].enPixelFormat = stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat;
	astIppuChnAttr[2].stFrameRate.s32SrcFrameRate = 25;
	astIppuChnAttr[2].stFrameRate.s32DstFrameRate = AL_SEND_FRAMERATE;//AL
	astIppuChnAttr[2].u32Depth      = 2;

	s32Ret = SAMPLE_COMM_IPPU_Start(stIppuDevAttr.s32IppuDev, abChnEnable, &stIppuDevAttr, astIppuChnAttr);
    if (s32Ret) {
        goto exit1;
    }
	#if 0
		IPPU_ROTATION_ATTR_S astIppuChnRot = {0};
		if (abChnEnable[2] == EI_TRUE) {
			astIppuChnRot.bEnable = EI_TRUE;
			astIppuChnRot.eRotation = ROTATION_180;
			EI_MI_IPPU_SetChnRotation(stIppuDevAttr.s32IppuDev, 2, &astIppuChnRot);
		}
	#endif
    stVissConfig.astVissInfo[0].stChnInfo.stChnAttr.u32Depth = 2;
    stVissConfig.s32WorkingVissNum = 1;

    s32Ret = SAMPLE_COMM_VISS_StartViss(&stVissConfig);
    EI_TRACE_VENC(EI_DBG_ERR," comm viss start test\n");
    if (s32Ret) {
        EI_TRACE_VENC(EI_DBG_ERR," comm viss start error\n");
    }
	if (viss_out_path != VISS_OUT_PATH_PIXEL) {
		stSrcViss[0].enModId = EI_ID_VISS;
		stSrcViss[0].s32ChnId = chn;
		stSrcViss[0].s32DevId = stVissConfig.astVissInfo[0].stDevInfo.VissDev;
		stSinkIsp[0].enModId = EI_ID_ISP;
		stSinkIsp[0].s32ChnId = 0;
		stSinkIsp[0].s32DevId = 0;
		EI_MI_MLINK_Link(&stSrcViss[0], &stSinkIsp[0]);
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
	}
	for (int lane_idx=0; lane_idx < BSD_LANE; lane_idx++) {
		pthread_mutex_init(&axframe_info[lane_idx].p_det_mutex, NULL);
	    axframe_info[lane_idx].p_det_info = malloc(sizeof(nn_bsd_out_t));
	    if (!axframe_info[lane_idx].p_det_info) {
			printf("axframe_info[%d].p_det_info malloc failed!\n", lane_idx);
			goto exit1;
	    }
		if (axframe_info[lane_idx].action == BSD_CALIB) {
			m_bsd_cali.cali_flag = BSD_CALI_START;
		}
	    memset(axframe_info[lane_idx].p_det_info, 0x00, sizeof(nn_bsd_out_t));
	    axframe_info[lane_idx].dev = stIppuDevAttr.s32IppuDev;//IPPU dev 0, for test ,set  chn same for diff lane_idx
	    axframe_info[lane_idx].chn = 2;//IPPU ch2, for test ,set  chn same for diff lane_idx
	    axframe_info[lane_idx].phyChn = chn;
	    axframe_info[lane_idx].frameRate = SDR_VISS_FRAMERATE;
	    axframe_info[lane_idx].u32Width = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
	    axframe_info[lane_idx].u32Height = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
		axframe_info[lane_idx].lane_idx = lane_idx;
		axframe_info[lane_idx].ax_fout = NULL;
	    sem_init(&axframe_info[lane_idx].frame_del_sem, 0, 0);

	    start_get_axframe(&axframe_info[lane_idx]);
	}
    drawframe_info.dev = stIppuDevAttr.s32IppuDev;
    drawframe_info.chn = 1;// draw to ax chn, IPPU ch 1
    drawframe_info.phyChn = chn;
    drawframe_info.frameRate = SDR_VISS_FRAMERATE;
    drawframe_info.u32Width = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
    drawframe_info.u32Height = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
	drawframe_info.draw_fout = NULL;
	drawframe_info.last_draw_rect_num = RGN_RECT_NUM;
	drawframe_info.last_draw_line_num = RGN_LINE_NUM;
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
	stream_para[0].dev = stIppuDevAttr.s32IppuDev;
	stream_para[0].VeChn = 0;
	stream_para[0].chn = 1;
	stream_para[0].bitrate = SDR_SAMPLE_VENC_BITRATE;
	stream_para[0].exit_flag = EI_FALSE;
	stream_para[0].phyChn = chn;
	watermark_init(0, VENC_RGN_WM_NUM, watermark_filename, MOD_VENC);
	if (drawframe_info.is_disp == EI_TRUE) {
		watermark_init(VENC_RGN_WM_NUM+RGN_ARRAY_NUM, VENC_RGN_WM_NUM, watermark_filename, MOD_PREVIEW);
	}
	EI_PRINT("mdp start rec\n");
	while (stream_para[0].exit_flag != EI_TRUE) {
#if 1
	    s32Ret = SAMPLE_COMM_VENC_Start(stream_para[0].VeChn, PT_H265,
	                SAMPLE_RC_CBR, &venc_config,
	                COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
	    if (EI_SUCCESS != s32Ret) {
	        EI_PRINT("%s %d %d ret:%d \n", __func__, __LINE__, stream_para[0].VeChn, s32Ret);
	    }
	    s32Ret = SAMPLE_COMM_IPPU_Link_VPU(stream_para[0].dev, stream_para[0].chn, stream_para[0].VeChn);
	    if (EI_SUCCESS != s32Ret) {
	        EI_PRINT("viss link vpu failed %d-%d-%d \n",
	            0, 0, stream_para[0].VeChn);
	        SAMPLE_COMM_VENC_Stop(stream_para[0].VeChn);
			EI_MI_IPPU_DisableChn(dev, stream_para[0].chn);
			return EI_FAILURE;
	    }
	    s32Ret = _start_get_stream(&stream_para[0]);
	    if (EI_SUCCESS != s32Ret) {
	        EI_PRINT("_StartGetStream failed %d \n", stream_para[0].VeChn);
	        SAMPLE_COMM_IPPU_UnLink_VPU(stream_para[0].dev, stream_para[0].chn, stream_para[0].VeChn);
	        SAMPLE_COMM_VENC_Stop(stream_para[0].VeChn);
	    }
	    loop_time = SDR_SAMPLE_VENC_TIME;
	    while (loop_time-- && stream_para[0].exit_flag != EI_TRUE) {
	        usleep(1000 * 1000);
	        if (loop_time % 5 == 4)
	            EI_PRINT("loop_time %d\n", loop_time);
	        EI_TRACE_LOG(EI_DBG_ERR,"loop_time %d\n", loop_time);
	    }
	    stream_para[0].bThreadStart = EI_FALSE;
	    _stop_get_stream(&stream_para[0]);
	    SAMPLE_COMM_IPPU_UnLink_VPU(stream_para[0].dev, stream_para[0].chn, stream_para[0].VeChn);
		SAMPLE_COMM_VENC_Stop(stream_para[0].VeChn);
#endif
	}
	EI_PRINT("mdp quit rec\n");
	printf("bsd wait exit0!\n");
	for (int lane_idx=0; lane_idx<BSD_LANE; lane_idx++) {
	    axframe_info[lane_idx].bThreadStart = EI_FALSE;
	    if (axframe_info[lane_idx].gs_framePid)
	           pthread_join(axframe_info[lane_idx].gs_framePid, NULL);
		printf("bsd wait exit1!\n");
	    sem_destroy(&axframe_info[lane_idx].frame_del_sem);
	}
	printf("bsd wait exit2!\n");
    drawframe_info.bThreadStart = EI_FALSE;
    if (drawframe_info.gs_framePid)
           pthread_join(drawframe_info.gs_framePid, NULL);
	printf("bsd wait exit3!\n");
	for (int lane_idx=0; lane_idx<BSD_LANE; lane_idx++) {
		pthread_mutex_destroy(&axframe_info[lane_idx].p_det_mutex);
	}
	if (drawframe_info.is_disp == EI_TRUE) {
		watermark_exit(VENC_RGN_WM_NUM, MOD_PREVIEW);
	}
	watermark_exit(VENC_RGN_WM_NUM, MOD_VENC);
	rgn_stop();
    printf("bsd start exit!\n");
exit1:
    SAMPLE_COMM_VISS_StopViss(&stVissConfig);
	if (drawframe_info.is_disp == EI_TRUE) {
	    EI_MI_VO_DisableChn(drawframe_info.VoLayer, drawframe_info.VoChn);
	    EI_MI_VO_DisableVideoLayer(drawframe_info.VoLayer);
	    SAMPLE_COMM_DOSS_StopDev(drawframe_info.VoDev);
	    EI_MI_VO_CloseFd();
	    EI_MI_DOSS_Exit();
	}
	if (viss_out_path != VISS_OUT_PATH_PIXEL) {
		EI_MI_MLINK_UnLink(&stSrcViss[0], &stSinkIsp[0]);
	}
	SAMPLE_COMM_IPPU_Stop(stIppuDevAttr.s32IppuDev, abChnEnable);
	EI_PRINT("exit %s, %d \n", __func__, __LINE__);
    SAMPLE_COMM_ISP_Stop(stIppuDevAttr.s32IppuDev);
	EI_PRINT("exit %s, %d \n", __func__, __LINE__);
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
	for (int lane_idx=0; lane_idx<BSD_LANE; lane_idx++) {
	    if (axframe_info[lane_idx].p_det_info) {
	        free(axframe_info[lane_idx].p_det_info);
	        axframe_info[lane_idx].p_det_info = NULL;
	    }
	}
    printf("show_1ch end exit!\n");

    return s32Ret;
}

int main(int argc, char **argv)
{
    EI_S32 s32Ret = EI_FAILURE;
    VISS_DEV dev = 0;
    VISS_CHN chn = 0;
    SNS_TYPE_E sns = TP9930_DVP_1920_1080_30FPS_4CH_YUV;
    int rot = 90;
    EI_BOOL flip = EI_FALSE;
    EI_BOOL mirror = EI_FALSE;
    unsigned int rect_x = 0;
    unsigned int rect_y = 0;
    unsigned int width = 720;
    unsigned int hight = 1280;
	unsigned int ch_num = 4;
	int viss_out_path = VISS_OUT_PATH_DMA;
    int c;
	int mirror_temp, flip_temp = 0;

	drawframe_info.is_disp = EI_TRUE;
	EI_PRINT("[%d] mdp_bsd_nn start version1.0_202303291500\n", __LINE__);
    while ((c = getopt(argc, argv, "d:c:r:s:n:o:m:f:a:p:h:")) != -1) {
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
		case 'n':
			ch_num = (unsigned int)strtol(argv[optind - 1], NULL, 10);
			break;
		case 'o':
			viss_out_path = (unsigned int)strtol(argv[optind - 1], NULL, 10);
			break;
		case 'm':
			mirror_temp = (unsigned int)strtol(argv[optind - 1], NULL, 10);
			if (mirror_temp == EI_FALSE || mirror_temp == EI_TRUE)
				mirror = mirror_temp;
			break;
		case 'f':
			flip_temp = (unsigned int)strtol(argv[optind - 1], NULL, 10);
			if (flip_temp == EI_FALSE || flip_temp == EI_TRUE)
				flip = flip_temp;
			break;
		case 'a':
			axframe_info[0].action = (unsigned int)strtol(argv[optind - 1], NULL, 10);
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
    } else {
		rot = ROTATION_0;
	}
	warning_info.mq_id = osal_mq_create("/waring_mq", MQ_MSG_NUM, MQ_MSG_LEN);
	if (warning_info.mq_id == 0) {
		EI_PRINT("[%s, %d]Create waring queue failed!\n", __func__, __LINE__);
		goto EXIT;
	}
	EI_PRINT("[%s, %d]\n", __func__, __LINE__);
	warning_info.bThreadStart = EI_TRUE;
    s32Ret = pthread_create(&(warning_info.g_Pid), NULL, warning_proc,
        (EI_VOID *)&warning_info);
    if (s32Ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));
	EI_PRINT("[%s, %d]\n", __func__, __LINE__);
    s32Ret = show_1ch(dev, chn, rot, mirror, flip, rect_x, rect_y, width, hight, sns, ch_num, viss_out_path);
	warning_info.bThreadStart = EI_FALSE;
	if (warning_info.g_Pid) {
           pthread_join(warning_info.g_Pid, NULL);
	}
	s32Ret = osal_mq_destroy(warning_info.mq_id, "/waring_mq");
	if (s32Ret) {
		EI_PRINT("[%s, %d]waring_mq destory failed!\n", __func__, __LINE__);
	}
	/* save calibrate param */
	al_para_save();
	al_para_exit();
	EI_PRINT("[%d] mdp_bsd_nn EXIT\n", __LINE__);
	exit(EXIT_SUCCESS);
EXIT:
    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
