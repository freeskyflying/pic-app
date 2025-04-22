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
#include "gps_recorder.h"
#include "media_video.h"
#include "imu/inv_imu_driver.h"

queue_list_t gGpsFileListQueue = { 0 };
queue_list_t gGpsFileFreeListQueue = { 0 };
extern volatile bool_t gStorageThreadStart;
extern BoardSysInfo_t gBoardInfo;

GpsRecorder_t gGpsRecorder = { 0 };

void gps_file_get_name(time_t timeval, char* filename)
{
	struct tm p;
	localtime_r(&timeval, &p);
	
	sprintf(filename, GPSDATA_FILENAME_FORMAT, (p.tm_year + YEAR_OFFSET), (p.tm_mon + MON_OFFSET), p.tm_mday, p.tm_hour, p.tm_min, p.tm_sec);
}

static FILE* gps_file_new_file(void)
{
	char filename[128] = { 0 };
    char tempFileName[128] = { 0 };

	int fileTime = currentTimeMillis()/1000;
	gps_file_get_name(fileTime, filename);
	
	int idx = (int)queue_list_popup(&gGpsFileFreeListQueue);
	if (idx > 0)
	{
		sprintf(tempFileName, GPSDATA_PATH"/%04d.free", idx);
	}
	else
	{
		time_t tfileTime = (time_t)queue_list_popup(&gGpsFileListQueue);
		
		gps_file_get_name(tfileTime, tempFileName);
	}
	
	if (strlen(tempFileName) == 0)
	{
		PNT_LOGE("get file %s failed.", filename);
		return NULL;
	}
	PNT_LOGE("rename file %s to %s.", tempFileName, filename);
	rename(tempFileName, filename);

	gGpsRecorder.fileTime = fileTime;

	FILE* pfile = fopen(filename, "wb");

	return pfile;
}

void gps_file_query(void)
{
    DIR *pDir;
    struct dirent *ptr;
    uint8_t bcdTime[6] = { 0 };
    struct stat sb;
	
    queue_list_clear(&gGpsFileListQueue);
	queue_list_clear(&gGpsFileFreeListQueue);

    pDir = opendir(GPSDATA_PATH);
    if (pDir == NULL)
    {
        PNT_LOGE("open %s failed.", GPSDATA_PATH);
        return;
    }
    
    long long starttime = currentTimeMillis();
	pthread_mutex_lock(&gGpsFileListQueue.m_mutex);
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
			queue_list_push(&gGpsFileFreeListQueue, (void*)idx);
		}
		else if (strstr(ptr->d_name, ".txt"))
		{
	        bcdTime[0] = ((ptr->d_name[2]-'0')<<4) + (ptr->d_name[3]-'0');
	        bcdTime[1] = ((ptr->d_name[4]-'0')<<4) + (ptr->d_name[5]-'0');
	        bcdTime[2] = ((ptr->d_name[6]-'0')<<4) + (ptr->d_name[7]-'0');
	        bcdTime[3] = ((ptr->d_name[9]-'0')<<4) + (ptr->d_name[10]-'0');
	        bcdTime[4] = ((ptr->d_name[11]-'0')<<4) + (ptr->d_name[12]-'0');
	        bcdTime[5] = ((ptr->d_name[13]-'0')<<4) + (ptr->d_name[14]-'0');

	        time_t timeStart = convertBCDToSysTime(bcdTime);

			cur = (queue_t*)malloc(sizeof(queue_t));
			memset(cur, 0, sizeof(queue_t));
			cur->m_content = (void*)timeStart;
			
			if (NULL == gGpsFileListQueue.m_head)
			{
				gGpsFileListQueue.m_head = cur;
			}
			else
			{
				temp = gGpsFileListQueue.m_head;
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
				gGpsFileListQueue.m_head->prev = cur;
			}
			tail = cur;
		}
    }

	PNT_LOGE("query file gpsfile cost time: %lld\n", currentTimeMillis() - starttime);
#if 0
	queue_t* head = gGpsFileListQueue.m_head;
	while(NULL != head)
	{
		char tempFileName[128] = { 0 };
		time_t fileTime = (time_t)head->m_content;
		struct tm* pTm = localtime(&fileTime);
		
		gps_file_get_name(fileTime, tempFileName);
		
		printf("time: %ld %s\n", fileTime, tempFileName);
		head = head->next;
	}
	head = gGpsFileFreeListQueue.m_head;
	while(NULL != head)
	{
		char tempFileName[128] = { 0 };
		int idx = (int)head->m_content;
		
		sprintf(tempFileName, GPSDATA_PATH"/%04d.free", idx);
		
		printf("idx: %ld %s\n", idx, tempFileName);
		head = head->next;
	}
#endif
	pthread_mutex_unlock(&gGpsFileListQueue.m_mutex);

    closedir(pDir);
}

static int LocationIPCIRcvDataProcess(char* buff, int len, int fd, void* arg)
{
	PNTIPC_t* ipc = (PNTIPC_t*)arg;
	GpsRecorder_t* handle = (GpsRecorder_t*)ipc->mOwner;

    if (len >= sizeof(GpsLocation_t))
    {
        memcpy(&handle->location, buff, sizeof(GpsLocation_t));

        return len;
    }
    else
    {
        return 0;
    }
}

