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

volatile bool_t gStorageThreadStart = FALSE;
volatile bool_t gStorageThreadFlag = FALSE;

queue_list_t gVideoListQueue[MAX_VIDEO_NUM] = { 0 };
queue_list_t gVideoFreeListQueue[MAX_VIDEO_NUM] = { 0 };
volatile char gVideoListQueueQueryState[MAX_VIDEO_NUM] = { 0 };

static uint8_t gRecLedStatus = 0;

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

    return ret;
}

int event_process(sys_event_e ev)
{
	extern volatile uint8_t gUpdateTFcardScanFlag;
	
	PNT_LOGI("Receive %d event", ev);
	switch (ev)
    {
	case SYS_EVENT_SDCARD_IN:
        gStorageThreadStart = TRUE;
        gUpdateTFcardScanFlag = 1;
	    break;
	case SYS_EVENT_SDCARD_OUT:
        gUpdateTFcardScanFlag = 0;
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

void get_videoname_by_time(int logicChn, time_t timeval, char* videoName)
{
	struct tm p;
	localtime_r(&timeval, &p);
	
	sprintf(videoName, VIDEO_FILENAME_FORMAT, logicChn, logicChn, (p.tm_year + YEAR_OFFSET), (p.tm_mon + MON_OFFSET), p.tm_mday, p.tm_hour, p.tm_min, p.tm_sec, (logicChn==1)?"f":"r");
}

int get_videofile_duration(char* videofile)
{
	struct stat buf = { 0 };
	memset(&buf, 0, sizeof(struct stat));
	stat(videofile, &buf);
	
	int fd = open(videofile, O_RDONLY);
	int duration = 60;
	pread(fd, &duration, 4, buf.st_size-4);
	close(fd);

	return duration;
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

void close_video_file(int venChn, char* fileName)
{
    if (gVideoListQueueQueryState[venChn])
    {
		int fd = open(fileName, O_RDWR);
		if (fd >= 0)
		{
			time_t fileTime = 0;
			int filesize = (venChn==0?CH1_VIDEO_STREAM_SIZE:CH2_VIDEO_STREAM_SIZE);
			int duration = 0;

			pread(fd, &fileTime, 4, filesize - 8);

			duration = (currentTimeMillis()/1000 - fileTime);
			pwrite(fd, &duration, 4, filesize - 4);
			close(fd);

			PNT_LOGI("push file:%s -  %d.\n", fileName, fileTime);
        	queue_list_push(&gVideoListQueue[venChn], (void*)fileTime);
		}
    }
}

int fallocate_video_file(char* fileName, time_t fileTime, int chn, int size)
{
    int ret = 0, retry = 10;
    SDCardInfo_t sdInfo = { 0 };
    char tempFileName[128] = { 0 };
	
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
		pwrite(fd, &fileTime, 4, (chn==0?CH1_VIDEO_STREAM_SIZE:CH2_VIDEO_STREAM_SIZE) - 8);
		close(fd);
	}

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
		}
    }

	printf("query file [%d] cost time: %lld\n", chn, currentTimeMillis() - starttime);
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
	if (RET_FAILED == SDCard_VideoInit())
	{
		PNT_LOGE("sdcard video init failed.");
		return RET_FAILED;
	}

	long long timestart = currentTimeMillis();

    SDCardInfo_t sdInfo = { 0 };
	storage_info_get(SDCARD_PATH, &sdInfo);
	int count = ((sdInfo.free)<(sdInfo.total-SDCARD_NORMAL_RESV_SIZE)?sdInfo.free:(sdInfo.total-SDCARD_NORMAL_RESV_SIZE))/(CH1_VIDEO_STREAM_SIZE/1024+CH2_VIDEO_STREAM_SIZE/1024);
	int i = 0, state = 0;
	while (i<count)
	{
    	char tempFileName[128] = { 0 };
		
		sprintf(tempFileName, VIDEO_CH_PATH_FORMAT"/%04d.free", 1, i+1);
		do_fallocate(tempFileName, CH1_VIDEO_STREAM_SIZE);
	
		sprintf(tempFileName, VIDEO_CH_PATH_FORMAT"/%04d.free", 2, i+1);
		do_fallocate(tempFileName, CH2_VIDEO_STREAM_SIZE);
		
		i ++;
		if (i % 10 == 0)
		{
        	setLedStatus(REC_LED1, state);
        	setLedStatus(REC_LED2, state);
			state = !state;
		}
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

void* media_storage_manage_proc(void* arg)
{
    char prname[30]={0};
    sprintf(prname,"StorageManager");
    prctl(PR_SET_NAME, prname, 0,0,0);

    time_t nowtime = time(NULL);
    struct tm p;
    localtime_r(&nowtime,&p);
    time_t oldtime = nowtime, oldAccTime = 0;

    for (int i=0; i<MAX_VIDEO_NUM; i++)
    {
        queue_list_create(&gVideoListQueue[i], 99999);
        queue_list_create(&gVideoFreeListQueue[i], 99999);
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
        gVideoListQueueQueryState[i] = 0;
    }

	setFlagStatus("tfcard", 2);
    while (FALSE == gStorageThreadStart)
    {
        usleep(1000*50);
    	if (cnt++ == 60)
		{
	    	if (1 != getFlagStatus("restart") && !tips)
	    	{
	    		tips = 1;
	       	 	playAudio("no_tfcard.pcm");
	    	}
    	}
    }

	setFlagStatus("tfcard", 1);

	cnt = 0;
    while (gStorageThreadStart)
    {
    	if (cnt++ == 100)
		{
			if (remountTfCard(SDCARD_PATH, "/dev/mmcblk0p1"))
			{
				setFlagStatus("tfcard", 13);
				int ret = formatAndMountTFCard(SDCARD_PATH, "/dev/mmcblk0p1");
				if (-1 == ret)
				{
					setFlagStatus("tfcard", 99);
				}
				else if (-2 == ret)
				{
					setFlagStatus("tfcard", 3);
				}
			}
    	}
        if (RET_SUCCESS == checkTFCard(NULL))
        {
            break;
        }
        usleep(1000*100);
    }
	setFlagStatus("tfcard", 0);

    PNT_LOGE("find sdcard, check time");
    while (gStorageThreadStart)
    {
        if (1690000000 < currentTimeMillis()/1000) //未校时不进行录像
        {
            break;
        }
        sleep(1);
    }
    if (!gStorageThreadStart)
    {
        goto wait_tfcard;
    }
    
    PNT_LOGE("find sdcard, start to record");
	while (gStorageThreadFlag)
	{
	    if (RET_SUCCESS == media_storage_filesystem_format())
	    {
	    	for (int i=0; i<MAX_VIDEO_NUM; i++)
	    	{
				dir_video_query(i);
	    	}
			MediaStorage_AlarmRecycle();
	        MediaVideo_SetRecordStatus(TRUE);
			break;
	    }
        usleep(1000*50);
	}

    while (gStorageThreadFlag)
    {
        nowtime = time(NULL);
        localtime_r(&nowtime,&p);
        // to create a new file
        //if(nowtime/60 >= (oldtime/60+PER_FILE_RECORD_TIME))
        if((nowtime-oldtime)/60 >= (PER_FILE_RECORD_TIME))
        {
            oldtime = nowtime;
            //setAudioSaveControl(true);
            MediaVideo_SetSyncCount();
        }
        
        usleep(1000*50);
        if (FALSE == gStorageThreadStart)
        {
            goto wait_tfcard;
        }
    }

    return NULL;
}

void MediaStorage_AlarmRecycle(void)
{
	SDCardInfo_t info = { 0 };

	int retry = 0;

retry_del:
	storage_info_get(SDCARD_PATH, &info);

	if (info.total > 0)
	{
		if (info.free < SDCARD_ALARM_RESV_SIZE)
		{
		    DIR *dirp;
		    struct dirent *dp;
			char filename[32] = "33";
			
		    dirp = opendir(ALARM_PATH);
			while ((dp = readdir(dirp)) != NULL)
    		{
    			if(strcmp(dp->d_name,".")==0 || strcmp(dp->d_name,"..")==0)    ///current dir OR parrent dir
		        {
		        	continue;
		        }
				// 删除时间最早的文件，直接使用文件名判断，因为文件名即是时间
				if (strcmp(dp->d_name, filename) < 0)
				{
					memcpy(filename, dp->d_name, 12);
				}
		    }
			
		    (void) closedir(dirp);

			if (strlen(filename) > 3)
			{
				char cmd[128] = { 0 };
				sprintf(cmd, "rm -rf %s/%s*", ALARM_PATH, filename);
				PNT_LOGE(cmd);
				system(cmd);
			}
			retry ++;
			if (5 > retry)
			{
				goto retry_del;
			}
		}
	}
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
}

