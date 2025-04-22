#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pnt_log.h"
#include "jt808_utils.h"
#include "jt808_circlearea.h"
#include "jt808_controller.h"

void JT808CircleArea_LoadAllItems(JT808CircleArea_t* handle)
{
    sqlite3_stmt* stmt = NULL;
	JT808AreaDb_t* dbHandle = &handle->mDB;
	
	pthread_mutex_lock(&dbHandle->mMutex);
	
    if(sqlite3_prepare_v2(dbHandle->mDB, kSqlQueryAllArea, strlen(kSqlQueryAllArea), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s", sqlite3_errmsg(dbHandle->mDB));
		pthread_mutex_unlock(&dbHandle->mMutex);
        return;
    }

	while(sqlite3_step(stmt) == SQLITE_ROW)
	{
		int len = 0;
		const unsigned char* data = NULL;
        int column = sqlite3_column_count(stmt);
		
        for(int i = 0; i < column; i++)
		{
            const char* name = sqlite3_column_name(stmt, i);
            if(!strcmp(name, "areaId"))
			{
                //areaId = (DWORD)sqlite3_column_int(stmt, i);
            }
			else if(!strcmp(name, "data"))
			{
                data = sqlite3_column_blob(stmt, i);
				len = sqlite3_column_bytes(stmt, i);
            }
        }

		if (NULL != data)
		{
			int dataLen = 0;
			JT808CircleAreaItem_t* item = (JT808CircleAreaItem_t*)malloc(sizeof(JT808CircleAreaItem_t));
			if (NULL != item)
			{
				memset(item, 0, sizeof(JT808CircleAreaItem_t));
				dataLen = NetbufPopDWORD((BYTE*)data, &item->mAreaID, dataLen);
				dataLen = NetbufPopWORD((BYTE*)data, &item->mAttr, dataLen);
				dataLen = NetbufPopDWORD((BYTE*)data, &item->mLatitude, dataLen);
				dataLen = NetbufPopDWORD((BYTE*)data, &item->mLongitude, dataLen);
				dataLen = NetbufPopDWORD((BYTE*)data, &item->mRadius, dataLen);
				if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_TIMEREGION)
				{
					unsigned char tmbcd[6] = { 0 };
					dataLen = NetbufPopBuffer((BYTE*)data, tmbcd, dataLen, 6);
					item->mTimeStart = convertBCDToSysTime(tmbcd);
					dataLen = NetbufPopBuffer((BYTE*)data, tmbcd, dataLen, 6);
					item->mTimeStop = convertBCDToSysTime(tmbcd);
				}
				if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
				{
					dataLen += 3;
				}
				if (len >= dataLen+2)
				{
					if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
					{
						dataLen += 2;
					}
					dataLen = NetbufPopWORD((BYTE*)data, &item->mAreaNameLen, dataLen);
				}

				PNT_LOGI("load circlearea:%d,%04X,%d,%d,%d,%d", item->mAreaID, item->mAttr, item->mLatitude, item->mLongitude, item->mRadius, item->mAreaNameLen);

				queue_list_push(&handle->mList, item);
			}
		}
    }
	
    sqlite3_finalize(stmt);
	
	pthread_mutex_unlock(&dbHandle->mMutex);
}

