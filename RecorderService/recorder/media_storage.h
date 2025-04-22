#ifndef _MEDIA_STORAGE_H_
#define _MEDIA_STORAGE_H_

#include "queue_list.h"
#include "media_utils.h"

#define SDCARD_PATH                 "/mnt/card"

#define VIDEO_PATH                  SDCARD_PATH"/NORMAL"
#define VIDEO_CH_PATH_FORMAT        VIDEO_PATH"/CH%d"
#define VIDEO_THUMB_PATH			VIDEO_PATH"/THUMB"

#define VIDEO_THUMB_FILENAME_FORMAT	VIDEO_THUMB_PATH"/%s.jpg"
#define VIDEO_FILENAME_FORMAT       VIDEO_CH_PATH_FORMAT"/CH%d-%4d%02d%02d-%02d%02d%02d_%s.mp4"

#define ALARM_PATH                  SDCARD_PATH"/ALARM"
#define EVENT_PATH					SDCARD_PATH"/EVENT"
#define PARKING_PATH				SDCARD_PATH"/PARKING"

#define PARKPHOTO_CH_PATH_FORMAT    PARKING_PATH"/CH%d"
#define PARKPHOTO_FILENAME_FORMAT	PARKPHOTO_CH_PATH_FORMAT"/CH%d-%4d%02d%02d-%02d%02d%02d_%s.jpg"

#define SDCARD_RESV					(512*1024)
#define SDCARD_NORMAL_DUTY     		96//(2*1024*1024)
#define SDCARD_EVENT_DUTY      		1
#define SDCARD_PARKING_DUTY    		2
#define SDCARD_ALARM_DUTY      		1

#define YEAR_OFFSET                 (1900)
#define MON_OFFSET                  (1)

/*
* 常规录像mp4文件存储格式：
* 
* fileSize-20- mdat size
* fileSize-16- alertFlag
* fileSize-8 - filetime
* fileSize-4 - duration
*/

typedef struct
{

	long long alertFlag;
	unsigned int fileSize;
	unsigned int duration;

} VideoFileExtraInfo_t;

void close_video_file(int venChn, char* fileName);

int decrypt_video_file(char* fileName);

void MediaStorage_DBInserNewFile(int chn, int timeStart, uint8_t streamType, uint16_t duration, char* filename);

void MediaStorage_DBUpdateFileDuration(int chn, uint8_t streamType, uint16_t duration, char* filename);

int fallocate_video_file(char* fileName, time_t fileTime, int chn, int size);

void MediaStorage_Start(void);

void MediaStorage_Exit(void);

void get_thumbname_by_videoname(char* thumbName, char* videoName);

void get_videoname_by_time(int logicChn, time_t timeval, char* videoName);

void get_photoname_by_time(int logicChn, time_t timeval, char* photoName);

int get_videofile_duration(char* videofile);

void get_videofile_extraInfo(char* videofile, VideoFileExtraInfo_t* info);

extern queue_list_t gVideoListQueue[MAX_VIDEO_NUM];
extern queue_list_t gParkPhotoListQueue[MAX_VIDEO_NUM];

#endif

