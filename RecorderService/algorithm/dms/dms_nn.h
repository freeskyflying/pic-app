#ifndef _DMS_NN_H_
#define _DMS_NN_H_

#include <pthread.h>
#include <semaphore.h>
#include "algo_common.h"
#include "nn_dms_api.h"
#include "nn_headfacedet_api.h"
#include "eznn_api.h"

#include "mdp_al_config.h"

#ifdef __cplusplus
extern "C"{
#endif

#define NN_DMS_PATH	 "/usr/share/ax/dms"
#define LIVINGDET_PATH	 "/usr/share/ax/liveness"

#define DMS_RGN_ARRAY_NUM 2
#define DMS_RGN_RECT_NUM 16 /* rect max num */
#define DMS_RGN_LINE_NUM 16 /* line max num */

typedef enum
{

	DMS_CALI_START = 1,
	DMS_CALI_END,
	DMS_CALI_MAX,
	
} dms_cali_flag_e;

typedef enum
{
	
	DMS_RECOGNITION,
	DMS_CALIB,
	DMS_ACTION_END,
	
} DMS_ACTION;
	
typedef enum 
{

	LIVING_DET_NONE,
	LIVING_DET_SINGLE_RGB,
	LIVING_DET_SINGLE_NIR,
	LIVING_DET_MAX,
	
} dms_livingdet_flag_e;
	
typedef struct
{

	float vanish_point_x;
	float vanish_point_y;
	float bottom_line_y;
	dms_cali_flag_e cali_flag;
	unsigned int dms_calibstep;
	int auto_calib_flag;
	
} dms_calibrate_para_t;

typedef struct
{

	DMS_ACTION action;
    EI_BOOL bThreadStart;

	pthread_mutex_t p_det_mutex;
    sem_t frame_del_sem;
	nn_dms_cfg_t m_nn_dms_cfg;
	nn_dms_out_t *p_det_info;
	void *nn_hdl;
	ezax_img_t face;
	
	EI_S32 livingdet_flag; /* value 1:single rgb; value 2: single nir; others: disabled */
	void *livingdet_hdl;
	ezax_livingdet_cfg_t livingdet_cfg;
	ezax_rt_t *livingdet_out;
	ezax_livingdet_rt_t *score;

    VISS_CHN phyChn;
    VISS_DEV dev;
    VISS_CHN chn;

    EI_U32 u32Width;
    EI_U32 u32Height;
	EI_U32 frameRate;

	EI_S32 gray_flag; /* value 0:send camera image ; value 1: uv memset 0x80, send gray scale image */
	void *gray_uv_vir_addr;
	EI_U64 gray_uv_phy_addr;

	RGN_ATTR_S stRegion[2];
    RGN_CHN_ATTR_S stRgnChnAttr[DMS_RGN_ARRAY_NUM];
    RGN_HANDLE Handle[DMS_RGN_ARRAY_NUM];
    RECTANGLEEX_ARRAY_SUB_ATTRS stRectArray[DMS_RGN_RECT_NUM];
    LINEEX_ARRAY_SUB_ATTRS stLineArray[DMS_RGN_LINE_NUM];
	MDP_CHN_S stRgnChn;
	int last_draw_num;

    pthread_t gs_framePid;

	FILE* ax_fout;

	int lastNodriverCoverFlag;
	int lastNodriverCoverCnt;

	char cover_area_handle;
	
} dms_nn_t;

int dms_nn_init(int vissDev, int vissChn, int chn, int width, int height);

void dms_cover_init(int lt_x, int lt_y, int rb_x, int rb_y);

void dms_cover_deinit(void);

void dms_nn_stop(void);

#ifdef __cplusplus
}
#endif

#endif

