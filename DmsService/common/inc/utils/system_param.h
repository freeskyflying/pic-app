#ifndef _SYSTEM_PARAM_H_
#define _SYSTEM_PARAM_H_

#include "utils.h"

#define GLOBAL_SYS_PARAMS_FILE			"/data/etc/GlobalSysParam.bin"

typedef enum
{

	PROTOCAL_808_2013,
	PROTOCAL_905_2014,

} PROTOCAL_TYPE_E;

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

} GlobalSysParam_t;

extern GlobalSysParam_t* gGlobalSysParam;

void Global_SystemParam_Init(void);

void Global_SystemParam_DeInit(void);

#endif

