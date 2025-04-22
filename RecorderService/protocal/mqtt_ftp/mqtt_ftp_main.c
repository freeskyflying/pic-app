#include <string.h>

#include "pnt_log.h"
#include "mqtt_ftp_param.h"
#include "utils.h"

#include "algo_common.h"
#include "dms/dms_config.h"

#include "cJSON.h"
#include "rild.h"
#include "media_storage.h"
#include "media_cache.h"
#include "media_snap.h"
#include "jt808_utils.h"

#include "mqtt_ftp_main.h"


MqttFtpHandle_t gMqttFtpHandle = { 0 };

//消息回调函数
int onMessrecv(void* context,char*topicName,int topicLen,MQTTAsync_message*message)
{
    PNT_LOGI("recv name:\ntopic:%s\npayload:%s\n", topicName, (char*)message->payload);
	
    MQTTAsync_free(topicName);
    MQTTAsync_free(message);
	
    return 1;         
}

void onSubscribe(void* context,MQTTAsync_successData* response)
{
    PNT_LOGI("sub success!");
}

//连接mqtt服务器
void onConnect(void *context,MQTTAsync_successData* response)
{
    MQTTAsync client =(MQTTAsync)context;
    int ret;
    MQTTAsync_responseOptions response_opt=MQTTAsync_responseOptions_initializer;
	
    PNT_LOGI("Succeed in connecting to mqtt-server!\n");
#if 0	
    response_opt.onSuccess=onSubscribe;
    ret=MQTTAsync_subscribe(client, gMqttFtpParams->mMqttTopic, 1, &response_opt);
    
    if(ret!=MQTTASYNC_SUCCESS)
	{
        PNT_LOGI("fail to sub!\n");
    }
#endif
	gMqttFtpHandle.mConnectFlag = 1;
}

void disConnect(void *context,MQTTAsync_failureData* response)
{
    PNT_LOGI("Failed to connect  mqtt-server! [%d]\n", response ? response->code : 0);

	gMqttFtpHandle.mConnectFlag = 0;
}

void onDisconnect(void* context, MQTTAsync_successData* response)
{
	PNT_LOGI("Successful disconnection\n");
}

void onSend(void* context,MQTTAsync_successData* response)
{
    PNT_LOGI("send message to mqtt server success!\n");
}

void onSendFailure(void* context, MQTTAsync_failureData* response)
{
    PNT_LOGI("send message to mqtt server failed!\n");
	gMqttFtpHandle.mConnectFlag = 0;
}

void connlost(void *context, char *cause)
{
	PNT_LOGI("\nConnection lost cause: %s", cause?cause:"");
	gMqttFtpHandle.mConnectFlag = 0;
}

void onReconnected(void* context, char* cause)
{
	PNT_LOGI("Reconnecting\n");
	gMqttFtpHandle.mConnectFlag = 1;
}

