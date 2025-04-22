#ifndef _PNT_LOG_H_
#define _PNT_LOG_H_

#include <stdio.h>
#include<sys/stat.h>
#include<sys/types.h>

#ifdef ANDROID_LOGCAT
#include "log/log.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	      
	lev_error = 1,       
	lev_info = 2,      
	lev_debug = 3, 
	
} jsatl_log_level_e;

void LogW(int log_level, const char* func, int line, const char* format, ...);

#define LOG_LEVEL               lev_info

#ifdef ANDROID_LOGCAT

#define LOGFUNC             printf

#define PNT_LOGE(...)           {LogW(lev_error, __FUNCTION__, __LINE__, __VA_ARGS__);}
#define PNT_LOGI(...)           {LogW(lev_info, __FUNCTION__, __LINE__, __VA_ARGS__);}
#define PNT_LOGD(...)           {LogW(lev_debug, __FUNCTION__, __LINE__, __VA_ARGS__);}

#else

#define LOGFUNC             printf

#define PNT_LOGE(...)           {LogW(lev_error, __FUNCTION__, __LINE__, __VA_ARGS__);}
#define PNT_LOGI(...)           {LogW(lev_info, __FUNCTION__, __LINE__, __VA_ARGS__);}
#define PNT_LOGD(...)           {LogW(lev_debug, __FUNCTION__, __LINE__, __VA_ARGS__);}

#define PNT_LOGARRAY(a,l)		{\
	for(int i=0; i<l; i++){\
		LOGFUNC("%02X ", a[i]);\
	}\
	LOGFUNC("\n");\
}

#endif

void LogSaveInit(void);

void LogSaveToFile(char* str, int flush);

#ifdef __cplusplus
}
#endif

#endif
