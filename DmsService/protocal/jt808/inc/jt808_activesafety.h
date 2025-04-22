#ifndef _JT808_ACTIVESAFETY_H_
#define _JT808_ACTIVESAFETY_H_

#include "jt808_data.h"

#define ACTIVESAFETY_ALARM_ADAS		0x60
#define ACTIVESAFETY_ALARM_DMS		0x80

#define FILETYPE_PHOTO				0
#define FILETYPE_VOICE				1
#define FILETYPE_VIDEO				2
#define FILETYPE_TEXT				3
#define FILETYPE_OTHER				4

#define BIG2LITENDIAN_DWORD(a)	(((a>>24)&0xFF) + ((a>>16)&0xFF00) + ((a<<24)&0xFF000000) + ((a<<16)&0xFF0000))
#define BIG2LITENDIAN_WORD(a)	(((a>>8)&0xFF) + ((a<<8)&0xFF00))

typedef enum
{

	ADAS_ALERT_FCW = 0x01,
	ADAS_ALERT_LDW,
	ADAS_ALERT_HMW,
	ADAS_ALERT_PCW,
	ADAS_ALERT_LCW,
	
	ADAS_ALERT_ACTIVE_PHOTO = 0x11,

} ADAS_ALERT_TYPE_E;

typedef enum
{

	DMS_ALERT_FATI = 0x01,
	DMS_ALERT_CALL,
	DMS_ALERT_SMOK,
	DMS_ALERT_DIST,
	DMS_ALERT_ABNOM,
		
	DMS_ALERT_ACTIVE_PHOTO = 0x10,
	DMS_ALERT_DRIVERCHANGE = 0x11,

} DMS_ALERT_TYPE_E;

/* 报警标识号 */
typedef struct 
{

	BYTE m_dev_id[7]; 					/* 终端ID */
	BYTE m_datetime[6];					/* 时间日期 */
	BYTE m_event_id;					/* 事件序号：同一时间点报警序号，0累加*/
	BYTE m_files_cnt; 					/* 对应的附件数量 */
	BYTE m_resv;						/* 预留 */

} jt808_alert_flag_t;

/* adas报警指令消息体 */
typedef struct 
{

	BYTE m_alert_type;					/* 报警 / 事件类型 */
	BYTE m_alert_level;					/* 报警级别 */
	BYTE m_front_car_speed;				/* 前车车速 */
	BYTE m_front_abs_distance;			/* 前车/行人距离 */
	BYTE m_departure_type;				/* 偏离类型 */
	BYTE m_road_flag_type;				/* 道路标识类型 */
	BYTE m_road_flag_count;				/* 道路标识识别数 */
	
} jsatl_alert_adas_t;

/* dsm报警指令消息体 */
typedef struct
{

	BYTE m_alert_type;					/* 报警 / 事件类型 */
	BYTE m_alert_level;					/* 报警级别 */
	BYTE m_fati_degree;		 			/* 疲劳程度 */
	BYTE m_reserved[4];					/* 预留 */

} jsatl_alert_dsm_t;

/* 附加信息（报警）消息体 */
typedef union
{

	jsatl_alert_adas_t m_adas_event;	/* ADAS报警信息 */
	jsatl_alert_dsm_t m_dsm_event;		/* DSM报警信息 */

} jt808_alert_info_u;

typedef struct
{

	DWORD m_alert_id;					/* 报警ID */
	BYTE m_status_flag;					/* 标志状态 */

	jt808_alert_info_u m_type_info; 	/* 类型信息 */

	BYTE m_speed; 						/* 车速 */
	WORD m_altitude;					/* 高程 */
	DWORD m_latitude;					/* 纬度 */
	DWORD m_longtitude;			 		/* 经度 */
	BYTE m_datetime[6];					/* 日期时间 */
	WORD m_vehicel_status;				/* 车辆状态 */
	jt808_alert_flag_t m_alert_flag;	/* 报警标识 */

} jt808_alert_info_t;


