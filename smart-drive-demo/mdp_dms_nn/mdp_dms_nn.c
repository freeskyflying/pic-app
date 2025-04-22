/*
 *------------------------------------------------------------------------------
 * @File      :    mdp_dms_nn.c
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
#include "nn_dms_api.h"
#include "nn_headfacedet_api.h"
#include "eznn_api.h"
#include "mdp_dms_config.h"

#define compare_distance(a0, a1, a2, a3) (((a3-a2)/(a1-a0))> 0.9 && ((a3-a2)/(a1-a0))< 1.1)?1:0

#define NN_DMS_PATH	 "/usr/share/ax/dms"
#define LIVINGDET_PATH	 "/usr/share/ax/liveness"
#define COUNT_CALIBRATE_NUM 20
#define PATH_ROOT "/mnt/card" /* save file root path */
#define MQ_MSG_NUM 128 /* message queue number */
#define MQ_MSG_LEN 128 /* every message max lenght */
#define ALARM_INTERVAL 0 /* same warning message interval time unit:ms */
/* #define ALARM_THINING *//* defined more level for alarm tone */
#define CALI_FAIL_ALARM_INTERVAL 20000 /* same warning message interval time unit:ms */
#define SDR_SAMPLE_VENC_TIME 120 /* venc time interval unit:s */
#define SDR_SAMPLE_VENC_BITRATE 2000000 /* venc file bitrate unit:bit/s */
#define SDR_VISS_FRAMERATE 25
#define FULLNAME_LEN_MAX 128 /* file len max, full name, include path */
#define RGN_ARRAY_NUM 2  /* rgn  array 0:rect 1:line */
#define VENC_RGN_WM_SOURCE_NUM 12  /* watermark rgn source num */
#define VENC_RGN_WM_NUM 18  /* watermark rgn num */
#define RGN_RECT_NUM 16 /* rect max num */
#define RGN_LINE_NUM 16 /* line max num */
#define RGN_DRAW /* rgn draw, if not define this macro, use software draw */
#define AL_SEND_FRAMERATE 10 /* send al frame rate */
#define AL_PRO_FRAMERATE_STATISTIC /* al static frame rate */
#define SCN_WIDTH 1280
#define SCN_HEIGHT 720
/* #define SAVE_AX_YUV_SP *//* save yuv frame send to al module */
/* #define SAVE_LIVINGDET_YUV_SP */ /* save yuv frame send to al module */
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
	LB_WARNING_DMS_DRIVER_LEAVE,
	LB_WARNING_DMS_NO_BELT,
	LB_WARNING_DMS_CALL,
	LB_WARNING_DMS_DRINK,
	LB_WARNING_DMS_SMOKE,
#ifdef ALARM_THINING
	LB_WARNING_DMS_ATTATION_LOOKAROUND,
	LB_WARNING_DMS_ATTATION_LOOKUP,
	LB_WARNING_DMS_ATTATION_LOOKDOWN,
	LB_WARNING_DMS_REST_LEVEL2,
	LB_WARNING_DMS_REST_LEVEL4,
	LB_WARNING_DMS_REST_LEVEL6,
	LB_WARNING_DMS_REST_LEVEL8,
	LB_WARNING_DMS_REST_LEVEL10,
#endif
	LB_WARNING_DMS_ATTATION,
	LB_WARNING_DMS_REST,
	LB_WARNING_DMS_CAMERA_COVER,
	LB_WARNING_DMS_INFRARED_BLOCK,
	LB_WARNING_DMS_CALIBRATE_START,
	LB_WARNING_DMS_CALIBRATE_SUCCESS,
	LB_WARNING_DMS_CALIBRATE_FAIL,
	LB_WARNING_SD_PLUGIN,
	LB_WARNING_SD_PLUGOUT,
	LB_WARNING_USERDEF,
	LB_WARNING_END,
} WARNING_MSG_TYPE;

typedef enum _SYSTEM_MSG_TYPE_ {
	LB_SYSMSG_BEGIN = 0xF000,
	LB_SYSMSG_SD_PLUGIN,
	LB_SYSMSG_SD_PLUGOUT,
	LB_SYSMSG_SD_INIT_OK,
	LB_SYSMSG_SD_INIT_FAIL,
	LB_SYSMSG_SD_MOUNT_INV_VOL, /* sdcard not format by device */
	LB_SYSMSG_SD_LOW_PERF,
	LB_SYSMSG_USBD_PLUGIN,
	LB_SYSMSG_USBD_PLUGOUT,
	LB_SYSMSG_USBH_PLUGIN,
	LB_SYSMSG_USBH_PLUGOUT,
	LB_SYSMSG_FS_PART_MOUNT_OK,
	LB_SYSMSG_FS_PART_MOUNT_FAIL,
	LB_SYSMSG_FS_PART_UNMOUNT_PREPARE, /* app finish fs op and notify fs umount */
	LB_SYSMSG_FS_PART_UNMOUNT,
	LB_SYSMSG_STANDBY,
	LB_SYSMSG_RESUME,
	LB_SYSMSG_SCREEN_OPEN,
	LB_SYSMSG_SCREEN_CLOSE,
	LB_SYSMSG_SCREEN_LOCK,
	LB_SYSMSG_SCREEN_UNLOCK,
	LB_SYSMSG_POWER_OFF,
	LB_SYSMSG_POWER_LOW,
	LB_SYSMSG_USB_POWER_CONNECT,
	LB_SYSMSG_USB_POWER_DISCONNECT,
	LB_SYSMSG_KEY,
	LB_SYSMSG_AV_PLUGIN,
	LB_SYSMSG_AV_PLUGOUT,
	LB_SYSMSG_BACK_ON,
	LB_SYSMSG_BACK_OFF,
	LB_SYSMSG_ACC_ON,
	LB_SYSMSG_ACC_OFF,
	LB_SYSMSG_RECORDER_FILE_FULL,
	LB_SYSMSG_LOCKFILE_FULL,
	LB_SYSMSG_GSENSOR,
	LB_SYSMSG_ALARM_COLLIDE,
	LB_SYSMSG_ALARM_PRESSURE,
	LB_SYSMSG_ALARM_LAUNCH,
	LB_SYSMSG_ALARM_DISTANCE,
	LB_SYSMSG_ALARM_BSD_LEFT,
	LB_SYSMSG_ALARM_BSD_RIGHT,
	LB_SYSMSG_ALARM_ACC_CHANGE,
	LB_SYSMSG_DIALOG,
	LB_SYSMSG_DMS_PLUGIN,
	LB_SYSMSG_DMS_PLUGOUT,
	LB_SYSMSG_BACKDOOR,
	LB_SYSMSG_DMS_NO_FACE,
	LB_SYSMSG_DMS_CALL,
	LB_SYSMSG_DMS_DRINK,
	LB_SYSMSG_DMS_YAWN,
	LB_SYSMSG_DMS_SMOKE,
	LB_SYSMSG_DMS_ATTATION,
	LB_SYSMSG_DMS_REST,
	LB_SYSMSG_DMS_CAMERA_COVER,
	LB_SYSMSG_DMS_INFRARED_BLOCK,
	LB_SYSMSG_DMS_CALIBRATE_SUCCESS,
	LB_SYSMSG_USERDEF,
	LB_SYSMSG_END,
} SYSTEM_MSG_TYPE;

typedef enum _dms_cali_flag_{
	DMS_CALI_START = 1,
	DMS_CALI_END,
	DMS_CALI_MAX,
} dms_cali_flag_e;

typedef enum _DMS_ACTION_ {
	DMS_RECOGNITION,
	DMS_CALIB,
	DMS_ACTION_END,
} DMS_ACTION;

typedef enum _WM_MOD_TYPE_ {
	MOD_PREVIEW,
	MOD_VENC,
} WM_MOD_TYPE;
typedef enum _dms_livingdet_flag_{
	LIVING_DET_NONE,
	LIVING_DET_SINGLE_RGB,
	LIVING_DET_SINGLE_NIR,
	LIVING_DET_MAX,
} dms_livingdet_flag_e;

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
	DMS_ACTION action;
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
	nn_dms_cfg_t m_nn_dms_cfg;
	nn_dms_out_t *p_det_info;
	void *nn_hdl;
	ezax_livingdet_cfg_t livingdet_cfg;
	ezax_rt_t *livingdet_out;
	void *livingdet_hdl;
	ezax_img_t face;
	ezax_livingdet_rt_t *score;
	EI_S32 livingdet_flag; /* value 1:single rgb; value 2: single nir; others: disabled */
	EI_S32 brightness;
	FILE *ax_fout;
	EI_S32 gray_flag; /* value 0:send camera image ; value 1: uv memset 0x80, send gray scale image */
	FILE *livingdet_fout;
	void *gray_uv_vir_addr;
	EI_U64 gray_uv_phy_addr;
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
#ifdef RGN_DRAW
	RGN_ATTR_S stRegion[3];/* 0:OVERLAYEX_RGN;1:RECTANGLEEX_ARRAY_RGN;2:LINEEX_ARRAY_RGN */
    RGN_CHN_ATTR_S stRgnChnAttr[RGN_ARRAY_NUM+VENC_RGN_WM_NUM];
    RGN_HANDLE Handle[RGN_ARRAY_NUM+VENC_RGN_WM_NUM];/* 0~VENC_RGN_WM_NUM: OVERLAYEX_RGN;VENC_RGN_WM_NUM:RECTANGLEEX_ARRAY_RGN;VENC_RGN_WM_NUM+1:LINEEX_ARRAY_RGN*/
    RECTANGLEEX_ARRAY_SUB_ATTRS stRectArray[RGN_RECT_NUM];
    LINEEX_ARRAY_SUB_ATTRS stLineArray[RGN_LINE_NUM];
	MDP_CHN_S stRgnChn[2];/* 0:ai rect;1:time watermark */
	char *wm_data[VENC_RGN_WM_SOURCE_NUM];
	int last_draw_num;
#endif
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
	int sdcard_status_flag; /* 0: sdcard plugout;1: sdcard plugin;other:reserved */
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

typedef struct _dms_calibrate_para_ {
	float calib_yaw;
	float calib_pitch;
	int count_face_cb;
	dms_cali_flag_e cali_flag;
	int auto_cali_count;
	int m_face_rect[4];
} dms_calibrate_para_t;

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
static dms_calibrate_para_t m_dms_cali;
static pool_info_t pool_info[VBUF_POOL_MAX_NUM] = {0};

void preview_help(void)
{
    OSAL_LOGW("usage: mdp_dms_nn \n");
    OSAL_LOGW("such as for dms recognition: mdp_dms_nn\n");
    OSAL_LOGW("such as for dms recognition: mdp_dms_nn -d 0 -c 0 -r 90\n");
	OSAL_LOGW("such as for calib: mdp_dms_nn -d 0 -c 0 -r 90 -a 1\n");

    OSAL_LOGW("arguments:\n");
    OSAL_LOGW("-d    select input device, Dev:0~2, 0:dvp,1:mipi0, 2:mipi1, default: 0, means dvp interface\n");
    OSAL_LOGW("-c    select chn id, support:0~3, default: 0\n");
    OSAL_LOGW("-r    select rotation, 0/90/180/270, default: 90\n");
	OSAL_LOGW("-s    sensor or rx macro value, see ei_comm_camera.h, such as 200800, default: 100301\n");
	OSAL_LOGW("-a    action 0: dms recognition 1: dms calib, default: 0\n");
	OSAL_LOGW("-b    brightness -64-63, default: 0\n");
	OSAL_LOGW("-l    living det flag, 0/1/2, default: 0, disable living_det\n");
	OSAL_LOGW("-g    gray sacle flag, 0/1, default: 0, use camera image\n");
	OSAL_LOGW("-p    disp enable or diable, 0: disable 1: disp enable, default: 1\n");
    OSAL_LOGW("-h    help\n");
}