int Mqtt_Ftp_Alarm(int type, int extra, int extra2, int speed)
{
	unsigned int* plast_time = NULL;
	unsigned int intervel_time = 0, camChn = 0, last_timeval = 0;
	unsigned char speed_shreshod = 0, photo_cnt = 1;
	unsigned char alert_type = 0, enable_flag = 0;
	char audioPath[128] = { 0 };
	char filename[128] = { 0 };
	char bcdTime[128] = { 0 };

	int ret = 0;
	
	MqttFtpHandle_t* handle = &gMqttFtpHandle;

	if (!handle->mInitFlag)
	{
		return ret;
	}

	switch (type)
	{
		case LB_WARNING_DMS_DRIVER_LEAVE:
			plast_time = &(handle->lasttime_abnomal);
			intervel_time = gDmsAlgoParams->nodriver_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/weijiancedaojiashiyuan.pcm");
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALARM_NODRIVER;
			enable_flag = gDmsAlgoParams->nodriver_enable;
			break;
		case LB_WARNING_DMS_CALL:
			plast_time = &(handle->lasttime_call);
			intervel_time = gDmsAlgoParams->call_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingwudadianhua.pcm");
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALARM_CALL;
			enable_flag = gDmsAlgoParams->call_enable;
			break;
		case LB_WARNING_DMS_DRINK:
			plast_time = &(handle->lasttime_dist);
			intervel_time = gDmsAlgoParams->dist_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingwuheshui.pcm");
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALARM_DIST;
			enable_flag = gDmsAlgoParams->dist_enable;
			break;
		case LB_WARNING_DMS_SMOKE:
			plast_time = &(handle->lasttime_smok);
			intervel_time = gDmsAlgoParams->smok_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingwuxiyan.pcm");
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALARM_SMOK;
			enable_flag = gDmsAlgoParams->smok_enable;
			break;
		case LB_WARNING_DMS_ATTATION:
			plast_time = &(handle->lasttime_dist);
			intervel_time = gDmsAlgoParams->dist_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingzhuanxinjiashi.pcm");
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALARM_DIST;
			enable_flag = gDmsAlgoParams->dist_enable;
			break;
		case LB_WARNING_DMS_REST:
			plast_time = &(handle->lasttime_fati);
			intervel_time = gDmsAlgoParams->fati_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingzhuyixiuxi.pcm");
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALARM_FATI;
			enable_flag = gDmsAlgoParams->fati_enable;
			break;
		case LB_WARNING_DMS_CAMERA_COVER:
			plast_time = &(handle->lasttime_abnomal);
			intervel_time = gDmsAlgoParams->cover_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingwuzhedangshexiangtou.pcm");
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALARM_COVER;
			enable_flag = gDmsAlgoParams->cover_enable;
			break;
		case LB_WARNING_DMS_INFRARED_BLOCK:
			plast_time = &(handle->lasttime_abnomal);
			intervel_time = gDmsAlgoParams->glass_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingwupeidaifanhongwaiyanjing.pcm");
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALARM_INFRARED_BLOCK;
			enable_flag = gDmsAlgoParams->glass_enable;
			break;
		case LB_WARNING_DMS_NO_BELT:
			plast_time = &(handle->lasttime_nobelt);
			intervel_time = gDmsAlgoParams->glass_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALARM_NOBELT;
			strcpy(audioPath, "dms_warning/qingxihaoanquandai.pcm");
			break;
			
		default:
			break;
	}

	if (NULL == plast_time)
	{
		return ret;
	}

	speed_shreshod = 0;
//	intervel_time = 20;

	if (speed < speed_shreshod)
	{
		PNT_LOGE("speed low %d < %d, ignore alarm [%d].", speed, speed_shreshod, type);
		return ret;
	}

	if (!enable_flag)
	{
		PNT_LOGE("disabled, ignore alarm [%d].", type);
		return ret;
	}

	last_timeval = (*plast_time);

	if (currentTimeMillis()/1000 - last_timeval >= intervel_time)
	{
		last_timeval = (currentTimeMillis()/1000);
		
		playAudio(audioPath);
		ret = 1;
		
		if (0 != getFlagStatus("tfcard") || 1660000000 > currentTimeMillis()/1000) // 未插tf卡或未校时
		{
			PNT_LOGE("no tfcard or time error, ignore alarm [%d].", type);
			//return ret;
		}
		else
		{
			getLocalBCDTime((char*)bcdTime);

			sprintf(filename, ALARM_PATH"/%02X%02X%02X%02X%02X%02X_%02X.jpg",
			bcdTime[0],	bcdTime[1],	bcdTime[2],	bcdTime[3],	bcdTime[4],	bcdTime[5], alert_type);
			
			if (RET_SUCCESS == MediaSnap_CreateSnap(camChn, photo_cnt, 1280, 720, 0, filename))
			{
				MqttFtpAlarm_t* alarm = (MqttFtpAlarm_t*)malloc(sizeof(MqttFtpAlarm_t));
				if (NULL != alarm)
				{
					alarm->mJpegCount = 1;
					alarm->mTimestamp = currentTimeMillis()/1000;
					alarm->mType = alert_type;

					if (queue_list_push(&handle->mSendMsgList, alarm))
					{
						free(alarm);
					}
				}
			}

			ret = 1;
		}
	}
	else
	{
		unsigned int timeresv = intervel_time - ((currentTimeMillis()/1000) - last_timeval);
		PNT_LOGE("time not in intervel scale reserve [%d s] [%d].", timeresv, type);
	}

	*plast_time = last_timeval;

	return 1;
}