typedef struct
{

	BYTE m_speed_alert_threshold;       /* 报警使能速度阈值 */
	BYTE m_alert_volume;                /* 报警提示音量 */
	BYTE m_active_photo;                /* 主动拍照策略 */
	WORD m_active_photo_timer_interval;/* 主动定时拍照时间间隔 */
	WORD m_active_photo_dist_interval; /* 主动定距拍照距离间隔 */
	BYTE m_active_photo_cnt;            /* 每次主动拍照张数 */
	BYTE m_active_photo_time_interval;  /* 每次主动拍照时间间隔 */
	BYTE m_photo_resolution;     		/* 拍照分辨率 */
	BYTE m_video_resolution;            /* 视频录制分辨率 */
	DWORD m_alerts_enable_set;            /* 报警使能设置 */
	DWORD m_event_enable_set;             /* 事件使能设置 */
	WORD m_smok_check_time_interval;  	/* 吸烟报警判断时间间隔 */
	WORD m_call_check_time_interval;  	/* 接打电话报警判断时间间隔 */
	BYTE m_reserved1[3];                /* 预留字段 */
	BYTE m_fati_speed_alert_threshold;  /* 疲劳驾驶报警分级速度阈值 */
	BYTE m_fati_video_record_time;   	/* 疲劳驾驶报警前后视频录制时间 */
	BYTE m_fati_photo_cnt;           	/* 疲劳驾驶报警拍照张数 */
	BYTE m_fati_photo_time_interval; 	/* 疲劳驾驶报警拍照间隔时间 */
	BYTE m_call_speed_alert_threshold;  /* 接打电话报警分级速度阈值 */
	BYTE m_call_video_record_time;     /* 打电话报警前后视频录制时间 */
	BYTE m_call_photo_cnt;             /* 接打电话报警拍驾驶员面部特征照片张数 */
	BYTE m_call_photo_time_interval;   /* 接打电话报警拍驾驶员面部特征照片间隔时间 */
	BYTE m_smok_speed_alert_threshold;  /* 抽烟报警分级速度阈值 */
	BYTE m_smok_video_record_time;     /* 抽烟报警前后视频录制时间 */
	BYTE m_smok_photo_cnt;             /* 抽烟报警拍驾驶员完整面部特征照片张数 */
	BYTE m_smok_photo_time_interval;   /* 抽烟报警拍驾驶员完整面部特征照片间隔时间 */
	BYTE m_dist_speed_alert_threshold;  /* 分神驾驶报警分级速度阈值 */
	BYTE m_dist_video_record_time;  	/* 分神驾驶报警前后视频录制时间 */
	BYTE m_dist_photo_cnt;          	/* 分神驾驶报警拍照张数 */
	BYTE m_dist_photo_time_interval;	/* 分神驾驶报警拍照间隔时间 */
	BYTE m_abnm_speed_alert_threshold;  /* 驾驶异常报警分级速度阈值 */
	BYTE m_abnm_video_time;         	/* 驾驶异常视频录制时间 */
	BYTE m_abnm_photo_cnt;          	/* 驾驶异常抓拍照片张数 */
	BYTE m_abnm_photo_time_interval;	/* 驾驶异常拍照间隔 */
	BYTE m_driver_iden_trigger;         /* 驾驶员身份识别触发 */		

	BYTE m_reserved2[2];

} __attribute__((packed))jt808_setting_info_dsm_t;

