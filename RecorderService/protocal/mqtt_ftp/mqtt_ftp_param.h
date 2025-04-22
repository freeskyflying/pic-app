#ifndef _MQTT_FTP_PARAM_H_
#define _MQTT_FTP_PARAM_H_

#define MQTT_FTP_PARAM_FILE		"/data/etc/mqtt_ftp_params"

typedef struct
{

	char mFtpUrl[128];
	char mFtpUser[32];
	char mFtpPswd[32];

	char mMqttUrl[128];
	char mMqttUser[32];
	char mMqttPswd[32];
	char mMqttTopic[128];
	
	char mDeviceId[20];

} MqttFtpParam_t;

extern MqttFtpParam_t*	gMqttFtpParams;

void Mqtt_Ftp_Param_Init(void);
void Mqtt_Ftp_Param_DeInit(void);

#endif

