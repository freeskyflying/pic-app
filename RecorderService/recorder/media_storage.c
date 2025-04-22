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
#include "osal.h"

#include "pnt_log.h"
#include "utils.h"
#include "media_video.h"
#include "media_utils.h"
#include "media_storage.h"
#include "normal_record_db.h"
#include "media_cache.h"
#include "gpio_led.h"
#include "strnormalize.h"
#include "gps_recorder.h"
#include "system_param.h"
#include "media_snap.h"

volatile bool_t gStorageThreadStart = FALSE;
volatile bool_t gStorageThreadFlag = FALSE;

queue_list_t gVideoListQueue[MAX_VIDEO_NUM] = { 0 };
queue_list_t gVideoFreeListQueue[MAX_VIDEO_NUM] = { 0 };
volatile char gVideoListQueueQueryState[MAX_VIDEO_NUM] = { 0 };
queue_list_t gParkPhotoListQueue[MAX_VIDEO_NUM] = { 0 };

long long gVideoVehicelAlarmFlag[MAX_VIDEO_NUM] = { 0 };

static uint8_t gRecLedStatus = 0;
volatile uint8_t gUpdateTFcardScanFlag = 0;
static pthread_mutex_t gStorageMutex;

int SDCard_VideoInit(void)
{
    int ret = RET_SUCCESS;
    char chPath[128] = { 0 };

    if (-1 == access(VIDEO_PATH, F_OK))
    {
        mkdir(VIDEO_PATH, 0666);
    }

    if (-1 == access(VIDEO_PATH, W_OK | R_OK))
    {
        ret = RET_FAILED;
    }
    else
    {
    	int count = 0;
        for (int i=1; i<=3; i++)
        {
            sprintf(chPath, VIDEO_CH_PATH_FORMAT, i);
            if (0 == access(chPath, F_OK))
            {
            	count ++;
            }
        }

		if (count != 0 && count != MAX_VIDEO_NUM)
		{
			remove(VIDEO_PATH"/.format");
			PNT_LOGE("fatal error, version channel count not equel with tfcard, reformat.");
		}
		
        for (int i=1; i<=MAX_VIDEO_NUM; i++)
        {
            sprintf(chPath, VIDEO_CH_PATH_FORMAT, i);
            if (-1 == access(chPath, F_OK))
            {
                mkdir(chPath, 0666);
            }
        }

	    if (-1 == access(VIDEO_THUMB_PATH, F_OK))
	    {
	        mkdir(VIDEO_THUMB_PATH, 0666);
	    }
    }

	if (-1 == access(ALARM_PATH, F_OK))
	{
        mkdir(ALARM_PATH, 0666);
	}

	if (-1 == access(PARKING_PATH, F_OK))
	{
        mkdir(PARKING_PATH, 0666);
		
        for (int i=1; i<=MAX_VIDEO_NUM; i++)
        {
            sprintf(chPath, PARKPHOTO_CH_PATH_FORMAT, i);
            if (-1 == access(chPath, F_OK))
            {
                mkdir(chPath, 0666);
            }
        }
	}

	if (-1 == access(EVENT_PATH, F_OK))
	{
        mkdir(EVENT_PATH, 0666);
	}

	if (-1 == access(GPSDATA_PATH, F_OK))
	{
        mkdir(GPSDATA_PATH, 0666);
	}

    return ret;
}

int event_process(sys_event_e ev)
{
	PNT_LOGI("Receive %d event", ev);
	switch (ev)
    {
	case SYS_EVENT_SDCARD_IN:
        gStorageThreadStart = TRUE;
        gUpdateTFcardScanFlag = 1;
	    break;
	case SYS_EVENT_SDCARD_OUT:
        gUpdateTFcardScanFlag = 0;
		if (gStorageThreadStart)
		{
			system("reboot -f");
		}
		else
		{
			playAudio("tfcard_out.pcm");
		}
        gStorageThreadStart = FALSE;
        MediaVideo_SetRecordStatus(FALSE);
	    break;
	default:
	    break;
	}

	return 0;
}

int event_monitor_init()
{
	monitor_dev_t dev;

	/* sdcard storage */
	PNT_LOGI("register sdcard detect.");
	memset(&dev, 0, sizeof(dev));
	strcpy(dev.name, "sdcard");
	dev.type = EVENT_DEV_TYPE_SDCARD;
	dev.gpio = -1;
	osal_event_monitor_register(&dev);

	osal_event_monitor_start(event_process);

	return 0;
}

