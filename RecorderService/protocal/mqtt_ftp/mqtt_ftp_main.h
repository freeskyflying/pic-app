#ifndef _MQTT_FTP_MAIN_H_
#define _MQTT_FTP_MAIN_H_

#include <MQTTAsync.h>
#include <pthread.h>

#include "ftpUtil.h"
#include "queue_list.h"

typedef enum
{

	DMS_ALARM_CALL = 0xA1,
	DMS_ALARM_SMOK,
	DMS_ALARM_DIST,
	DMS_ALARM_FATI,
	DMS_ALARM_COVER,
	DMS_ALARM_NODRIVER,
	DMS_ALARM_INFRARED_BLOCK,
	DMS_ALARM_NOBELT,

} DMS_ALARM_TYPE_E;

typedef struct
{

	MQTTAsync client;

	unsigned int mInitFlag;
	unsigned int mRunFlag;

	volatile unsigned int mConnectFlag;

//	volatile unsigned int mFinishFlag;
	
	queue_list_t mSendMsgList;
	queue_list_t mUploadList;

	FtpUtil_t ftpUtil;

	unsigned int lasttime_call;
	unsigned int lasttime_smok;
	unsigned int lasttime_fati;
	unsigned int lasttime_dist;
	unsigned int lasttime_abnomal;
	unsigned int lasttime_nobelt;

	unsigned int lasttime_lcw;
	unsigned int lasttime_fcw;
	unsigned int lasttime_ldw;
	unsigned int lasttime_pcw;
	unsigned int lasttime_hmw;

} MqttFtpHandle_t;

typedef struct
{

	unsigned int mType;
	unsigned int mTimestamp;
	unsigned int mJpegCount;

} MqttFtpAlarm_t;

void Mqtt_Ftp_Start(void);

#endif