#ifdef RGN_DRAW
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

static void dms_draw_location_rgn(VIDEO_FRAME_INFO_S *drawFrame, nn_dms_out_t *p_det_info)
{
	int draw_num;
	int index = 0;
	int loc_x0, loc_y0, loc_x1, loc_y1;
	EI_S32 s32Ret = EI_FAILURE;

	if (axframe_info.action == DMS_CALIB) {
		if (p_det_info->faces.size) {
			if (p_det_info->faces.driver_idx < 0 || p_det_info->faces.driver_idx >= p_det_info->faces.size) {
				//OSAL_LOGE("p_det_info->faces.driver_idx %d\n", p_det_info->faces.driver_idx);
				/* if driver location not found , use idx 0 */
				loc_x0 = p_det_info->faces.p[0].box_smooth[0];
		        loc_y0 = p_det_info->faces.p[0].box_smooth[1];
		        loc_x1 = p_det_info->faces.p[0].box_smooth[2];
		        loc_y1 = p_det_info->faces.p[0].box_smooth[3];
			} else {
		        loc_x0 = p_det_info->faces.p[p_det_info->faces.driver_idx].box_smooth[0];
		        loc_y0 = p_det_info->faces.p[p_det_info->faces.driver_idx].box_smooth[1];
		        loc_x1 = p_det_info->faces.p[p_det_info->faces.driver_idx].box_smooth[2];
		        loc_y1 = p_det_info->faces.p[p_det_info->faces.driver_idx].box_smooth[3];
			}
	        drawframe_info.stRectArray[0].stRectAttr.stRect.s32X = loc_x0;
			drawframe_info.stRectArray[0].stRectAttr.stRect.s32Y = loc_y0;
			drawframe_info.stRectArray[0].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
			drawframe_info.stRectArray[0].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
			drawframe_info.stRectArray[0].stRectAttr.u32BorderSize = 4;
			if (m_dms_cali.cali_flag == DMS_CALI_END)
				drawframe_info.stRectArray[0].stRectAttr.u32Color = 0xff00ff00;
			else
				drawframe_info.stRectArray[0].stRectAttr.u32Color = 0xffff0000;
			drawframe_info.stRectArray[0].isShow = EI_TRUE;
			drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].bShow = EI_TRUE;
			s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
			if(s32Ret) {
				OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
			}
	    } else {
			drawframe_info.stRectArray[0].isShow = EI_FALSE;
			s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
			if(s32Ret) {
				OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
			}
		}

	} else {
		draw_num = p_det_info->faces.size + p_det_info->obj_boxes[0].size +
			p_det_info->obj_boxes[1].size + p_det_info->obj_boxes[2].size +
			p_det_info->obj_boxes[3].size + p_det_info->heads.size + p_det_info->persons.size;
		if (draw_num > RGN_RECT_NUM)
			return;
		if (drawframe_info.last_draw_num > draw_num) {
			for (int i = drawframe_info.last_draw_num-1; i >= draw_num; i--) {
				drawframe_info.stRectArray[i].isShow = EI_FALSE;
				s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM],
					&drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
				if(s32Ret) {
					OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
				}
			}
		}
	    if (p_det_info->faces.size) {
			//OSAL_LOGE("p_det_info->faces.size %d \n", p_det_info->faces.size);
			for (int cnt_faces = 0; cnt_faces < p_det_info->faces.size; cnt_faces++) {
		        loc_x0 = p_det_info->faces.p[cnt_faces].box_smooth[0];
		        loc_y0 = p_det_info->faces.p[cnt_faces].box_smooth[1];
		        loc_x1 = p_det_info->faces.p[cnt_faces].box_smooth[2];
		        loc_y1 = p_det_info->faces.p[cnt_faces].box_smooth[3];
		        drawframe_info.stRectArray[cnt_faces].stRectAttr.stRect.s32X = loc_x0;
				drawframe_info.stRectArray[cnt_faces].stRectAttr.stRect.s32Y = loc_y0;
				drawframe_info.stRectArray[cnt_faces].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
				drawframe_info.stRectArray[cnt_faces].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
				drawframe_info.stRectArray[cnt_faces].stRectAttr.u32BorderSize = 4;
				if (cnt_faces == p_det_info->faces.driver_idx) {
					drawframe_info.stRectArray[cnt_faces].stRectAttr.u32Color = 0xff8000ff;
				} else {
					OSAL_LOGE("p_det_info->faces.driver_idx %d \n", p_det_info->faces.driver_idx);
					drawframe_info.stRectArray[cnt_faces].stRectAttr.u32Color = 0xffffff80;
				}
				drawframe_info.stRectArray[cnt_faces].isShow = EI_TRUE;
				drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].bShow = EI_TRUE;
				s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
				if(s32Ret) {
					OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
				}
			}
	    }
		index = index + p_det_info->faces.size;
	    /* for (int cnt_drinks = 0; cnt_drinks < p_det_info->obj_boxes[0].size; cnt_drinks++) {
	      loc_x0 = p_det_info->obj_boxes[0].p[cnt_drinks].box[0];
	        loc_y0 = p_det_info->obj_boxes[0].p[cnt_drinks].box[1];
	        loc_x1 = p_det_info->obj_boxes[0].p[cnt_drinks].box[2];
	        loc_y1 = p_det_info->obj_boxes[0].p[cnt_drinks].box[3];
	        drawframe_info.stRectArray[index+cnt_drinks].stRectAttr.stRect.s32X = loc_x0;
			drawframe_info.stRectArray[index+cnt_drinks].stRectAttr.stRect.s32Y = loc_y0;
			drawframe_info.stRectArray[index+cnt_drinks].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
			drawframe_info.stRectArray[index+cnt_drinks].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
			drawframe_info.stRectArray[index+cnt_drinks].stRectAttr.u32BorderSize = 4;
			drawframe_info.stRectArray[index+cnt_drinks].stRectAttr.u32Color = 0xff00ff00;
			drawframe_info.stRectArray[index+cnt_drinks].isShow = EI_TRUE;
			drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].bShow = EI_TRUE;
			s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]); 
			if(s32Ret) {
				OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
			}
	    } */
		index = index + p_det_info->obj_boxes[0].size;
	    for (int cnt_phones = 0; cnt_phones < p_det_info->obj_boxes[1].size; cnt_phones++) {
	        loc_x0 = p_det_info->obj_boxes[1].p[cnt_phones].box[0];
	        loc_y0 = p_det_info->obj_boxes[1].p[cnt_phones].box[1];
	        loc_x1 = p_det_info->obj_boxes[1].p[cnt_phones].box[2];
	        loc_y1 = p_det_info->obj_boxes[1].p[cnt_phones].box[3];
	        drawframe_info.stRectArray[index+cnt_phones].stRectAttr.stRect.s32X = loc_x0;
			drawframe_info.stRectArray[index+cnt_phones].stRectAttr.stRect.s32Y = loc_y0;
			drawframe_info.stRectArray[index+cnt_phones].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
			drawframe_info.stRectArray[index+cnt_phones].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
			drawframe_info.stRectArray[index+cnt_phones].stRectAttr.u32BorderSize = 4;
			drawframe_info.stRectArray[index+cnt_phones].stRectAttr.u32Color = 0xffff0000;
			drawframe_info.stRectArray[index+cnt_phones].isShow = EI_TRUE;
			drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].bShow = EI_TRUE;
			s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
			if(s32Ret) {
				OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
	    	}
	    }
		index = index + p_det_info->obj_boxes[1].size;
	    for (int cnt_smokes = 0; cnt_smokes < p_det_info->obj_boxes[2].size; cnt_smokes++) {
	        loc_x0 = p_det_info->obj_boxes[2].p[cnt_smokes].box[0];
	        loc_y0 = p_det_info->obj_boxes[2].p[cnt_smokes].box[1];
	        loc_x1 = p_det_info->obj_boxes[2].p[cnt_smokes].box[2];
	        loc_y1 = p_det_info->obj_boxes[2].p[cnt_smokes].box[3];
	        drawframe_info.stRectArray[index+cnt_smokes].stRectAttr.stRect.s32X = loc_x0;
			drawframe_info.stRectArray[index+cnt_smokes].stRectAttr.stRect.s32Y = loc_y0;
			drawframe_info.stRectArray[index+cnt_smokes].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
			drawframe_info.stRectArray[index+cnt_smokes].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
			drawframe_info.stRectArray[index+cnt_smokes].stRectAttr.u32BorderSize = 4;
			drawframe_info.stRectArray[index+cnt_smokes].stRectAttr.u32Color = 0xff0000ff;
			drawframe_info.stRectArray[index+cnt_smokes].isShow = EI_TRUE;
			drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].bShow = EI_TRUE;
			s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
			if(s32Ret) {
				OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
	    	}
	    }
		index = index + p_det_info->obj_boxes[2].size;
		for (int cnt_belt = 0; cnt_belt < p_det_info->obj_boxes[3].size; cnt_belt++) {
			loc_x0 = p_det_info->obj_boxes[3].p[cnt_belt].box[0];
			loc_y0 = p_det_info->obj_boxes[3].p[cnt_belt].box[1];
			loc_x1 = p_det_info->obj_boxes[3].p[cnt_belt].box[2];
			loc_y1 = p_det_info->obj_boxes[3].p[cnt_belt].box[3];
			drawframe_info.stRectArray[index+cnt_belt].stRectAttr.stRect.s32X = loc_x0;
			drawframe_info.stRectArray[index+cnt_belt].stRectAttr.stRect.s32Y = loc_y0;
			drawframe_info.stRectArray[index+cnt_belt].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
			drawframe_info.stRectArray[index+cnt_belt].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
			drawframe_info.stRectArray[index+cnt_belt].stRectAttr.u32BorderSize = 4;
			drawframe_info.stRectArray[index+cnt_belt].stRectAttr.u32Color = 0xffff00ff;
			drawframe_info.stRectArray[index+cnt_belt].isShow = EI_TRUE;
			drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].bShow = EI_TRUE;
			s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
			if(s32Ret) {
				OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
			}
		}
		index = index + p_det_info->obj_boxes[3].size;
		if (p_det_info->heads.size) {
			//OSAL_LOGE("p_det_info->heads.size %d \n", p_det_info->heads.size);
			for (int cnt_heads = 0; cnt_heads < p_det_info->heads.size; cnt_heads++) {
			        loc_x0 = p_det_info->heads.p[cnt_heads].box_smooth[0];
			        loc_y0 = p_det_info->heads.p[cnt_heads].box_smooth[1];
			        loc_x1 = p_det_info->heads.p[cnt_heads].box_smooth[2];
			        loc_y1 = p_det_info->heads.p[cnt_heads].box_smooth[3];
					//OSAL_LOGE("heads %d %d %d %d \n", loc_x0, loc_y0, loc_x1, loc_y1);
			        drawframe_info.stRectArray[index+cnt_heads].stRectAttr.stRect.s32X = loc_x0;
					drawframe_info.stRectArray[index+cnt_heads].stRectAttr.stRect.s32Y = loc_y0;
					drawframe_info.stRectArray[index+cnt_heads].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
					drawframe_info.stRectArray[index+cnt_heads].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
					drawframe_info.stRectArray[index+cnt_heads].stRectAttr.u32BorderSize = 4;
					if (cnt_heads == p_det_info->heads.driver_idx) {
						drawframe_info.stRectArray[index+cnt_heads].stRectAttr.u32Color = 0xffffffff;
					} else {
						//OSAL_LOGE("p_det_info->heads.driver_idx %d \n", p_det_info->heads.driver_idx);
						drawframe_info.stRectArray[index+cnt_heads].stRectAttr.u32Color = 0xff000000;
					}
					drawframe_info.stRectArray[index+cnt_heads].isShow = EI_TRUE;
					drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].bShow = EI_TRUE;
					s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
					if(s32Ret) {
						OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
					}
				}
		}
		index = index + p_det_info->heads.size;
		if (p_det_info->persons.size) {
			//OSAL_LOGE("p_det_info->persons.size %d \n", p_det_info->persons.size);
			for (int cnt_persons = 0; cnt_persons < p_det_info->persons.size; cnt_persons++) {
			        loc_x0 = p_det_info->persons.p[cnt_persons].box_smooth[0];
			        loc_y0 = p_det_info->persons.p[cnt_persons].box_smooth[1];
			        loc_x1 = p_det_info->persons.p[cnt_persons].box_smooth[2];
			        loc_y1 = p_det_info->persons.p[cnt_persons].box_smooth[3];
					//OSAL_LOGE("persons %d %d %d %d \n", loc_x0, loc_y0, loc_x1, loc_y1);
			        drawframe_info.stRectArray[index+cnt_persons].stRectAttr.stRect.s32X = loc_x0;
					drawframe_info.stRectArray[index+cnt_persons].stRectAttr.stRect.s32Y = loc_y0;
					drawframe_info.stRectArray[index+cnt_persons].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
					drawframe_info.stRectArray[index+cnt_persons].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
					drawframe_info.stRectArray[index+cnt_persons].stRectAttr.u32BorderSize = 4;
					if (cnt_persons == p_det_info->persons.driver_idx) {
						drawframe_info.stRectArray[index+cnt_persons].stRectAttr.u32Color = 0xff00ff00;
					} else {
						//OSAL_LOGE("p_det_info->persons.driver_idx %d \n", p_det_info->persons.driver_idx);
						drawframe_info.stRectArray[index+cnt_persons].stRectAttr.u32Color = 0xffff00ff;
					}
					drawframe_info.stRectArray[index+cnt_persons].isShow = EI_TRUE;
					drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM].bShow = EI_TRUE;
					s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[VENC_RGN_WM_NUM], &drawframe_info.stRgnChn[1], &drawframe_info.stRgnChnAttr[VENC_RGN_WM_NUM]);
					if(s32Ret) {
						OSAL_LOGE("EI_MI_RGN_SetChnAttr \n");
					}
				}
		}
		drawframe_info.last_draw_num = draw_num;
	}
}
#else
static void dms_draw_location(VIDEO_FRAME_INFO_S *drawFrame, nn_dms_out_t *p_det_info)
{
    int pframe_width , pframe_height ;
    uint8_t *addr[3] = {NULL};
    yuv_g2d_fmt_t format = YUV_G2D_FMT_YUV420SP;
    EI_S32 s32Ret;
	int loc_x0, loc_y0, loc_x1, loc_y1;
	yuv_g2d_point_t point0, point1;

    s32Ret = EI_MI_VBUF_FrameMmap(drawFrame, VBUF_REMAP_MODE_CACHED);
    if (s32Ret != EI_SUCCESS)
    {
        OSAL_LOGE("save yuv buffer error %x\n", s32Ret);
        return ;
    }
#ifdef SAVE_DRAW_YUV_SP
    char srcfilename[128];
    if (!drawframe_info.draw_fout) {
        sprintf(srcfilename, "%s/dms_draw_ch%d.yuv", PATH_ROOT, axframe_info.chn);
        drawframe_info.draw_fout = fopen(srcfilename, "wb+");
        if (drawframe_info.draw_fout == NULL) {
            OSAL_LOGE("file open error1\n");
        }
    }
#endif
    pframe_width = drawFrame->stCommFrameInfo.u32Width;
    pframe_height = drawFrame->stCommFrameInfo.u32Height;
    addr[0] = (uint8_t *)drawFrame->stVFrame.ulPlaneVirAddr[0];
    addr[1] = (uint8_t *)drawFrame->stVFrame.ulPlaneVirAddr[1];

    uint32_t linestride[3] = {pframe_width, pframe_width, 0};
    if (p_det_info->faces.size) {
		if (p_det_info->faces.driver_idx < 0 || p_det_info->faces.driver_idx >= p_det_info->faces.size) {
			OSAL_LOGE("p_det_info->faces.driver_idx %d\n", p_det_info->faces.driver_idx);
			/* if driver location not found , use idx 0 */
			loc_x0 = p_det_info->faces.p[0].box[0];
	        loc_y0 = p_det_info->faces.p[0].box[1];
	        loc_x1 = p_det_info->faces.p[0].box[2];
	        loc_y1 = p_det_info->faces.p[0].box[3];
		} else {
	        loc_x0 = p_det_info->faces.p[p_det_info->faces.driver_idx].box[0];
	        loc_y0 = p_det_info->faces.p[p_det_info->faces.driver_idx].box[1];
	        loc_x1 = p_det_info->faces.p[p_det_info->faces.driver_idx].box[2];
	        loc_y1 = p_det_info->faces.p[p_det_info->faces.driver_idx].box[3];
		}
        point0.x = loc_x0;
        point0.y = loc_y0;
        point1.x = loc_x1;
        point1.y = loc_y0;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0xff0000);

        point0.x = loc_x0;
        point0.y = loc_y0;
        point1.x = loc_x0;
        point1.y = loc_y1;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0xff0000);

        point0.x = loc_x1;
        point0.y = loc_y0;
        point1.x = loc_x1;
        point1.y = loc_y1;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0xff0000);

        point0.x = loc_x0;
        point0.y = loc_y1;
        point1.x = loc_x1;
        point1.y = loc_y1;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0xff0000);
    }
    for (int cnt_drinks = 0; cnt_drinks < p_det_info->obj_boxes[0].size; cnt_drinks++) {
        loc_x0 = p_det_info->obj_boxes[0].p[cnt_drinks].box[0];
        loc_y0 = p_det_info->obj_boxes[0].p[cnt_drinks].box[1];
        loc_x1 = p_det_info->obj_boxes[0].p[cnt_drinks].box[2];
        loc_y1 = p_det_info->obj_boxes[0].p[cnt_drinks].box[3];
        point0.x = loc_x0;
        point0.y = loc_y0;
        point1.x = loc_x1;
        point1.y = loc_y0;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0x00ff00);

        point0.x = loc_x0;
        point0.y = loc_y0;
        point1.x = loc_x0;
        point1.y = loc_y1;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0x00ff00);

        point0.x = loc_x1;
        point0.y = loc_y0;
        point1.x = loc_x1;
        point1.y = loc_y1;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0x00ff00);

        point0.x = loc_x0;
        point0.y = loc_y1;
        point1.x = loc_x1;
        point1.y = loc_y1;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0x00ff00);
    }
    for (int cnt_phones = 0; cnt_phones < p_det_info->obj_boxes[1].size; cnt_phones++) {
        loc_x0 = p_det_info->obj_boxes[1].p[cnt_phones].box[0];
        loc_y0 = p_det_info->obj_boxes[1].p[cnt_phones].box[1];
        loc_x1 = p_det_info->obj_boxes[1].p[cnt_phones].box[2];
        loc_y1 = p_det_info->obj_boxes[1].p[cnt_phones].box[3];
        point0.x = loc_x0;
        point0.y = loc_y0;
        point1.x = loc_x1;
        point1.y = loc_y0;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0x0000ff);

        point0.x = loc_x0;
        point0.y = loc_y0;
        point1.x = loc_x0;
        point1.y = loc_y1;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0x0000ff);

        point0.x = loc_x1;
        point0.y = loc_y0;
        point1.x = loc_x1;
        point1.y = loc_y1;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0x0000ff);

        point0.x = loc_x0;
        point0.y = loc_y1;
        point1.x = loc_x1;
        point1.y = loc_y1;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0x0000ff);
    }
    for (int cnt_smokes = 0; cnt_smokes < p_det_info->obj_boxes[2].size; cnt_smokes++) {
        loc_x0 = p_det_info->obj_boxes[2].p[cnt_smokes].box[0];
        loc_y0 = p_det_info->obj_boxes[2].p[cnt_smokes].box[1];
        loc_x1 = p_det_info->obj_boxes[2].p[cnt_smokes].box[2];
        loc_y1 = p_det_info->obj_boxes[2].p[cnt_smokes].box[3];
        point0.x = loc_x0;
        point0.y = loc_y0;
        point1.x = loc_x1;
        point1.y = loc_y0;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0x555555);

        point0.x = loc_x0;
        point0.y = loc_y0;
        point1.x = loc_x0;
        point1.y = loc_y1;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0x555555);

        point0.x = loc_x1;
        point0.y = loc_y0;
        point1.x = loc_x1;
        point1.y = loc_y1;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0x555555);

        point0.x = loc_x0;
        point0.y = loc_y1;
        point1.x = loc_x1;
        point1.y = loc_y1;
        osal_yuvg2d_drawline(format, addr, linestride, pframe_width, pframe_height,
            YUV_G2D_BREADRH_2, point0, point1, 0x555555);
    }