void get_thumbname_by_videoname(char* thumbName, char* videoName)
{
	int i = strlen(videoName);
	while (i>0)
	{
		if (videoName[i-1] == '/')
		{
			break;
		}
		i--;
	}

	sprintf(thumbName, VIDEO_THUMB_FILENAME_FORMAT, &videoName[i]);
}

const char FILESTR[6][2] = {"f", "i", "r", "l", "r", "l"};

void get_videoname_by_time(int logicChn, time_t timeval, char* videoName)
{
	struct tm p;
	localtime_r(&timeval, &p);
	
	sprintf(videoName, VIDEO_FILENAME_FORMAT, logicChn, logicChn, 
		(p.tm_year + YEAR_OFFSET), (p.tm_mon + MON_OFFSET), p.tm_mday, p.tm_hour, p.tm_min, p.tm_sec, 
		FILESTR[logicChn-1]);
}

void get_photoname_by_time(int logicChn, time_t timeval, char* photoName)
{
	struct tm p;
	localtime_r(&timeval, &p);
	
	sprintf(photoName, PARKPHOTO_FILENAME_FORMAT, logicChn, logicChn, 
		(p.tm_year + YEAR_OFFSET), (p.tm_mon + MON_OFFSET), p.tm_mday, p.tm_hour, p.tm_min, p.tm_sec, 
		FILESTR[logicChn-1]);
}

int get_videofile_duration(char* videofile)
{
	struct stat buf = { 0 };
	memset(&buf, 0, sizeof(struct stat));
	stat(videofile, &buf);
	
	int duration = 0;
	int fd = open(videofile, O_RDONLY);
	if (fd > 0)
	{
		pread(fd, &duration, 4, buf.st_size-4);
		close(fd);
	}

	return duration;
}

void get_videofile_extraInfo(char* videofile, VideoFileExtraInfo_t* info)
{
	struct stat buf = { 0 };
	memset(&buf, 0, sizeof(struct stat));
	stat(videofile, &buf);
	
	int fd = open(videofile, O_RDONLY);

	info->fileSize = buf.st_size;

	info->alertFlag = 0;
	pread(fd, &info->alertFlag, 4, buf.st_size-16);
	
	info->duration = 60;
	pread(fd, &info->duration, 4, buf.st_size-4);

	close(fd);
}

int do_fallocate(const char *path, unsigned long fsize)
{
    int res;
    int fd;

    fd = open(path, O_RDWR);
    if (fd >= 0)
    {
        close(fd);
        return 0;
    }

    fd = open(path, O_WRONLY | O_CREAT);
    if (fd < 0)
    {
        PNT_LOGE("open/create %s fail\n", path);
        return -1;
    }

    res = fallocate(fd, FALLOC_FL_KEEP_SIZE, 0, fsize);

	int duration = 0;
	pwrite(fd, &duration, 4, fsize-4);

    close(fd);

    if (res)
    {
        PNT_LOGE("fallocate fail(%i)\n", res);
    }

    return res;
}

void encrypt_video_file(int fd, int filesize)
{
	char buff[4] = { 0 };
	uint32_t offset = 0, mdatSize = 0;
	
	pread(fd, buff, 4, 0);
	offset = ((buff[0]<<24)&0xFF000000) + ((buff[1]<<16)&0x00FF0000) + ((buff[2]<<8)&0x0000FF00) + ((buff[3])&0x000000FF);
	
	pread(fd, buff, 4, offset);
	mdatSize = ((buff[0]<<24)&0xFF000000) + ((buff[1]<<16)&0x00FF0000) + ((buff[2]<<8)&0x0000FF00) + ((buff[3])&0x000000FF);
	
	buff[0] = offset >> 24;
	buff[1] = offset >> 16;
	buff[2] = offset >> 8;
	buff[3] = offset;
	pwrite(fd, buff, 4, offset);
	
	pwrite(fd, buff, 4, offset+mdatSize+4);
	
	buff[0] = mdatSize >> 24;
	buff[1] = mdatSize >> 16;
	buff[2] = mdatSize >> 8;
	buff[3] = mdatSize;
	pwrite(fd, buff, 4, filesize - 20);
	
	PNT_LOGI("mdatsize: %d", mdatSize);
}

