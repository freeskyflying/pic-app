#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <errno.h>
#include <sys/prctl.h>
#include <linux/falloc.h>


#include "cJSON.h"
#include "updateConfig.h"

#define LOGE		printf

cJSON * parseCJSONFile(const char* filename)
{
	FILE *cfg_fd;
	long len;
	char *data = NULL;
	cJSON *json = NULL;

	cfg_fd = fopen(filename, "rb");
	if (cfg_fd == NULL)
	{
		LOGE("open file failed %s\n", filename);
		return NULL;
	}
	fseek(cfg_fd, 0, SEEK_END);
	len = ftell(cfg_fd);
	fseek(cfg_fd, 0, SEEK_SET);

	data = (char *)malloc(len + 1);

	if (data)
	{
		memset(data, 0, len + 1);
		fread(data, 1, len, cfg_fd);
		data[len] = '\0';
		json = cJSON_Parse(data);
		if (!json)
		{
			LOGE("Error before: [%s]\n", cJSON_GetErrorPtr());
		}
	}
	else
	{
		LOGE("malloc failed.\n");
		return NULL;
	}
	
	free(data);

	fclose(cfg_fd);
	
	return json;
}

void handleMCUConfigJson(cJSON *para_root, OtaConfig_t* configs)
{
	McuUpdate_t mcu = { 0 };
	cJSON* cjson = cJSON_GetObjectItem(para_root, "mcu");
	
	if (cjson)
	{
		cJSON* item = cJSON_GetObjectItem(cjson, "ver");
		if (NULL == item)
		{
			LOGE("parse mcu ver failed.\n");
			return;
		}
		strncpy(mcu.ver, item->valuestring, sizeof(mcu.ver));
		
		item = cJSON_GetObjectItem(cjson, "file");
		if (NULL == item)
		{
			LOGE("parse mcu file failed.\n");
			return;
		}
		strncpy(mcu.file, item->valuestring, sizeof(mcu.file));
		
		item = cJSON_GetObjectItem(cjson, "md5");
		if (NULL == item)
		{
			LOGE("parse mcu md5 failed.\n");
			return;
		}
		strncpy(mcu.md5, item->valuestring, sizeof(mcu.md5));

		configs->mcu = (McuUpdate_t*)malloc(sizeof(McuUpdate_t));
		if (NULL == configs->mcu)
		{
			LOGE("parse mcu malloc failed.\n");
			return;
		}
		memcpy(configs->mcu, &mcu, sizeof(McuUpdate_t));
	}
}

void handleSOCConfigJson(cJSON *para_root, OtaConfig_t* configs)
{
	SocUpdate_t soc = { 0 };
	cJSON* cjson_soc = cJSON_GetObjectItem(para_root, "soc");
	
	if (cjson_soc)
	{
		int arraySize = cJSON_GetArraySize(cjson_soc);
		
		configs->soc = (SocUpdate_t*)malloc(sizeof(SocUpdate_t)*arraySize);
		if (NULL == configs->soc)
		{
			LOGE("parse soc malloc failed.\n");
			return;
		}
		
		memset(configs->soc, 0, sizeof(SocUpdate_t)*arraySize);
		configs->soc_cnt = arraySize;

		for (int i=0; i<arraySize; i++)
		{
			cJSON* cjson = cJSON_GetArrayItem(cjson_soc, i);
		
			cJSON* item = cJSON_GetObjectItem(cjson, "target_ver");
			if (NULL == item)
			{
				LOGE("parse soc ver failed.\n");
				return;
			}
			strncpy(soc.target_ver, item->valuestring, sizeof(soc.target_ver));
			
			item = cJSON_GetObjectItem(cjson, "current_ver");
			if (NULL == item)
			{
				LOGE("parse soc current_ver failed.\n");
				return;
			}
			strncpy(soc.current_ver, item->valuestring, sizeof(soc.current_ver));
			
			item = cJSON_GetObjectItem(cjson, "hw_ver");
			if (NULL == item)
			{
				LOGE("parse soc hw_ver failed.\n");
				return;
			}
			strncpy(soc.hw_ver, item->valuestring, sizeof(soc.hw_ver));
			
			item = cJSON_GetObjectItem(cjson, "file");
			if (NULL == item)
			{
				LOGE("parse soc file failed.\n");
				return;
			}
			strncpy(soc.file, item->valuestring, sizeof(soc.file));
			
			item = cJSON_GetObjectItem(cjson, "md5");
			if (NULL == item)
			{
				LOGE("parse soc md5 failed.\n");
				return;
			}
			strncpy(soc.md5, item->valuestring, sizeof(soc.md5));

			memcpy(&configs->soc[i], &soc, sizeof(SocUpdate_t));
		}
	}
}