#ifdef SAVE_DRAW_YUV_SP
    if (drawframe_info.draw_fout) {
        fwrite((void *)addr[0], 1,  pframe_width*pframe_height, drawframe_info.draw_fout);
        fwrite((void *)addr[1], 1,  pframe_width*pframe_height/2, drawframe_info.draw_fout);
    }
#endif
    EI_MI_VBUF_FrameMunmap(drawFrame, VBUF_REMAP_MODE_CACHED);
}
#endif

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


static int check_face_in_limit_rect(int box[])
{
	if ((box[0] > 0) &&
		(box[1] > 0)
		&& (box[2] < (axframe_info.u32Width - 1))
		&& (box[3] < (axframe_info.u32Height - 1))) {

		return 0;
	}
	return -1;
}

int dms_update_para(float yawn, float pitch)
{
	OSAL_LOGE("********dms_update()******");
	int ret = -1;
	if (axframe_info.nn_hdl == NULL) {
		return ret;
	}
	axframe_info.m_nn_dms_cfg.warn_cfg.calib_yaw = yawn;
	axframe_info.m_nn_dms_cfg.warn_cfg.calib_pitch = pitch;
	ret = nn_dms_cmd(axframe_info.nn_hdl, NN_DMS_SET_PARAM, &axframe_info.m_nn_dms_cfg);
	if( ret == -1) {
		OSAL_LOGE("NN_DMS_SET_PARAM, failed failed!!");
	}
	return ret;
}

