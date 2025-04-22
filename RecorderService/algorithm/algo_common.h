#ifndef _ALGO_COMMON_H_
#define _ALGO_COMMON_H_

#include "sample_comm.h"

#ifdef __cplusplus
extern "C"{
#endif

#define VBUF_BUFFER_MAX_NUM 64
#define VBUF_POOL_MAX_NUM 4

#define ALARM_INTERVAL 5

typedef enum 
{

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
	LB_WARNING_ADAS_ACTIVE_PHOTO,
	
	LB_WARNING_DMS_DRIVER_LEAVE = 0xF100,
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
	LB_WARNING_DMS_ACTIVE_PHOTO,
	LB_WARNING_DMS_DRIVER_CHANGE,
	
	LB_WARNING_USERDEF,
	LB_WARNING_END,
	
} WARNING_MSG_TYPE;

typedef struct
{

    VBUF_BUFFER u32BufID;
	VIDEO_FRAME_INFO_S stVfrmInfo;
	
} VBUF_MAP_S;

typedef struct
{

	VBUF_BUFFER Buffer[VBUF_BUFFER_MAX_NUM];
	VBUF_POOL Pool;
    VBUF_REMAP_MODE_E enRemapMode;
    EI_U32 u32BufNum;
    VBUF_MAP_S *pstBufMap;
	
} pool_info_t;

typedef struct
{

	pool_info_t pool_info[VBUF_POOL_MAX_NUM];

	unsigned int mCurrentSpeed;
	unsigned int mGpsValid;

	unsigned char mAdasRender;
	unsigned char mDmsRender;
	unsigned char mBsdRender;

} AlCommonParam_t;

int Vbuf_Pool_init(VBUF_CONFIG_S *pstVbConfig);

int Vbuf_Pool_Deinit(VBUF_CONFIG_S *pstVbConfig);

int al_warning_proc(WARNING_MSG_TYPE warnType, int extra, int extra2, float ttc);

extern AlCommonParam_t gAlComParam;

void Algo_Start(void);

void Algo_Stop(void);

#ifdef __cplusplus
}
#endif

#endif


