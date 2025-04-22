#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/statfs.h>

#include "utils.h"
#include "queue_list.h"
#include "md5.h"
#include "ftpUtil.h"
#include "updateConfig.h"
#include "updateHandler.h"
#include "encrypt.h"
#include "ftp_update.h"
#include "pnt_log.h"

#define LOGE		printf

char FTP_URL[128] = { 0 };

char devModel[16] = { 0 };
char devID[16] = { 0 };
int gEncryptStatus = 0;
char devSwVer[16] = { 0 };
char devHwVer[16] = { 0 };
char devParamTime[16] = { 0 };

static int gUploadUpdate = 0;

#ifdef INDIA
#define PROJECT
#endif

void UpdateDeviceInfoFile(void)
{
	getStringByFile("/data/.param_time", devParamTime, sizeof(devParamTime));

	char product_name[32] = { 0 };
	char temp[1024] = { 0 };

	get_product_name(product_name);
	
	sprintf(temp, "{\n\"sw_ver\" : \"%s\",\n\"hw_ver\" : \"%s\",\n\"encrypt\" : \"%d\",\n\"param_time\" : \"%s\",\n\"project\" : \"%s\"\n}", 
					devSwVer, devHwVer, gEncryptStatus, devParamTime, product_name);

	setStringToFile("/data/deviceInfo.json", temp);
}

void* ftpDeviceInfoUpload(void* pArgs)
{
	FtpUtil_t ftpUtil = { 0 };
	CURLcode result = CURLE_OK;
	
	gUploadUpdate = 1;
	
	gEncryptStatus = EncryptCheck();

	while (1)
	{
		if (!gUploadUpdate)
		{
			sleep(10);
			continue;
		}
		else
		{
			UpdateDeviceInfoFile();
		}
		sprintf(ftpUtil.mRemotePath, "%s/%s/devices/%s_info.json", FTP_URL, devModel, devID);
		strcpy(ftpUtil.mLocalPath, "/data/deviceInfo.json");
		strcpy(ftpUtil.mUserName, FTP_USERNAME);
		strcpy(ftpUtil.mPassWord, FTP_PASSWORD);

		ftpUtil.mDirection = 1;
		
		result = handleUploadFiles(&ftpUtil, 0, 0);

		if (result != CURLE_OK)
		{
			LOGE("fatal exception!!! fail to upload %s, result are %d\n", ftpUtil.mLocalPath, result);
			sleep(10);
		}
		else
		{
			LOGE("upload ota packet:%s success!!!\n", ftpUtil.mLocalPath);
			gUploadUpdate = 0;
		}
	}

	return NULL;
}

static int OTAFileDownload(FtpUtil_t* pUtil, char* filepath, char* md5)
{
	char fileName[200] = {0};
	CURLcode result = CURLE_OK;
	
	sprintf(pUtil->mRemotePath, "%s/%s/devices/%s.json", FTP_URL, devModel, devID);
	strcpy(pUtil->mUserName, FTP_USERNAME);
	strcpy(pUtil->mPassWord, FTP_PASSWORD);

	pUtil->mDirection = 0;
	
	mkdir(OTA_FILE_DIR, 077);
	
	// 下载文件
	LOGE("start to download %s.\n", filepath);
	
    char *ptr = strrchr((char*)filepath, '/');
    sprintf(fileName, "%s", ptr + 1);
    size_t fileNameLen = strlen(fileName);
    fileName[fileNameLen] = '\0';
	
	sprintf(pUtil->mRemotePath, "%s/%s/%s", FTP_URL, devModel, filepath);
	sprintf(pUtil->mLocalPath, "%s/%s", OTA_FILE_DIR, fileName);
	pUtil->mFileName[0] = '\0';

	clearTargetDirExt(OTA_FILE_DIR, fileName);
	
	result = handleDownloadFiles(pUtil, 0, 0);

	if (result != CURLE_OK)
	{
        LOGE("fatal exception!!! fail to download %s, result are %d\n", pUtil->mLocalPath, result);
		return -1;
    }
	else
	{
        LOGE("download ota packet:%s success!!!\n", pUtil->mLocalPath);
		// check md5
		char md5sum[33] = { 0 };
		computeFileMd5(pUtil->mLocalPath, md5sum);
		if (0 == strcmp(md5sum, md5))
		{
			return 0;
		}
		else
		{
        	LOGE("%s md5 check failed!!!\n", pUtil->mLocalPath);
			remove(pUtil->mLocalPath);
			return -1;
		}
	}
}