static void dms_cali_fun(nn_dms_out_t *out)
{
	nn_dms_face_t *face = NULL;
	if (m_dms_cali.cali_flag == DMS_CALI_START) {
		if (out->faces.size > 0) {
			OSAL_LOGE("**************");
			if (out->faces.driver_idx < 0 || out->faces.driver_idx >= out->faces.size) {
				OSAL_LOGE("out->faces.driver_idx %d\n", out->faces.driver_idx);
				/* if driver location not found , don't calib */
				return ;
			} else {
				face = &out->faces.p[out->faces.driver_idx];
			}
			int ck = check_face_in_limit_rect(face->box);
			OSAL_LOGE("ck : %d %d %f %f\n", ck, m_dms_cali.count_face_cb, face->face_attr.headpose[0], face->face_attr.headpose[1]);
			if (ck == -1) {
				m_dms_cali.count_face_cb = 0;
				return ;
			}
			if (m_dms_cali.count_face_cb == 0) {
				m_dms_cali.m_face_rect[0] = face->box[0];
				m_dms_cali.m_face_rect[1] = face->box[1];
				m_dms_cali.m_face_rect[2] = face->box[2];
				m_dms_cali.m_face_rect[3] = face->box[3];
				m_dms_cali.calib_yaw = 0.0;
				m_dms_cali.calib_pitch = 0.0;
				m_dms_cali.count_face_cb ++;
			} else {
				int t1 = compare_distance((float)m_dms_cali.m_face_rect[0], (float)m_dms_cali.m_face_rect[2],
								(float)face->box[0], (float)face->box[2]);
				if (t1 == 1) {
					m_dms_cali.calib_yaw += face->face_attr.headpose[0];
					m_dms_cali.calib_pitch += face->face_attr.headpose[1];
					if(m_dms_cali.count_face_cb == COUNT_CALIBRATE_NUM){
						m_dms_cali.calib_yaw = m_dms_cali.calib_yaw/COUNT_CALIBRATE_NUM;
						m_dms_cali.calib_pitch = m_dms_cali.calib_pitch/COUNT_CALIBRATE_NUM;
						m_dms_cali.count_face_cb = 0;
						m_dms_cali.cali_flag = DMS_CALI_END;
						m_dms_cali.auto_cali_count = 0;
						dms_save_cali_parm(m_dms_cali.calib_yaw, m_dms_cali.calib_pitch);
						dms_update_para(m_dms_cali.calib_yaw, m_dms_cali.calib_pitch);
						OSAL_LOGE("m_dms_cali.calib_yaw %f; m_dms_cali.calib_pitch %f\n", m_dms_cali.calib_yaw, m_dms_cali.calib_pitch);
						osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_CALIBRATE_SUCCESS, NULL, 0);
					} else {
						m_dms_cali.count_face_cb ++;
					}
				} else {
					m_dms_cali.count_face_cb = 0;
				}
			}
		} else {
			m_dms_cali.count_face_cb = 0;
		}
	}
}

int dms_warning(nn_dms_out_t *data)
{
	nn_dms_warn_status_t *warn_status = NULL;
	int64_t cur_time = osal_get_msec();
	uint32_t index;
#ifdef ALARM_THINING
	nn_dms_face_t *face = NULL;
	nn_dms_state_t *state = NULL;

	if (data->faces.driver_idx < 0 || data->faces.driver_idx >= data->faces.size) {
		OSAL_LOGE("data->faces.driver_idx %d\n", data->faces.driver_idx);
		/* if driver location not found , use idx 0 */
		face = &data->faces.p[0];
	} else {
		face = &data->faces.p[data->faces.driver_idx];
	}
	state = &face->face_attr.dms_state;
#endif
	warn_status = &data->warn_status;
	OSAL_LOGE("dms warn_status drink-%d call-%d smoke-%d distracted-%d fatigue_level-%d red_resist_glasses-%d cover-%d driver_leave-%d no_face_mask-%d no_face-%d no_belt-%d\n",
			warn_status->drink, warn_status->call, warn_status->smoke,
			warn_status->distracted, warn_status->fatigue_level,
			warn_status->red_resist_glasses, warn_status->cover,
			warn_status->driver_leave, warn_status->no_face_mask, warn_status->no_face, warn_status->no_belt);
	OSAL_LOGE("dms current frame warn_status cover_state:%d driver_leave:%d no_face:%d no_belt:%d\n",
		data->cover_state, data->driver_leave, data->no_face, data->no_belt);
	OSAL_LOGE("yawn_count-%d, eyeclose_count-%d %d %d %d\n", data->debug_info.yawn_count,
		data->debug_info.eyeclose_count[0], data->debug_info.eyeclose_count[1], data->debug_info.eyeclose_count[2],  data->debug_info.eyeclose_count[3]);
	if (data->faces.size > 0) {
		/* if (warn_status->drink) {
			index = LB_WARNING_DMS_DRINK-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_DRINK, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_DRINK;
			}
		} */
		if (warn_status->call) {
			index = LB_WARNING_DMS_CALL-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_CALL, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_CALL;
			}
		}
		if (warn_status->smoke) {
			index = LB_WARNING_DMS_SMOKE-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_SMOKE, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_SMOKE;
			}
		}
	#ifdef ALARM_THINING
		if (warn_status->fatigue_level == 2) {
			index = LB_WARNING_DMS_REST_LEVEL2-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_REST_LEVEL2, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_REST_LEVEL2;
			}
		} else if (warn_status->fatigue_level == 4) {
			index = LB_WARNING_DMS_REST_LEVEL4-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_REST_LEVEL4, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_REST_LEVEL4;
			}

		} else if (warn_status->fatigue_level == 6) {
			index = LB_WARNING_DMS_REST_LEVEL6-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_REST_LEVEL6, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_REST_LEVEL6;
			}
		} else if (warn_status->fatigue_level == 8) {
			index = LB_WARNING_DMS_REST_LEVEL8-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_REST_LEVEL8, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_REST_LEVEL8;
			}
		} else if (warn_status->fatigue_level == 10) {
			index = LB_WARNING_DMS_REST_LEVEL10-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_REST_LEVEL10, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_REST_LEVEL10;
			}
		}
		if (warn_status->distracted) {
			if (state->lookaround) {
				index = LB_WARNING_DMS_ATTATION-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_ATTATION_LOOKAROUND, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_DMS_ATTATION_LOOKAROUND;
				}
			} else if (state->lookup) {
				index = LB_WARNING_DMS_ATTATION-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_ATTATION_LOOKUP, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_DMS_ATTATION_LOOKUP;
				}
			} else if (state->lookdown){
				index = LB_WARNING_DMS_ATTATION-LB_WARNING_BEGIN-1;
				if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_ATTATION_LOOKDOWN, NULL, 0);
					warning_info.last_warn_time[index] = cur_time;
					warning_info.last_warn_status = LB_WARNING_DMS_ATTATION_LOOKDOWN;
				}
			}
		}
	#else
		if (warn_status->fatigue_level == 2) {
			index = LB_WARNING_DMS_REST-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_REST, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_REST;
			}
		} else if (warn_status->fatigue_level == 4) {
			index = LB_WARNING_DMS_REST-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_REST, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_REST;
			}

		} else if (warn_status->fatigue_level == 6) {
			index = LB_WARNING_DMS_REST-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_REST, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_REST;
			}
		} else if (warn_status->fatigue_level == 8) {
			index = LB_WARNING_DMS_REST-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_REST, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_REST;
			}
		} else if (warn_status->fatigue_level == 10) {
			index = LB_WARNING_DMS_REST-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_REST, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_REST;
			}
		}
		if (warn_status->distracted) {
			index = LB_WARNING_DMS_ATTATION-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_ATTATION, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_ATTATION;
			}
		}
	#endif
		if (warn_status->red_resist_glasses) {
			index = LB_WARNING_DMS_INFRARED_BLOCK-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_INFRARED_BLOCK, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_INFRARED_BLOCK;
			}
		}
		if (warn_status->no_belt) {
			index = LB_WARNING_DMS_NO_BELT-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_NO_BELT, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_NO_BELT;
			}
		}
	} else {
		if (warn_status->cover) {
			index = LB_WARNING_DMS_CAMERA_COVER-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_CAMERA_COVER, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				index = LB_WARNING_DMS_DRIVER_LEAVE-LB_WARNING_BEGIN-1;
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_CAMERA_COVER;
			}
		} 
		if (warn_status->driver_leave) {
			index = LB_WARNING_DMS_DRIVER_LEAVE-LB_WARNING_BEGIN-1;
			if ((cur_time - warning_info.last_warn_time[index]) > ALARM_INTERVAL) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_DRIVER_LEAVE, NULL, 0);
				warning_info.last_warn_time[index] = cur_time;
				index = LB_WARNING_DMS_CAMERA_COVER-LB_WARNING_BEGIN-1;
				warning_info.last_warn_time[index] = cur_time;
				warning_info.last_warn_status = LB_WARNING_DMS_DRIVER_LEAVE;
			}
		}
	}
	return 0;
}

