#ifndef _COMMON_TEST_H_
#define _COMMON_TEST_H_

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>
#include <ctype.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/errno.h>

#include "gpio_led.h"
#include "utils.h"
#include "pnt_log.h"

typedef struct
{

	int result;
	char strInfo[128];

} TestItemResult_t;

typedef enum
{

	ITEM_WIFI,
	ITEM_NET4G,
	ITEM_GPS,
	ITEM_TF,
	ITEM_USB,
	ITEM_SOS,
	ITEM_ACC,
	
	ITEM_IMEI,
	ITEM_VERSION,

	ITEM_GSENSOR,
	ITEM_WIFIKEY,
	
	ITEM_PWRADC,
	
	ITEM_LED,
	ITEM_SPEAKER,
	ITEM_CAM1,
	ITEM_CAM2,
	ITEM_CAM3,
	ITEM_RTC,
	
	ITEM_SN,
	ITEM_ENCRYPT,

	ITEM_MAX,

} TEST_ITEM_e;

extern volatile int gTestRunningFlag;

extern TestItemResult_t gTestResults[ITEM_MAX];

void TFCardTest_Start(void);
void GPSTest_Start(void);
void SOSTest_Start(void);
void WifiKeyTest_Start(void);
void RTCTest_Start(void);
void Net4GTest_Start(void);
void GSensorTest_Start(void);
void ACCTest_Start(void);
void MICTest_Start(void);
void WifiTest_Start(void);
void CameraTest_Start(void);
int CameraTest_Snap(int chn, char* info);

#endif

