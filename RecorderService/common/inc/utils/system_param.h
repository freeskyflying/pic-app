#ifndef _SYSTEM_PARAM_H_
#define _SYSTEM_PARAM_H_

#include "utils.h"

#define GLOBAL_SYS_PARAMS_FILE			"/data/etc/GlobalSysParam.bin"

typedef enum
{

	PROTOCAL_808_2013,
	PROTOCAL_905_2014,
	PROTOCAL_808_2019,
	PROTOCAL_MQTT_FTP,

} PROTOCAL_TYPE_E;

typedef enum
{

	LANGUAGE_zh_CN,
	LANGUAGE_en_US,
	LANGUAGE_Spanish,
	LANGUAGE_MAX,

} LANGUAGE_E;

#define LANGUAGE_STR(a) (LANGUAGE_zh_CN==a?"zh_CN":((a==LANGUAGE_en_US)?"en_US":((a==LANGUAGE_Spanish)?"Spanish":"zh_CN")))

typedef struct
{

	char serialNum[16];
	
	char wifiSSID[16];
	char wifiPswd[16];

	unsigned char dsmEnable;
	unsigned char adasEnable;

	unsigned char recDuration;
	unsigned char recResolution;

	unsigned char mVolume;

	int	timezone;
	int	test;

	unsigned char protocalType;
	unsigned char osdEnable;
	unsigned char micEnable;

	char wifiSSIDEx[32];
	char wifiPswdEx[16];

	int language;

	int limitSpeed;

	unsigned char APN[64];
	unsigned char apnUser[32];
	unsigned char apnPswd[32];

	unsigned char mirror[8];

	unsigned short parking_monitor;
	unsigned short timelapse_rate;

	unsigned short overspeed_report_intv;

	unsigned short wakealarm_enable;
	unsigned short wakealarm_interval;

	unsigned short mSOSDisable;		// 控制sos按键功能开关
	unsigned short mSpdOsdType;		// 水印速度单位类型，0-km/h 1-mil/h

} GlobalSysParam_t;

#ifdef __cplusplus
extern "C" {
#endif

extern GlobalSysParam_t* gGlobalSysParam;

void Global_SystemParam_Init(void);

void Global_SystemParam_DeInit(void);

#ifdef __cplusplus
}
#endif

#endif