void JT808CircleArea_AddItem(JT808CircleArea_t* handle, JT808_Circle_Area_t* areaInfo)
{
	JT808AreaDb_t* dbHandle = &handle->mDB;
	pthread_mutex_lock(&dbHandle->mMutex);

    char sql[1024] = {0}, hasSame = 0;

    sqlite3_stmt *stmt = NULL;

	if (queue_list_get_count(&handle->mList) >= AREA_COUNT)
	{
		JT808CircleAreaItem_t* item = (JT808CircleAreaItem_t*)queue_list_popup(&handle->mList);
		sprintf(sql, kSqlDeleteOneArea, item->mAreaID);
        ExecSql(dbHandle->mDB, sql, NULL, NULL);
		free(item);
	}

	pthread_mutex_lock(&handle->mList.m_mutex);
	queue_t* head = handle->mList.m_head;
	JT808CircleAreaItem_t* item = NULL;

	while (head)
	{
		item = (JT808CircleAreaItem_t*)head->m_content;

		if (areaInfo->mAreaID == item->mAreaID)
		{
			item->mAttr = areaInfo->mAreaAttribute;
			item->mLatitude = areaInfo->mAreaRoundotLatitude;
			item->mLongitude = areaInfo->mAreaRoundotLongitude;
			item->mRadius = areaInfo->mAreaRadius;
			item->mAreaNameLen = areaInfo->mAreaNameLen;
			if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_TIMEREGION)
			{
				item->mTimeStart = convertBCDToSysTime(areaInfo->mStartTime);
				item->mTimeStop = convertBCDToSysTime(areaInfo->mStopTime);
			}
			item->mIsInArea = 0;
			PNT_LOGI("update circlearea:%d,%04X,%d,%d,%d,%d", item->mAreaID, item->mAttr, item->mLatitude, item->mLongitude, item->mRadius, item->mAreaNameLen);
			hasSame = 1;
			break;
		}

		head = head->next;
	}
	pthread_mutex_unlock(&handle->mList.m_mutex);

	if (!hasSame)
	{
		item = (JT808CircleAreaItem_t*)malloc(sizeof(JT808CircleAreaItem_t));
		if (NULL != item)
		{
			memset(item, 0, sizeof(JT808CircleAreaItem_t));
			item->mAreaID = areaInfo->mAreaID;
			item->mAttr = areaInfo->mAreaAttribute;
			item->mLatitude = areaInfo->mAreaRoundotLatitude;
			item->mLongitude = areaInfo->mAreaRoundotLongitude;
			item->mRadius = areaInfo->mAreaRadius;
			item->mAreaNameLen = areaInfo->mAreaNameLen;
			queue_list_push(&handle->mList, item);
			if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_TIMEREGION)
			{
				item->mTimeStart = convertBCDToSysTime(areaInfo->mStartTime);
				item->mTimeStop = convertBCDToSysTime(areaInfo->mStopTime);
			}
			PNT_LOGI("add circlearea:%d,%04X,%d,%d,%d,%d", item->mAreaID, item->mAttr, item->mLatitude, item->mLongitude, item->mRadius, item->mAreaNameLen);
		}
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

	BYTE data[2048] = { 0 };
	int dataLen = 0;
	dataLen = NetbufPushDWORD(data, areaInfo->mAreaID, dataLen);
	dataLen = NetbufPushWORD(data, areaInfo->mAreaAttribute, dataLen);
	dataLen = NetbufPushDWORD(data, areaInfo->mAreaRoundotLatitude, dataLen);
	dataLen = NetbufPushDWORD(data, areaInfo->mAreaRoundotLongitude, dataLen);
	dataLen = NetbufPushDWORD(data, areaInfo->mAreaRadius, dataLen);
	if (areaInfo->mAreaAttribute & JT808_AREA_ATTRIBUTE_USE_TIMEREGION)
	{
		dataLen = NetbufPushBuffer(data, areaInfo->mStartTime, dataLen, 6);
		dataLen = NetbufPushBuffer(data, areaInfo->mStopTime, dataLen, 6);

		item->mTimeStart = convertBCDToSysTime(areaInfo->mStartTime);
		item->mTimeStop = convertBCDToSysTime(areaInfo->mStopTime);
	}
	if (areaInfo->mAreaAttribute & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
	{
		dataLen = NetbufPushWORD(data, areaInfo->mSpeedLimit, dataLen);
		dataLen = NetbufPushBYTE(data, areaInfo->mOverspeedContinueTime, dataLen);
	}
	if (areaInfo->mAreaNameLen > 0)
	{
		if (areaInfo->mAreaAttribute & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
		{
			dataLen = NetbufPushWORD(data, areaInfo->mSpeedLimitNight, dataLen);
		}
		dataLen = NetbufPushWORD(data, areaInfo->mAreaNameLen, dataLen);
		dataLen = NetbufPushBuffer(data, (BYTE*)areaInfo->mAreaName, dataLen, areaInfo->mAreaNameLen);
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

void JT808CircleArea_DeleteItem(JT808CircleArea_t* handle, unsigned int areaId)
{
	JT808AreaDb_t* dbHandle = &handle->mDB;
	JT808Controller_t* controller = handle->mOwner;
	
    char sql[1024] = {0};

	pthread_mutex_lock(&handle->mList.m_mutex);
	queue_t* head = handle->mList.m_head;

	while (head)
	{
		JT808CircleAreaItem_t* item = (JT808CircleAreaItem_t*)head->m_content;

		if (areaId == item->mAreaID)
		{
			head->prev->next = head->next;
			free(head);
			if (item->mIsInArea)
			{
				controller->mCurrentAlertFlag &= (~JT808_ALERT_FLAG_GETINOUTAREA);
			}
			free(item);
			break;
		}

		head = head->next;
	}
	pthread_mutex_unlock(&handle->mList.m_mutex);

	pthread_mutex_lock(&dbHandle->mMutex);
	
	sprintf(sql, kSqlDeleteOneArea, areaId);
    ExecSql(dbHandle->mDB, sql, NULL, NULL);
	
	pthread_mutex_unlock(&dbHandle->mMutex);
}

void JT808CircleArea_DeleteAllItem(JT808CircleArea_t* handle)
{
	JT808AreaDb_t* dbHandle = &handle->mDB;
	JT808Controller_t* controller = handle->mOwner;
	
    char sql[1024] = {0};

	while (1)
	{
		JT808CircleAreaItem_t* item = queue_list_popup(&handle->mList);
		if (NULL == item)
		{
			break;
		}
		if (item->mIsInArea)
		{
			controller->mCurrentAlertFlag &= (~JT808_ALERT_FLAG_GETINOUTAREA);
		}
		free(item);
	}

	pthread_mutex_lock(&dbHandle->mMutex);
	
	sprintf(sql, kSqlDeleteAllArea);
    ExecSql(dbHandle->mDB, sql, NULL, NULL);
	
	pthread_mutex_unlock(&dbHandle->mMutex);
}

int JT808CircleArea_GetItemInfo(JT808CircleArea_t* handle, JT808_Circle_Area_t* area, JT808CircleAreaItem_t* item)
{
    sqlite3_stmt* stmt = NULL;
	JT808AreaDb_t* dbHandle = &handle->mDB;
	
    char sql[1024] = {0};
	sprintf(sql, kSqlSelectOneArea, item->mAreaID);
	pthread_mutex_lock(&dbHandle->mMutex);
	
    if(sqlite3_prepare_v2(dbHandle->mDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s", sqlite3_errmsg(dbHandle->mDB));
		pthread_mutex_unlock(&dbHandle->mMutex);
        return -1;
    }

	while(sqlite3_step(stmt) == SQLITE_ROW)
	{
		const unsigned char* data = NULL;
        int column = sqlite3_column_count(stmt);
		
        for(int i = 0; i < column; i++)
		{
            const char* name = sqlite3_column_name(stmt, i);
            if(!strcmp(name, "areaId"))
			{
                //areaId = (DWORD)sqlite3_column_int(stmt, i);
            }
			else if(!strcmp(name, "data"))
			{
                data = sqlite3_column_blob(stmt, i);
            }
        }

		if (NULL != data)
		{
			int index = 0;
			memset(area, 0, sizeof(JT808_Circle_Area_t));
			index = NetbufPopDWORD((BYTE*)data, &area->mAreaID, index);
			index = NetbufPopWORD((BYTE*)data, &area->mAreaAttribute, index);
			index = NetbufPopDWORD((BYTE*)data, &area->mAreaRoundotLatitude, index);
			index = NetbufPopDWORD((BYTE*)data, &area->mAreaRoundotLongitude, index);
			index = NetbufPopDWORD((BYTE*)data, &area->mAreaRadius, index);
			if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_TIMEREGION)
			{
				index = NetbufPopBuffer((BYTE*)data, area->mStartTime, index, 6);
				index = NetbufPopBuffer((BYTE*)data, area->mStopTime, index, 6);
			}
			if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
			{
				index = NetbufPopWORD((BYTE*)data, &area->mSpeedLimit, index);
				index = NetbufPopBYTE((BYTE*)data, &area->mOverspeedContinueTime, index);
			}
			if (item->mAreaNameLen)
			{
				if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
				{
					index = NetbufPopWORD((BYTE*)data, &area->mSpeedLimitNight, index);
				}
				index = NetbufPopWORD((BYTE*)data, &area->mAreaNameLen, index);
				index = NetbufPopBuffer((BYTE*)data, (BYTE*)area->mAreaName, index, item->mAreaNameLen);
			}
		}
    }
	
    sqlite3_finalize(stmt);
	
	pthread_mutex_unlock(&dbHandle->mMutex);

	return 0;
}

int JT808CircleArea_CalcDataLength(JT808CircleArea_t* handle, unsigned int areaId, unsigned int* count)
{
	int dataLen = 0, areaCount = 0;
	
	pthread_mutex_lock(&handle->mList.m_mutex);
	queue_t* head = handle->mList.m_head;
	JT808CircleAreaItem_t* item = NULL;

	while (head)
	{
		item = (JT808CircleAreaItem_t*)head->m_content;

		if (0 == areaId || areaId == item->mAreaID)
		{
			dataLen += 18;
			if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_TIMEREGION)
			{
				dataLen += 12;
			}
			if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
			{
				dataLen += 3;
			}
			if (item->mAreaNameLen)
			{
				dataLen += 2;
				if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
				{
					dataLen += 2;
				}
				dataLen += item->mAreaNameLen;
			}
			if (0 != areaId)
			{
				break;
			}
			areaCount ++;
		}

		head = head->next;
	}
	pthread_mutex_unlock(&handle->mList.m_mutex);

	if (NULL != count)
	{
		*count = areaCount;
	}

	return dataLen;
}

int JT808CircleArea_QueryEncodeData(JT808CircleArea_t* handle, unsigned int areaId, unsigned char* buff)
{
	int offset = 0;
    sqlite3_stmt* stmt = NULL;
	JT808AreaDb_t* dbHandle = &handle->mDB;
 	char sql[1024] = {0};

	if (0 == areaId)
	{
		sprintf(sql, kSqlQueryAllArea);
		
	}
	else
	{
		sprintf(sql, kSqlSelectOneArea, areaId);
	}
	
	pthread_mutex_lock(&dbHandle->mMutex);
	
	if(sqlite3_prepare_v2(dbHandle->mDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
		PNT_LOGE("sqlite3_prepare_v2 error! %s", sqlite3_errmsg(dbHandle->mDB));
		pthread_mutex_unlock(&dbHandle->mMutex);
		return 0;
	}
	
	while(sqlite3_step(stmt) == SQLITE_ROW)
	{
		const unsigned char* data = NULL;
		int len = 0;
		int column = sqlite3_column_count(stmt);
		
		for(int i = 0; i < column; i++)
		{
			const char* name = sqlite3_column_name(stmt, i);
			if(!strcmp(name, "areaId"))
			{
			}
			else if(!strcmp(name, "data"))
			{
				data = sqlite3_column_blob(stmt, i);
				len = sqlite3_column_bytes(stmt, i);
			}
		}
	
		if (NULL != data)
		{
			memcpy(buff+offset, data, len);
			offset += len;
		}
	}
	
	sqlite3_finalize(stmt);
	
	pthread_mutex_unlock(&dbHandle->mMutex);

	return offset;
}

void* JT808CircleAreaMainTask(void* pArg)
{
	pthread_detach(pthread_self());

	JT808CircleArea_t* handle = (JT808CircleArea_t*)pArg;
	JT808Controller_t* controller = &gJT808Controller;

	while (1)
	{
		if (!controller->mLocation.valid)
		{
			usleep(100*1000);
			continue;
		}

		long long startTime = currentTimeMillis();
		int timeBCDOfDay = getTimeBCDOfDay();
		
		pthread_mutex_lock(&handle->mList.m_mutex);
		queue_t* head = handle->mList.m_head;

		while (head)
		{
			JT808CircleAreaItem_t* item = (JT808CircleAreaItem_t*)head->m_content;
			
			if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_TIMEREGION)
			{
				if (!JT808Area_IsInValidTime(item->mTimeStart, item->mTimeStop, startTime/1000, timeBCDOfDay))
				{
					head = head->next;
					usleep(1000);
					item->mIsInArea = 0;
					continue;
				}
			}

			int lon1 = item->mLongitude;
			int lat1 = item->mLatitude;
			int lon2 = controller->mLocation.longitude*1000000;
			int lat2 = controller->mLocation.latitude*1000000;

			if (controller->mLocation.lat == 'S')
			{
				lat2 = -lat2;
			}
			if (controller->mLocation.lon == 'W')
			{
				lon2 = -lon2;
			}
			
			int dist = get_distance_by_lat_long(lon1, lat1, lon2, lat2);
			
			if (dist <= item->mRadius)
			{
				if (!item->mIsInArea)
				{
					JT808_Circle_Area_t areaInfo = { 0 };
					JT808CircleArea_GetItemInfo(handle, &areaInfo, item);
					
					if (item->mAttr & JT808_AREA_ATTRIBUTE_IN_REPORT2PLATFORM)
					{
						JT808MsgBody_0200Ex_t ext = { 0 };
						ext.mExtraInfoID = JT808_EXTRA_MSG_ID_ENTER_OR_EXIT_SPECIFIED_AREA_OR_ROUTE_WARNING;
						ext.mExtraInfoLen = 6;
						ext.mExtraInfoData[0] = 1;
						NetbufPushDWORD(ext.mExtraInfoData, item->mAreaID, 1);
						ext.mExtraInfoData[5] = 0;
						JT808Controller_Insert0200Ext(&ext);
						controller->mCurrentAlertFlag |= JT808_ALERT_FLAG_GETINOUTAREA;
					}
					if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
					{
						JT808Overspeed_SetSpeedLimit(&controller->mOverspeed, areaInfo.mSpeedLimit, 1, item->mAreaID, areaInfo.mOverspeedContinueTime);
					}
					if (item->mAttr & JT808_AREA_ATTRIBUTE_IN_REPORT2DRIVER)
					{
					}
				}
				item->mIsInArea = 1;
			}
			else if (item->mIsInArea)
			{
				if (item->mAttr & JT808_AREA_ATTRIBUTE_OUT_REPORT2PLATFORM)
				{
					JT808MsgBody_0200Ex_t ext = { 0 };
					ext.mExtraInfoID = JT808_EXTRA_MSG_ID_ENTER_OR_EXIT_SPECIFIED_AREA_OR_ROUTE_WARNING;
					ext.mExtraInfoLen = 6;
					ext.mExtraInfoData[0] = 1;
					NetbufPushDWORD(ext.mExtraInfoData, item->mAreaID, 1);
					ext.mExtraInfoData[5] = 1;
					JT808Controller_Insert0200Ext(&ext);
					controller->mCurrentAlertFlag |= JT808_ALERT_FLAG_GETINOUTAREA;
				}
				if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
				{
					JT808Overspeed_SetSpeedLimit(&controller->mOverspeed, 0, 0, 0, 0);
				}
				if (item->mAttr & JT808_AREA_ATTRIBUTE_OUT_REPORT2DRIVER)
				{
				}
				
				item->mIsInArea = 0;
			}

			if (item->mIsInArea)
			{
				break;
			}

			head = head->next;
			usleep(1000);
		}
		pthread_mutex_unlock(&handle->mList.m_mutex);

		long long costTime = currentTimeMillis() - startTime;

		if (costTime < 900)
		{
			usleep((900-costTime)*1000);
		}
	}

	return NULL;
}

void JT808CircleArea_Init(JT808CircleArea_t* handle, void* owner)
{
	memset(handle, 0, sizeof(JT808CircleArea_t));

	JT808AreaDb_t* dbHandle = &handle->mDB;
	
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

	pthread_t pid;
	pthread_create(&pid, NULL, JT808CircleAreaMainTask, handle);
}