void Mqtt_Ftp_Upload_File(MqttFtpHandle_t* handle, MqttFtpAlarm_t* alarm)
{
	unsigned char bcdTime[6];
	char filename[128] = { 0 };
	
	convertSysTimeToBCD(alarm->mTimestamp, bcdTime);
	
	sprintf(filename, ALARM_PATH"/%02X%02X%02X%02X%02X%02X_%02X.jpg",
		bcdTime[0],	bcdTime[1],	bcdTime[2],	bcdTime[3],	bcdTime[4],	bcdTime[5], alarm->mType);

	if (access(filename, R_OK))
	{
		PNT_LOGE("%s file not exist.", filename);
		return;
	}

	memset(&handle->ftpUtil, 0, sizeof(handle->ftpUtil));

	handle->ftpUtil.mOwner = handle;
	
	strncpy(handle->ftpUtil.mUserName, (char*)gMqttFtpParams->mFtpUser, sizeof(handle->ftpUtil.mUserName));
	strncpy(handle->ftpUtil.mPassWord, (char*)gMqttFtpParams->mFtpPswd, sizeof(handle->ftpUtil.mPassWord));
	strcpy(handle->ftpUtil.mLocalPath, filename);

	sprintf(filename, "%s_%s_%d_%02X.jpg", gNet4GRild.IMEI, gMqttFtpParams->mDeviceId, alarm->mTimestamp, alarm->mType);
	sprintf(handle->ftpUtil.mRemotePath, "ftp://%s/%s", gMqttFtpParams->mFtpUrl, filename);

	CURLcode result = handleUploadFiles(&handle->ftpUtil, 0, 0);

	if (result != CURLE_OK)
	{
		PNT_LOGE("fatal exception!!! fail to upload %s, result are %d", handle->ftpUtil.mLocalPath, result);
	}
	else
	{
		PNT_LOGE("upload ota packet:%s success!!!", handle->ftpUtil.mLocalPath);
	}
}

void Mqtt_Ftp_Report(MqttFtpHandle_t* handle)
{
	MqttFtpAlarm_t* alarm = queue_list_popup(&handle->mSendMsgList);

    cJSON * cjsonObjReport = cJSON_CreateObject();

	char tmp[1024] = { 0 };
	unsigned char bcdTime[6];
	char filename[128] = { 0 };

	if (NULL == alarm)
	{
		return;
	}

	if (alarm->mType != 0x00)
	{
		convertSysTimeToBCD(alarm->mTimestamp, bcdTime);
		
		sprintf(filename, ALARM_PATH"/%02X%02X%02X%02X%02X%02X_%02X.jpg",
			bcdTime[0],	bcdTime[1],	bcdTime[2],	bcdTime[3],	bcdTime[4],	bcdTime[5], alarm->mType);

		if (access(filename, R_OK))
		{
			PNT_LOGE("%s file not exist.", filename);
			queue_list_push(&handle->mSendMsgList, alarm);
			return;
		}
		
		queue_list_push(&handle->mUploadList, alarm);
	}
	
	cJSON_AddStringToObject(cjsonObjReport, "IMEI", gNet4GRild.IMEI);
	
	cJSON_AddNumberToObject(cjsonObjReport, "online", 1);

	sprintf(tmp, "%02X", alarm->mType);
	cJSON_AddStringToObject(cjsonObjReport, "alarmflag", tmp);

	if (alarm->mType != 0x00)
	{
		sprintf(tmp, "%s_%s_%d_%02X.jpg", gNet4GRild.IMEI, gMqttFtpParams->mDeviceId, alarm->mTimestamp, alarm->mType);
		cJSON_AddStringToObject(cjsonObjReport, "img", tmp);
	}
	else
	{
		cJSON_AddStringToObject(cjsonObjReport, "img", "");
	}
	
	cJSON_AddNumberToObject(cjsonObjReport, "timestamp", alarm->mTimestamp);
	
	PNT_LOGI("%s\n", cJSON_Print(cjsonObjReport));

	MQTTAsync_message message=MQTTAsync_message_initializer;
    MQTTAsync_responseOptions res_option=MQTTAsync_responseOptions_initializer;
	
    message.payload=cJSON_Print(cjsonObjReport);
    message.payloadlen=strlen(cJSON_Print(cjsonObjReport));
    message.qos=1;
	message.retained = 1;
    res_option.onSuccess=onSend;
	res_option.onFailure = onSendFailure;
	res_option.context = handle->client;
	
	//sprintf(tmp, "org/dgy/zama/product/600241/device/%s", gMqttFtpParams->mDeviceId);
	
	if(MQTTAsync_sendMessage(handle->client, "org/dgy/zama/product/600242/device/60014997", &message, &res_option) != MQTTASYNC_SUCCESS)
	{
        PNT_LOGE("Param error!");
    }

    cJSON_Delete(cjsonObjReport);

	if (alarm->mType == 0x00)
	{
		free(alarm);
	}
}

