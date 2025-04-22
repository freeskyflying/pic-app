#ifndef _BSD_NN_H_
#define _BSD_NN_H_

#include <pthread.h>
#include <semaphore.h>
#include "algo_common.h"
#include "nn_bsd_api.h"

#ifdef __cplusplus
extern "C"{
#endif

#define NN_BSD_PATH	 "/usr/share/ax/bsd"

#define BSD_RGN_ARRAY_NUM 2
#define BSD_RGN_RECT_NUM 16 /* rect max num */
#define BSD_RGN_LINE_NUM 16 /* line max num */

typedef enum
{

	BSD_CALI_START = 1,
	BSD_CALI_END,
	BSD_CALI_MAX,
	
} bsd_cali_flag_e;

typedef enum
{
	
	BSD_RECOGNITION,
	BSD_CALIB,
	BSD_ACTION_END,
	
} BSD_ACTION;

typedef enum 
{

	LEFT_LANE_LINE,
	RIGHT_LANE_LINE,
	VANISH_Y_LINE,
	VANISH_X_LINE,
	BOTTOM_Y_LINE,
	
} BSD_LINE;
	
typedef struct
{

	float vanish_point_x;
	float vanish_point_y;
	float bottom_line_y;
	bsd_cali_flag_e cali_flag;
	BSD_LINE bsd_calibline;
	unsigned int bsd_calibstep;
	int auto_calib_flag;
	
} bsd_calibrate_para_t;

typedef struct
{

	BSD_ACTION action;
    EI_BOOL bThreadStart;

	pthread_mutex_t p_det_mutex;
    sem_t frame_del_sem;
	nn_bsd_cfg_t m_nn_bsd_cfg;
	nn_bsd_out_t *p_det_info;
	void *nn_hdl;

    VISS_CHN phyChn;
    VISS_DEV dev;
    VISS_CHN chn;

    EI_U32 u32Width;
    EI_U32 u32Height;
	EI_U32 frameRate;
	
	RGN_ATTR_S stRegion[2];
	RGN_CHN_ATTR_S stRgnChnAttr[BSD_RGN_ARRAY_NUM];
	RGN_HANDLE Handle[BSD_RGN_ARRAY_NUM];
	RECTANGLEEX_ARRAY_SUB_ATTRS stRectArray[BSD_RGN_RECT_NUM];
	LINEEX_ARRAY_SUB_ATTRS stLineArray[BSD_RGN_LINE_NUM];
	MDP_CHN_S stRgnChn;
	int last_draw_rect_num;
	int last_draw_line0_num;
	int last_draw_line1_num;

	EI_U32 u32scnWidth;
	EI_U32 u32scnHeight;

    pthread_t gs_framePid;

} bsd_nn_t;

int bsd_nn_init(int vissDev, int vissChn, int chn, int width, int height);

void bsd_nn_stop(void);

#ifdef __cplusplus
}
#endif

#endif

