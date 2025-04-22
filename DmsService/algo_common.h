#ifndef _ALGO_COMMON_H_
#define _ALGO_COMMON_H_

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
#include "nn_adas_api.h"
#include "nn_headfacedet_api.h"
#include "eznn_api.h"

#include "utils.h"

#define NN_DMS_PATH	 		"/usr/share/ax/dms"
#define LIVINGDET_PATH	 	"/usr/share/ax/liveness"

#define NN_ADAS_PATH	 "/usr/share/ax/adas"

#define MQ_MSG_NUM 128 /* message queue number */
#define MQ_MSG_LEN 128 /* every message max lenght */

#define VBUF_POOL_MAX_NUM 4
#define VBUF_BUFFER_MAX_NUM 64

#define AL_SEND_FRAMERATE 15 /* send al frame rate */

#define compare_distance(a0, a1, a2, a3) (((a3-a2)/(a1-a0))> 0.9 && ((a3-a2)/(a1-a0))< 1.1)?1:0

#define CALI_FAIL_ALARM_INTERVAL 20000 /* same warning message interval time unit:ms */

typedef enum _adas_cali_flag_{
	ADAS_CALI_START = 1,
	ADAS_CALI_END,
	ADAS_CALI_MAX,
} adas_cali_flag_e;

typedef enum _dms_cali_flag_{
	DMS_CALI_START = 1,
	DMS_CALI_END,
	DMS_CALI_MAX,
} dms_cali_flag_e;

typedef enum _DMS_ACTION_ 
{
	DMS_RECOGNITION,
	DMS_CALIB,
	DMS_ACTION_END,
} DMS_ACTION;

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

typedef enum _dms_livingdet_flag_{
	LIVING_DET_NONE,
	LIVING_DET_SINGLE_RGB,
	LIVING_DET_SINGLE_NIR,
	LIVING_DET_MAX,
} dms_livingdet_flag_e;

typedef enum _WARNING_MSG_TYPE_ {
	LB_WARNING_BEGIN = 0xF000,
	LB_WARNING_DMS_DRIVER_LEAVE,
	LB_WARNING_DMS_CALL,
	LB_WARNING_DMS_DRINK,
	LB_WARNING_DMS_SMOKE,
	LB_WARNING_DMS_ATTATION,
	LB_WARNING_DMS_REST,
	LB_WARNING_DMS_CAMERA_COVER,
	LB_WARNING_DMS_INFRARED_BLOCK,
//	LB_WARNING_DMS_CALIBRATE_START,
//	LB_WARNING_DMS_CALIBRATE_SUCCESS,
//	LB_WARNING_DMS_CALIBRATE_FAIL,
	
	LB_WARNING_ADAS_LAUNCH,//前车启动
	LB_WARNING_ADAS_DISTANCE,//保持车距
	LB_WARNING_ADAS_COLLIDE,//注意碰撞
	LB_WARNING_ADAS_PRESSURE,//车道偏移
	LB_WARNING_ADAS_PEDS,//小心行人
	LB_WARNING_ADAS_CHANGLANE,//车辆变道
//	LB_WARNING_ADAS_CALIBRATE_START,//校准开始，not used, 用于手动校准
//	LB_WARNING_ADAS_CALIBRATE_SUCCESS,//校准成功, not used, 用于手动校准
//	LB_WARNING_ADAS_CALIBRATE_FAIL,//校准失败，not used, 用于手动校准

	LB_WARNING_USERDEF,//自定义系统消息
	LB_WARNING_END,
} WARNING_MSG_TYPE;

typedef struct _warning_info_s {
    EI_BOOL bThreadStart;
    int mq_id;
	int64_t last_warn_time[LB_WARNING_END-LB_WARNING_BEGIN-1];
	int last_warn_status;
    pthread_t g_Pid;
} warning_info_s;

typedef struct _dms_calibrate_para_ {
	float calib_yaw;
	float calib_pitch;
	int count_face_cb;
	dms_cali_flag_e cali_flag;
	int auto_cali_count;
	int m_face_rect[4];
} dms_calibrate_para_t;

typedef struct _adas_calibrate_para_ {
	float vanish_point_x;
	float vanish_point_y;
	float bottom_line_y;
	adas_cali_flag_e cali_flag;
	ADAS_LINE adas_calibline;
	unsigned int adas_calibstep;
	int auto_calib_flag;
} adas_calibrate_para_t;

typedef struct
{
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
	FILE *ax_fout;
	FILE *livingdet_fout;
} axframe_info_s;

typedef struct
{
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
} adasaxframe_info_s;

typedef struct tag_warn_msg {
	int32_t		type;
	int32_t		len;
	char		data[120];
} tag_warn_msg_t;

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

int al_warning_init(void);

int al_warning_proc(WARNING_MSG_TYPE warnType, int extra, int extra2);

void al_warning_msg(int msg);

int dms_nn_init(int vissDev, int vissChn, int width, int height);

void dms_nn_stop(void);

int adas_nn_init(int vissDev, int vissChn, int width, int height);

void adas_nn_stop(void);

#endif
