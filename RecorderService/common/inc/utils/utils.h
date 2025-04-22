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
#define VOICE_FILE_PATH_SPANISH			"/etc/audio_spain"

#define TERMINAL_OTA_PATH				"/mnt/card"
#define JT808_FTP_UPGRADE_TMP_PATH		"/data/jt808_ftp_upgrade_tmp"
#define JT808_FILE_UPGRADE_FILENAME		TERMINAL_OTA_PATH"/%s_map.zip"

#define JT808_FTP_DOWNLOAD_URL			"/data/.jt808FtpDownlaodUrl"

#define UPGRADE_FLAG_RESULT_FLAG		"/data/upgrade.flag"

typedef	int(*SQLExeCallBack)(void *NotUsed, int argc, char **argv, char **azColName);

typedef struct
{

	int m_lat;
	int m_lng;

} LatLng_t;

typedef struct
{

    char mProductModel[12];
    char mHwVersion[12];
    char mSwVersion[12];
    
} BoardSysInfo_t;

typedef struct
{

    unsigned int total; // Kb
    unsigned int free; // Kb

} SDCardInfo_t;

typedef struct
{

	char ip[64];
	int	port;
	char plateNum[64];
	char phoneNum[21];
	char plateColor;
	char protocalType;

} AppPlatformParams_t;

unsigned char bcdToDec(unsigned char data);

int setNonBlocking(int fd);

long long currentTimeMillis();

unsigned long getUptime();

long long str2num(const char *str);

bool_t deleteFile(const char *szFileName);

int getLocalBCDTime(char* bcd_time);

void getLocalTime(struct tm *t);

time_t convertBCDToSysTime(unsigned char* bcd);

void convertSysTimeToBCD(time_t timev, unsigned char* bcd);

int getTimeBCDOfDay(void);

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

void clearTargetDirExt(const char *dir, const char* filename);

int copyFile(const char *targetFile, const char *originFile);

int getBoardSysInfo(BoardSysInfo_t* info);

int playAudio(char* file);

void storage_info_get(const char *mount_point, SDCardInfo_t *info);

void getWifiMac(char* wifiNode, char* mac);

int mountTfCard(const char* path, const char* node);

int formatAndMountTFCard(const char* path, const char* node);

void setFlagStatus(const char* name, int status);

int getFlagStatus(const char* name);

int getProcessPID(char* filter, char* tag);

void setSpeakerVolume(int level);

void getAppPlatformParams(AppPlatformParams_t* param, int group);

void setAppPlatformParamItems(int item, char* value, int group);

void setAppPlatformParams(AppPlatformParams_t* param, int group);

int checkTFCard(const char* sdPath);

int get_distance_by_lat_long(int longitude1, int latitude1, int longitude2, int latitude2);

int is_in_rectangle(int lt_lat, int lt_lng, int rb_lat, int rb_lng, LatLng_t p);

int is_inside_polygon(LatLng_t *polygon,int N,LatLng_t p);

int get_distance_point_to_line(int longitude1, int latitude1, int longitude2, int latitude2, LatLng_t p);

void setUpgradeFileFlag(char* version);

int getUpgradeResult(char* curVersion);

void trigUpgrade(void);

float computeDistanceAndBearing(double lat1, double lon1, double lat2, double lon2);

unsigned int getDirStorageSize(const char* path);

extern void MediaStorage_UpdateVideoAlertFlag(int flag, int offset);

void remountTFCard(const char* path, const char* node);

void set_tz(void);

int URLDecode(const char* str, const int strSize, char* result, const int resultSize);

int getStringByFile(char* path, char* value, int len);

int setStringToFile(char* path, char* value);

void updateSysLastTime(void);

void loadSysLastTime(void);

int getCpuTemprature(void);

int playAudioBlock(char* file);

extern int gTimezoneOffset;

extern volatile unsigned char gACCState;
extern int gCPUTemprature;

#ifdef __cplusplus
}
#endif

#endif