int decrypt_video_file(char* fileName)
{
	char buff[4] = { 0 };
	uint32_t offset = 0, mdatSize = 0;
	
	struct stat buf = { 0 };
	memset(&buf, 0, sizeof(struct stat));
	stat(fileName, &buf);
	
	int fd = open(fileName, O_RDWR);
	if (fd >= 0)
	{
		pread(fd, buff, 4, 0);
		offset = ((buff[0]<<24)&0xFF000000) + ((buff[1]<<16)&0x00FF0000) + ((buff[2]<<8)&0x0000FF00) + ((buff[3])&0x000000FF);
		
		pread(fd, buff, 4, offset);
		mdatSize = ((buff[0]<<24)&0xFF000000) + ((buff[1]<<16)&0x00FF0000) + ((buff[2]<<8)&0x0000FF00) + ((buff[3])&0x000000FF);
		
		if (mdatSize == offset) // 确认是加密文件，需要解密
		{
			pread(fd, buff, 4, buf.st_size - 20);
			mdatSize = ((buff[0]<<24)&0xFF000000) + ((buff[1]<<16)&0x00FF0000) + ((buff[2]<<8)&0x0000FF00) + ((buff[3])&0x000000FF);

			pwrite(fd, buff, 4, offset);
			
			buff[0] = 'm';
			buff[1] = 'o';
			buff[2] = 'o';
			buff[3] = 'v';
			pwrite(fd, buff, 4, offset+mdatSize+4);
		}

		close(fd);
	}
	else
	{
		return RET_FAILED;
	}

	return RET_SUCCESS;
}

void close_video_file(int venChn, char* fileName)
{
    if (gVideoListQueueQueryState[venChn])
    {
		int fd = open(fileName, O_RDWR);
		if (fd >= 0)
		{
			time_t fileTime = 0;
			int filesize = gMediaStream[venChn].param.videoSize;
			int duration = 0;

			pread(fd, &fileTime, 4, filesize - 8);
			#if (defined VDENCRYPT)
			encrypt_video_file(fd, filesize);
			#endif
			pwrite(fd, &gVideoVehicelAlarmFlag[venChn], 8, filesize - 16);

			duration = (currentTimeMillis()/1000 - fileTime)/gMediaStream[0].param.minutePerFile;
			pwrite(fd, &duration, 4, filesize - 4);
			close(fd);

			PNT_LOGE("push file:%s -  %d.\n", fileName, fileTime);
        	queue_list_push(&gVideoListQueue[venChn], (void*)fileTime);
		}
		else
		{
			PNT_LOGE("open file:%s -  failed.\n", fileName);
		}
    }
	else
	{
		PNT_LOGE("close file:%s - not finish.\n", fileName);
	}
}

int fallocate_video_file(char* fileName, time_t fileTime, int chn, int size)
{
    int ret = 0, retry = 10;
    SDCardInfo_t sdInfo = { 0 };
    char tempFileName[128] = { 0 };

	if (0 == queue_list_get_count(&gVideoFreeListQueue[chn]) && 0 == queue_list_get_count(&gVideoListQueue[chn]))
	{
		PNT_LOGE("record storage fatal error.");
		sleep(1);
		system("reboot -f");
		return RET_FAILED;
	}
	
	get_videoname_by_time(chn+1, fileTime, fileName);

	int idx = (int)queue_list_popup(&gVideoFreeListQueue[chn]);
	if (idx > 0)
	{
		sprintf(tempFileName, VIDEO_CH_PATH_FORMAT"/%04d.free", (chn+1), idx);
	}
	else
	{
		time_t tfileTime = (time_t)queue_list_popup(&gVideoListQueue[chn]);
		
		get_videoname_by_time(chn+1, tfileTime, tempFileName);

		char thumbTempName[128] = { 0 };
		char thumbName[128] = { 0 };
		get_thumbname_by_videoname(thumbTempName, tempFileName);
		get_thumbname_by_videoname(thumbName, fileName);
		rename(thumbTempName, thumbName);
	}

	if (strlen(tempFileName) == 0)
	{
		PNT_LOGE("get file %s failed.", fileName);
		return RET_FAILED;
	}
	PNT_LOGE("rename file %s to %s.", tempFileName, fileName);
	rename(tempFileName, fileName);

	int fd = open(fileName, O_WRONLY);
	if (fd >= 0)
	{
		pwrite(fd, &fileTime, 4, gMediaStream[chn].param.videoSize - 8);

		fileTime = 0;
		pwrite(fd, &fileTime, 4, gMediaStream[chn].param.videoSize - 4);
		
		close(fd);
	}

	gVideoVehicelAlarmFlag[chn] = 0;

    if (!(gRecLedStatus & (1<<chn)))
    {
        setLedStatus(chn==0?REC_LED1:REC_LED2, 1);
        gRecLedStatus |= (1<<chn);
    }

    return RET_SUCCESS;
}