static int nn_dms_result_cb(int event, void *data)
{
    nn_dms_out_t *out = NULL;
	nn_dms_cfg_t *cfg= NULL;
	uint32_t index;
	nn_dms_face_t *face = NULL;
	nn_dms_state_t *state = NULL;
	if (event==NN_DMS_AUTO_CALIBRATE_DONE) {
		printf("NN_DMS_AUTO_CALIBRATE_DONE\n");

		cfg= (nn_dms_cfg_t*)data;
		dms_save_cali_parm(cfg->warn_cfg.calib_yaw, cfg->warn_cfg.calib_pitch);
		osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_CALIBRATE_SUCCESS, NULL, 0);
		OSAL_LOGE("calib_yaw:%f,calib_pitch: %f calib_position %d %d %d %d\n", cfg->warn_cfg.calib_yaw, cfg->warn_cfg.calib_pitch, cfg->warn_cfg.calib_position[0],
		cfg->warn_cfg.calib_position[1], cfg->warn_cfg.calib_position[2], cfg->warn_cfg.calib_position[3]);
		return 0;
	}
		
	out = (nn_dms_out_t*)data;
	pthread_mutex_lock(&axframe_info.p_det_mutex);
	/* faces copy */
    if (axframe_info.p_det_info->faces.p) {
        free(axframe_info.p_det_info->faces.p);
        axframe_info.p_det_info->faces.p = NULL;
        axframe_info.p_det_info->faces.size = 0;
    }
    axframe_info.p_det_info->faces.p =
        malloc(out->faces.size * sizeof(nn_dms_face_t));
    if (axframe_info.p_det_info->faces.p) {
        memcpy(axframe_info.p_det_info->faces.p, out->faces.p,
            out->faces.size * sizeof(nn_dms_face_t));
        axframe_info.p_det_info->faces.size = out->faces.size;
		axframe_info.p_det_info->faces.driver_idx = out->faces.driver_idx;
    }
	/* persons copy */
	if (axframe_info.p_det_info->persons.p) {
        free(axframe_info.p_det_info->persons.p);
        axframe_info.p_det_info->persons.p = NULL;
        axframe_info.p_det_info->persons.size = 0;
    }
    axframe_info.p_det_info->persons.p =
        malloc(out->persons.size * sizeof(nn_dms_person_t));
    if (axframe_info.p_det_info->persons.p) {
        memcpy(axframe_info.p_det_info->persons.p, out->persons.p,
            out->persons.size * sizeof(nn_dms_person_t));
        axframe_info.p_det_info->persons.size = out->persons.size;
		axframe_info.p_det_info->persons.driver_idx = out->persons.driver_idx;
    }
	/* heads copy */
	if (axframe_info.p_det_info->heads.p) {
        free(axframe_info.p_det_info->heads.p);
        axframe_info.p_det_info->heads.p = NULL;
        axframe_info.p_det_info->heads.size = 0;
    }
    axframe_info.p_det_info->heads.p =
        malloc(out->heads.size * sizeof(nn_dms_head_t));
    if (axframe_info.p_det_info->heads.p) {
        memcpy(axframe_info.p_det_info->heads.p, out->heads.p,
            out->heads.size * sizeof(nn_dms_head_t));
        axframe_info.p_det_info->heads.size = out->heads.size;
		axframe_info.p_det_info->heads.driver_idx = out->heads.driver_idx;
    }
    axframe_info.p_det_info->cover_state = out->cover_state;
	axframe_info.p_det_info->driver_leave = out->driver_leave;
	axframe_info.p_det_info->no_face = out->no_face;
	axframe_info.p_det_info->warn_status = out->warn_status;
	/* obj copy */
    for (int i = 0; i < sizeof(axframe_info.p_det_info->obj_boxes)/sizeof(axframe_info.p_det_info->obj_boxes[0]); i++) {
        if (axframe_info.p_det_info->obj_boxes[i].p) {
            free(axframe_info.p_det_info->obj_boxes[i].p);
            axframe_info.p_det_info->obj_boxes[i].p = NULL;
            axframe_info.p_det_info->obj_boxes[i].size = 0;
        }
        if (out->obj_boxes[i].size == 0)
            continue;
		OSAL_LOGE("out->obj_boxes[%d].size:%d\n", i, out->obj_boxes[i].size);
        axframe_info.p_det_info->obj_boxes[i].p =
            malloc(sizeof(nn_dmsobj_box_t) *
            out->obj_boxes[i].size);
        if (axframe_info.p_det_info->obj_boxes[i].p) {
            memcpy(axframe_info.p_det_info->obj_boxes[i].p,
                out->obj_boxes[i].p,
                sizeof(nn_dmsobj_box_t) *
                out->obj_boxes[i].size);
            axframe_info.p_det_info->obj_boxes[i].size = out->obj_boxes[i].size;
        }
    }
	pthread_mutex_unlock(&axframe_info.p_det_mutex);
	if (axframe_info.action == DMS_CALIB) {  
			dms_cali_fun(out);
			index = LB_WARNING_DMS_CALIBRATE_FAIL-LB_WARNING_BEGIN-1;
			if ((m_dms_cali.cali_flag != DMS_CALI_END) &&
				((osal_get_msec() - warning_info.last_warn_time[index]) > CALI_FAIL_ALARM_INTERVAL)) {
				osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_CALIBRATE_FAIL, NULL, 0);
				warning_info.last_warn_time[index] = osal_get_msec();
			} else if (m_dms_cali.cali_flag == DMS_CALI_END) {
				axframe_info.action = DMS_RECOGNITION;
			}
	} else {
		if (out->faces.size > 0) {
			if (out->faces.driver_idx < 0 || out->faces.driver_idx >= out->faces.size) {
				OSAL_LOGE("out->faces.driver_idx %d\n", out->faces.driver_idx);
				face = &out->faces.p[0];
			} else {
				face = &out->faces.p[out->faces.driver_idx];
			}
			state = &face->face_attr.dms_state;
			/*  注意: 要传要检测的人脸坐标和关键点信息*/
			axframe_info.face.roi.x0 = face->box[0];
			axframe_info.face.roi.y0 = face->box[1];
			axframe_info.face.roi.x1 = face->box[2];
			axframe_info.face.roi.y1 = face->box[3];
			if (axframe_info.livingdet_flag == LIVING_DET_SINGLE_RGB) {
				int ret;
				ezax_face_kpts_t *face_kpts = axframe_info.face.roi.in_ex_inform;
				/* face_kpts 是人脸的关键点信息，先要识别到人脸，再做活体检测 */
				for (int i=0; i < 10; i++) {
					face_kpts->kpts[i] = (float)face->pts[i];
				}
				ret = nna_livingdet_process(axframe_info.livingdet_hdl, NULL, &axframe_info.face,
					axframe_info.livingdet_out);
				if (!ret) {
					axframe_info.score  = (ezax_livingdet_rt_t *)axframe_info.livingdet_out->out_ex_inform;
					OSAL_LOGE("rgb live_score: %d\n", axframe_info.score->live_score);
				}
			} else if(axframe_info.livingdet_flag == LIVING_DET_SINGLE_NIR) {
				int ret;
				ezax_face_kpts_t *face_kpts = axframe_info.face.roi.in_ex_inform;
				/* face_kpts 是人脸的关键点信息，先要识别到人脸，再做活体检测 */
				for (int i=0; i < 10; i++) {
					face_kpts->kpts[i] = (float)face->pts[i];
				}
				ret = nna_livingdet_process(axframe_info.livingdet_hdl, &axframe_info.face, NULL,
					axframe_info.livingdet_out);
				if (!ret) {
					axframe_info.score  = (ezax_livingdet_rt_t *)axframe_info.livingdet_out->out_ex_inform;
					OSAL_LOGE("nir live_score: %d\n", axframe_info.score->live_score);
				}
			}
			#ifdef SAVE_LIVINGDET_YUV_SP
		    if (axframe_info.livingdet_flag == LIVING_DET_SINGLE_RGB ||
				axframe_info.livingdet_flag == LIVING_DET_SINGLE_NIR) {
				if (axframe_info.livingdet_fout && axframe_info.face.img_handle.fmt == EZAX_YUV420_SP) {
					fwrite(axframe_info.face.img_handle.pVir,
						1,  axframe_info.face.img_handle.w*axframe_info.face.img_handle.h, axframe_info.livingdet_fout);
					fwrite(axframe_info.face.img_handle.pVir_UV,
						1,  axframe_info.face.img_handle.w*axframe_info.face.img_handle.h/2, axframe_info.livingdet_fout);
	            }
		    }
			#endif
			OSAL_LOGE("action: drink:%d,  call:%d, smoke:%d, yawn:%d, eyeclosed:%d\n",
	                state->drink, state->call, state->smoke, state->yawn, state->eyeclosed);
	        OSAL_LOGE("action: lookaround:%d, lookup:%d, lookdown:%d, red_resist_glasses:%d, cover_state:%d, no_face_mask:%d, headpose:%f %f %f\n",
	                state->lookaround, state->lookup, state->lookdown, face->face_attr.red_resist_glasses, out->cover_state, face->face_attr.no_face_mask,
	                face->face_attr.headpose[0], face->face_attr.headpose[1], face->face_attr.headpose[2]);
	    } else {
			OSAL_LOGE("cover_state:%d no_face: %d\n", out->cover_state, out->no_face);
		}
		dms_warning(out);
	}
    sem_post(&axframe_info.frame_del_sem);

    return 0;
}


