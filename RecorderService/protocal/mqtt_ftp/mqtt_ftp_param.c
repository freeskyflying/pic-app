#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <wait.h>

#include "media_utils.h"
#include "pnt_log.h"
#include "system_param.h"
#include "mqtt_ftp_param.h"

MqttFtpParam_t*	gMqttFtpParams = NULL;

void Mqtt_Ftp_Param_Init(void)
{
	MqttFtpParam_t param;

	memset(&param, 0, sizeof(MqttFtpParam_t));
	
	strcpy(param.mFtpUrl, "ftp.dlgcjc.com");
	strcpy(param.mFtpUser, "pd1");
	strcpy(param.mFtpPswd, "krE75c93");
	
	strcpy(param.mMqttUrl, "mqtt.dlgcjc.com:1883");
	strcpy(param.mMqttUser, "ykgwyc");
	strcpy(param.mMqttPswd, "cvV#YnWL0sxh");
	strcpy(param.mMqttTopic, "org/dgy/zama/product/pd_1");
	
	strcpy(param.mDeviceId, gGlobalSysParam->serialNum);
	
	int fd = open(MQTT_FTP_PARAM_FILE, O_RDWR | O_CREAT);
	if (fd < 0)
	{
		PNT_LOGE("create %s failed.\n", MQTT_FTP_PARAM_FILE);
		return;
	}
	
	int size = lseek(fd, 0, SEEK_END);

	if (size < sizeof(MqttFtpParam_t))
	{
		PNT_LOGE("%s resize %d", MQTT_FTP_PARAM_FILE, size);
		
		if (0 < size)
		{
			lseek(fd, 0, SEEK_SET);
			read(fd, &param, size);
		}
		ftruncate(fd, sizeof(MqttFtpParam_t)-size);
		lseek(fd, 0, SEEK_SET);
		write(fd, &param, sizeof(MqttFtpParam_t));
	}

	gMqttFtpParams = mmap(NULL, sizeof(MqttFtpParam_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (MAP_FAILED == gMqttFtpParams)
	{
		PNT_LOGE("mmap file %s failed.\n", MQTT_FTP_PARAM_FILE);
		close(fd);
		return;
	}

	PNT_LOGI("gMqttFtpParams mFtpUrl: %s.\n", gMqttFtpParams->mFtpUrl);
	PNT_LOGI("gMqttFtpParams mFtpUser: %s.\n", gMqttFtpParams->mFtpUser);
	PNT_LOGI("gMqttFtpParams mFtpPswd: %s.\n", gMqttFtpParams->mFtpPswd);
	
	PNT_LOGI("gMqttFtpParams mMqttUrl: %s.\n", gMqttFtpParams->mMqttUrl);
	PNT_LOGI("gMqttFtpParams mMqttUser: %s.\n", gMqttFtpParams->mMqttUser);
	PNT_LOGI("gMqttFtpParams mMqttPswd: %s.\n", gMqttFtpParams->mMqttPswd);
	PNT_LOGI("gMqttFtpParams mMqttTopic: %s.\n", gMqttFtpParams->mMqttTopic);

	close(fd);
}

void Mqtt_Ftp_Param_DeInit(void)
{
	if (gMqttFtpParams)
	{
		munmap(gMqttFtpParams, sizeof(MqttFtpParam_t));
	}
}