void dir_video_query(int chn)
{
    DIR *pDir;
    struct dirent *ptr;
    char dirPath[128];
    uint8_t bcdTime[6] = { 0 };
    struct stat sb;
	
    queue_list_clear(&gVideoListQueue[chn]);
	queue_list_clear(&gVideoFreeListQueue[chn]);

    sprintf(dirPath, VIDEO_CH_PATH_FORMAT, (chn+1));

    pDir = opendir(dirPath);
    if (pDir == NULL)
    {
    	gVideoListQueueQueryState[chn] = 1;
        PNT_LOGE("open %s failed.", dirPath);
        return;
    }
    
    long long starttime = currentTimeMillis();
	pthread_mutex_lock(&gVideoListQueue[chn].m_mutex);
	queue_t* tail = NULL, *cur = NULL, *temp = NULL, *tempLast = NULL;
	
	int j = 0;
    while (1)
    {
        ptr = readdir(pDir);

        if (ptr == NULL)
        {
            break;
        }
        if (!gStorageThreadStart)
        {
            break;
        }

        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;

		if (strstr(ptr->d_name, ".free"))
		{
			memset(bcdTime, 0, sizeof(bcdTime));
			memcpy(bcdTime, ptr->d_name, 4);
			int idx = atoi((char*)bcdTime);
			queue_list_push(&gVideoFreeListQueue[chn], (void*)idx);
		}
		else if (strstr(ptr->d_name, ".mp4"))
		{
	        bcdTime[0] = ((ptr->d_name[6]-'0')<<4) + (ptr->d_name[7]-'0');
	        bcdTime[1] = ((ptr->d_name[8]-'0')<<4) + (ptr->d_name[9]-'0');
	        bcdTime[2] = ((ptr->d_name[10]-'0')<<4) + (ptr->d_name[11]-'0');
	        bcdTime[3] = ((ptr->d_name[13]-'0')<<4) + (ptr->d_name[14]-'0');
	        bcdTime[4] = ((ptr->d_name[15]-'0')<<4) + (ptr->d_name[16]-'0');
	        bcdTime[5] = ((ptr->d_name[17]-'0')<<4) + (ptr->d_name[18]-'0');

	        time_t timeStart = convertBCDToSysTime(bcdTime);

			cur = (queue_t*)malloc(sizeof(queue_t));
			memset(cur, 0, sizeof(queue_t));
			cur->m_content = (void*)timeStart;
			
			if (NULL == gVideoListQueue[chn].m_head)
			{
				gVideoListQueue[chn].m_head = cur;
			}
			else
			{
				temp = gVideoListQueue[chn].m_head;
				if (tail->m_content > cur->m_content)
				{
					while (temp)
					{
						if (cur->m_content < temp->m_content)
						{
							timeStart = (time_t)cur->m_content;
							cur->m_content = temp->m_content;
							temp->m_content = (void*)timeStart;
						}
						temp = temp->next;
					}
				}
				
				cur->prev = tail;
				tail->next = cur;
				gVideoListQueue[chn].m_head->prev = cur;
			}
			tail = cur;
			
			usleep(10);
		}
    }

	PNT_LOGE("query file [%d] cost time: %lld\n", chn, currentTimeMillis() - starttime);
#if 0
	queue_t* head = gVideoListQueue[chn].m_head;
	while(NULL != head)
	{
		char tempFileName[128] = { 0 };
		time_t fileTime = (time_t)head->m_content;
		struct tm* pTm = localtime(&fileTime);
		
		sprintf(tempFileName, VIDEO_FILENAME_FORMAT, (chn+1), (chn+1), (pTm->tm_year + YEAR_OFFSET), 
			(pTm->tm_mon + MON_OFFSET), pTm->tm_mday, pTm->tm_hour, pTm->tm_min, pTm->tm_sec, (chn==0)?"f":"r");
		
		printf("time: %ld %s\n", fileTime, tempFileName);
		head = head->next;
	}
	head = gVideoFreeListQueue[chn].m_head;
	while(NULL != head)
	{
		char tempFileName[128] = { 0 };
		int idx = (int)head->m_content;
		
		sprintf(tempFileName, VIDEO_CH_PATH_FORMAT"/%04d.free", (chn+1), idx);
		
		printf("idx: %ld %s\n", idx, tempFileName);
		head = head->next;
	}
#endif
	pthread_mutex_unlock(&gVideoListQueue[chn].m_mutex);

    gVideoListQueueQueryState[chn] = 1;
    
    closedir(pDir);
}

