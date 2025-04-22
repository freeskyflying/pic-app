#ifndef _UTILS_H_
#define _UTILS_H_

#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include "typedef.h"
#include <sqlite3.h>
#include "md5.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DB_IS_MALFORMED         1
#define DB_IS_INTEGRITY         0

#define READ_DATA_SIZE	1024
#define MD5_SIZE		16
#define MD5_STR_LEN		(MD5_SIZE * 2)

#define VOICE_FILE_PATH_CHINESE			"/etc/audio"
#define VOICE_FILE_PATH_ENGLISH			"/etc/audio_eng"

typedef	int(*SQLExeCallBack)(void *NotUsed, int argc, char **argv, char **azColName);

typedef struct
{

    char mProductModel[12];
    char mHwVersion[12];
    char mSwVersion[12];
    
} BoardSysInfo_t;

typedef struct
{

    unsigned int total;
    unsigned int free;

} SDCardInfo_t;

typedef struct
{

	char ip[20];
	int	port;
	char plateNum[64];
	char phoneNum[21];
	char plateColor;
	char protocalType;

} AppPlatformParams_t;

unsigned char bcdToDec(unsigned char data);

int setNonBlocking(int fd);

long long currentTimeMillis();

long long str2num(const char *str);

bool_t deleteFile(const char *szFileName);

int getLocalBCDTime(char* bcd_time);

void getLocalTime(struct tm *t);

time_t convertBCDToSysTime(unsigned char* bcd);

void convertSysTimeToBCD(time_t timev, unsigned char* bcd);

void SleepMs(unsigned int ms);

void SleepUs(unsigned int us);

void SleepSs(unsigned int se);

int checkDBIntegrity(sqlite3* db);

int ExecSql(sqlite3* db, const char* sql, SQLExeCallBack cb, void* param);

int getFileSize(const char* path);

bool_t ensureTargetDirExist(const char *path);

int computeFileMd5(const char *filePath, char *targetMd5);

int computeStrMd5(unsigned char *destStr, unsigned int destLen, char *targetMd5);

void clearTargetDir(const char *dir);

int copyFile(const char *targetFile, const char *originFile);

int getBoardSysInfo(BoardSysInfo_t* info);

int playAudio(char* file);

void storage_info_get(const char *mount_point, SDCardInfo_t *info);

void getWifiMac(char* wifiNode, char* mac);

int mountTfCard(const char* path, const char* node);

int remountTfCard(const char* path, const char* node);

int formatAndMountTFCard(const char* path, const char* node);

void setFlagStatus(const char* name, char status);

char getFlagStatus(const char* name);

int getProcessPID(char* filter, char* tag);

void setSpeakerVolume(int level);

void getAppPlatformParams(AppPlatformParams_t* param, int group);

void setAppPlatformParamItems(int item, char* value, int group);

void setAppPlatformParams(AppPlatformParams_t* param, int group);

int checkTFCard(const char* sdPath);

#ifdef __cplusplus
}
#endif

#endif
