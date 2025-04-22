
#include "queue_list.h"
#include "jt1078_fileupload.h"
#include "media_storage.h"
#include "jt808_controller.h"
#include "pnt_log.h"
#include "ftpUtil.h"

void JT1078FileUpload_Response(unsigned char* sessionStr, unsigned short serialNum, int result)
{
	JT808Controller_t* controller = &gJT808Controller;

	JT808MsgBody_1206_t msgBody = { 0 };

	msgBody.mReqSerialNum = serialNum;
	msgBody.mResult = result;
	
	for (int i=0; i<JT808SESSION_COUNT_INTIME; i++)
	{
		JT808Session_t* session = controller->mSession[i];

		if (NULL != session)
		{
			char sessionStr[128] = { 0 };
			sprintf(sessionStr, "%s:%d", session->mSessionData->mAddress, session->mSessionData->mPort);
			if (!strcmp(sessionStr, sessionStr))
			{
				JT808Session_SendMsg(session, 0x1206, &msgBody, sizeof(msgBody), 0);
			}
		}
	}
}

void JT1078FileUpload_InsertRequest(JT1078FileUploadHandle_t* handle, unsigned short serialNum, unsigned char* url, unsigned int port, unsigned char* username, 
											unsigned char* password, unsigned char* remotePath, unsigned char* localPath, unsigned char* session)
{
	if (NULL == handle->mDB)
	{
		return;
	}

    char sql[1024] = {0};
    sprintf(sql, kSql1078FileUploadCount);
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(handle->mDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 :%s error!", sql);
        return;
    }
    if (sqlite3_step(stmt) != SQLITE_ROW)
	{
        PNT_LOGE("sqlite3_step!");
        return;
    }

    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    stmt = NULL;
    if(count >= JT1078FILE_UPLOAD_COUNT)
	{
        memset(sql, 0, 1024);
        sprintf(sql, kDeleteExpire1078FileUpload, 20);
        ExecSql(handle->mDB, sql, NULL, NULL);
    }

    memset(sql, 0, 1024);
    sprintf(sql, kSqlInsert1078FileUploadItem, serialNum, url, port, username, password, remotePath, localPath, JT1078FileUploadState_Running, session);
	
    ExecSql(handle->mDB, sql, NULL, NULL);
}