void dir_photo_query(int chn)
{
    DIR *pDir;
    struct dirent *ptr;
    char dirPath[128];
    uint8_t bcdTime[6] = { 0 };
    struct stat sb;
	
    queue_list_clear(&gParkPhotoListQueue[chn]);

    sprintf(dirPath, PARKPHOTO_CH_PATH_FORMAT, (chn+1));

    pDir = opendir(dirPath);
    if (pDir == NULL)
    {
    	gVideoListQueueQueryState[chn] = 1;
        PNT_LOGE("open %s failed.", dirPath);
        return;
    }
    
    long long starttime = currentTimeMillis();
	pthread_mutex_lock(&gParkPhotoListQueue[chn].m_mutex);
	queue_t* tail = NULL, *cur = NULL, *temp = NULL, *tempLast = NULL;
	
	int j = 0;
    while (1)
    {
        ptr = readdir(pDir);

        if (ptr == NULL)
        {
            break;
        }
        if (!gStorageThreadStart)
        {
            break;
        }

        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;

		if (strstr(ptr->d_name, ".jpg"))
		{
	        bcdTime[0] = ((ptr->d_name[6]-'0')<<4) + (ptr->d_name[7]-'0');
	        bcdTime[1] = ((ptr->d_name[8]-'0')<<4) + (ptr->d_name[9]-'0');
	        bcdTime[2] = ((ptr->d_name[10]-'0')<<4) + (ptr->d_name[11]-'0');
	        bcdTime[3] = ((ptr->d_name[13]-'0')<<4) + (ptr->d_name[14]-'0');
	        bcdTime[4] = ((ptr->d_name[15]-'0')<<4) + (ptr->d_name[16]-'0');
	        bcdTime[5] = ((ptr->d_name[17]-'0')<<4) + (ptr->d_name[18]-'0');

	        time_t timeStart = convertBCDToSysTime(bcdTime);

			cur = (queue_t*)malloc(sizeof(queue_t));
			memset(cur, 0, sizeof(queue_t));
			cur->m_content = (void*)timeStart;
			
			if (NULL == gParkPhotoListQueue[chn].m_head)
			{
				gParkPhotoListQueue[chn].m_head = cur;
			}
			else
			{
				temp = gParkPhotoListQueue[chn].m_head;
				if (tail->m_content > cur->m_content)
				{
					while (temp)
					{
						if (cur->m_content < temp->m_content)
						{
							timeStart = (time_t)cur->m_content;
							cur->m_content = temp->m_content;
							temp->m_content = (void*)timeStart;
						}
						temp = temp->next;
					}
				}
				
				cur->prev = tail;
				tail->next = cur;
				gParkPhotoListQueue[chn].m_head->prev = cur;
			}
			tail = cur;
			usleep(10);
		}
    }

	PNT_LOGE("query parking photo file [%d] cost time: %lld\n", chn, currentTimeMillis() - starttime);

	pthread_mutex_unlock(&gParkPhotoListQueue[chn].m_mutex);
    
    closedir(pDir);
}

int media_storage_filesystem_format(void)
{
	MediaCache_FileInit();

	if (RET_FAILED == SDCard_VideoInit())
	{
		PNT_LOGE("sdcard video init failed.");
		return RET_FAILED;
	}
	
	if (0 == access(VIDEO_PATH"/.format", F_OK))
	{
		PNT_LOGI("sdcard filesystem has formated.");
		return RET_SUCCESS;
	}

	system("rm -rf "VIDEO_PATH"/*");
	system("rm -rf "ALARM_PATH);
	system("rm -rf "EVENT_PATH);
	system("rm -rf "PARKING_PATH);
	if (RET_FAILED == SDCard_VideoInit())
	{
		PNT_LOGE("sdcard video init failed.");
		return RET_FAILED;
	}

	MediaCache_FileInit();

	long long timestart = currentTimeMillis();

    SDCardInfo_t sdInfo = { 0 };
	storage_info_get(SDCARD_PATH, &sdInfo);
	
	unsigned int oneMiniteSize = 0;
	for (int chn=0; chn<MAX_VIDEO_NUM; chn++)
	{
		oneMiniteSize += gMediaStream[chn].param.videoSize;
	}

	int resvSize = (sdInfo.free)<(sdInfo.total/100*SDCARD_NORMAL_DUTY)?sdInfo.free:(sdInfo.total/100*SDCARD_NORMAL_DUTY);
	int count = resvSize/(oneMiniteSize/1024);
	
	int i = 0, state = 0;
	while (i<count)
	{
    	char tempFileName[128] = { 0 };
		
		for (int chn=0; chn<MAX_VIDEO_NUM; chn++)
		{
			sprintf(tempFileName, VIDEO_CH_PATH_FORMAT"/%04d.free", chn+1, i+1);
			do_fallocate(tempFileName, gMediaStream[chn].param.videoSize);
		}
		
		i ++;
		if (i % 10 == 0)
		{
        	setLedStatus(REC_LED1, state);
        	setLedStatus(REC_LED2, state);
			state = !state;
		}
	}

	i = 0;
	while (i<count)
	{
    	char tempFileName[128] = { 0 };
		sprintf(tempFileName, GPSDATA_PATH"/%04d.free", i+1);
		do_fallocate(tempFileName, 12*1024);
		i ++;
	}
	
	setLedStatus(REC_LED1, 0);
	setLedStatus(REC_LED2, 0);
	
	printf("format cost time : %lld ms.\n", currentTimeMillis() - timestart);

	int fd = open(VIDEO_PATH"/.format", O_WRONLY|O_CREAT);
	if (fd < 0)
	{
		PNT_LOGE("sdcard filesystem set formated flag failed.");
		return RET_FAILED;
	}
	close(fd);

	return RET_SUCCESS;
}