typedef struct
{

	BYTE m_speed_alert_threshold;       /* 报警使能速度阈值 */
	BYTE m_alert_volume;                /* 报警提示音量 */
	BYTE m_active_photo;                /* 主动拍照策略 */
	WORD m_active_photo_timer_interval;	/* 主动定时拍照时间间隔 */
	WORD m_active_photo_dist_interval; 	/* 主动定距拍照距离间隔 */
	BYTE m_active_photo_cnt;            /* 每次主动拍照张数 */
	BYTE m_active_photo_time_interval;  /* 每次主动拍照时间间隔 */
	BYTE m_photo_resolution;     		/* 拍照分辨率 */
	BYTE m_video_resolution;            /* 视频录制分辨率 */
	DWORD m_alert_enables;				/* 报警使能 */
	DWORD m_event_enables;				/* 事件使能 */
	BYTE m_reserved1;				    /* 预留字段 */

	BYTE m_abs_distance_threshold;      /* 障碍物报警距离阈值：单位100ms，取值10-50，默认30 */
	BYTE m_abs_speed_level;			  	/* 分级速度 */
	BYTE m_abs_video_recod_time;        /* 障碍物报警前后视频录制时间 */
	BYTE m_abs_photo_cnt;          	 	/* 障碍物报警牌照张数 */
	BYTE m_abs_photo_time_interval;     /* 障碍物报警牌照间隔：1-10*100ms，默认2 */

	BYTE m_lcw_check_time_interval;   	 /* 频繁变道报警判断时间段：30-120s，默认60 */
	BYTE m_lcw_check_times;           	 /* 频繁变道报警判断次数 */
	BYTE m_lcw_speed_level;			  	/* 分级速度 */
	BYTE m_lcw_video_record_time;	     /* 频繁变道报警前后视频录制时间 */
	BYTE m_lcw_photo_cnt;               /* 频繁变道报警拍照张数 */
	BYTE m_lcw_photo_time_interval;	 	/* 频繁变道报警拍照间隔 */

	BYTE m_ldw_speed_level;			  	/* 分级速度 */
	BYTE m_ldw_video_record_time;     	 /* 车道偏离报警前后视频录制时间 */
	BYTE m_ldw_photo_cnt;             	 /* 车道偏离报警拍照张数 */
	BYTE m_ldw_photo_time_interval;   	 /* 车道偏离报警拍照间隔 */

	BYTE m_fcw_time_threshold;	 	 	 /* 前向碰撞报警时间阈值 */
	BYTE m_fcw_speed_level;			  	/* 分级速度 */
	BYTE m_fcw_video_record_time;       /* 前向碰撞报警前后视频录制时间 */
	BYTE m_fcw_photo_cnt;	 	 		 /* 前向碰撞报警拍照张数 */
	BYTE m_fcw_photo_time_interval;	 	/* 前向碰撞报警拍照间隔 */

	BYTE m_pcw_time_threshold;          /* 行人碰撞报警时间阈值 */
	BYTE m_pcw_speed_level;			  	/* 使能速度 */
	BYTE m_pcw_video_record_time;		 /* 行人碰撞报警前后视频录制时间 */
	BYTE m_pcw_photo_cnt;	 	 		 /* 前向碰撞报警拍照张数 */
	BYTE m_pcw_photo_time_interval;	 	/* 前向碰撞报警拍照间隔 */

	BYTE m_hmw_distance_threshold;      /* 车距过近报警距离阈值 */
	BYTE m_hmw_speed_level;			  	/* 分级速度 */
	BYTE m_hmw_video_record_time;		 /* 车距过近报警前后视频录制时间 */
	BYTE m_hmw_photo_cnt;               /* 车距过近报警拍照张数 */
	BYTE m_hmw_photo_time_interval;  	 /* 车距过近报警拍照间隔 */

	BYTE m_lmi_photo_cnt;               /* 道路标识识别拍照张数 */
	BYTE m_lmi_photo_time_interval;     /* 道路标识识别拍照间隔 */
	
	BYTE m_reserved3[4];              	/* 保留字段 */

} __attribute__((packed))jt808_setting_info_adas_t;

typedef struct
{

	DWORD lasttime_call;
	DWORD lasttime_smok;
	DWORD lasttime_fati;
	DWORD lasttime_dist;
	DWORD lasttime_abnomal;

	DWORD lasttime_lcw;
	DWORD lasttime_fcw;
	DWORD lasttime_ldw;
	DWORD lasttime_pcw;
	DWORD lasttime_hmw;

	jt808_setting_info_dsm_t* dmsParam;
	jt808_setting_info_adas_t* adasParam;

	int alertID;

} JT808ActiveSafety_t;

void JT808ActiveSafety_Start(void* p808controller);

void JT808ActiveSafety_ProcAlarm(int type, int extra, int extra2, int speed);

void JT808ActiveSafety_GetFilename(jt808_alert_flag_t* alertFlag, char* filename, char fileType);

#endif