void UpdateConfigHandler(FtpUtil_t* pUtil, UpdateConfig_t* configs)
{
	char fileName[200] = {0};
	CURLcode result = CURLE_OK;
	
	sprintf(pUtil->mRemotePath, "%s/%s/devices/%s.json", FTP_URL, devModel, devID);
	strcpy(pUtil->mLocalPath, CONFIG_JSON_FILE);
	strcpy(pUtil->mUserName, FTP_USERNAME);
	strcpy(pUtil->mPassWord, FTP_PASSWORD);

	pUtil->mDirection = 0;
	
	if (strlen(configs->ota_config) > 0)
	{
		// 下载文件
		LOGE("start to download %s.\n", configs->ota_config);
		
		remove(OTA_JSON_FILE);
		sprintf(pUtil->mRemotePath, "%s/%s/%s", FTP_URL, devModel, configs->ota_config);
		strcpy(pUtil->mLocalPath, OTA_JSON_FILE);
		pUtil->mFileName[0] = '\0';
		
		result = handleDownloadFiles(pUtil, 0, 0);

		if (result != CURLE_OK)
		{
	        LOGE("fatal exception!!! fail to download %s, result are %d\n", pUtil->mLocalPath, result);
	    }
		else
		{
	        LOGE("download ota packet:%s success!!!\n", pUtil->mLocalPath);
			// 解析ota.json
			OtaConfig_t* ota = handleOtaJson(OTA_JSON_FILE);
			if (ota)
			{
				if (ota->soc)
				{
					for (int i=0; i<ota->soc_cnt; i++)
					{
						SocUpdate_t* soc = &ota->soc[i];

						if (0 == strcmp(soc->hw_ver, devHwVer))
						{
							if ((0 == strlen(soc->target_ver) && strcmp(devSwVer, soc->current_ver) < 0) ||
								(0 == strcmp(soc->target_ver, devSwVer)))
							{
								if (0 == OTAFileDownload(pUtil, soc->file, soc->md5))
								{
									setUpgradeFileFlag(soc->current_ver);
									updateHandler_SOCUpdate(pUtil->mLocalPath);
								}

								break;
							}
						}
					}
					free(ota->soc);
					ota->soc = NULL;
				}
				free(ota); ota = NULL;
			}
		}
	}
	
	if (1690000000 < currentTimeMillis()/1000) // 校准时间后再下载配置文件，防止重复下载
	{
		if (configs->param)
		{
			getStringByFile("/data/.param_time", devParamTime, sizeof(devParamTime));

			for (int i=0; i<configs->param_cnt; i++)
			{
				ParamSetting_t* param = &configs->param[i];
				if (0 < strcmp(param->time, devParamTime))
				{
					if (0 == OTAFileDownload(pUtil, param->file, param->md5))
					{
						updateHandler_ParamUpdate(pUtil->mLocalPath);
						gUploadUpdate = 1;
					
						char bcd_time[6] = { 0 }, tmp[16] = { 0 };
						getLocalBCDTime(bcd_time);
						if (0x50 <= bcd_time[0])
						{
							bcd_time[0] = 0x00;
						}
						sprintf(tmp, "%02x%02x%02x.%02x%02x", bcd_time[0], bcd_time[1], bcd_time[2], bcd_time[3], bcd_time[4]);
						setStringToFile("/data/.param_time", tmp);
					}
				}
			}
		
			free(configs->param);
			configs->param = NULL;
		}
	}
	
	free(configs);
	configs = NULL;
}

void* ftpUpdateStart(void* pArgs)
{
	// 创建下载configs.json任务
	FtpUtil_t* pUtil = (FtpUtil_t*)malloc(sizeof(FtpUtil_t));
	char fileName[200] = {0};
	CURLcode result = CURLE_OK;

	memset(pUtil, 0, sizeof(FtpUtil_t));

	while (1)
	{
		remove(CONFIG_JSON_FILE);

		sprintf(pUtil->mRemotePath, "%s/%s/devices/%s.json", FTP_URL, devModel, devID);
		strcpy(pUtil->mLocalPath, CONFIG_JSON_FILE);
		strcpy(pUtil->mUserName, FTP_USERNAME);
		strcpy(pUtil->mPassWord, FTP_PASSWORD);
		pUtil->mFileName[0] = '\0';

		pUtil->mDirection = 0;

		result = handleDownloadFiles(pUtil, 0, 0);

		if (result != CURLE_OK)
		{
	        LOGE("fatal exception!!! fail to download %s, result are %d\n", pUtil->mLocalPath, result);
			sleep(1*60);
	    }
		else
		{
	        LOGE("download ota packet:%s success!!!\n", pUtil->mLocalPath);
			
			// 下载完成后解析configs.json
			UpdateConfig_t* configs = handleConfigJson(CONFIG_JSON_FILE);

			if (NULL != configs)
			{
				UpdateConfigHandler(pUtil, configs);
			}
			sleep(5*60);
	    }
	}

	return NULL;
}

int ftpUpdateCreate(void)
{
	LOGE("Hello world.\n");

	BoardSysInfo_t boardInfo = { 0 };
	getBoardSysInfo(&boardInfo);
	strcpy(devModel, boardInfo.mProductModel);
	strcpy(devSwVer, boardInfo.mSwVersion);
	strcpy(devHwVer, boardInfo.mHwVersion);

	FILE* pfile=fopen("/data/sn", "rb");
	if (getStringByFile("/data/sn", devID, sizeof(devID)))
	{
		strcpy(devID, "P-DC-100000001");
	}

	while (1)
	{
		if (getStringByFile("/data/etc/ftpUpdateURL", FTP_URL, sizeof(FTP_URL)))
		{
			strcpy(FTP_URL, "ftp://112.74.62.57:8021");
		}

		if (strlen(FTP_URL))
		{
			break;
		}

		sleep(10);
	}
	
	PNT_LOGI("DevSN:%s FTP_URL:%s", devID, FTP_URL);

	pthread_t pid;
	
	pthread_create(&pid, NULL, ftpDeviceInfoUpload, NULL);
	
	pthread_create(&pid, NULL, ftpUpdateStart, NULL);

	return 0;
}