void MediaStorage_DeleteOldestFiles(char* dir, int count, int duty)
{
    struct dirent *dp;
	char filename[512] = { 0 };
	struct stat sta = { 0 };
	unsigned int dirSize = 0;
    DIR *dirp = opendir(dir);
	unsigned int filecount = 0;

	if (NULL == dirp)
	{
		return;
	}

	if (count > 50)
	{
		count = 50;
	}

	char removeFileList[50][32];
	time_t removeFileTimeList[50];

	memset(removeFileList, 0, 50*32);
	memset(removeFileTimeList, 0, sizeof(removeFileTimeList));
	
	while ((dp = readdir(dirp)) != NULL)
	{
		if(strcmp(dp->d_name,".")==0 || strcmp(dp->d_name,"..")==0)    ///current dir OR parrent dir
        {
        	continue;
        }

		sprintf(filename, "%s/%s", dir, dp->d_name);
		
		memset(&sta, 0, sizeof(struct stat));
		stat(filename, &sta);

		dirSize += sta.st_size;

		if (0 == removeFileTimeList[0] || removeFileTimeList[0] > sta.st_mtime)
		{
			for (int i=count-1; i>=0; i--)
			{
				if (0 == removeFileTimeList[i] || removeFileTimeList[i] > sta.st_mtime)
				{
					if (0 != i && 0 != removeFileTimeList[i])
					{
						memcpy(removeFileList, removeFileList[1], i*32); // 往前移
						memcpy(removeFileTimeList, &removeFileTimeList[1], i*sizeof(time_t));
					}

					removeFileTimeList[i] = sta.st_mtime;
					strcpy(removeFileList[i], dp->d_name);
					break;
				}
			}
		}
		usleep(1000);
		filecount ++;
    }
	
    (void) closedir(dirp);

	SDCardInfo_t info = { 0 };
	storage_info_get(SDCARD_PATH, &info);

	int dirSizeMax = info.total*duty/100;

	if (dirSize/1024 >= dirSizeMax || ((info.total > 0) && (info.free < SDCARD_RESV)))
	{
		for (int i=0; i<count; i++)
		{
			if (0 != removeFileTimeList[i])
			{
				char cmd[128] = { 0 };
				sprintf(cmd, "rm -rf %s/%s &", dir, removeFileList[i]);
				PNT_LOGE(cmd);
				system(cmd);
			}
		}
	}
}

void media_video_recycle(void)
{
    SDCardInfo_t sdInfo = { 0 };
	char fileName[128] = { 0 };
	char cmd[128] = { 0 };

	storage_info_get(SDCARD_PATH, &sdInfo);

	if (sdInfo.free < (SDCARD_RESV/2))
	{
		for (int chn=0; chn<MAX_VIDEO_NUM; chn++)
		{
			if (0 < queue_list_get_count(&gVideoFreeListQueue[chn]))
			{
				int idx = (int)queue_list_popup(&gVideoFreeListQueue[chn]);
				sprintf(cmd, "rm -rf "VIDEO_CH_PATH_FORMAT"/%04d.free", (chn+1), idx);
				PNT_LOGE(cmd);
				system(cmd);
			}
			else if (0 < queue_list_get_count(&gVideoListQueue[chn]))
			{
				time_t tfileTime = (time_t)queue_list_popup(&gVideoListQueue[chn]);
				get_videoname_by_time(chn+1, tfileTime, fileName);
				sprintf(cmd, "rm -rf %s", fileName);
				PNT_LOGE(cmd);
				system(cmd);
			}
		}
	}
}

