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

#include "media_cache.h"
#include "jt808_data.h"
#include "jt808_controller.h"
#include "jt808_session.h"
#include "jt808_utils.h"
#include "jt808_mediaevent.h"
#include "pnt_log.h"
#include "media_snap.h"

int JT808MediaEvent_GetVersion(JT808MediaEvent_t* handle)
{
    sqlite3_stmt *stmt = NULL;
	
    if (sqlite3_prepare_v2(handle->mDB, kSqlMediaGetVersion, strlen(kSqlMediaGetVersion), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error!");
        return -1;
    }
    if (sqlite3_step(stmt) != SQLITE_ROW)
	{
        PNT_LOGE("sqlite3_step!");
        return -1;
    }

    int version = sqlite3_column_int(stmt, 0);
	
    sqlite3_finalize(stmt);
	
    return version;
}

void JT808MediaEvent_SetVersion(JT808MediaEvent_t* handle, int version)
{
	char sql[1024] = {0};
	
	sprintf(sql, kSqlMediaSetVersion, version);
	
	ExecSql(handle->mDB, sql, NULL, NULL);
}

int JT808MediaEvent_InsertRecord(JT808MediaEvent_t* handle, BYTE eventType, BYTE mediaType, BYTE chnNum, unsigned int time, BYTE* location)
{
	pthread_mutex_lock(&handle->mMutex);

    char sql[1024] = {0};
    bool_t flag = TRUE;

    sqlite3_stmt *stmt = NULL;

    sprintf(sql, kSqlInsertOneMedia, eventType, mediaType, chnNum, time, 0, 0);
	
    if(sqlite3_prepare_v2(handle->mDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s %s", sqlite3_errmsg(handle->mDB), __FUNCTION__);
		pthread_mutex_unlock(&handle->mMutex);
        return -1;
    }

    sqlite3_exec(handle->mDB, "begin", 0, 0, NULL);
    sqlite3_bind_blob(stmt,1, location, 28, NULL);
    int ret = sqlite3_step(stmt);
	
    if(ret != SQLITE_DONE)
	{
        PNT_LOGE("sqlite3_step error!%d", ret);
        flag = FALSE;
    }
	
    if(!flag)
	{
        sqlite3_exec(handle->mDB, "rollback", 0, 0, NULL);;
    }
	else
	{
        sqlite3_exec(handle->mDB, "commit", 0, 0, NULL);
    }
    sqlite3_finalize(stmt);

	unsigned int mediaId = sqlite3_last_insert_rowid(handle->mDB);;

	pthread_mutex_unlock(&handle->mMutex);

	return mediaId;
}

void JT808MediaEvent_DeleteRecordByMediaID(JT808MediaEvent_t* handle, DWORD mediaId)
{
    char sql[1024] = {0};
	
	pthread_mutex_lock(&handle->mMutex);
	
	sprintf(sql, kSqlDeleteOneMedia, mediaId);
    ExecSql(handle->mDB, sql, NULL, NULL);
	
	pthread_mutex_unlock(&handle->mMutex);
}

void JT808MediaEvent_MediaVideoEvent(JT808MediaEvent_t* handle, BYTE eventType, BYTE chnNum, BYTE before, BYTE after)
{
	char filename[128] = { 0 };

	JT808MsgBody_0200_t body = { 0 };
	JT808Session_t* session = (JT808Session_t*)handle->owner;
	JT808Controller_t* controller = (JT808Controller_t*)session->mOnwer;

	BYTE location[28] = { 0 };
    int index = 0;

	index = NetbufPushDWORD(location, controller->mCurrentAlertFlag, index);
	index = NetbufPushDWORD(location, controller->mCurrentTermState, index);
	index = NetbufPushDWORD(location, (DWORD)(controller->mLocation.latitude*1000000), index);
	index = NetbufPushDWORD(location, (DWORD)(controller->mLocation.longitude*1000000), index);
	index = NetbufPushWORD(location, (WORD)(controller->mLocation.altitude), index);
	index = NetbufPushWORD(location, (WORD)(controller->mLocation.speed*10), index);
	index = NetbufPushWORD(location, (WORD)controller->mLocation.bearing, index);
	getLocalBCDTime((char*)location+index);
	
	unsigned int mediaId = JT808MediaEvent_InsertRecord(handle, eventType, JT808MediaType_Video, chnNum, currentTimeMillis()/1000, location);
	if (mediaId == -1)
	{
		return;
	}
	
	JT808MEDIAEVENT_FILENAME(mediaId, filename, JT808MediaType_Video, eventType, chnNum);

	if (RET_FAILED == MediaCache_VideoCreate(before, after, chnNum - 1, filename))
	{
		return;
	}
	
	JT808MediaEvent_ReportEvent(handle, mediaId, JT808MediaType_Video, eventType, chnNum);
}

const unsigned short mediaphoto_snap_resolutions[] =
{
	160, 120,
	320, 240,
	640, 480,
	800, 600,
	1024, 704,
	176, 144,
	352, 288,
	704, 288,
	704, 576,
	1280, 720,
};

void* JT808MediaEvent_PhotoSnapTask(void* pArg)
{
	pthread_detach(pthread_self());
	
	JT808MediaEvent_PhotoTask_t* snapPhoto = (JT808MediaEvent_PhotoTask_t*)pArg;
	JT808MediaEvent_t* handle = (JT808MediaEvent_t*)snapPhoto->owner;
	JT808Session_t* session = (JT808Session_t*)handle->owner;
	JT808Controller_t* controller = (JT808Controller_t*)session->mOnwer;

	int count = 0;
	char filename[128] = { 0 };
	BYTE location[28] = { 0 };
    int index = 0;
	unsigned long long lastSnapTime = 0;

	while (count < snapPhoto->mCount)
	{
		JT808MsgBody_0200_t body = { 0 };
		index = 0;

		index = NetbufPushDWORD(location, controller->mCurrentAlertFlag, index);
		index = NetbufPushDWORD(location, controller->mCurrentTermState, index);
		index = NetbufPushDWORD(location, (DWORD)(controller->mLocation.latitude*1000000), index);
		index = NetbufPushDWORD(location, (DWORD)(controller->mLocation.longitude*1000000), index);
		index = NetbufPushWORD(location, (WORD)(controller->mLocation.altitude), index);
		index = NetbufPushWORD(location, (WORD)(controller->mLocation.speed*10), index);
		index = NetbufPushWORD(location, (WORD)controller->mLocation.bearing, index);
		getLocalBCDTime((char*)location+index);

		unsigned int mediaId = JT808MediaEvent_InsertRecord(handle, snapPhoto->mEventType, JT808MediaType_Photo, snapPhoto->mChannel, currentTimeMillis()/1000, location);
		if (mediaId == 0)
		{
			continue;
		}

		JT808MEDIAEVENT_FILENAME(mediaId, filename, JT808MediaType_Photo, snapPhoto->mEventType, snapPhoto->mChannel);

		MediaSnap_CreateSnap(snapPhoto->mChannel-1, 1, mediaphoto_snap_resolutions[snapPhoto->mResolution*2], mediaphoto_snap_resolutions[snapPhoto->mResolution*2+1], 0, filename);

		lastSnapTime = currentTimeMillis();

		JT808MediaEvent_ReportEvent(handle, mediaId, JT808MediaType_Photo, snapPhoto->mEventType, snapPhoto->mChannel);

		if (0 == snapPhoto->mSaveFlag)
		{
		    char sql[1024] = {0};
			
			pthread_mutex_lock(&handle->mMutex);

			sprintf(sql, kSqlUpdateUploadFlag, 1, 0, mediaId);
		    ExecSql(handle->mDB, sql, NULL, NULL);
			
			pthread_mutex_unlock(&handle->mMutex);
			
			DWORD* pId = (DWORD*)malloc(sizeof(DWORD));
			*pId = mediaId;

			usleep(400*1000);
			queue_list_push(&handle->mUploadList, (void*)pId);
		}

		while (currentTimeMillis() - lastSnapTime < (snapPhoto->mIntervel*1000)) usleep(100*1000);

		count ++;
	}

	free(snapPhoto);
	snapPhoto = NULL;

	return NULL;
}

void JT808MediaEvent_MediaPhotoEvent(JT808MediaEvent_t* handle, BYTE eventType, BYTE chnNum, int count, char saveFlag, int intervel, JT808MediaPhotoResolution_e resolution)
{
	pthread_t pid;
	JT808MediaEvent_PhotoTask_t* snapPhoto = (JT808MediaEvent_PhotoTask_t*)malloc(sizeof(JT808MediaEvent_PhotoTask_t));
	if (NULL != snapPhoto)
	{
		snapPhoto->mChannel = chnNum;
		snapPhoto->mCount = count;
		snapPhoto->mEventType = eventType;
		snapPhoto->mResolution = resolution;
		snapPhoto->mSaveFlag = saveFlag;
		snapPhoto->mIntervel = intervel;
		snapPhoto->owner = handle;
	
		pthread_create(&pid, NULL, JT808MediaEvent_PhotoSnapTask, snapPhoto);
	}
}

void JT808MediaEvent_ReportEvent(JT808MediaEvent_t* handle, unsigned int mediaId, BYTE mediaType, BYTE eventType, BYTE chnNum)
{
	JT808MsgBody_0800_t msg = { 0 };

	pthread_mutex_lock(&handle->mMutex);
	
	msg.mChannelID = chnNum;
	msg.mEventType = eventType;
	msg.mMultiMediaDataEncodeType = JT808MEDIAEVENT_FILEENCTYPE(mediaType);
	msg.mMultiMediaDataType = mediaType;
	msg.mMultiMediaDataID = mediaId;

	JT808Session_t* session = (JT808Session_t*)handle->owner;

	JT808Session_SendMsg(session, 0x0800, &msg, sizeof(msg), 0);
	
	pthread_mutex_unlock(&handle->mMutex);
}

void JT808MediaEvent_DeleteOldestFiles(JT808MediaEvent_t* handle, char* dir, int count, int duty)
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
						memcpy(removeFileList, removeFileList[1], i*32);
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
				
				int mediaId = ((removeFileList[i][0]-'0')<<24) + ((removeFileList[i][1]-'0')<<16) + ((removeFileList[i][2]-'0')<<8) + (removeFileList[i][3]-'0');

				JT808MediaEvent_DeleteRecordByMediaID(handle, mediaId);
			}
		}
	}
}