void* Mqtt_Ftp_Upload_Routine(void* pArgs)
{
	MqttFtpHandle_t* handle = (MqttFtpHandle_t*)pArgs;
	
	SleepMs(1000);

	while (handle->mRunFlag)
	{
		MqttFtpAlarm_t* alarm = queue_list_popup(&handle->mUploadList);
		if (NULL != alarm)
		{
			Mqtt_Ftp_Upload_File(handle, alarm);
			
			free(alarm);
		}
		
		SleepMs(100);
	}

	return NULL;
}

void* Mqtt_Ftp_Main_Routine(void* pArgs)
{
	int ret = 0;
	MqttFtpHandle_t* handle = (MqttFtpHandle_t*)pArgs;
	MqttFtpParam_t* params = gMqttFtpParams;

	MQTTAsync_connectOptions conn_opt = MQTTAsync_connectOptions_initializer;
	
    ret=MQTTAsync_create(&handle->client, params->mMqttUrl, params->mDeviceId, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	
    if(ret!=MQTTASYNC_SUCCESS)
    {
        PNT_LOGE("Cannot create mqtt client [%s]!\n", params->mMqttUrl);
        return NULL;
    }

	//初始化接收消息回调
    ret=MQTTAsync_setCallbacks(handle->client, NULL, connlost, onMessrecv, NULL);
    if(ret!=MQTTASYNC_SUCCESS)
	{
        PNT_LOGE("cannnot set call back function!\n");
        return  NULL;
    }
    conn_opt.onSuccess=onConnect;
    conn_opt.onFailure=disConnect;
    conn_opt.context = handle->client;
    conn_opt.cleansession = 1;
	conn_opt.keepAliveInterval = 20;
	conn_opt.username = params->mMqttUser;
	conn_opt.password = params->mMqttPswd;
	
	conn_opt.connectTimeout = 10;
    conn_opt.automaticReconnect = 1;
	conn_opt.minRetryInterval = 3;
	conn_opt.maxRetryInterval = 20;
	
    ret=MQTTAsync_connect(handle->client,&conn_opt);
	
    if(ret!=MQTTASYNC_SUCCESS)
    {
        PNT_LOGE("Cannot start a mqttt server connect!\n");
        return NULL;
    }
	if ((ret = MQTTAsync_setConnected(handle->client, handle->client, onReconnected)) != MQTTASYNC_SUCCESS)
	{
        PNT_LOGE("Cannot set reconnect!\n");
	}

	handle->mRunFlag = 1;

	handle->mInitFlag = 1;

	queue_list_create(&handle->mSendMsgList, 100);
	queue_list_create(&handle->mUploadList, 100);

	unsigned int lastReportTime = 0;

	while (strlen(gNet4GRild.IMEI) == 0) SleepMs(100);

	while (handle->mRunFlag)
	{
		if (gMqttFtpHandle.mConnectFlag)
		{
			if (currentTimeMillis()/1000 - lastReportTime >= 60)
			{
				MqttFtpAlarm_t* alarm = (MqttFtpAlarm_t*)malloc(sizeof(MqttFtpAlarm_t));
				if (NULL != alarm)
				{
					alarm->mJpegCount = 1;
					alarm->mTimestamp = currentTimeMillis()/1000;
					alarm->mType = 0x00;

					if (queue_list_push(&handle->mSendMsgList, alarm))
					{
						free(alarm);
					}
					lastReportTime = currentTimeMillis()/1000;
				}
			}
		
			Mqtt_Ftp_Report(handle);
		}

		SleepMs(100);
	}

	if (NULL != handle->client)
	{
		MQTTAsync_destroy(&handle->client);
		handle->client = NULL;
	}

	return NULL;
}

void Mqtt_Ftp_Start(void)
{
	Mqtt_Ftp_Param_Init();

	pthread_t pid;
	pthread_create(&pid, NULL, Mqtt_Ftp_Main_Routine, &gMqttFtpHandle);
	
	pthread_create(&pid, NULL, Mqtt_Ftp_Upload_Routine, &gMqttFtpHandle);
}

