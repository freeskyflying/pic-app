
#include "jt808_circlearea.h"

void JT808CircleArea_LoadAllItems(JT808CircleArea_t* handle)
{
    sqlite3_stmt* stmt = NULL;
	jt808AreaDb_t* dbHandle = &handle->mDB;
	
	pthread_mutex_lock(&dbHandle->mMutex);
	
    if(sqlite3_prepare_v2(dbHandle->mDB, kSqlQueryAllArea, strlen(kSqlQueryAllArea), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s", sqlite3_errmsg(dbHandle->mDB));
		pthread_mutex_unlock(&dbHandle->mMutex);
        return;
    }

	while(sqlite3_step(stmt) == SQLITE_ROW)
	{
		const unsigned char* data = NULL;
		unsigned int areaId = 0;
        int column = sqlite3_column_count(stmt);
		
        for(int i = 0; i < column; i++)
		{
            const char* name = sqlite3_column_name(stmt, i);
            if(!strcmp(name, "areaId"))
			{
                areaId = (DWORD)sqlite3_column_int(stmt, i);
            }
			else if(!strcmp(name, "data"))
			{
                data = sqlite3_column_blob(stmt, i);
            }
        }

		if (NULL != data)
		{
			JT808_Circle_Area_t areaInfo = { 0 };
			memcpy(&areaInfo, data, sizeof(JT808_Circle_Area_t));

			JT808CircleAreaItem_t* item = (JT808CircleAreaItem_t*)malloc(sizeof(JT808CircleAreaItem_t));
			item->mAreaID = areaInfo.mAreaID;
			item->mLatitude = areaInfo.mAreaRoundotLatitude;
			item->mLongitude = areaInfo.mAreaRoundotLongitude;
			item->mRadius = areaInfo.mAreaRadius;

			PNT_LOGI("circlearea:%d,%d,%d,%d", item->mAreaID, item->mLatitude, item->mLongitude, item->mRadius);

			queue_list_push(&handle->mList, item);
		}
    }
	
    sqlite3_finalize(stmt);
	
	pthread_mutex_unlock(&dbHandle->mMutex);
}

void JT808CircleArea_AddItem(JT808CircleArea_t* handle, JT808_Circle_Area_t* areaInfo)
{
	jt808AreaDb_t* dbHandle = &handle->mDB;
	pthread_mutex_lock(&dbHandle->mMutex);

    char sql[1024] = {0};

    sqlite3_stmt *stmt = NULL;

	if (queue_list_get_count(&handle->mList) >= AREA_COUNT)
	{
		JT808CircleAreaItem_t* item = (JT808CircleAreaItem_t*)queue_list_popup(&handle->mList);
		sprintf(sql, kSqlDeleteOneArea, item->mAreaID);
	}

    memset(sql, 0, 1024);
    sprintf(sql, kSqlInsertOrUpdateArea, areaInfo->mAreaID);
    if(sqlite3_prepare_v2(dbHandle->mDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s %s", sqlite3_errmsg(dbHandle->mDB), __FUNCTION__);
		pthread_mutex_unlock(&dbHandle->mMutex);
        return;
    }
	
    bool_t flag = TRUE;

	char data[2048] = { 0 };
	int dataLen = 0;
	dataLen = NetbufPushDWORD(data, areaInfo->mAreaID, dataLen);
	dataLen = NetbufPushWORD(data, areaInfo->mAreaAttribute, dataLen);
	dataLen = NetbufPushDWORD(data, areaInfo->mAreaRoundotLatitude, dataLen);
	dataLen = NetbufPushDWORD(data, areaInfo->mAreaRoundotLongitude, dataLen);
	dataLen = NetbufPushDWORD(data, areaInfo->mAreaRadius, dataLen);
	if (areaInfo->mAreaAttribute & 0x01)
	{
		dataLen = NetbufPushBuffer(data, areaInfo->mStartTime, dataLen, 6);
		dataLen = NetbufPushBuffer(data, areaInfo->mStopTime, dataLen, 6);
	}
	if (areaInfo->mAreaAttribute & 0x02)
	{
		dataLen = NetbufPushWORD(data, areaInfo->mSpeedLimit, dataLen);
		dataLen = NetbufPushBYTE(data, areaInfo->mOverspeedContinueTime, dataLen);
		dataLen = NetbufPushWORD(data, areaInfo->mSpeedLimitNight, dataLen);
	}
	if (areaInfo->mAreaNameLen > 0)
	{
		dataLen = NetbufPushWORD(data, areaInfo->mAreaNameLen, dataLen);
		dataLen = NetbufPushBuffer(data, areaInfo->mAreaName, dataLen, areaInfo->mAreaNameLen);
	}
	
    sqlite3_exec(dbHandle->mDB, "begin", 0, 0, NULL);
    sqlite3_bind_blob(stmt,1, data, dataLen, NULL);
    int ret = sqlite3_step(stmt);
	
    if(ret != SQLITE_DONE)
	{
        PNT_LOGE("sqlite3_step error!%d", ret);
        flag = FALSE;
    }
	
    if(!flag)
	{
        sqlite3_exec(dbHandle->mDB, "rollback", 0, 0, NULL);;
    }
	else
	{
        sqlite3_exec(dbHandle->mDB, "commit", 0, 0, NULL);
    }
    sqlite3_finalize(stmt);

	pthread_mutex_unlock(&dbHandle->mMutex);
}

void JT808CircleArea_Init(JT808CircleArea_t* handle, void* owner)
{
	memset(handle, 0, sizeof(JT808CircleArea_t));

	jt808AreaDb_t* dbHandle = &handle->mDB;
	
	pthread_mutex_init(&dbHandle->mMutex, NULL);
	
	queue_list_create(&handle->mList, 1000);

	int ret = sqlite3_open_v2(JT808_DATABASE_CIRCLEAREA, &dbHandle->mDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

	if (SQLITE_OK != ret)
	{
		PNT_LOGE("open/create DB %s failed.", JT808_DATABASE_CIRCLEAREA);
		return;
	}

	PNT_LOGI("start to create session table");
	ExecSql(dbHandle->mDB, kSqlCreateAreaTable, NULL, NULL);
	ExecSql(dbHandle->mDB, kSqlCreateAreaIndex, NULL, NULL);

	handle->mOwner = owner;

	JT808CircleArea_LoadAllItems(handle);
}