int nn_dms_start(VIDEO_FRAME_INFO_S *frame, void *nn_hdl)
{
    EI_S32 s32Ret;
    static nn_dms_in_t m_dms_in;
	int pool_idx = 0;
	/* 获取保存的映射虚拟地址 */
	for (pool_idx = 0; pool_idx < VBUF_POOL_MAX_NUM; pool_idx++) {
		if (frame->u32PoolId == pool_info[pool_idx].Pool) {
			break;
		}
	}
	if (pool_idx >= VBUF_POOL_MAX_NUM)
		return 0;
    m_dms_in.img.y_phy = (void *)frame->stVFrame.u64PlanePhyAddr[0];
    m_dms_in.img.y = (void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[0];
	if (axframe_info.gray_flag == 1) {
		m_dms_in.img.uv_phy = (void *)axframe_info.gray_uv_phy_addr;
		m_dms_in.img.uv = (void *)axframe_info.gray_uv_vir_addr;
	} else {
		m_dms_in.img.uv_phy = (void *)frame->stVFrame.u64PlanePhyAddr[1];
		m_dms_in.img.uv = (void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[1];
	}
    m_dms_in.img.width = 1280;
    m_dms_in.img.height = 720;
    m_dms_in.img.format = YUV420_SP;
    m_dms_in.cover_det_en = 1;
    m_dms_in.red_resist_glasses_en = 1;
	m_dms_in.auto_calib_en = 0;
	if (axframe_info.livingdet_flag == LIVING_DET_SINGLE_RGB ||
		axframe_info.livingdet_flag == LIVING_DET_SINGLE_NIR) {
		if (axframe_info.livingdet_hdl) {
			axframe_info.face.img_handle.fmt = EZAX_YUV420_SP;
			axframe_info.face.img_handle.w = axframe_info.u32Width;//输入人脸图像的大小
			axframe_info.face.img_handle.h = axframe_info.u32Height;
			axframe_info.face.img_handle.stride = axframe_info.u32Width;
			axframe_info.face.img_handle.c = 3; // chanel,reserved
			axframe_info.face.nUID = 0;// mark id,reserved
			axframe_info.face.img_handle.pVir = (void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[0];
			axframe_info.face.img_handle.pPhy = (unsigned int)frame->stVFrame.u64PlanePhyAddr[0];
			axframe_info.face.img_handle.pVir_UV = (void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[1];
			axframe_info.face.img_handle.pPhy_UV = (unsigned int)frame->stVFrame.u64PlanePhyAddr[1];
			axframe_info.face.roi.x0 = 0;//检测人脸的有效区域
			axframe_info.face.roi.y0 = 0;
			axframe_info.face.roi.x1 = axframe_info.u32Width-1;
			axframe_info.face.roi.y1 = axframe_info.u32Height-1;
		}
	}
    if (nn_hdl != NULL) {
        s32Ret = nn_dms_cmd(nn_hdl, NN_DMS_SET_DATA, &m_dms_in);
		if (s32Ret < 0) {
			OSAL_LOGE("NN_DMS_SET_DATA failed %d!\n", s32Ret);
		}
    }
#ifdef SAVE_AX_YUV_SP
#if 0
    OSAL_LOGE("save yuv buffer sizey %d, stride %d %d %d\n", frame->stVFrame.u32PlaneSizeValid[0], frame->stVFrame.u32PlaneStride[0], frame->stCommFrameInfo.u32Width, frame->stCommFrameInfo.u32Height);
    OSAL_LOGE("save yuv buffer sizeuv %d, u32PlaneNum %d\n", frame->stVFrame.u32PlaneSizeValid[1], frame->stVFrame.u32PlaneNum);

    OSAL_LOGE("save yuv buffer addr %llx %llx %llx\n",frame->stVFrame.u64PlanePhyAddr[0],
            frame->stVFrame.u64PlanePhyAddr[1], frame->stVFrame.u64PlanePhyAddr[2]);

    OSAL_LOGE("save yuv buffer virtual addr %x %x %x\n",frame->stVFrame.ulPlaneVirAddr[0],
            frame->stVFrame.ulPlaneVirAddr[1], frame->stVFrame.ulPlaneVirAddr[2]);

    OSAL_LOGE("save yuv buffer ID %x\n",frame->u32BufferId);
#endif
    if (axframe_info.ax_fout) {
		if (m_dms_in.img.format == YUV420_SP) {
            fwrite((void *)m_dms_in.img.y, 1,
				m_dms_in.img.width*m_dms_in.img.height, axframe_info.ax_fout);
            fwrite((void *)m_dms_in.img.uv, 1,
				m_dms_in.img.width*m_dms_in.img.height/2, axframe_info.ax_fout);
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
#if 0
	char dmslib_ver[64]={0};
	nn_dms_cfg_t nn_dms_config_test = {0};
#endif
    axframe_info = (axframe_info_s *)p;
    OSAL_LOGE("get_axframe_proc bThreadStart = %d\n", axframe_info->bThreadStart);
#ifdef SAVE_AX_YUV_SP
    char srcfilename[FULLNAME_LEN_MAX];
	sprintf(srcfilename, "%s/dms_raw_ch%d.yuv", PATH_ROOT, axframe_info->chn);
    axframe_info->ax_fout = fopen(srcfilename, "wb+");
    if (axframe_info->ax_fout == NULL) {
        OSAL_LOGE("file open error1\n");
    }
#endif
#ifdef SAVE_LIVINGDET_YUV_SP
    char livingdet_filename[FULLNAME_LEN_MAX];
	if (axframe_info->livingdet_flag == LIVING_DET_SINGLE_NIR) {
		sprintf(livingdet_filename, "%s/living_nir_ch%d.yuv", PATH_ROOT, axframe_info->chn);
	} else {
		sprintf(livingdet_filename, "%s/living_rgb_ch%d.yuv", PATH_ROOT, axframe_info->chn);
	}
    axframe_info->livingdet_fout = fopen(livingdet_filename, "wb+");
    if (axframe_info->livingdet_fout == NULL) {
        OSAL_LOGE("file open error1\n");
    }
#endif

#ifdef AL_PRO_FRAMERATE_STATISTIC
	int64_t time1 = 0LL, time2 = 0LL, time_GetChnFrame1 = 0LL, time_GetChnFrame2 = 0LL,
	time_AlProFrame1 = 0LL, time_AlProFrame2 = 0LL;
	int64_t frame_cnt = 0;
#endif
    OSAL_LOGE("********start_nn_dms******\n");
    memset(&axframe_info->m_nn_dms_cfg, 0, sizeof(nn_dms_cfg_t));
    /* init nn_dms, cfg */
    axframe_info->m_nn_dms_cfg.img_width = axframe_info->u32Width; //算法输入图像宽
    axframe_info->m_nn_dms_cfg.img_height = axframe_info->u32Height; //算法输入图像高
    axframe_info->m_nn_dms_cfg.format = YUV420_SP;
    axframe_info->m_nn_dms_cfg.model_basedir = NN_DMS_PATH;
    axframe_info->m_nn_dms_cfg.cb_func = nn_dms_result_cb;
    axframe_info->m_nn_dms_cfg.dms_behave_en = 1; //哈欠 闭眼 开关
    axframe_info->m_nn_dms_cfg.headpose_en = 1; //左顾右盼 抬头 低头探测开关
    axframe_info->m_nn_dms_cfg.dms_obj_det_en = 1; //喝水 打电话 吸烟探测开关
    axframe_info->m_nn_dms_cfg.cover_det_en = 1; //摄像头遮住探测开关
    axframe_info->m_nn_dms_cfg.red_resist_glasses_en = 1; //支持佩戴防红外眼镜检测使能
    axframe_info->m_nn_dms_cfg.face_mask_en = 1; //支持佩戴口罩检测使能
    axframe_info->m_nn_dms_cfg.person_det_en = 1; //支持人型检测使能
    axframe_info->m_nn_dms_cfg.no_belt_en = 1; //支持安全带检测使能
    axframe_info->m_nn_dms_cfg.headface_det_en = 1; //支持人脸和人头检测，如果置1，选择headfacedet.bin模型，否则使用dmsfacedet.bin相关模型
	axframe_info->m_nn_dms_cfg.headface_det_resolution = 1; //1是大分辨率检测，占用资源多，检测效果好，0是小分辨率检测，占用资源小，效果没有1好
	axframe_info->m_nn_dms_cfg.auto_calib_en = 1; //DMS 自动校准使能位
	if (axframe_info->action == DMS_CALIB) {
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_yaw = 0.0;
    	axframe_info->m_nn_dms_cfg.warn_cfg.calib_pitch = 0.0;
		osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_CALIBRATE_START, NULL, 0);
	} else {
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_yaw = dms_get_cali_yaw();
    	axframe_info->m_nn_dms_cfg.warn_cfg.calib_pitch = dms_get_cali_pitch();
		/* if calib_position not set, driver detcect from whole region,other , driver detect for calib_position region */
		#if 0
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[0] = 0;
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[1] = 0;
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[2] = 640;
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[3] = 720;
		#endif
	}
    OSAL_LOGE(" calib_yaw:%f, calib_pitch:%f \n", axframe_info->m_nn_dms_cfg.warn_cfg.calib_yaw, axframe_info->m_nn_dms_cfg.warn_cfg.calib_pitch);
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.yaw_thresh[0] = -40.0; //驾驶员分神状态，相对于校准点的水平偏离门限角度，左负右正
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.yaw_thresh[1] = 40.0; //驾驶员分神状态，相对于校准点的水平偏离门限角度，左负右正
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.warn_interval = 5.0; //报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.count = 1; // 触发一次报警状态的act 状态个数
    axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.time = 3.0;// 3.0s  报警状态统计总时间
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.act.ratio = 0.7; // 0.7  百分比，单个act.time中检测到70%  act动作即触发一个act状态
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.act.time = 3.0; // 3.0s，单次act统计时间 

	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.pitch_thresh[0] = -20.0; //驾驶员分神状态，相对于校准点的垂直偏离门限角度，上正下负
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.pitch_thresh[1] = 30.0; //驾驶员分神状态，相对于校准点的垂直偏离门限角度，上正下负
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.warn_interval = 5.0; //报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.count = 1;// 触发一次报警状态的act 状态个数
    axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.time = 3.0;// 3.0s   报警状态统计总时间
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.act.ratio = 0.7;// 0.7  百分比，单个act.time中检测到70%  act动作即触发一个act状态
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.act.time = 3.0;// 3.0s，单次act统计时间 

    axframe_info->m_nn_dms_cfg.warn_cfg.drink_cfg.warn_interval = 5.0;//驾驶员喝水报警时间间隔
    axframe_info->m_nn_dms_cfg.warn_cfg.drink_cfg.count = 1;
    axframe_info->m_nn_dms_cfg.warn_cfg.drink_cfg.time = 2.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.drink_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.drink_cfg.act.time = 2.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.drink_det_thresh = 0.7; //det level, low:0.6;mid:0.7;high:0.8

	axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.warn_interval = 5.0;//驾驶员打电话报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.count = 1;
    axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.time = 3.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.act.time = 3.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.call_det_thresh = 0.7; //det level, low:0.6;mid:0.7;high:0.8

	axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.warn_interval = 5.0; //驾驶员抽烟报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.count = 1;
    axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.time = 2.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.act.time = 2.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.smoke_det_thresh = 0.7; //det level, low:0.6;mid:0.7;high:0.8

	axframe_info->m_nn_dms_cfg.warn_cfg.red_resist_glasses_cfg.warn_interval = 5.0;//驾驶员佩戴防红外眼镜报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.red_resist_glasses_cfg.count = 1;
    axframe_info->m_nn_dms_cfg.warn_cfg.red_resist_glasses_cfg.time = 5.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.red_resist_glasses_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.red_resist_glasses_cfg.act.time = 5.0;

	axframe_info->m_nn_dms_cfg.warn_cfg.face_mask_cfg.warn_interval = 5.0;//驾驶员是否 佩戴口罩报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.face_mask_cfg.count = 1;
	axframe_info->m_nn_dms_cfg.warn_cfg.face_mask_cfg.time = 5.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.face_mask_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.face_mask_cfg.act.time = 5.0;

	axframe_info->m_nn_dms_cfg.warn_cfg.cover_cfg.warn_interval = 5.0;//遮挡摄像头报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.cover_cfg.count = 1;
    axframe_info->m_nn_dms_cfg.warn_cfg.cover_cfg.time = 5.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.cover_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.cover_cfg.act.time = 5.0;

	axframe_info->m_nn_dms_cfg.warn_cfg.driver_leave_cfg.warn_interval = 5.0;//驾驶员离开报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.driver_leave_cfg.count = 1;
    axframe_info->m_nn_dms_cfg.warn_cfg.driver_leave_cfg.time = 5.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.driver_leave_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.driver_leave_cfg.act.time = 5.0;
	/* no_face_cfg 用来单独检测人脸的*/
	axframe_info->m_nn_dms_cfg.warn_cfg.no_face_cfg.warn_interval = 5.0;//未检测到人脸报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.no_face_cfg.count = 1;
    axframe_info->m_nn_dms_cfg.warn_cfg.no_face_cfg.time = 5.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.no_face_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.no_face_cfg.act.time = 5.0;
	/* 安全带检测*/
	axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.warn_interval = 5.0;//未检测到安全带报警间隔时间
	axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.count = 1;
    axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.time = 5.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.act.time = 5.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.belt_det_thresh = 0.35;//det level, low:0.25;mid:0.35;high:0.65
	/* level 2 fragile */
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.warn_interval = 10.0;//驾驶员打哈欠报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.count = 1; // time 内检测到act 状态>= 3次，触发报警
    axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.time = 3;// 300s ,报警状态统计总时间
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.act.ratio = 0.7;// 0.7  百分比，单个act.time中检测到70%  act动作即触发一个act状态
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.act.time = 3.0;// 2.0s，单次act统计时间 
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.yawn_headpose_range[0] = -60; //打哈欠报警的人脸头部姿态检测范围，左负右正，上正下负，数组索引顺序: 左右下上
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.yawn_headpose_range[1] = 60;
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.yawn_headpose_range[2] = -35;
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.yawn_headpose_range[3] = 45;
	/* level 4 fragile */
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[0].warn_interval = 10.0;//驾驶员眨眼报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[0].count = 3;// time 内检测到act 状态>=3次，触发报警
    axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[0].time = 2 * 60;// 120s ,报警状态统计总时间
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[0].act.ratio = 0.7;// 0.5  百分比，单个act.time中检测到50%  act动作即触发一个act状态
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[0].act.time = 1.0;// 1.0s，单次act统计时间
	/* level 6 fragile */
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].warn_interval = 10.0;//驾驶员眨眼报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].count = 6;// time 内检测到act 状态>=6次，触发报警
    axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].time = 1 * 60;// 60s ,报警状态统计总时间
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].act.ratio = 0.7;// 0.5 百分比，单个act.time中检测到50%  act动作即触发一个act状态
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].act.time = 1.0;// 1.0s，单次act统计时间
	/* level 8 fragile */
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].warn_interval = 10.0;//驾驶员闭眼报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].count = 1;// time 内检测到act 状态>=1次，触发报警
    axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].time = 2;// 2s ,报警状态统计总时间
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].act.ratio = 0.7;// 0.7  百分比，单个act.time中检测到70%  act动作即触发一个act状态
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].act.time = 2.0;// 2.0s，单次act统计时间 
	/* level 10 fragile */
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].warn_interval = 10.0;//驾驶员闭眼报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].count = 1;// time 内检测到act 状态>=1次，触发报警
    axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].time = 4.0;// 4s ,报警状态统计总时间
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].act.ratio = 0.7;// 0.7  百分比，单个act.time中检测到70%  act动作即触发一个act状态
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].act.time = 4.0;// 4.0s，单次act统计时间 

	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.eyeclosed_headpose_range[0] = -45; //闭眼报警的人脸头部姿态检测范围，左负右正，上正下负，数组索引顺序: 左右下上
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.eyeclosed_headpose_range[1] = 45;
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.eyeclosed_headpose_range[2] = -15;
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.eyeclosed_headpose_range[3] = 45;
	axframe_info->m_nn_dms_cfg.dump_cfg = 1;//dump 配置参数

    axframe_info->m_nn_dms_cfg.interest_box[0] = 0;//算法检测区域左上角X坐标
    axframe_info->m_nn_dms_cfg.interest_box[1] = 0;//算法检测区域左上角y坐标
    axframe_info->m_nn_dms_cfg.interest_box[2] = axframe_info->u32Width - 1;//算法检测区域右下角x坐标
    axframe_info->m_nn_dms_cfg.interest_box[3] = axframe_info->u32Height - 1;//算法检测区域右下角y坐标
    OSAL_LOGE("interest_box x1:%d,y1:%d,x2:%d,y2:%d \n", axframe_info->m_nn_dms_cfg.interest_box[0], axframe_info->m_nn_dms_cfg.interest_box[1],
        axframe_info->m_nn_dms_cfg.interest_box[2], axframe_info->m_nn_dms_cfg.interest_box[3]);

    /* open libdms.so*/
    nn_hdl = nn_dms_open(&axframe_info->m_nn_dms_cfg);
    if (nn_hdl == NULL) {
        OSAL_LOGE("nn_dms_open() failed!");
        return NULL;
    }
	axframe_info->nn_hdl = nn_hdl;
	OSAL_LOGE("axframe_info.livingdet_flag %d %s %d!\n", axframe_info->livingdet_flag, __FILE__, __LINE__);
	if (axframe_info->livingdet_flag == LIVING_DET_SINGLE_RGB) {
		axframe_info->livingdet_cfg.model_rootpath = LIVINGDET_PATH;
		axframe_info->livingdet_cfg.freq.id = 0;// nu 模块
		axframe_info->livingdet_cfg.freq.freq = 300;
		axframe_info->livingdet_cfg.single_rgb_model = 0;// use single rgb， only 0 valid
		axframe_info->livingdet_cfg.living_color_enable = 1;// support rgb color
		axframe_info->livingdet_cfg.mask_en = 0;
		axframe_info->livingdet_cfg.mask_living_enable = 0;
		axframe_info->livingdet_cfg.living_confidence_thr = 0;
		axframe_info->livingdet_cfg.livingdet_mode = 1; // 0:rgb+nir(live) 1:rgb(live) 2:nir(live)
		axframe_info->livingdet_cfg.rgb_exposure_time_rate = 0;
		axframe_info->livingdet_cfg.rgb_exposure_dark_rate = 0;
		axframe_info->livingdet_cfg.nir_avg_lum_rate = 0;
		axframe_info->livingdet_hdl = nna_livingdet_open(&axframe_info->livingdet_cfg);
		OSAL_LOGE("axframe_info->livingdet_hdl %p %s %d\n", axframe_info->livingdet_hdl, __FILE__, __LINE__);
	} else if (axframe_info->livingdet_flag == LIVING_DET_SINGLE_NIR) {
		axframe_info->livingdet_cfg.model_rootpath = LIVINGDET_PATH;
		axframe_info->livingdet_cfg.freq.id = 0;// nu 模块
		axframe_info->livingdet_cfg.freq.freq = 300;
		axframe_info->livingdet_cfg.single_rgb_model = 0;// use single rgb， only 0 valid
		axframe_info->livingdet_cfg.living_color_enable = 0;// 1: support rgb color 0: not support
		axframe_info->livingdet_cfg.mask_en = 0;
		axframe_info->livingdet_cfg.mask_living_enable = 0;
		axframe_info->livingdet_cfg.living_confidence_thr = 0;
		axframe_info->livingdet_cfg.livingdet_mode = 3; // 0:rgb+nir(live) 1:rgb(live) 2:nir+nir(live) 3:nir(live)
		axframe_info->livingdet_cfg.rgb_exposure_time_rate = 0;
		axframe_info->livingdet_cfg.rgb_exposure_dark_rate = 0;
		axframe_info->livingdet_cfg.nir_avg_lum_rate = 0;
		axframe_info->livingdet_hdl = nna_livingdet_open(&axframe_info->livingdet_cfg);
		OSAL_LOGE("axframe_info->livingdet_hdl %p %s %d\n", axframe_info->livingdet_hdl, __FILE__, __LINE__);
	}
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time1 = osal_get_msec();
#endif
	for (int i= 0; i < LB_WARNING_END-LB_WARNING_BEGIN-1; i++) {
		warning_info.last_warn_time[i] = osal_get_msec();
	}
	if (axframe_info->m_nn_dms_cfg.auto_calib_en == 1) {
		osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_CALIBRATE_START, NULL, 0);
	}
	if (axframe_info->gray_flag == 1) {
		EI_MI_MBASE_MemAlloc(&axframe_info->gray_uv_phy_addr, &axframe_info->gray_uv_vir_addr, "gray_scale_image", NULL, axframe_info->u32Width*axframe_info->u32Height/2);
		memset(axframe_info->gray_uv_vir_addr, 0x80, axframe_info->u32Width*axframe_info->u32Height/2);
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
        nn_dms_start(&axFrame, nn_hdl);
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
#if 0
		nn_dms_cmd(nn_hdl, NN_DMS_GET_VERSION, dmslib_ver);
		OSAL_LOGE("dmslib_ver %s\n", dmslib_ver);
		if (axframe_info->m_nn_dms_cfg.warn_cfg.face_mask_cfg.warn_interval != 20) {
			axframe_info->m_nn_dms_cfg.warn_cfg.face_mask_cfg.warn_interval = 20;
			nn_dms_cmd(nn_hdl, NN_DMS_SET_PARAM, &axframe_info->m_nn_dms_cfg);
			memset(&nn_dms_config_test,0,sizeof(nn_dms_cfg_t));
			nn_dms_cmd(nn_hdl, NN_DMS_GET_PARAM, &nn_dms_config_test);
			OSAL_LOGE("get face_mask_cfg %f %f\n", nn_dms_config_test.warn_cfg.face_mask_cfg.warn_interval, nn_dms_config_test.warn_cfg.face_mask_cfg.time);
		}
#endif
        EI_MI_VISS_ReleaseChnFrame(axframe_info->dev, axframe_info->chn, &axFrame);
    }
    EI_MI_VISS_DisableChn(axframe_info->dev, axframe_info->chn);
	if (axframe_info->livingdet_flag == LIVING_DET_SINGLE_RGB ||
		axframe_info->livingdet_flag == LIVING_DET_SINGLE_NIR) {
		if (axframe_info->livingdet_hdl) {
			nna_livingdet_close(axframe_info->livingdet_hdl);
		}
	}
    nn_dms_close(nn_hdl);
