#include <stdlib.h>
#include <string.h>
#include "pnt_log.h"
#include "jt808_session.h"
#include "jt808_blind_upload.h"

static void JT808BlindUpload_GetDBName(char* name, uint8_t* session_ip, int session_port)
{
	sprintf(name, "%s%s_%d", kDBPath, session_ip, session_port);
	
    int len = strlen(name);
    for (int i = 0; i < len; ++i)
	{
        if (name[i] == '.')
		{
            name[i] = '_';
        }
    }

	sprintf(name, "%s.db", name);
}

void JT808BlindUpload_InsertLocationData(JT808BlindUpload_t* blindUpload, WORD serialNum, WORD size, BYTE* data)
{
	if (NULL == blindUpload->mLocationDB || NULL == data)
	{
		return;
	}
	if (size > 255)
	{
		PNT_LOGE("blind datasize over 255 (%d), ignore.", size);
		return;
	}
	
	pthread_mutex_lock(&blindUpload->mMutex);

    char sql[1024] = {0};
    sprintf(sql, kGpsDataCount, blindUpload->mTableName);
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(blindUpload->mLocationDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 :%s error!", sql);
		pthread_mutex_unlock(&blindUpload->mMutex);
        return;
    }
    if (sqlite3_step(stmt) != SQLITE_ROW)
	{
        PNT_LOGE("sqlite3_step!");
		pthread_mutex_unlock(&blindUpload->mMutex);
        return;
    }
	
	unsigned char* time = data + 22;
	unsigned int timestamp = convertBCDToSysTime(time);
    if (1690000000 > timestamp) //未校时不保存盲区数据
    {
		pthread_mutex_unlock(&blindUpload->mMutex);
        return;
    }

    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    stmt = NULL;
    if(count >= kTotalGpsData)
	{
        memset(sql, 0, 1024);
        sprintf(sql, kDeleteExpireGpsData, blindUpload->mTableName, blindUpload->mTableName, kGpsDataCountThreshold);
        ExecSql(blindUpload->mLocationDB, sql, NULL, NULL);
    }

    memset(sql, 0, 1024);
    sprintf(sql, kInsertGpsData, blindUpload->mTableName, serialNum, size, timestamp);
    if(sqlite3_prepare_v2(blindUpload->mLocationDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s %s", sqlite3_errmsg(blindUpload->mLocationDB), __FUNCTION__);
		pthread_mutex_unlock(&blindUpload->mMutex);
        return;
    }
	
    bool_t flag = TRUE;
	
    sqlite3_exec(blindUpload->mLocationDB, "begin", 0, 0, NULL);
    sqlite3_bind_blob(stmt,1, data, size, NULL);
    int ret = sqlite3_step(stmt);
	
    if(ret != SQLITE_DONE)
	{
        PNT_LOGE("sqlite3_step error!%d", ret);
        flag = FALSE;
    }
	
    if(!flag)
	{
        sqlite3_exec(blindUpload->mLocationDB, "rollback", 0, 0, NULL);;
    }
	else
	{
        sqlite3_exec(blindUpload->mLocationDB, "commit", 0, 0, NULL);
    }
    sqlite3_finalize(stmt);

	pthread_mutex_unlock(&blindUpload->mMutex);
}

void JT808BlindUpload_DeleteLocationDataBySerialNum(JT808BlindUpload_t* blindUpload, WORD serialNum)
{
	if (NULL == blindUpload->mLocationDB)
	{
		return;
	}
	
	pthread_mutex_lock(&blindUpload->mMutex);
	
	char sql[1024] = {0};
	#if 0
	sprintf(sql, kDeleteGpsDataBySerialNum, blindUpload->mTableName, serialNum);
	#else
	sprintf(sql, kUpdateGpsData, blindUpload->mTableName, serialNum);
	#endif
	int result = ExecSql(blindUpload->mLocationDB, sql, NULL, NULL);
	PNT_LOGD("delete blind location data [%s] by serialNum(%d) result(%d)", blindUpload->mTableName, serialNum, result);

	pthread_mutex_unlock(&blindUpload->mMutex);
}

void JT808BlindUpload_DeleteLocationDataByBCDTime(JT808BlindUpload_t* blindUpload, uint8_t* bcdtime)
{
	if (NULL == blindUpload->mLocationDB)
	{
		return;
	}
	
	pthread_mutex_lock(&blindUpload->mMutex);
	
	unsigned long timestamp = convertBCDToSysTime(bcdtime);
	
	char sql[1024] = {0};
	#if 0
	sprintf(sql, kDeleteGpsDataByTime, blindUpload->mTableName, timestamp);
	#else
	sprintf(sql, kUpdateGpsDataByTime, blindUpload->mTableName, timestamp);
	#endif
	int result = ExecSql(blindUpload->mLocationDB, sql, NULL, NULL);
	PNT_LOGD("delete blind location data [%s] by time(%02X%02X%02X%02X%02X%02X) result(%d)", blindUpload->mTableName, bcdtime[0], 
		bcdtime[1], bcdtime[2], bcdtime[3], bcdtime[4], bcdtime[5], result);

	pthread_mutex_unlock(&blindUpload->mMutex);
}

void JT808BlindUpload_DeleteLocationDataByBatchSerialNum(JT808BlindUpload_t* blindUpload, WORD batchSerialNum)
{
	if (NULL == blindUpload->mLocationDB)
	{
		return;
	}
	
	pthread_mutex_lock(&blindUpload->mMutex);

	char sql[1024] = {0};
	sprintf(sql, kDeleteGpsDataByBatchSerialNum, blindUpload->mTableName, batchSerialNum);
	int result = ExecSql(blindUpload->mLocationDB, sql, NULL, NULL);
	
	PNT_LOGD("delete blind location data [%s] by batchSerialNum(%d) result(%d)", blindUpload->mTableName, batchSerialNum, result);

	pthread_mutex_unlock(&blindUpload->mMutex);
}

void JT808BlindUpload_DeleteLocationDataByState(JT808BlindUpload_t* blindUpload)
{
	if (NULL == blindUpload->mLocationDB)
	{
		return;
	}
	
	pthread_mutex_lock(&blindUpload->mMutex);

	char sql[1024] = {0};
	sprintf(sql, kDeleteGpsDataByState, blindUpload->mTableName);
	int result = ExecSql(blindUpload->mLocationDB, sql, NULL, NULL);
	
	PNT_LOGD("delete blind location data [%s] by state result(%d)", blindUpload->mTableName, result);

	pthread_mutex_unlock(&blindUpload->mMutex);
}

int JT808BlindUpload_GetDatasList(JT808BlindUpload_t * blindUpload, queue_list_t* list, int count, int time, int limitTime)
{
	if (NULL == blindUpload)
	{
		return 0;
	}
	
	pthread_mutex_lock(&blindUpload->mMutex);
	
	int act_count = 0;
	
    char sql[1024] = {0};
    sprintf(sql, kQueryDataGpsByTime, blindUpload->mTableName, time, limitTime, count);
    sqlite3_stmt* stmt = NULL;
    if(sqlite3_prepare_v2(blindUpload->mLocationDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s %s", sqlite3_errmsg(blindUpload->mLocationDB), __FUNCTION__);
		pthread_mutex_unlock(&blindUpload->mMutex);
        return 0;
    }

    while(sqlite3_step(stmt) == SQLITE_ROW)
	{
        int column = sqlite3_column_count(stmt);
        JT808LocationData_t* gpsData = (JT808LocationData_t*)malloc(sizeof(JT808LocationData_t));
		
        if(gpsData != NULL)
		{
            memset(gpsData, 0, sizeof(JT808LocationData_t));
            for(int i = 0; i < column; i++)
			{
                const char* name = sqlite3_column_name(stmt, i);
                if(!strcmp(name, "serialNum"))
				{
                    gpsData->mSerialNum = sqlite3_column_int(stmt, i);
                }
				else if(!strcmp(name, "size"))
				{
                    gpsData->mSize = sqlite3_column_int(stmt, i);
                }
				else if(!strcmp(name, "time"))
				{
                    gpsData->mTime = sqlite3_column_int64(stmt, i);
                } 
				else if(!strcmp(name, "data"))
				{
                    const void* data = sqlite3_column_blob(stmt, i);
					gpsData->pData = (BYTE*)malloc(gpsData->mSize);
					if (NULL != gpsData->pData)
					{
                   		memcpy(gpsData->pData, data, gpsData->mSize);
					}
                }
            }

			queue_list_push(list, gpsData);

			act_count ++;
        }
		else
		{
            PNT_LOGE("Malloc gps data fail!");
        }

    }
    sqlite3_finalize(stmt);

	pthread_mutex_unlock(&blindUpload->mMutex);

	return act_count;
}

int JT808BlindUpload_GetCacheNewestGpsTime(JT808BlindUpload_t* blindUpload)
{
	if (NULL == blindUpload)
	{
		PNT_LOGE("param in error.");
		return 0;
	}
	
	pthread_mutex_lock(&blindUpload->mMutex);
	
    char sql[1024] = {0};
    sprintf(sql, kQueryNewestGpsDataTime, blindUpload->mTableName);
    sqlite3_stmt* stmt = NULL;
    if(sqlite3_prepare_v2(blindUpload->mLocationDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s %s", sqlite3_errmsg(blindUpload->mLocationDB), __FUNCTION__);
		pthread_mutex_unlock(&blindUpload->mMutex);
        return 0;
    }
	
    int time = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW)
	{
        int column = sqlite3_column_count(stmt);
        for(int i = 0; i < column; i++)
		{
            const char* name = sqlite3_column_name(stmt, i);
            if(!strcmp(name, "time"))
			{
                time = sqlite3_column_int64(stmt, i);
            }
        }
    }
	
    sqlite3_finalize(stmt);
	
	pthread_mutex_unlock(&blindUpload->mMutex);
	
    return time;
}

void JT808BlindUpload_SetBatchSerialNum(JT808BlindUpload_t* blindUpload, unsigned int time, WORD serialNum)
{
	pthread_mutex_lock(&blindUpload->mMutex);

    char sql[1024] = {0};
    sprintf(sql, kUpdateGpsDataBatchSerialNum, blindUpload->mTableName, serialNum, time);
	int result = ExecSql(blindUpload->mLocationDB, sql, NULL, NULL);

	PNT_LOGD("set data [%s] batchSerialNum(%d) by time(%d) result(%d)", blindUpload->mTableName, serialNum, time, result);
	
	pthread_mutex_unlock(&blindUpload->mMutex);
}

void JT808BlindUpload_DBInit(JT808BlindUpload_t* blindUpload, void* owner)
{
	if (NULL == blindUpload || NULL == owner)
	{
		PNT_LOGE("param in error.");
		return;
	}

	JT808Session_t* session = (JT808Session_t*)owner;

	char tempstring[1024] = { 0 };

	JT808BlindUpload_GetDBName(tempstring, session->mSessionData->mAddress, session->mSessionData->mPort);

	int ret = sqlite3_open_v2(tempstring, &blindUpload->mLocationDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);

	if (SQLITE_OK != ret)
	{
		PNT_LOGE("open/create DB %s failed.", tempstring);
		return;
	}
	
	if (checkDBIntegrity(blindUpload->mLocationDB) == DB_IS_MALFORMED) {
        PNT_LOGE("fatal state error!!! the database:%s is corrupted!!! we need to delete:%s and create again", tempstring, tempstring);
        remove(tempstring);
    }

	sprintf((char*)blindUpload->mTableName, "session%d", session->mSessionData->mPort);
	
	sprintf(tempstring, kCreateTable, blindUpload->mTableName);
	int dbCreateRet = ExecSql(blindUpload->mLocationDB, tempstring, NULL, NULL);
	PNT_LOGD("create table:%s result:%d", blindUpload->mTableName, dbCreateRet);
	
	sprintf(tempstring, kCreateSerialNumIndex, blindUpload->mTableName, blindUpload->mTableName);
	int addSerialNumIdxRet = ExecSql(blindUpload->mLocationDB, tempstring, NULL, NULL);
	PNT_LOGD("add index serial num to table:%s result:%d", blindUpload->mTableName, addSerialNumIdxRet);
	
	sprintf(tempstring, kCreateGpsTimeIndex, blindUpload->mTableName, blindUpload->mTableName);
	int addTimeIdxRet = ExecSql(blindUpload->mLocationDB, tempstring, NULL, NULL);
	PNT_LOGD("add time index to table:%s result:%d", blindUpload->mTableName, addTimeIdxRet);

	blindUpload->mOwner = owner;

	pthread_mutex_init(&blindUpload->mMutex, NULL);
}

