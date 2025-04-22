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

#define SDCARD_NORMAL_RESV_SIZE           (2*1024*1024)

#define SDCARD_ALARM_RESV_SIZE            (128*1024)

#define YEAR_OFFSET                 (1900)
#define MON_OFFSET                  (1)

void close_video_file(int venChn, char* fileName);

void MediaStorage_DBInserNewFile(int chn, int timeStart, uint8_t streamType, uint16_t duration, char* filename);

void MediaStorage_DBUpdateFileDuration(int chn, uint8_t streamType, uint16_t duration, char* filename);

int fallocate_video_file(char* fileName, time_t fileTime, int chn, int size);

void MediaStorage_Start(void);

void MediaStorage_Exit(void);

void MediaStorage_AlarmRecycle(void);

void get_thumbname_by_videoname(char* thumbName, char* videoName);

void get_videoname_by_time(int logicChn, time_t timeval, char* videoName);

int get_videofile_duration(char* videofile);

extern queue_list_t gVideoListQueue[MAX_VIDEO_NUM];

#endif

