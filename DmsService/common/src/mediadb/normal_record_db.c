#include "normal_record_db.h"
#include "pnt_log.h"
#include "utils.h"
#include "queue_list.h"
#include "media_utils.h"

void NormalRecordDB_TableName(char* tableName, int chnNum, uint8_t streamType)
{
	if (1 == streamType)
	{
		sprintf(tableName, "NorMedia_%d", chnNum+MAX_VIDEO_NUM);
	}
	else
	{
		sprintf(tableName, "NorMedia_%d", chnNum);
	}
}

void NormalRecordDB_BackupMediaInfos(sqlite3* db, queue_list_t* plist, int chnNum)
{
	sqlite3_stmt *stmt;

	char tableName[16] = { 0 };
	sprintf(tableName, "NorMedia_%d", chnNum);
	
	char sql[1024] = { 0 };
	sprintf(sql, kQueryAllNormalMediaRecordSql, tableName);

	int ret = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		PNT_LOGE("fatal state error!!! sqlite3_prepare_v2 error! %d %s", ret, sqlite3_errmsg(db));
		return;
	}
	
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		NormalRecordInfo_t *info = (NormalRecordInfo_t*) malloc(sizeof(NormalRecordInfo_t));
		
		if (info != NULL)
		{
			memset(info, 0, sizeof(NormalRecordInfo_t));
			
			int column = sqlite3_column_count(stmt);
			
			for (int i = 0; i < column; i++)
			{
				const char *name = sqlite3_column_name(stmt, i);
				
				if (!strcmp(name, "time"))
				{
					info->mTimeStart = sqlite3_column_int(stmt, i);
				}
				else if (!strcmp(name, "channel"))
				{
					info->mChannelNum = sqlite3_column_int(stmt, i);
				}
				else if (!strcmp(name, "type"))
				{
					info->mStreamType = sqlite3_column_int(stmt, i);
				}
				else if (!strcmp(name, "duration"))
				{
					info->mDuration = sqlite3_column_int(stmt, i);
				}
				else if (!strcmp(name, "name"))
				{
					const unsigned char *path = sqlite3_column_text(stmt, i);
					memcpy(info->mFileName, path, strlen((char *) path));
				}
			}

			queue_list_push(plist, info);
		}
	}

	sqlite3_finalize(stmt);
}

void NormalRecordDB_RestoreMediaInfos(sqlite3* db, queue_list_t* plist, int chnNum)
{
	int ret;
	sqlite3_stmt* stmt;

	char tableName[16] = { 0 };
	sprintf(tableName, "NorMedia_%d", chnNum);

	char sql[1024] = { 0 };
	sprintf(sql, kSqlCreateMediaRecordTable, tableName);
	ret = ExecSql(db, sql, NULL, NULL);
	if (SQLITE_OK != ret)
	{
		PNT_LOGE("fatal error !!! restore media table %s create failed, cause by %s.", tableName, sqlite3_errmsg(db));
		return;
	}

	sprintf(sql, "INSERT OR REPLACE INTO %s VALUES (NULL, ?, ?, ?, ?, ?);", tableName);
	
	ret = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		PNT_LOGE("sqlite3_prepare_v2 error! %d %s", ret, sqlite3_errmsg(db));
		return;
	}

	sqlite3_exec(db, "begin", 0, 0, NULL);

	while (1)
	{
		NormalRecordInfo_t *info = queue_list_popup(plist);
		if (NULL == info)
		{
			break;
		}
		
		sqlite3_reset(stmt);
		ret = sqlite3_bind_int(stmt, 1, info->mTimeStart);
		ret = sqlite3_bind_int(stmt, 2, info->mChannelNum);
		ret = sqlite3_bind_int(stmt, 3, info->mStreamType);
		ret = sqlite3_bind_int(stmt, 4, info->mDuration);
		ret = sqlite3_bind_text(stmt, 5, (char*)info->mFileName, -1, SQLITE_TRANSIENT);
		ret = sqlite3_step(stmt);
		if (ret != SQLITE_OK)
		{
			PNT_LOGE("fatal state error!!! fail to insert the normal record: channel:%d, path:%s",
				 info->mChannelNum, info->mFileName);
		}
	}

	int tryCount = 0;
	while ((ret = sqlite3_exec(db, "commit", 0, 0, NULL)) != SQLITE_OK)
	{
		if (tryCount++ > 3)
		{
			PNT_LOGE("fatal error !!! has exceed the max retry count, cannot commit the insert 808 media record transaction!!!");
			break;
		}
		usleep(1000 * 100);
	}

	sqlite3_finalize(stmt);
}