int JT1078FileUpload_GetRequestItem(JT1078FileUploadHandle_t* handle, JT1078FileUploadItem_t* item)
{
	int ret = RET_FAILED;
    sqlite3_stmt* stmt = NULL;
	
	pthread_mutex_lock(&handle->mMutex);
	
    if(sqlite3_prepare_v2(handle->mDB, kSql1078FileUploadQuery, strlen(kSql1078FileUploadQuery), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s", sqlite3_errmsg(handle->mDB));
		pthread_mutex_unlock(&handle->mMutex);
        return ret;
    }
	
    while(sqlite3_step(stmt) == SQLITE_ROW)
	{
		const unsigned char* data = NULL;
        int column = sqlite3_column_count(stmt);
		
        for(int i = 0; i < column; i++)
		{
            const char* name = sqlite3_column_name(stmt, i);
			
            if(!strcmp(name, "serialNum"))
			{
                item->serialNum = (DWORD)sqlite3_column_int(stmt, i);
            }
			else if(!strcmp(name, "url"))
			{
                data = sqlite3_column_text(stmt, i);
				if (NULL != data)
					strncpy((char*)item->url, (char*)data, sizeof(item->url));
            }
			else if(!strcmp(name, "port"))
			{
                item->port = (DWORD)sqlite3_column_int(stmt, i);
            }
			else if(!strcmp(name, "userName"))
			{
                data = sqlite3_column_text(stmt, i);
				if (NULL != data)
					strncpy((char*)item->userName, (char*)data, sizeof(item->userName));
            }
			else if(!strcmp(name, "password"))
			{
                data = sqlite3_column_text(stmt, i);
				if (NULL != data)
					strncpy((char*)item->password, (char*)data, sizeof(item->password));
            }
			else if(!strcmp(name, "remotePath"))
			{
                data = sqlite3_column_text(stmt, i);
				if (NULL != data)
					strncpy((char*)item->remotePath, (char*)data, sizeof(item->remotePath));
            }
			else if(!strcmp(name, "localPath"))
			{
                data = sqlite3_column_text(stmt, i);
				if (NULL != data)
					strncpy((char*)item->localPath, (char*)data, sizeof(item->localPath));
            }
			else if(!strcmp(name, "state"))
			{
                item->state = (BYTE)sqlite3_column_int(stmt, i);
            }
			else if(!strcmp(name, "session"))
			{
                data = sqlite3_column_text(stmt, i);
				if (NULL != data)
					strncpy((char*)item->sessionStr, (char*)data, sizeof(item->sessionStr));
            }
        }

		if (item->state == JT1078FileUploadState_Running || item->state == JT1078FileUploadState_Finish)
		{
			ret = RET_SUCCESS;
			break;
		}
    }
	
    sqlite3_finalize(stmt);
	
	pthread_mutex_unlock(&handle->mMutex);

	return ret;
}

int JT1078FileUpload_GetRequestItemBySerialNum(JT1078FileUploadHandle_t* handle, JT1078FileUploadItem_t* item, unsigned int serialNum)
{
	int ret = RET_FAILED;
    sqlite3_stmt* stmt = NULL;
	
	pthread_mutex_lock(&handle->mMutex);
	
    char sql[1024] = {0};
    sprintf(sql, kSql1078FileUploadQueryBySerialNum, serialNum);
	
    if(sqlite3_prepare_v2(handle->mDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s", sqlite3_errmsg(handle->mDB));
		pthread_mutex_unlock(&handle->mMutex);
        return ret;
    }
	
    while(sqlite3_step(stmt) == SQLITE_ROW)
	{
		const unsigned char* data = NULL;
        int column = sqlite3_column_count(stmt);
		
        for(int i = 0; i < column; i++)
		{
            const char* name = sqlite3_column_name(stmt, i);
			
            if(!strcmp(name, "serialNum"))
			{
                item->serialNum = (DWORD)sqlite3_column_int(stmt, i);
            }
			else if(!strcmp(name, "url"))
			{
                data = sqlite3_column_text(stmt, i);
				if (NULL != data)
					strncpy((char*)item->url, (char*)data, sizeof(item->url));
            }
			else if(!strcmp(name, "port"))
			{
                item->port = (DWORD)sqlite3_column_int(stmt, i);
            }
			else if(!strcmp(name, "userName"))
			{
                data = sqlite3_column_text(stmt, i);
				if (NULL != data)
					strncpy((char*)item->userName, (char*)data, sizeof(item->userName));
            }
			else if(!strcmp(name, "password"))
			{
                data = sqlite3_column_text(stmt, i);
				if (NULL != data)
					strncpy((char*)item->password, (char*)data, sizeof(item->password));
            }
			else if(!strcmp(name, "remotePath"))
			{
                data = sqlite3_column_text(stmt, i);
				if (NULL != data)
					strncpy((char*)item->remotePath, (char*)data, sizeof(item->remotePath));
            }
			else if(!strcmp(name, "localPath"))
			{
                data = sqlite3_column_text(stmt, i);
				if (NULL != data)
					strncpy((char*)item->localPath, (char*)data, sizeof(item->localPath));
            }
			else if(!strcmp(name, "state"))
			{
                item->state = (BYTE)sqlite3_column_int(stmt, i);
            }
			else if(!strcmp(name, "session"))
			{
                data = sqlite3_column_text(stmt, i);
				if (NULL != data)
					strncpy((char*)item->sessionStr, (char*)data, sizeof(item->sessionStr));
            }
        }

		if (item->state == JT1078FileUploadState_Running)
		{
			ret = RET_SUCCESS;
			break;
		}
    }
	
    sqlite3_finalize(stmt);
	
	pthread_mutex_unlock(&handle->mMutex);

	return ret;
}

void JT1078FileUpload_DeleteBySerialNum(JT1078FileUploadHandle_t* handle, unsigned int serialNum)
{
	pthread_mutex_lock(&handle->mMutex);
	
	char sql[1024] = {0};
	sprintf(sql, kSqlDelete1078FileUploadBySerialNum, serialNum);
    ExecSql(handle->mDB, sql, NULL, NULL);

	PNT_LOGI("delete by %d", serialNum);

	pthread_mutex_unlock(&handle->mMutex);
}

void JT1078FileUpload_UpdateState(JT1078FileUploadHandle_t* handle, unsigned char* localPath, int state)
{
	pthread_mutex_lock(&handle->mMutex);
	
    char sql[1024] = {0};
    memset(sql, 0, 1024);
    sprintf(sql, kUpdate1078FileUploadState, state, localPath);
    ExecSql(handle->mDB, sql, NULL, NULL);

	pthread_mutex_unlock(&handle->mMutex);
}

void JT1078FileUpload_UpdateStateBySerialNum(JT1078FileUploadHandle_t* handle, unsigned int serialNum, int state)
{
	pthread_mutex_lock(&handle->mMutex);
	
    char sql[1024] = {0};
    memset(sql, 0, 1024);
    sprintf(sql, kUpdate1078FileUploadStateBySerialNum, state, serialNum);
    ExecSql(handle->mDB, sql, NULL, NULL);

	pthread_mutex_unlock(&handle->mMutex);
}

int JT1078FileUpload_FtpUtils(JT1078FileUploadHandle_t* handle, JT1078FileUploadItem_t* item)
{
	if (access((char*)item->localPath, R_OK))
	{
		return RET_SUCCESS;
	}

	memset(&handle->ftpUtil, 0, sizeof(handle->ftpUtil));

	handle->ftpUtil.mOwner = handle;
	
	strncpy(handle->ftpUtil.mUserName, (char*)item->userName, sizeof(handle->ftpUtil.mUserName));
	strncpy(handle->ftpUtil.mPassWord, (char*)item->password, sizeof(handle->ftpUtil.mPassWord));
	strcpy(handle->ftpUtil.mLocalPath, (char*)item->localPath);
	sprintf(handle->ftpUtil.mRemotePath, "ftp://%s:%d%s", item->url, item->port, item->remotePath);

	CURLcode result = handleUploadFiles(&handle->ftpUtil, 0, 0);

	if (result != CURLE_OK)
	{
		PNT_LOGE("fatal exception!!! fail to upload %s, result are %d", handle->ftpUtil.mLocalPath, result);
		return RET_FAILED;
	}
	else
	{
		PNT_LOGE("upload ota packet:%s success!!!", handle->ftpUtil.mLocalPath);
		return RET_SUCCESS;
	}
}

void* JT1078FileUploadTask(void* pArg)
{
	pthread_detach(pthread_self());

	JT1078FileUploadHandle_t* handle = (JT1078FileUploadHandle_t*)pArg;

	JT1078FileUploadItem_t item = { 0 };
	
	int ret = RET_SUCCESS;

	sleep(5);

	while (1)
	{
		memset(&item, 0, sizeof(JT1078FileUploadItem_t));
		
		ret = JT1078FileUpload_GetRequestItem(handle, &item);

		if (RET_SUCCESS == ret)
		{
			if (JT1078FileUploadState_Finish == item.state)
			{
				while (1)
				{
					item.state = JT1078FileUploadState_Cancle;
					if (RET_FAILED == JT1078FileUpload_GetRequestItemBySerialNum(handle, &item, item.serialNum))
					{
						JT1078FileUpload_Response(item.sessionStr, item.serialNum, 0);
						JT1078FileUpload_DeleteBySerialNum(handle, item.serialNum);
						break;
					}
					
					PNT_LOGI("upload serialNum[%d]: %s %s %s", item.serialNum, item.url, item.remotePath, item.localPath);
					
					if (RET_SUCCESS == JT1078FileUpload_FtpUtils(handle, &item))
					{
						JT1078FileUpload_UpdateState(handle, item.localPath, JT1078FileUploadState_Finish);
					}

					usleep(1000*1000);
				}
			}
			else
			{
				PNT_LOGI("upload[%d]: %s %s %s", item.serialNum, item.url, item.remotePath, item.localPath);
				
				if (RET_SUCCESS == JT1078FileUpload_FtpUtils(handle, &item))
				{
					JT1078FileUpload_UpdateState(handle, item.localPath, JT1078FileUploadState_Finish);
				}
			}
		}

		usleep(1000*1000);
	}

	return NULL;
}

void JT1078FileUpload_Init(JT1078FileUploadHandle_t* handle, void* owner)
{
	int ret = sqlite3_open_v2(JT808_DATABASE_FILEUPLOAD, &handle->mDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
	
	if (SQLITE_OK != ret)
	{
		PNT_LOGE("open/create DB %s failed.", JT808_DATABASE_FILEUPLOAD);
		return;
	}

	if (checkDBIntegrity(handle->mDB) == DB_IS_MALFORMED)
	{
        PNT_LOGE("fatal state error!!! the database:%s is corrupted!!! we need to delete:%s and create again", JT808_DATABASE_FILEUPLOAD, JT808_DATABASE_FILEUPLOAD);
        remove(JT808_DATABASE_FILEUPLOAD);
    }

	ExecSql(handle->mDB, kSqlCreate1078FileUploadTable, NULL, NULL);

	handle->mOwner = owner;

	pthread_mutex_init(&handle->mMutex, NULL);

	pthread_mutex_lock(&handle->mMutex);

    char sql[1024] = {0};

	sprintf(sql, kSqlDelete1078FileUploadByState, JT1078FileUploadState_Cancle);
    ExecSql(handle->mDB, sql, NULL, NULL);

	sprintf(sql, kSqlDelete1078FileUploadByState, JT1078FileUploadState_Finish);
    ExecSql(handle->mDB, sql, NULL, NULL);
	
	pthread_mutex_unlock(&handle->mMutex);
	
	pthread_t pid;
	pthread_create(&pid, NULL, JT1078FileUploadTask, handle);
}

void JT1078FileUpload_RequestProcess(unsigned short serialNum, JT808MsgBody_9206_t* req, JT1078FileUploadHandle_t* handle, JT808Session_t* session)
{
	if (NULL == handle || NULL == req || NULL == session)
	{
		return;
	}

	queue_t* head = gVideoListQueue[req->mLogicChnNum-1].m_head;

	time_t timeStart = convertBCDToSysTime(req->mStartBcdTime);
	time_t timeEnd = convertBCDToSysTime(req->mEndBcdTime);
	int timeStartFlag = (req->mStartBcdTime[0] | req->mStartBcdTime[1] | req->mStartBcdTime[2]);
	int timeEndFlag = (req->mEndBcdTime[0] | req->mEndBcdTime[1] | req->mEndBcdTime[2]);

	char filename[128] = { 0 };
	char sessionStr[128] = { 0 };

	sprintf(sessionStr, "%s:%d", session->mSessionData->mAddress, session->mSessionData->mPort);

	pthread_mutex_lock(&handle->mMutex);

	while (head)
	{
		time_t fileTime = (time_t)head->m_content;

		if ((!timeStartFlag || fileTime >= timeStart) && (!timeEndFlag || fileTime <= timeEnd))
		{
			get_videoname_by_time(req->mLogicChnNum, fileTime, filename);

			JT1078FileUpload_InsertRequest(handle, serialNum, req->mAddresss, req->mFTPPort, req->mUserName, req->mPswdName, req->mFilePath, (unsigned char*)filename, (unsigned char*)sessionStr);
		}

		head = head->next;
	}

	pthread_mutex_unlock(&handle->mMutex);
}

void JT1078FileUpload_RequestControl(JT1078FileUploadHandle_t* handle, JT808MsgBody_9207_t* ctrl)
{
	pthread_mutex_lock(&handle->mMutex);

    char sql[1024] = {0};

	sprintf(sql, kSqlDelete1078FileUploadByState, JT1078FileUploadState_Cancle);
    ExecSql(handle->mDB, sql, NULL, NULL);

	sprintf(sql, kSqlDelete1078FileUploadByState, JT1078FileUploadState_Finish);
    ExecSql(handle->mDB, sql, NULL, NULL);
	
    sprintf(sql, kUpdate1078FileUploadStateBySerialNum, ctrl->mControlCmd, ctrl->mReqSerialNum);
    ExecSql(handle->mDB, sql, NULL, NULL);
	
	pthread_mutex_unlock(&handle->mMutex);
}