static uint32_t gPhotoDirSizes[MAX_VIDEO_NUM] = { 0 };
static void dir_parkphoto_querysize(int chn)
{
	struct stat sta = { 0 };
	unsigned int dirSize = 0;
	char photoname[128] = { 0 };
	unsigned int fileCount = 0;

	if (chn >= MAX_VIDEO_NUM)
	{
		return;
	}

	PNT_LOGI("start to query photo filesizes.");

	queue_t* head = gParkPhotoListQueue[chn].m_head;
	while (head && ((gGlobalSysParam->parking_monitor) && 0 == gACCState) && fileCount < 2000)
	{
		get_photoname_by_time(chn+1, (time_t)head->m_content, photoname);
		if (0 == access(photoname, F_OK))
		{
			memset(&sta, 0, sizeof(struct stat));
			stat(photoname, &sta);
			dirSize += sta.st_size;

			fileCount ++;
		}
		
		head = head->next;
		usleep(10);
	}

	if (fileCount >= 2000)
	{
		SDCardInfo_t info = { 0 };
		storage_info_get(SDCARD_PATH, &info);
		gPhotoDirSizes[chn] = info.total;
	}
	else
	{
		gPhotoDirSizes[chn] = dirSize/1024;
	}
	
	PNT_LOGI("end to query photo filesizes.");
}
static void dir_parkphoto_recycle(void)
{
	unsigned int dirSize = 0;
	char photoname[128] = { 0 };
	
	SDCardInfo_t info = { 0 };
	storage_info_get(SDCARD_PATH, &info);
	
	for (int i=0; i<MAX_VIDEO_NUM; i++)
	{
		dirSize += gPhotoDirSizes[i];
	}

	int dirSizeMax = info.total*SDCARD_PARKING_DUTY/100;

	if (dirSize >= dirSizeMax || ((info.total > 0) && (info.free < SDCARD_RESV)))
	{
		for (int i=0; i<MAX_VIDEO_NUM; i++)
		{
			for (int j=0; j<5; j++)
			{
				time_t timephoto = (time_t)queue_list_popup(&gParkPhotoListQueue[i]);
				if (0 != timephoto)
				{
					get_photoname_by_time(i+1, timephoto, photoname);
					remove(photoname);
				}
			}
		}
	}
}