inv_imu_sensor_event_t gImuSensorEvent = { 0 };
void imu_datas_callback(inv_imu_sensor_event_t *event)
{
	memcpy(&gImuSensorEvent, event, sizeof(gImuSensorEvent));
}

void* gps_file_record_task(void* parg)
{
	GpsRecorder_t* handle = (GpsRecorder_t*)parg;

	handle->runFlag = 1;

	char str[1024] = { 0 };
	struct tm p;
	time_t timeval = 0, lastTimeval = 0;

    LibPnt_IPCConnect(&handle->locationIPC, PNT_IPC_GPS_NAME, LocationIPCIRcvDataProcess, NULL, 256, handle);

	if (0 == strcmp(gBoardInfo.mProductModel, "P-DC-2") || 0 == strcmp(gBoardInfo.mProductModel, "P-DC-3"))
	{
		inv_imu_start(imu_datas_callback);
	}
	else
	{
		G_Sensor_SC7A20_Init(0);
	}

	while (handle->runFlag)
	{
        pthread_mutex_lock(&gGpsRecorder.mutex);
		
		if (REC_START == handle->recStatus || REC_STOP == handle->recStatus)
		{
			if (NULL != handle->pfile)
			{
				sync();
				fclose(handle->pfile);
				queue_list_push(&gGpsFileListQueue, (void*)gGpsRecorder.fileTime);
				handle->pfile = NULL;
			}
			
			if (REC_START == handle->recStatus)
			{
				handle->pfile = gps_file_new_file();
				handle->recStatus = REC_RECORDING;
			}
		}
		else
		{
			if (NULL != handle->pfile)
			{
				timeval = currentTimeMillis()/1000;

				if (timeval - lastTimeval >= gMediaStream[0].param.minutePerFile)
				{
					localtime_r(&timeval, &p);

					memset(str, 0, sizeof(str));

					sprintf(str, "%04d/%02d/%02d %02d:%02d:%02d", (p.tm_year + YEAR_OFFSET), (p.tm_mon + MON_OFFSET), p.tm_mday, p.tm_hour, p.tm_min, p.tm_sec);

					sprintf(str, "%s %c:%.6f %c:%.6f %.2f km/h %.2f %.2f %d", str, 
							handle->location.lat, handle->location.latitude, handle->location.lon, handle->location.longitude, handle->location.speed, 
							handle->location.bearing, handle->location.altitude, handle->location.satellites);

					double x = 0.0, y = 0.0, z = 0.0;
					if (0 == strcmp(gBoardInfo.mProductModel, "P-DC-2") || 0 == strcmp(gBoardInfo.mProductModel, "P-DC-3"))
					{
						get_imu_data();
						x = gImuSensorEvent.accel[0];
						y = gImuSensorEvent.accel[1];
						z = gImuSensorEvent.accel[2];
					}
					else
					{
						SC7A20_GetXYZAngle(&x, &y, &z);
					}

					sprintf(str, "%s x:%.3f y:%.3f z:%.3f\n", str, x, y, z);

					fwrite(str, 1, strlen(str), handle->pfile);

					lastTimeval = timeval;
				}
			}
			else
			{
				handle->pfile = gps_file_new_file();
			}
		}
		
        pthread_mutex_unlock(&gGpsRecorder.mutex);

		SleepMs(100);
	}

	LibPnt_IPCClientRelease(&handle->locationIPC);

	return NULL;
}

void GpsRec_SetRecordStatus(bool_t status)
{
    pthread_mutex_lock(&gGpsRecorder.mutex);
    if (status)
    {
        if (gGpsRecorder.recStatus == REC_STOP)
        {
            gGpsRecorder.recStatus = REC_START;
        }
    }
    else
    {
        gGpsRecorder.recStatus = REC_STOP;
    }
    pthread_mutex_unlock(&gGpsRecorder.mutex);
}

void GpsRec_SetSyncCount(void)
{
    pthread_mutex_lock(&gGpsRecorder.mutex);

    if (gGpsRecorder.recStatus == REC_RECORDING)
    {
        gGpsRecorder.recStatus = REC_START;
    }
        
    pthread_mutex_unlock(&gGpsRecorder.mutex);
}

void gps_file_recorder_init(void)
{
    queue_list_create(&gGpsFileListQueue, 99999);
    queue_list_create(&gGpsFileFreeListQueue, 99999);
	
    int ret = 0;

    pthread_mutex_init(&gGpsRecorder.mutex, NULL);

	memset(&gGpsRecorder, 0, sizeof(gGpsRecorder));

    ret = pthread_create(&gGpsRecorder.pid, 0, gps_file_record_task, &gGpsRecorder);
    if (ret)
    {
        PNT_LOGE("errno=%d, reason(%s)\n", errno, strerror(errno));
    }
}

void gps_file_recorder_stop(void)
{
	GpsRec_SetRecordStatus(0);
	
	gGpsRecorder.runFlag = 0;
}


