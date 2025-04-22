#ifndef _ADAS_NN_H_
#define _ADAS_NN_H_

#include <pthread.h>
#include <semaphore.h>
#include "algo_common.h"
#include "nn_adas_api.h"

#include "mdp_al_config.h"

#ifdef __cplusplus
extern "C"{
#endif

#define NN_ADAS_PATH	 "/usr/share/ax/adas"

#define ADAS_RGN_ARRAY_NUM 2
#define ADAS_RGN_RECT_NUM 16 /* rect max num */
#define ADAS_RGN_LINE_NUM 16 /* line max num */

typedef enum
{

	ADAS_CALI_START = 1,
	ADAS_CALI_END,
	ADAS_CALI_MAX,
	
} adas_cali_flag_e;

typedef enum
{
	
	ADAS_RECOGNITION,
	ADAS_CALIB,
	ADAS_ACTION_END,
	
} ADAS_ACTION;

typedef enum 
{

	LEFT_LANE_LINE,
	RIGHT_LANE_LINE,
	VANISH_Y_LINE,
	VANISH_X_LINE,
	BOTTOM_Y_LINE,
	
} ADAS_LINE;
	
typedef struct
{

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

	ADAS_ACTION action;
    EI_BOOL bThreadStart;

	pthread_mutex_t p_det_mutex;
    sem_t frame_del_sem;
	nn_adas_cfg_t m_nn_adas_cfg;
	nn_adas_out_t *p_det_info;
	void *nn_hdl;

    VISS_CHN phyChn;
    VISS_DEV dev;
    VISS_CHN chn;

    EI_U32 u32Width;
    EI_U32 u32Height;
	EI_U32 frameRate;
	
	RGN_ATTR_S stRegion[2];
	RGN_CHN_ATTR_S stRgnChnAttr[ADAS_RGN_ARRAY_NUM];
	RGN_HANDLE Handle[ADAS_RGN_ARRAY_NUM];
	RECTANGLEEX_ARRAY_SUB_ATTRS stRectArray[ADAS_RGN_RECT_NUM];
	LINEEX_ARRAY_SUB_ATTRS stLineArray[ADAS_RGN_LINE_NUM];
	MDP_CHN_S stRgnChn;
	int last_draw_rect_num;
	int last_draw_line0_num;
	int last_draw_line1_num;

	EI_U32 u32scnWidth;
	EI_U32 u32scnHeight;

    pthread_t gs_framePid;

} adas_nn_t;

int adas_nn_init(int vissDev, int vissChn, int chn, int width, int height);

void adas_nn_stop(void);

#ifdef __cplusplus
}
#endif

#endif

