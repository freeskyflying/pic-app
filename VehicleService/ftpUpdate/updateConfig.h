#ifndef _UPDATECONFIG_H_
#define _UPDATECONFIG_H_


typedef struct
{

	char ver[16];
	char file[128];
	char md5[33];

} McuUpdate_t;

typedef struct
{

	char target_ver[16];
	char current_ver[16];
	char hw_ver[16];
	char file[128];
	char md5[33];

} SocUpdate_t;

typedef struct
{

	char target_ver[16];
	char file[128];
	char md5[33];
	char time[16];

} ParamSetting_t;

typedef struct
{

	McuUpdate_t* mcu;

	SocUpdate_t* soc;
	unsigned short soc_cnt;

} OtaConfig_t;

typedef struct
{

	char ota_config[64];

	ParamSetting_t* param;
	unsigned short param_cnt;

} UpdateConfig_t;

#if __cplusplus
extern "C" {
#endif

UpdateConfig_t* handleConfigJson(const char* path);

OtaConfig_t* handleOtaJson(const char* path);

#if __cplusplus
} // extern "C"
#endif

#endif