void* media_storage_manage_proc(void* arg)
{
    char prname[30]={0};
    sprintf(prname,"StorageManager");
    prctl(PR_SET_NAME, prname, 0,0,0);

    time_t nowtime = time(NULL);
    struct tm p;
    localtime_r(&nowtime,&p);
    time_t oldtime = nowtime, oldAccTime = 0;

	gps_file_recorder_init();

    for (int i=0; i<MAX_VIDEO_NUM; i++)
    {
        queue_list_create(&gVideoListQueue[i], 99999);
        queue_list_create(&gVideoFreeListQueue[i], 99999);
        queue_list_create(&gParkPhotoListQueue[i], 99999);
    }

	int tips = 0, cnt = 0;
	gStorageThreadFlag = TRUE;

	pthread_mutex_init(&gStorageMutex, NULL);

wait_tfcard:

    setLedStatus(REC_LED1, 0);
    setLedStatus(REC_LED2, 0);
    gRecLedStatus = 0;

    for (int i=0; i<MAX_VIDEO_NUM; i++)
    {
        queue_list_clear(&gVideoListQueue[i]);
        queue_list_clear(&gVideoFreeListQueue[i]);
        queue_list_clear(&gParkPhotoListQueue[i]);
        gVideoListQueueQueryState[i] = 0;
    }

    while (!gStorageThreadStart)
    {
        SleepMs(100);
    }

    PNT_LOGI("find sdcard, check time and writable");
    while (gStorageThreadStart)
    {
        if (1690000000 < currentTimeMillis()/1000 && RET_SUCCESS == checkTFCard(NULL)) //未校时不进行录像
        {
            break;
        }
        SleepMs(1000);
    }
    
    PNT_LOGI("check time pass.");
	while (gStorageThreadFlag)
	{
	    if (RET_SUCCESS == media_storage_filesystem_format())
	    {
	    	gps_file_query();
			
			for (int i=0; i<MAX_VIDEO_NUM; i++)
			{
				dir_video_query(i);
			}
			if (!(gGlobalSysParam->parking_monitor) || gACCState)
			{
	        	GpsRec_SetRecordStatus(1);
		        MediaVideo_SetRecordStatus(TRUE);
				
	        	nowtime = time(NULL);
				localtime_r(&nowtime,&p);
				oldtime = nowtime;
			}
			else
			{
				oldtime = 0;
			}
			
			for (int i=0; i<MAX_VIDEO_NUM; i++)
			{
				dir_photo_query(i);
			}
			break;
	    }
        SleepMs(50);
	}

    PNT_LOGI("start Record success.");
	time_t last = nowtime;
	int queryIndex = 0;

park_mode:
	while ((gGlobalSysParam->parking_monitor) && 0 == gACCState)
	{
        nowtime = time(NULL);
        localtime_r(&nowtime,&p);

		if (nowtime<oldtime)
		{
			oldtime = nowtime;
		}

        if((nowtime-oldtime) >= 30)
        {			
        	PNT_LOGI("start to change snap file.");
            oldtime = nowtime;

			char photoname[128] = { 0 };
			time_t timephoto = currentTimeMillis()/1000;
	        for (int i=0; i<MAX_VIDEO_NUM; i++)
	        {
				setLedStatus(REC_LED1, 1);
				setLedStatus(REC_LED2, 1);
	        	get_photoname_by_time(i+1, timephoto, photoname);
				MediaSnap_CreateSnap(i, 1, 1280, 720, 10, photoname);
				queue_list_push(&gParkPhotoListQueue[i], (void*)timephoto);
	        }
			dir_parkphoto_querysize(queryIndex%MAX_VIDEO_NUM);
			dir_parkphoto_recycle();
			queryIndex ++;
        }

        SleepMs(50);
		
        if (FALSE == gStorageThreadStart)
        {
            goto wait_tfcard;
        }
	}

	PNT_LOGI("change to recorder.");
	
	if (0 == MediaVideo_GetRecordStatus(0) || 0 == MediaVideo_GetRecordStatus(1) || 0 == MediaVideo_GetRecordStatus(2))
	{
		GpsRec_SetRecordStatus(1);
		MediaVideo_SetRecordStatus(TRUE);
		
		nowtime = time(NULL);
		localtime_r(&nowtime,&p);
		oldtime = nowtime;
	}

    while (gStorageThreadFlag)
    {
        nowtime = time(NULL);
        localtime_r(&nowtime,&p);

		if (nowtime<oldtime)
		{
			PNT_LOGE("%lld %lld", nowtime, oldtime);
			oldtime = nowtime;
		}

        if((nowtime-oldtime)/60 >= (PER_FILE_RECORD_TIME*gMediaStream[0].param.minutePerFile))
        {
        	PNT_LOGI("start to change record file [%d - %d].", gGlobalSysParam->parking_monitor, gACCState);
            oldtime = nowtime;
			if ((gGlobalSysParam->parking_monitor) && 0 == gACCState)
			{
	        	GpsRec_SetRecordStatus(0);
				MediaVideo_SetRecordStatus(0);
				PNT_LOGI("acc off and parking monitor on, change parking mode.");
				goto park_mode;
			}
			else
			{
				GpsRec_SetSyncCount();
	            MediaVideo_SetSyncCount();
				
				media_video_recycle();
			}
        }
		if (nowtime - last >= 1)
		{
			last = nowtime;
			PNT_LOGI("second [%d]", (nowtime-oldtime));
		}

        SleepMs(50);
        if (FALSE == gStorageThreadStart)
        {
        	GpsRec_SetRecordStatus(0);
			MediaVideo_SetRecordStatus(0);
            goto wait_tfcard;
        }
    }
	
	PNT_LOGI("media_storage_manage_proc thread exit.");

    return NULL;
}

void MediaStorage_Start(void)
{    
    int ret = 0;

    pthread_t pid;
    ret = pthread_create(&pid, 0, media_storage_manage_proc, NULL);
    if (ret)
    {
        PNT_LOGE("errno=%d, reason(%s)\n", errno, strerror(errno));
    }

    sleep(1);

    event_monitor_init();
}

void MediaStorage_Exit(void)
{
	gStorageThreadFlag = FALSE;

    setLedStatus(REC_LED1, 0);
    setLedStatus(REC_LED2, 0);
	
	MediaVideo_SetRecordStatus(0);

	gps_file_recorder_stop();
}

void MediaStorage_UpdateVideoAlertFlag(int alert, int offset)
{
	for (int i=0; i<MAX_VIDEO_NUM; i++)
	{
		gVideoVehicelAlarmFlag[i] |= (((long long)alert)<<offset);
	}
}