void handleParamConfigJson(cJSON *para_root, UpdateConfig_t* configs)
{
	ParamSetting_t param = { 0 };
	cJSON* cjson_param = cJSON_GetObjectItem(para_root, "param");
	
	if (cjson_param)
	{
		int arraySize = cJSON_GetArraySize(cjson_param);
		
		configs->param = (ParamSetting_t*)malloc(sizeof(ParamSetting_t)*arraySize);
		if (NULL == configs->param)
		{
			LOGE("parse soc malloc failed.\n");
			return;
		}
		
		memset(configs->param, 0, sizeof(ParamSetting_t)*arraySize);
		configs->param_cnt = arraySize;

		for (int i=0; i<arraySize; i++)
		{
			cJSON* cjson = cJSON_GetArrayItem(cjson_param, i);
			
			cJSON* item = cJSON_GetObjectItem(cjson, "file");
			if (NULL == item)
			{
				LOGE("parse param file failed.\n");
				return;
			}
			strncpy(param.file, item->valuestring, sizeof(param.file));
			
			item = cJSON_GetObjectItem(cjson, "md5");
			if (NULL == item)
			{
				LOGE("parse param md5 failed.\n");
				return;
			}
			strncpy(param.md5, item->valuestring, sizeof(param.md5));
			
			item = cJSON_GetObjectItem(cjson, "time");
			if (NULL == item)
			{
				LOGE("parse param time failed.\n");
				return;
			}
			strncpy(param.time, item->valuestring, sizeof(param.time));

			memcpy(&configs->param[i], &param, sizeof(ParamSetting_t));
		}
	}
}

UpdateConfig_t* handleConfigJson(const char* path)
{
	cJSON *para_root = parseCJSONFile(path);
	if (NULL == para_root)
	{
		LOGE("parse %s failed, Json format error.\n", path);
		return NULL;
	}

	UpdateConfig_t* configs = (UpdateConfig_t*)malloc(sizeof(UpdateConfig_t));
	if (NULL == configs)
	{
		LOGE("parse %s failed, malloc failed.\n", path);
		return NULL;
	}

	memset(configs, 0, sizeof(UpdateConfig_t));

	cJSON* item = cJSON_GetObjectItem(para_root, "ota");
	if (NULL != item)
	{
		strncpy(configs->ota_config, item->valuestring, sizeof(configs->ota_config));
	}

	handleParamConfigJson(para_root, configs);

	cJSON_Delete(para_root);

	return configs;
}

OtaConfig_t* handleOtaJson(const char* path)
{
	cJSON *para_root = parseCJSONFile(path);
	if (NULL == para_root)
	{
		LOGE("parse %s failed, Json format error.\n", path);
		return NULL;
	}

	OtaConfig_t* configs = (OtaConfig_t*)malloc(sizeof(OtaConfig_t));
	if (NULL == configs)
	{
		LOGE("parse %s failed, malloc failed.\n", path);
		return NULL;
	}

	memset(configs, 0, sizeof(OtaConfig_t));

	handleMCUConfigJson(para_root, configs);
	handleSOCConfigJson(para_root, configs);

	cJSON_Delete(para_root);

	return configs;
}