void NormalRecordDB_InsertMediaInfo(NormalRecordDB_t* nRecDB, int32_t timestamp, int chnNum, uint8_t streamType, uint16_t duration, char* filename)
{
	pthread_mutex_lock(&nRecDB->mMutex);

	char tableName[16] = { 0 };
	NormalRecordDB_TableName(tableName, chnNum, streamType);

	char sql[1024] = { 0 };
	sprintf(sql, kInsertMediaRecordData, tableName, timestamp, chnNum, streamType, duration, filename);
	ExecSql(nRecDB->mDB, sql, NULL, NULL);

	pthread_mutex_unlock(&nRecDB->mMutex);
}

void NormalRecordDB_UpdateDuration(NormalRecordDB_t* nRecDB, int chnNum, uint8_t streamType, char* filename, uint16_t duration)
{
	pthread_mutex_lock(&nRecDB->mMutex);
	
	char tableName[16] = { 0 };
	NormalRecordDB_TableName(tableName, chnNum, streamType);

	char sql[1024] = { 0 };
	sprintf(sql, kSqlUpdateMediaRecord, tableName, duration, filename);
	ExecSql(nRecDB->mDB, sql, NULL, NULL);

	pthread_mutex_unlock(&nRecDB->mMutex);
}

int NormalRecordDB_GetMediaInfoByTimeArea(NormalRecordDB_t* nRecDB, int chnNum, uint8_t streamType, int32_t timeStart, int32_t timeEnd, queue_list_t* plist)
{
	pthread_mutex_lock(&nRecDB->mMutex);
	int count = 0;
    sqlite3_stmt* stmt;
	
	char tableName[16] = { 0 };
	NormalRecordDB_TableName(tableName, chnNum, streamType);

	char sql[1024] = { 0 };
	sprintf(sql, kQueryMediaRecordByTime, tableName, timeEnd, timeStart);

	if(sqlite3_prepare_v2(nRecDB->mDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s", sqlite3_errmsg(nRecDB->mDB));
		pthread_mutex_unlock(&nRecDB->mMutex);
        return 0;
    }

    while(sqlite3_step(stmt) == SQLITE_ROW)
	{
        int column = sqlite3_column_count(stmt);
        NormalRecordInfo_t* info = (NormalRecordInfo_t*)malloc(sizeof(NormalRecordInfo_t));
		
        if(info != NULL)
		{
            memset(info, 0, sizeof(NormalRecordInfo_t));
			
            for(int i = 0; i < column; i++)
			{
                const char* name = sqlite3_column_name(stmt, i);

                if(!strcmp(name, "time"))
				{
                    info->mTimeStart = sqlite3_column_int(stmt, i);
                }
				else if(!strcmp(name, "channel"))
				{
                    info->mChannelNum = sqlite3_column_int(stmt, i);
                }
				else if(!strcmp(name, "type"))
				{
                    info->mStreamType = sqlite3_column_int(stmt, i);
                }
				else if(!strcmp(name, "duration"))
				{
                    info->mDuration = sqlite3_column_int(stmt, i);
				}
				else if(!strcmp(name, "name"))
				{
                    const char* data = (const char*)sqlite3_column_text(stmt, i);
					strcpy(info->mFileName, data);
                }
            }

			queue_list_push(plist, info);

			count ++;
        }
		else
		{
            PNT_LOGE("Malloc NormalRecordInfo_t data fail!");
        }

    }
    sqlite3_finalize(stmt);

	pthread_mutex_unlock(&nRecDB->mMutex);

	return count;
}

void NormalRecordDB_GetAndDeleteOldestMediaInfo(NormalRecordDB_t* nRecDB, int chnNum, uint8_t streamType, char* fileName)
{
	pthread_mutex_lock(&nRecDB->mMutex);
	
	char tableName[16] = { 0 };
	NormalRecordDB_TableName(tableName, chnNum, streamType);
	
	char sql[1024] = {0};
    sprintf(sql, kQueryOldestMediaRecord, tableName);
	
    sqlite3_stmt* stmt = NULL;
    if(sqlite3_prepare_v2(nRecDB->mDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s", sqlite3_errmsg(nRecDB->mDB));
		pthread_mutex_unlock(&nRecDB->mMutex);
        return;
    }
	
    int time = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW)
	{
        int column = sqlite3_column_count(stmt);
        for(int i = 0; i < column; i++)
		{
            const char* name = sqlite3_column_name(stmt, i);
            if(!strcmp(name, "name"))
			{
                const char* data = (const char*)sqlite3_column_text(stmt, i);
				strcpy(fileName, data);
            }
        }
    }
	
    sqlite3_finalize(stmt);

	if (strlen(fileName) > 0)
	{
		sprintf(sql, kDeleteMediaRecordByName, tableName, fileName);
		int result = ExecSql(nRecDB->mDB, sql, NULL, NULL);
		PNT_LOGE("delete db media info %s %s result %d.", tableName, fileName, result);
	}
	
	pthread_mutex_unlock(&nRecDB->mMutex);
}

void NormalRecordDB_Init(NormalRecordDB_t* nRecDB, int videoChnMax, int audioChnMax)
{
	pthread_mutex_init(&nRecDB->mMutex, NULL);

	pthread_mutex_lock(&nRecDB->mMutex);

	int ret = sqlite3_open_v2(MEDIARECORDDB, &nRecDB->mDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
	
	if (SQLITE_OK != ret)
	{
		PNT_LOGE("open/create DB %s failed.", MEDIARECORDDB);
		pthread_mutex_unlock(&nRecDB->mMutex);
		return;
	}

	nRecDB->mVideoChnMax = videoChnMax;
	nRecDB->mAudioChnMax = audioChnMax;
	
	if (checkDBIntegrity(nRecDB->mDB) == DB_IS_MALFORMED)
	{
        PNT_LOGE("fatal state error!!! the database:%s is corrupted!!! we need to delete:%s and create again backup", MEDIARECORDDB, MEDIARECORDDB);
		
		sqlite3* DBTemp = NULL;
        sqlite3_open_v2(MEDIARECORDDBTEMP, &DBTemp, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);

		queue_list_t list = { 0 };
		queue_list_create(&list, 999999);

		for (int i=1; i<=videoChnMax; i++)
		{
        	NormalRecordDB_BackupMediaInfos(nRecDB->mDB, &list, i);

			NormalRecordDB_RestoreMediaInfos(DBTemp, &list, i);
		}

		for (int i=1; i<=audioChnMax; i++)
		{
        	NormalRecordDB_BackupMediaInfos(nRecDB->mDB, &list, i+MAX_VIDEO_NUM);

			NormalRecordDB_RestoreMediaInfos(DBTemp, &list, i+MAX_VIDEO_NUM);
		}

		sqlite3_close(nRecDB->mDB);
		sqlite3_close(DBTemp);
		
        ret = remove(MEDIARECORDDB);
		if (copyFile(MEDIARECORDDB, MEDIARECORDDBTEMP))
		{
			PNT_LOGE("fatal error !!! backup %s failed.", MEDIARECORDDB);
			pthread_mutex_unlock(&nRecDB->mMutex);
			return;
		}
		else
		{
       	 	ret = remove(MEDIARECORDDBTEMP);
			ret = sqlite3_open_v2(MEDIARECORDDB, &nRecDB->mDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
	
			if (SQLITE_OK != ret)
			{
				PNT_LOGE("open/create DB %s failed.", MEDIARECORDDB);
				pthread_mutex_unlock(&nRecDB->mMutex);
				return;
			}
		}
    }
	
	char tableName[16] = { 0 };
	char sql[1024] = { 0 };
	
	for (int i=1; i<=videoChnMax; i++)
	{
		sprintf(tableName, "NorMedia_%d", i);
		sprintf(sql, kSqlCreateMediaRecordTable, tableName);
		ret = ExecSql(nRecDB->mDB, sql, NULL, NULL);
		
		sprintf(sql, kCreateMediaRecordTimeIndex, tableName, tableName);
		ret = ExecSql(nRecDB->mDB, sql, NULL, NULL);
	}

	for (int i=1; i<=audioChnMax; i++)
	{
		sprintf(tableName, "NorMedia_%d", i+MAX_VIDEO_NUM);
		sprintf(sql, kSqlCreateMediaRecordTable, tableName);
		ret = ExecSql(nRecDB->mDB, sql, NULL, NULL);
		
		sprintf(sql, kCreateMediaRecordTimeIndex, tableName, tableName);
		ret = ExecSql(nRecDB->mDB, sql, NULL, NULL);
	}
	
	pthread_mutex_unlock(&nRecDB->mMutex);
}