void JT808MediaEvent_GetRecordInfos(JT808MediaEvent_t* handle, JT808MsgBody_0801_t* info, int* uploadFlag, int* delFlag, int id)
{
    char sql[1024] = {0};
	
	pthread_mutex_lock(&handle->mMutex);
	
	sprintf(sql, "SELECT * FROM jt808Media WHERE id=%d;", id);
	
    sqlite3_stmt* stmt = NULL;
    if(sqlite3_prepare_v2(handle->mDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s %s", sqlite3_errmsg(handle->mDB), __FUNCTION__);
		pthread_mutex_unlock(&handle->mMutex);
        return;
    }

	if (stmt == NULL)
	{
		PNT_LOGE("not find %d media.", id);
		return;
	}

	while(sqlite3_step(stmt) == SQLITE_ROW)
	{
	    int column = sqlite3_column_count(stmt);
		
	    for(int i = 0; i < column; i++)
		{
	        const char* name = sqlite3_column_name(stmt, i);
			
	        if(!strcmp(name, "id"))
			{
	            info->mMultiMediaDataID = sqlite3_column_int(stmt, i);
	        }
			else if(!strcmp(name, "chn"))
			{
	            info->mChannelID = sqlite3_column_int(stmt, i);
	        }
			else if(!strcmp(name, "eventType"))
			{
	            info->mEventType = sqlite3_column_int(stmt, i);
	        }
			else if(!strcmp(name, "mediaType"))
			{
	            info->mMultiMediaDataType = sqlite3_column_int(stmt, i);
	        }
			else if(!strcmp(name, "upload"))
			{
	            *uploadFlag = sqlite3_column_int(stmt, i);
	        }
			else if(!strcmp(name, "flag"))
			{
	            *delFlag = sqlite3_column_int(stmt, i);
	        }
			else if(!strcmp(name, "location"))
			{
	            const void* data = sqlite3_column_blob(stmt, i);
				memcpy(info->m0200BaseInfo, data, sizeof(info->m0200BaseInfo));
	        }
	    }
	}
	
    sqlite3_finalize(stmt);
	
	pthread_mutex_unlock(&handle->mMutex);
}

void* JT808MediaEvent_MediasTask(void* arg)
{
	pthread_detach(pthread_self());

	JT808MediaEvent_t* handle = (JT808MediaEvent_t*)arg;
	JT808Session_t* session = (JT808Session_t*)handle->owner;
	JT808Controller_t* controller = (JT808Controller_t*)session->mOnwer;

	pthread_mutex_lock(&handle->mMutex);
    ExecSql(handle->mDB, kSqlRefreshMedias, NULL, NULL);
	
    sqlite3_stmt *stmt = NULL;

    if(sqlite3_prepare_v2(handle->mDB, "SELECT* FROM jt808Media WHERE upload=1 ORDER BY id;", strlen("SELECT* FROM jt808Media WHERE upload=1 ORDER BY id;"), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s %s", sqlite3_errmsg(handle->mDB), __FUNCTION__);
    }
	else
	{
	    while(sqlite3_step(stmt) == SQLITE_ROW)
		{
	        int column = sqlite3_column_count(stmt);
			
	        for(int i = 0; i < column; i++)
			{
	            const char* name = sqlite3_column_name(stmt, i);
	            if(!strcmp(name, "id"))
				{
	                int mediaId = sqlite3_column_int(stmt, i);
					
					DWORD* pId = (DWORD*)malloc(sizeof(DWORD));
					*pId = mediaId;
					queue_list_push(&handle->mUploadList, (void*)pId);
	            }
	        }
	    }
		
	    sqlite3_finalize(stmt);
	}
	
	pthread_mutex_unlock(&handle->mMutex);

    time_t nowtime = time(NULL);
    struct tm p;
    localtime_r(&nowtime,&p);
    time_t oldtime = nowtime;

	char filename[128] = { 0 };
	BYTE* buff = NULL;
	JT808MsgBody_0801_t info = { 0 };
	int uploadFlag = 0;
	int delFlag = 0;
	int index = 0;

    while (controller->mStartFlag)
    {
    	if (JT808SessionState_Running == session->mMainProcessState)
    	{
	    	DWORD* pId = (DWORD*)queue_list_popup(&handle->mUploadList);
			if (pId != NULL)
			{
				DWORD id = *pId;
				free(pId);
				
				PNT_LOGI("start to upload %d", id);
				memset(&info, 0, sizeof(info));
				uploadFlag = 0;
				delFlag = 0;
				index = 0;
				
				JT808MediaEvent_GetRecordInfos(handle, &info, &uploadFlag, &delFlag, id);

				PNT_LOGI("get %d uploadFlag %d", id, uploadFlag);

				if (1 == uploadFlag)
				{
					JT808MEDIAEVENT_FILENAME(id, filename, info.mMultiMediaDataType, info.mEventType, info.mChannelID);
					if (0 == access(filename, R_OK))
					{
						struct stat buf = { 0 };
						memset(&buf, 0, sizeof(struct stat));
						stat(filename, &buf);

						if (0 < buf.st_size)
						{
							buff = (BYTE*)malloc(buf.st_size + 36);
							if (NULL != buff)
							{
								FILE* pf = fopen(filename, "rb");
								if (NULL != pf)
								{
									fread(buff+36, 1, buf.st_size, pf);
									fclose(pf);

									index = NetbufPushDWORD(buff, info.mMultiMediaDataID, index);
									index = NetbufPushBYTE(buff, info.mMultiMediaDataType, index);
									index = NetbufPushBYTE(buff, JT808MEDIAEVENT_FILEENCTYPE(info.mMultiMediaDataType), index);
									index = NetbufPushBYTE(buff, info.mEventType, index);
									index = NetbufPushBYTE(buff, info.mChannelID, index);
									index = NetbufPushBuffer(buff, info.m0200BaseInfo, index, sizeof(info.m0200BaseInfo));

									handle->mUploadFlag = 0;
									unsigned int starTime = currentTimeMillis()/1000;
									
									reSend:
									JT808Session_SendMsg(session, 0x0801, buff, 36 + buf.st_size, 1);

									while (0 == handle->mUploadFlag)
									{
										if ((currentTimeMillis()/1000) - starTime > 120)
										{
											goto reSend;
										}
										usleep(100*1000);
									}
								}

								free(buff);
								buff = NULL;
							}
							else
							{
								PNT_LOGE("file %s upload malloc failed.", filename);
							}
						}
					}
					else
					{
						PNT_LOGE("file %s not exist.", filename);
					}
					
					pthread_mutex_lock(&handle->mMutex);
					sprintf(filename, kSqlUpdateUpload, 2, id);
				    ExecSql(handle->mDB, filename, NULL, NULL);
					pthread_mutex_unlock(&handle->mMutex);
					
					if (delFlag)
					{
						JT808MediaEvent_DeleteRecordByMediaID(handle, id);
					}
				}
			}
    	}
    
        nowtime = time(NULL);
        localtime_r(&nowtime,&p);

        if((nowtime-oldtime) >= 60)
        {
            oldtime = nowtime;

			JT808MediaEvent_DeleteOldestFiles(handle, EVENT_PATH, 10, SDCARD_EVENT_DUTY);
        }
		
        sleep(1);
    }

	return NULL;
}

void JT808MediaEvent_Init(JT808MediaEvent_t* handle, void* owner)
{
	memset(handle, 0, sizeof(JT808MediaEvent_t));

	handle->owner = owner;

	JT808Session_t* session = (JT808Session_t*)handle->owner;
	
	pthread_mutex_init(&handle->mMutex, NULL);

	char dbFilename[128] = { 0 };

	sprintf(dbFilename, JT808_DATABASE_MEDIAEVENT, session->mSessionData->mAddress);

	int ret = sqlite3_open_v2(dbFilename, &handle->mDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

	int oldVersion = JT808MediaEvent_GetVersion(handle);

	if (oldVersion != -1 && oldVersion < JT808_MEDIAEVENT_DB_VERSION)
	{
        ExecSql(handle->mDB, kSqlDropMediaTable, NULL, NULL);
		
        JT808MediaEvent_SetVersion(handle, JT808_MEDIAEVENT_DB_VERSION);
	}

	if (SQLITE_OK != ret)
	{
		PNT_LOGE("open/create DB %s failed.", dbFilename);
		return;
	}

	PNT_LOGI("start to create session table");
	ExecSql(handle->mDB, kSqlCreateMediaTable, NULL, NULL);

	queue_list_create(&handle->mUploadList, 1000);

	pthread_t pid;
	pthread_create(&pid, NULL, JT808MediaEvent_MediasTask, handle);
}

void JT808MediaEvent_UploadMediaFileStatus(JT808MediaEvent_t* handle, JT808MsgBody_8800_t* req)
{
	if (0 == req->mReuploadPackCount)
	{
		handle->mUploadFlag = 1;
	}
}

void JT808MediaEvent_QueryMediaFiles(JT808MediaEvent_t* handle, JT808MsgBody_8802_t* req, JT808MsgBody_0802_t* ack)
{
    char sql[1024] = {0};
	
	pthread_mutex_lock(&handle->mMutex);

	unsigned int timestart = convertBCDToSysTime(req->mStartTime);
	unsigned int timestop = convertBCDToSysTime(req->mEndTime);
	
	if (0 != req->mChannelID)
	{
		sprintf(sql, "SELECT COUNT(*) FROM jt808Media WHERE time <= %d AND time >= %d AND eventType = %d AND mediaType=%d AND chn=%d ORDER BY id;", timestop, timestart, 
			req->mEventType, req->mMultiMediaDataType, req->mChannelID);
	}
	else
	{
		sprintf(sql, "SELECT COUNT(*) FROM jt808Media WHERE time <= %d AND time >= %d AND eventType = %d AND mediaType=%d ORDER BY id;", timestop, timestart, 
			req->mEventType, req->mMultiMediaDataType);
	}

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(handle->mDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 :%s error!", sql);
		pthread_mutex_unlock(&handle->mMutex);
        return;
    }
    if (sqlite3_step(stmt) != SQLITE_ROW)
	{
        PNT_LOGE("sqlite3_step!");
		pthread_mutex_unlock(&handle->mMutex);
        return;
    }
	
    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    stmt = NULL;

	if (0 != req->mChannelID)
	{
		sprintf(sql, "SELECT* FROM jt808Media WHERE time <= %d AND time >= %d AND eventType = %d AND mediaType=%d AND chn=%d ORDER BY id;", timestop, timestart, 
			req->mEventType, req->mMultiMediaDataType, req->mChannelID);
	}
	else
	{
		sprintf(sql, "SELECT* FROM jt808Media WHERE time <= %d AND time >= %d AND eventType = %d AND mediaType=%d ORDER BY id;", timestop, timestart, 
			req->mEventType, req->mMultiMediaDataType);
	}
	
    if(sqlite3_prepare_v2(handle->mDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s %s", sqlite3_errmsg(handle->mDB), __FUNCTION__);
		pthread_mutex_unlock(&handle->mMutex);
        return;
    }

	ack->mItemsCount = count;
	ack->pItemDatas = (BYTE*)malloc(ack->mItemsCount*35);
	if (NULL == ack->pItemDatas)
	{
		ack->mItemsCount = 0;
	}
	else
	{
		ack->mDataLen = 0;
		memset(ack->pItemDatas, 0, ack->mItemsCount*35);
	
	    while(sqlite3_step(stmt) == SQLITE_ROW)
		{
	        int column = sqlite3_column_count(stmt);
			
			JT808MsgBody_0802_Item_t item = { 0 };

			item.mEventType = req->mEventType;
			item.mMultiMediaDataType = req->mMultiMediaDataType;
			
	        for(int i = 0; i < column; i++)
			{
	            const char* name = sqlite3_column_name(stmt, i);
	            if(!strcmp(name, "id"))
				{
	                item.mMultiMediaDataID = sqlite3_column_int(stmt, i);
	            }
				else if(!strcmp(name, "chn"))
				{
	                item.mChannelID = sqlite3_column_int(stmt, i);
	            }
				else if(!strcmp(name, "location"))
				{
	                const void* data = sqlite3_column_blob(stmt, i);
					memcpy(item.mLocationData, data, sizeof(item.mLocationData));
	            }
	        }

			ack->mDataLen = NetbufPushDWORD(ack->pItemDatas, item.mMultiMediaDataID, ack->mDataLen);
			ack->mDataLen = NetbufPushBYTE(ack->pItemDatas, item.mMultiMediaDataType, ack->mDataLen);
			ack->mDataLen = NetbufPushBYTE(ack->pItemDatas, item.mChannelID, ack->mDataLen);
			ack->mDataLen = NetbufPushBYTE(ack->pItemDatas, item.mEventType, ack->mDataLen);
			ack->mDataLen = NetbufPushBuffer(ack->pItemDatas, item.mLocationData, ack->mDataLen, sizeof(item.mLocationData));
	    }
	}
	
    sqlite3_finalize(stmt);
	
	pthread_mutex_unlock(&handle->mMutex);
}

void JT808MediaEvent_UploadMediaFilesReq(JT808MediaEvent_t* handle, JT808MsgBody_8803_t* req)
{
    char sql[1024] = {0};
	
	pthread_mutex_lock(&handle->mMutex);

	unsigned int timestart = convertBCDToSysTime(req->mStartTime);
	unsigned int timestop = convertBCDToSysTime(req->mEndTime);
	
    sqlite3_stmt *stmt = NULL;

	if (0 != req->mChannelID)
	{
		sprintf(sql, "SELECT* FROM jt808Media WHERE time <= %d AND time >= %d AND eventType = %d AND mediaType=%d AND chn=%d ORDER BY id;", timestop, timestart, 
			req->mEventType, req->mMultiMediaDataType, req->mChannelID);
	}
	else
	{
		sprintf(sql, "SELECT* FROM jt808Media WHERE time <= %d AND time >= %d AND eventType = %d AND mediaType=%d ORDER BY id;", timestop, timestart, 
			req->mEventType, req->mMultiMediaDataType);
	}
	
    if(sqlite3_prepare_v2(handle->mDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s %s", sqlite3_errmsg(handle->mDB), __FUNCTION__);
		pthread_mutex_unlock(&handle->mMutex);
        return;
    }

	DWORD mediaId = 0;
	int uploadFlag = 0;

    while(sqlite3_step(stmt) == SQLITE_ROW)
	{
        int column = sqlite3_column_count(stmt);
		
        for(int i = 0; i < column; i++)
		{
            const char* name = sqlite3_column_name(stmt, i);
            if(!strcmp(name, "id"))
			{
                mediaId = sqlite3_column_int(stmt, i);
            }
			else if (!strcmp(name, "upload"))
			{
				uploadFlag = sqlite3_column_int(stmt, i);
			}
        }

		if (0 == uploadFlag)
		{
			DWORD* pId = (DWORD*)malloc(sizeof(DWORD));
			*pId = mediaId;
			queue_list_push(&handle->mUploadList, (void*)pId);
			
			sprintf(sql, kSqlUpdateUploadFlag, 1, req->mDeleteFlag, mediaId);
		    ExecSql(handle->mDB, sql, NULL, NULL);
		}
    }
	
    sqlite3_finalize(stmt);
	
	pthread_mutex_unlock(&handle->mMutex);
}

void JT808MediaEvent_UploadMediaFileReq(JT808MediaEvent_t* handle, JT808MsgBody_8805_t* req)
{
    char sql[1024] = {0};
	
	pthread_mutex_lock(&handle->mMutex);

	sprintf(sql, kSqlUpdateUploadFlag, 1, req->mDeleteFlag, req->mMultiMediaDataID);
    ExecSql(handle->mDB, sql, NULL, NULL);
	
	pthread_mutex_unlock(&handle->mMutex);

	DWORD* pId = (DWORD*)malloc(sizeof(DWORD));
	*pId = req->mMultiMediaDataID;
	queue_list_push(&handle->mUploadList, (void*)pId);
}