#ifdef SAVE_AX_YUV_SP
    if (axframe_info->ax_fout) {
        fclose(axframe_info->ax_fout);
        axframe_info->ax_fout = NULL;
    }
#endif
#ifdef SAVE_LIVINGDET_YUV_SP
	if (axframe_info->livingdet_fout) {
		fclose(axframe_info->livingdet_fout);
		axframe_info->livingdet_fout = NULL;
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

    sprintf(srcfilename, "%s/dms_draw_raw_ch%d.yuv", PATH_ROOT, drawframe_info->chn);
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
#ifdef RGN_DRAW
		dms_draw_location_rgn(&drawFrame, axframe_info.p_det_info);
#else
        dms_draw_location(&drawFrame, axframe_info.p_det_info);
#endif
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
			case LB_WARNING_DMS_DRIVER_LEAVE:
				warn_tone_play("/usr/share/out/res/dms_warning/weijiancedaojiashiyuan.wav");
				break;
			case LB_WARNING_DMS_NO_BELT:
				warn_tone_play("/usr/share/out/res/dms_warning/qingjianquandai.wav");
				break;
			case LB_WARNING_DMS_CALL:
				warn_tone_play("/usr/share/out/res/dms_warning/qingwudadianhua.wav");
				break;
			case LB_WARNING_DMS_DRINK:
				warn_tone_play("/usr/share/out/res/dms_warning/qingwuheshui.wav");
				break;
			case LB_WARNING_DMS_SMOKE:
				warn_tone_play("/usr/share/out/res/dms_warning/qingwuxiyan.wav");
				break;
#ifdef ALARM_THINING
			case LB_WARNING_DMS_ATTATION_LOOKAROUND:
				warn_tone_play("/usr/share/out/res/dms_warning/qingwuzuoguyoupan.wav");
				break;
			case LB_WARNING_DMS_ATTATION_LOOKUP:
				warn_tone_play("/usr/share/out/res/dms_warning/qingwutaitoujiashi.wav");
				break;
			case LB_WARNING_DMS_ATTATION_LOOKDOWN:
				warn_tone_play("/usr/share/out/res/dms_warning/qingwuditoujiashi.wav");
				break;
			case LB_WARNING_DMS_REST_LEVEL2:
				warn_tone_play("/usr/share/out/res/dms_warning/pilaodengji2.wav");
				break;
			case LB_WARNING_DMS_REST_LEVEL4:
				warn_tone_play("/usr/share/out/res/dms_warning/pilaodengji4.wav");
				break;
			case LB_WARNING_DMS_REST_LEVEL6:
				warn_tone_play("/usr/share/out/res/dms_warning/pilaodengji6.wav");
				break;
			case LB_WARNING_DMS_REST_LEVEL8:
				warn_tone_play("/usr/share/out/res/dms_warning/pilaodengji8.wav");
				break;
			case LB_WARNING_DMS_REST_LEVEL10:
				warn_tone_play("/usr/share/out/res/dms_warning/pilaodengji10.wav");
				break;
#else
			case LB_WARNING_DMS_ATTATION:
				warn_tone_play("/usr/share/out/res/dms_warning/qingzhuanxinjiashi.wav");
				break;
			case LB_WARNING_DMS_REST:
				warn_tone_play("/usr/share/out/res/dms_warning/qingzhuyixiuxi.wav");
				break;
#endif
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
			case LB_WARNING_SD_PLUGIN:
				stream_para[0].sdcard_status_flag = 1;
				break;
			case LB_WARNING_SD_PLUGOUT:
				stream_para[0].bThreadStart = EI_FALSE;
				stream_para[0].sdcard_status_flag = 0;
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
		OSAL_LOGE("creat file faile\n");
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
		if ((stStream.pstPack.u32Len[0] > 0) && (EI_TRUE == stream_para[0].bThreadStart)) {
            fwrite(stStream.pstPack.pu8Addr[0], 1, stStream.pstPack.u32Len[0],
				sdr_venc_para->flip_out);
        }
        if ((stStream.pstPack.u32Len[1] > 0) &&  (EI_TRUE == stream_para[0].bThreadStart)) {
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
		if (!stream_para[0].sdcard_status_flag && strstr(PATH_ROOT,"card")) {
			osal_unmount_storage("/mnt/card");
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
	VISS_CHN_ATTR_S       stVissChnAttr = {0};
	VISS_DEV_ATTR_S  stVissDevAttr = {0};
#ifndef SAVE_DRAW_AX_H265
	static VISS_EXT_CHN_ATTR_S stExtChnAttr;
#endif
    EI_S32 s32Ret = EI_FAILURE;
    VO_VIDEO_LAYER_ATTR_S stVoLayerAttr = {0};
    VO_CHN_ATTR_S stVoChnAttr = {0};
	SAMPLE_VENC_CONFIG_S venc_config = {0};
	SNS_IMAGE_S pstSts = {0};
	int loop_time;
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
    stVbConfig.astCommPool[0].u32BufCnt = 24;
    stVbConfig.astCommPool[0].enRemapMode = VBUF_REMAP_MODE_NOCACHE;

	stVbConfig.astFrameInfo[1].enFrameType = MDP_FRAME_TYPE_COMMON;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Align = 32;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width = 1280;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height = 720;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stVbConfig.astCommPool[1].u32BufCnt = 48;
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
		/* 这里在初始化的时候保存映射的虚拟地址，是避免在算法使用的时候映射会占用cpu 资源*/
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
	EI_MI_VISS_GetSensorImage(dev, &pstSts);
	OSAL_LOGW("br: %u, %u, %u, %u, %u, %d\n", pstSts.brightness[chn], pstSts.contrast[chn],
		pstSts.saturation[chn], pstSts.hue[chn], pstSts.sharpness[chn], __LINE__);
	if (axframe_info.brightness) {
		pstSts.brightness[chn] = axframe_info.brightness;
		EI_MI_VISS_SetSensorImage(dev, &pstSts);
		OSAL_LOGW("br: %u, %u, %u, %u, %u, %d\n", pstSts.brightness[chn], pstSts.contrast[chn],
		pstSts.saturation[chn], pstSts.hue[chn], pstSts.sharpness[chn], __LINE__);
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
    axframe_info.p_det_info = malloc(sizeof(nn_dms_out_t));
    if (!axframe_info.p_det_info) {
		OSAL_LOGW("axframe_info.p_det_info malloc failed!\n");
		goto exit1;
    }
	if (axframe_info.action == DMS_CALIB) {
		m_dms_cali.cali_flag = DMS_CALI_START;
	}
    memset(axframe_info.p_det_info, 0x00, sizeof(nn_dms_out_t));
	if (axframe_info.livingdet_flag == LIVING_DET_SINGLE_RGB ||
		axframe_info.livingdet_flag == LIVING_DET_SINGLE_NIR) {
		axframe_info.livingdet_out = (ezax_rt_t *)calloc(1, sizeof(ezax_rt_t));
		if (!axframe_info.livingdet_out) {
			OSAL_LOGW("axframe_info.livingdet_out malloc failed!\n");
			goto exit1;
		}
		/* 活体的输出结果，live_score大于0为活体，小于0为假体*/
		axframe_info.livingdet_out->out_ex_inform = (ezax_livingdet_rt_t *)calloc(1, sizeof(ezax_livingdet_rt_t));
		if (!axframe_info.livingdet_out->out_ex_inform) {
			OSAL_LOGW("axframe_info.livingdet_out->out_ex_inform malloc failed!\n");
			goto exit1;
		}
		memset(&axframe_info.face, 0x00, sizeof(axframe_info.face));
		/* 人脸的关键点信息，先要识别到人脸，再做活体检测*/
		axframe_info.face.roi.in_ex_inform = (ezax_face_kpts_t *)calloc(1, sizeof(ezax_face_kpts_t));
		if (!axframe_info.face.roi.in_ex_inform) {
			OSAL_LOGW("axframe_info.face.roi.in_ex_inform malloc failed!\n");
			goto exit1;
		}
	}
	OSAL_LOGW("begin init var %s %d\n", __FILE__, __LINE__);
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
#ifdef RGN_DRAW
	drawframe_info.last_draw_num = RGN_RECT_NUM;
	s32Ret = rgn_start();
	if (s32Ret == EI_FAILURE) {
		 OSAL_LOGE("rgn_start failed with %#x!\n", s32Ret);
	}
#endif
    start_get_drawframe(&drawframe_info);
    venc_config.enInputFormat   = stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat;
    venc_config.u32width	    = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
    venc_config.u32height	    = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
    venc_config.u32bitrate      = SDR_SAMPLE_VENC_BITRATE;
    venc_config.u32srcframerate = SDR_VISS_FRAMERATE;
    venc_config.u32dstframerate = SDR_VISS_FRAMERATE;
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
#ifndef SAVE_DRAW_AX_H264
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
	OSAL_LOGW("mdp start rec\n");
	while (stream_para[0].exit_flag != EI_TRUE) {
		if (!stream_para[0].sdcard_status_flag && strstr(PATH_ROOT,"card")) {
			usleep(100 * 1000);
			OSAL_LOGE("stream_para[0].sdcard_status_flag: %d\n", stream_para[0].sdcard_status_flag);
			continue;
		}
	    s32Ret = SAMPLE_COMM_VENC_Start(stream_para[0].VeChn, PT_H265,
	                SAMPLE_RC_CBR, &venc_config,
	                COMPRESS_MODE_NONE, VENC_GOPMODE_NORMAL_P);
	    if (EI_SUCCESS != s32Ret) {
	        OSAL_LOGE("%s %d %d ret:%d \n", __func__, __LINE__, stream_para[0].VeChn, s32Ret);
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
	        OSAL_LOGW("loop_time %d\n", loop_time);
			if (!stream_para[0].sdcard_status_flag && strstr(PATH_ROOT,"card")) {
				osal_unmount_storage("/mnt/card");
				break;
			}
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
	OSAL_LOGW("dms wait exit!\n");
    axframe_info.bThreadStart = EI_FALSE;
    if (axframe_info.gs_framePid)
		pthread_join(axframe_info.gs_framePid, NULL);
    sem_destroy(&axframe_info.frame_del_sem);

	pthread_mutex_destroy(&axframe_info.p_det_mutex);
	if (drawframe_info.is_disp == EI_TRUE) {
		watermark_exit(VENC_RGN_WM_NUM, MOD_PREVIEW);
	}
	watermark_exit(VENC_RGN_WM_NUM, MOD_VENC);
#ifdef RGN_DRAW
	rgn_stop();
#endif
    OSAL_LOGW("dms start exit!\n");
exit1:
	stVissConfig.astVissInfo[0].stChnInfo.enWorkMode = camera_info[0].chn_num; //add for mashup 4ch to disable viss ch
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
	if (axframe_info.gray_flag == 1) {
		EI_MI_MBASE_MemFree(axframe_info.gray_uv_phy_addr, axframe_info.gray_uv_vir_addr);
	}
    EI_MI_MLINK_Exit();
    EI_MI_VBUF_Exit();
    EI_MI_MBASE_Exit();
	if (axframe_info.p_det_info->faces.p) {
        free(axframe_info.p_det_info->faces.p);
        axframe_info.p_det_info->faces.p = NULL;
        axframe_info.p_det_info->faces.size = 0;
    }
	if (axframe_info.p_det_info->persons.p) {
        free(axframe_info.p_det_info->persons.p);
        axframe_info.p_det_info->persons.p = NULL;
        axframe_info.p_det_info->persons.size = 0;
    }
	if (axframe_info.p_det_info->heads.p) {
        free(axframe_info.p_det_info->heads.p);
        axframe_info.p_det_info->heads.p = NULL;
        axframe_info.p_det_info->heads.size = 0;
    }
    if (axframe_info.p_det_info) {
        free(axframe_info.p_det_info);
        axframe_info.p_det_info = NULL;
    }
	if (axframe_info.livingdet_flag == LIVING_DET_SINGLE_RGB ||
		axframe_info.livingdet_flag == LIVING_DET_SINGLE_NIR) {
		if (axframe_info.face.roi.in_ex_inform) {
			free(axframe_info.face.roi.in_ex_inform);
			axframe_info.face.roi.in_ex_inform = NULL;
		}
		if (axframe_info.livingdet_out->out_ex_inform) {
			free(axframe_info.livingdet_out->out_ex_inform);
			axframe_info.livingdet_out->out_ex_inform = NULL;
		}
		if (axframe_info.livingdet_out) {
			free(axframe_info.livingdet_out);
			axframe_info.livingdet_out = NULL;
		}
	}

    OSAL_LOGW("show_1ch end exit!\n");

    return s32Ret;

}

int event_process(sys_event_e ev)
{
	OSAL_LOGW("Receive %d event", ev);
	switch (ev) {
	case SYS_EVENT_SDCARD_IN:
		osal_mq_send(warning_info.mq_id, LB_WARNING_SD_PLUGIN, NULL, 0);
	break;
	case SYS_EVENT_SDCARD_OUT:
		osal_mq_send(warning_info.mq_id, LB_WARNING_SD_PLUGOUT, NULL, 0);
	break;
	default:
	break;
	}

	return 0;
}
int event_monitor_init()
{
	monitor_dev_t dev;

	/* sdcard storage */
	OSAL_LOGI("register sdcard detect.");
	memset(&dev, 0, sizeof(dev));
	strcpy(dev.name, "sdcard");
	dev.type = EVENT_DEV_TYPE_SDCARD;
	dev.gpio = -1;
	osal_event_monitor_register(&dev);
#if 0
	/* usb power */
	OSAL_LOGI("register usb power detect.");
	memset(&dev, 0, sizeof(dev));
	strcpy(dev.name, "usb power");
	dev.type = EVENT_DEV_TYPE_POWER;
	dev.gpio = -1; /* sio0 */
	osal_event_monitor_register(&dev);

	/* acc */
	OSAL_LOGI("register acc insert detect.");
	memset(&dev, 0, sizeof(dev));
	strcpy(dev.name, "acc");
	dev.type = EVENT_DEV_TYPE_GPIO_ACC;
	dev.gpio = dev_monitor_val.acc_gpio; /* SIO5 */
	osal_event_monitor_register(&dev);
	if (dev_monitor_val.back_gpio[0]) {
		/* back car */
		OSAL_LOGI("register back car detect.");
		memset(&dev, 0, sizeof(dev));
		strcpy(dev.name, "back car");
		dev.type = EVENT_DEV_TYPE_GPIO_BACK0;
		dev.gpio = dev_monitor_val.back_gpio[0];
		osal_event_monitor_register(&dev);
	}
	if (dev_monitor_val.back_gpio[1]) {
		/* back car */
		OSAL_LOGI("register back car detect.");
		memset(&dev, 0, sizeof(dev));
		strcpy(dev.name, "back car");
		dev.type = EVENT_DEV_TYPE_GPIO_BACK1;
		dev.gpio = dev_monitor_val.back_gpio[1];
		osal_event_monitor_register(&dev);
	}
	if (dev_monitor_val.back_gpio[2]) {
		/* back car */
		OSAL_LOGI("register back car detect.");
		memset(&dev, 0, sizeof(dev));
		strcpy(dev.name, "back car");
		dev.type = EVENT_DEV_TYPE_GPIO_BACK2;
		dev.gpio = dev_monitor_val.back_gpio[2];
		osal_event_monitor_register(&dev);
	}
	if (dev_monitor_val.back_gpio[3]) {
		/* back car */
		OSAL_LOGI("register back car detect.");
		memset(&dev, 0, sizeof(dev));
		strcpy(dev.name, "back car");
		dev.type = EVENT_DEV_TYPE_GPIO_BACK3;
		dev.gpio = dev_monitor_val.back_gpio[3];
		osal_event_monitor_register(&dev);
	}
	if (dev_monitor_val.back_gpio[4]) {
		/* back car */
		OSAL_LOGI("register back car detect.");
		memset(&dev, 0, sizeof(dev));
		strcpy(dev.name, "back car");
		dev.type = EVENT_DEV_TYPE_GPIO_BACK4;
		dev.gpio = dev_monitor_val.back_gpio[4];
		osal_event_monitor_register(&dev);
	}

	/* usb storage */
	OSAL_LOGI("register udisk detect.");
	memset(&dev, 0, sizeof(dev));
	strcpy(dev.name, "udisk");
	dev.type = EVENT_DEV_TYPE_UDISK;
	dev.gpio = -1;
	osal_event_monitor_register(&dev);

	/* acc */
	OSAL_LOGI("register acc insert detect.");
	memset(&dev, 0, sizeof(dev));
	strcpy(dev.name, "acc");
	dev.type = EVENT_DEV_TYPE_GPIO_ACC;
	dev.gpio = dev_monitor_val.acc_gpio; /* SIO5 */
	osal_event_monitor_register(&dev);

	/* back car */
	OSAL_LOGI("register back car detect.");
	memset(&dev, 0, sizeof(dev));
	strcpy(dev.name, "back car");
	dev.type = EVENT_DEV_TYPE_GPIO_BACK;
	dev.gpio = dev_monitor_val.back_gpio;  /* GPIOE7 */
	osal_event_monitor_register(&dev);

	/* gsensor det */
	OSAL_LOGI("register gsensor detect.");
	memset(&dev, 0, sizeof(dev));
	strcpy(dev.name, "gsensor det");
	dev.type = EVENT_DEV_TYPE_GSENSOR;
	dev.gpio = -1;
	osal_event_monitor_register(&dev);
	#endif
	osal_event_monitor_start(event_process);

	return 0;
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
	OSAL_LOGW("[%d] mdp_dms_nn start v1.0rc1_202309081400\n", __LINE__);
	drawframe_info.is_disp = EI_TRUE;
	axframe_info.gray_flag = EI_TRUE;
    while ((c = getopt(argc, argv, "d:c:r:s:a:b:l:g:p:h:")) != -1) {
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
		case 'b':
			axframe_info.brightness = (unsigned int)strtol(argv[optind - 1], NULL, 10);
 			break;
		case 'l':
			axframe_info.livingdet_flag = (unsigned int)strtol(argv[optind - 1], NULL, 10);
			break;
		case 'g':
			axframe_info.gray_flag = (unsigned int)strtol(argv[optind - 1], NULL, 10);
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
	OSAL_LOGW("dev %d chn %d rot %d sns %d action %d brightness %d livingdet_flag %d gray_flag %d %s %d\n",
	dev, chn, rot, sns, axframe_info.action, axframe_info.brightness, axframe_info.livingdet_flag, axframe_info.gray_flag, __FILE__, __LINE__);
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
	event_monitor_init();
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
	OSAL_LOGW("[%d] mdp_dms_nn EXIT\n", __LINE__);
	exit(EXIT_SUCCESS);
EXIT:
    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
