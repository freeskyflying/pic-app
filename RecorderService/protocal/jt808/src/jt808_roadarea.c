#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pnt_log.h"
#include "jt808_utils.h"
#include "jt808_roadarea.h"
#include "jt808_controller.h"

void JT808RoadPointFree(void* content)
{
	JT808RoadAreaItem_t* item = (JT808RoadAreaItem_t*)content;
	if (NULL != item)
	{
		if (NULL != item->mPoints)
		{
			free(item->mPoints);
			item->mPoints = NULL;
		}
		free(item);
		item = NULL;
		content = NULL;
	}
}

void JT808RoadArea_LoadAllItems(JT808RoadArea_t* handle)
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
		const unsigned char* data = NULL;
        int column = sqlite3_column_count(stmt);
		
        for(int i = 0; i < column; i++)
		{
            const char* name = sqlite3_column_name(stmt, i);
            if(!strcmp(name, "data"))
			{
                data = sqlite3_column_blob(stmt, i);
            }
        }

		if (NULL != data)
		{
			int dataLen = 0;
			JT808RoadAreaItem_t* item = (JT808RoadAreaItem_t*)malloc(sizeof(JT808RoadAreaItem_t));
			if (NULL != item)
			{
				memset(item, 0, sizeof(JT808RoadAreaItem_t));
				dataLen = NetbufPopDWORD((BYTE*)data, &item->mAreaID, dataLen);
				dataLen = NetbufPopWORD((BYTE*)data, &item->mAttr, dataLen);
				if (item->mAttr & JT808_ROUTE_ATTRIBUTE_USE_TIMEREGION)
				{
					unsigned char tmbcd[6] = { 0 };
					dataLen = NetbufPopBuffer((BYTE*)data, tmbcd, dataLen, 6);
					item->mTimeStart = convertBCDToSysTime(tmbcd);
					dataLen = NetbufPopBuffer((BYTE*)data, tmbcd, dataLen, 6);
					item->mTimeStop = convertBCDToSysTime(tmbcd);
				}
				dataLen = NetbufPopWORD((BYTE*)data, &item->mAreaPointCount, dataLen);
				if (item->mAreaPointCount > 0)
				{
					item->mPoints = (JT808_InflectionPoint_t*)malloc(sizeof(JT808_InflectionPoint_t)*item->mAreaPointCount);
					
					for (int i=0; i<item->mAreaPointCount; i++)
					{
						dataLen = NetbufPopDWORD((BYTE*)data, &item->mPoints[i].mInflectionPointID, dataLen);
						dataLen = NetbufPopDWORD((BYTE*)data, &item->mPoints[i].mRoadID, dataLen);
						dataLen = NetbufPopDWORD((BYTE*)data, &item->mPoints[i].mInflectPointLatitude, dataLen);
						dataLen = NetbufPopDWORD((BYTE*)data, &item->mPoints[i].mInflectPointLongitude, dataLen);
						dataLen = NetbufPopBYTE((BYTE*)data, &item->mPoints[i].mRoadWidth, dataLen);
						dataLen = NetbufPopBYTE((BYTE*)data, &item->mPoints[i].mRoadAttribute, dataLen);
						if (item->mPoints[i].mRoadAttribute & JT808_ROAD_ATTRIBUTE_USE_TIMELIMIT)
						{
							dataLen = NetbufPopWORD((BYTE*)data, &item->mPoints[i].mRoadLongTimeThreashold, dataLen);
							dataLen = NetbufPopWORD((BYTE*)data, &item->mPoints[i].mRoadInsufficientThreashold, dataLen);
						}
						if (item->mPoints[i].mRoadAttribute & JT808_ROAD_ATTRIBUTE_USE_SPEEDLIMIT)
						{
							dataLen = NetbufPopWORD((BYTE*)data, &item->mPoints[i].mSpeedLimit, dataLen);
							dataLen = NetbufPopBYTE((BYTE*)data, &item->mPoints[i].mOverspeedContinueTime, dataLen);
							dataLen = NetbufPopWORD((BYTE*)data, &item->mPoints[i].mSpeedLimitNight, dataLen);
						}
					}
				}
				dataLen = NetbufPopWORD((BYTE*)data, &item->mAreaNameLen, dataLen);

				PNT_LOGI("load RoadArea:%d,%04X,%d,%d, %d", item->mAreaID, item->mAttr, item->mAreaPointCount, item->mAreaNameLen, item->mIsInArea);

				queue_list_push(&handle->mList, item);
			}
		}
    }
	
    sqlite3_finalize(stmt);
	
	pthread_mutex_unlock(&dbHandle->mMutex);
}

void JT808RoadArea_AddItem(JT808RoadArea_t* handle, JT808MsgBody_8606_t* areaInfo)
{
	JT808AreaDb_t* dbHandle = &handle->mDB;
	pthread_mutex_lock(&dbHandle->mMutex);

    char sql[1024] = {0}, hasSame = 0;

    sqlite3_stmt *stmt = NULL;

	if (queue_list_get_count(&handle->mList) >= AREA_COUNT)
	{
		JT808RoadAreaItem_t* item = (JT808RoadAreaItem_t*)queue_list_popup(&handle->mList);
		sprintf(sql, kSqlDeleteOneArea, item->mAreaID);
        ExecSql(dbHandle->mDB, sql, NULL, NULL);
		if (NULL != item->mPoints)
			free(item->mPoints);
		free(item);
	}
	
	pthread_mutex_lock(&handle->mList.m_mutex);
	queue_t* head = handle->mList.m_head;
	JT808RoadAreaItem_t* item = NULL;

	while (head)
	{
		item = (JT808RoadAreaItem_t*)head->m_content;

		if (areaInfo->mRouteID == item->mAreaID)
		{
			item->mAttr = areaInfo->mRouteAttribute;
			item->mAreaNameLen = areaInfo->mAreaNameLen;
			if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_TIMEREGION)
			{
				item->mTimeStart = convertBCDToSysTime(areaInfo->mStartTime);
				item->mTimeStop = convertBCDToSysTime(areaInfo->mStopTime);
			}
			item->mAreaPointCount = areaInfo->mInflectionPointCount;
			if (NULL != item->mPoints)
			{
				free(item->mPoints);
				item->mPoints = NULL;
			}
			item->mPoints = (JT808_InflectionPoint_t*)malloc(sizeof(JT808_InflectionPoint_t)*item->mAreaPointCount);
			if (NULL != item->mPoints)
			{
				memcpy(item->mPoints, areaInfo->pInflectionPoints, sizeof(JT808_InflectionPoint_t)*item->mAreaPointCount);
			}
			else
			{
				item->mAreaPointCount = 0;
			}
			
			PNT_LOGI("update RoadArea:%d,%04X,%d,%d, %d", item->mAreaID, item->mAttr, item->mAreaPointCount, item->mAreaNameLen, item->mIsInArea);
			item->mIsInArea = 0;
			hasSame = 1;
			break;
		}

		head = head->next;
	}
	pthread_mutex_unlock(&handle->mList.m_mutex);

	if (!hasSame)
	{
		item = (JT808RoadAreaItem_t*)malloc(sizeof(JT808RoadAreaItem_t));
		if (NULL != item)
		{
			memset(item, 0, sizeof(JT808RoadAreaItem_t));
			item->mAreaID = areaInfo->mRouteID;
			item->mAttr = areaInfo->mRouteAttribute;
			if (item->mAttr & JT808_AREA_ATTRIBUTE_USE_TIMEREGION)
			{
				item->mTimeStart = convertBCDToSysTime(areaInfo->mStartTime);
				item->mTimeStop = convertBCDToSysTime(areaInfo->mStopTime);
			}
			item->mAreaPointCount = areaInfo->mInflectionPointCount;
			if (item->mAreaPointCount > 0)
			{
				item->mPoints = (JT808_InflectionPoint_t*)malloc(sizeof(JT808_InflectionPoint_t)*item->mAreaPointCount);
				if (NULL != item->mPoints)
				{
					memcpy(item->mPoints, areaInfo->pInflectionPoints, sizeof(JT808_InflectionPoint_t)*item->mAreaPointCount);
				}
				else
				{
					item->mAreaPointCount = 0;
				}
			}
			item->mAreaNameLen = areaInfo->mAreaNameLen;
			PNT_LOGI("add RoadArea:%d,%04X,%d,%d, %d", item->mAreaID, item->mAttr, item->mAreaPointCount, item->mAreaNameLen, item->mIsInArea);
			queue_list_push(&handle->mList, item);
		}
	}
	
    memset(sql, 0, 1024);
    sprintf(sql, kSqlInsertOrUpdateArea, areaInfo->mRouteID);
    if(sqlite3_prepare_v2(dbHandle->mDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s %s", sqlite3_errmsg(dbHandle->mDB), __FUNCTION__);
		pthread_mutex_unlock(&dbHandle->mMutex);
        return;
    }
	
    bool_t flag = TRUE;

	BYTE data[2048] = { 0 };
	int dataLen = 0;
	dataLen = NetbufPushDWORD(data, areaInfo->mRouteID, dataLen);
	dataLen = NetbufPushWORD(data, areaInfo->mRouteAttribute, dataLen);
	if (areaInfo->mRouteAttribute & JT808_ROUTE_ATTRIBUTE_USE_TIMEREGION)
	{
		dataLen = NetbufPushBuffer(data, areaInfo->mStartTime, dataLen, 6);
		dataLen = NetbufPushBuffer(data, areaInfo->mStopTime, dataLen, 6);
		
		item->mTimeStart = convertBCDToSysTime(areaInfo->mStartTime);
		item->mTimeStop = convertBCDToSysTime(areaInfo->mStopTime);
	}
	dataLen = NetbufPushWORD(data, areaInfo->mInflectionPointCount, dataLen);
	for (int i=0; i<areaInfo->mInflectionPointCount; i++)
	{
		dataLen = NetbufPushDWORD((BYTE*)data, areaInfo->pInflectionPoints[i].mInflectionPointID, dataLen);
		dataLen = NetbufPushDWORD((BYTE*)data, areaInfo->pInflectionPoints[i].mRoadID, dataLen);
		dataLen = NetbufPushDWORD((BYTE*)data, areaInfo->pInflectionPoints[i].mInflectPointLatitude, dataLen);
		dataLen = NetbufPushDWORD((BYTE*)data, areaInfo->pInflectionPoints[i].mInflectPointLongitude, dataLen);
		dataLen = NetbufPushBYTE((BYTE*)data, areaInfo->pInflectionPoints[i].mRoadWidth, dataLen);
		dataLen = NetbufPushBYTE((BYTE*)data, areaInfo->pInflectionPoints[i].mRoadAttribute, dataLen);
		if (areaInfo->pInflectionPoints[i].mRoadAttribute & JT808_ROAD_ATTRIBUTE_USE_TIMELIMIT)
		{
			dataLen = NetbufPushWORD((BYTE*)data, areaInfo->pInflectionPoints[i].mRoadLongTimeThreashold, dataLen);
			dataLen = NetbufPushWORD((BYTE*)data, areaInfo->pInflectionPoints[i].mRoadInsufficientThreashold, dataLen);
		}
		if (areaInfo->pInflectionPoints[i].mRoadAttribute & JT808_ROAD_ATTRIBUTE_USE_SPEEDLIMIT)
		{
			dataLen = NetbufPushWORD((BYTE*)data, areaInfo->pInflectionPoints[i].mSpeedLimit, dataLen);
			dataLen = NetbufPushBYTE((BYTE*)data, areaInfo->pInflectionPoints[i].mOverspeedContinueTime, dataLen);
			dataLen = NetbufPushWORD((BYTE*)data, areaInfo->pInflectionPoints[i].mSpeedLimitNight, dataLen);
		}
	}
	dataLen = NetbufPushWORD(data, areaInfo->mAreaNameLen, dataLen);
	if (areaInfo->mAreaNameLen > 0)
	{
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

void JT808RoadArea_DeleteItem(JT808RoadArea_t* handle, unsigned int areaId)
{
	JT808AreaDb_t* dbHandle = &handle->mDB;
	JT808Controller_t* controller = handle->mOwner;
	
    char sql[1024] = {0};

	pthread_mutex_lock(&handle->mList.m_mutex);
	queue_t* head = handle->mList.m_head;

	while (head)
	{
		JT808RoadAreaItem_t* item = (JT808RoadAreaItem_t*)head->m_content;

		if (areaId == item->mAreaID)
		{
			head->prev->next = head->next;
			free(head);
			if (item->mIsInArea)
			{
				controller->mCurrentAlertFlag &= (~JT808_ALERT_FLAG_GETINOUTAREA);
			}
			if (item->mPoints)
			{
				free(item->mPoints);
				item->mPoints = NULL;
			}
			free(item);
			item = NULL;
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

void JT808RoadArea_DeleteAllItem(JT808RoadArea_t* handle)
{
	JT808AreaDb_t* dbHandle = &handle->mDB;
	JT808Controller_t* controller = handle->mOwner;
	
    char sql[1024] = {0};

	while (1)
	{
		JT808RoadAreaItem_t* item = queue_list_popup(&handle->mList);
		if (NULL == item)
		{
			break;
		}
		if (item->mIsInArea)
		{
			controller->mCurrentAlertFlag &= (~JT808_ALERT_FLAG_GETINOUTAREA);
		}
		if (item->mPoints)
		{
			free(item->mPoints);
			item->mPoints = NULL;
		}
		free(item);
	}

	pthread_mutex_lock(&dbHandle->mMutex);
	
	sprintf(sql, kSqlDeleteAllArea);
    ExecSql(dbHandle->mDB, sql, NULL, NULL);
	
	pthread_mutex_unlock(&dbHandle->mMutex);
}

int JT808RoadArea_GetItemInfo(JT808RoadArea_t* handle, JT808MsgBody_8606_t* area, JT808RoadAreaItem_t* item)
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
            if(!strcmp(name, "data"))
			{
                data = sqlite3_column_blob(stmt, i);
            }
        }

		if (NULL != data)
		{
			int index = 0;
			memset(area, 0, sizeof(JT808MsgBody_8604_t));
			index = NetbufPopDWORD((BYTE*)data, &area->mRouteID, index);
			index = NetbufPopWORD((BYTE*)data, &area->mRouteAttribute, index);
			if (item->mAttr & JT808_ROUTE_ATTRIBUTE_USE_TIMEREGION)
			{
				index = NetbufPopBuffer((BYTE*)data, area->mStartTime, index, 6);
				index = NetbufPopBuffer((BYTE*)data, area->mStopTime, index, 6);
			}
			index += 2;
			if (item->mAreaPointCount > 0)
			{
				for (int i=0; i<item->mAreaPointCount; i++)
				{
					index += 18;
					if (item->mPoints[i].mRoadAttribute & JT808_ROAD_ATTRIBUTE_USE_TIMELIMIT)
					{
						index += 4;
					}
					if (item->mPoints[i].mRoadAttribute & JT808_ROAD_ATTRIBUTE_USE_SPEEDLIMIT)
					{
						index += 5;
					}
				}
			}
			index = NetbufPopWORD((BYTE*)data, &area->mAreaNameLen, index);
			if (item->mAreaNameLen)
			{
				index = NetbufPopBuffer((BYTE*)data, (BYTE*)area->mAreaName, index, item->mAreaNameLen);
			}
		}
    }
	
    sqlite3_finalize(stmt);
	
	pthread_mutex_unlock(&dbHandle->mMutex);

	return 0;
}

int JT808RoadArea_CalcDataLength(JT808RoadArea_t* handle, unsigned int areaId, unsigned int* count)
{
	int dataLen = 0, areaCount = 0;
	
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
			if(!strcmp(name, "data"))
			{
				data = sqlite3_column_blob(stmt, i);
				len = sqlite3_column_bytes(stmt, i);
			}
		}
	
		if (NULL != data)
		{
			dataLen += len;
			areaCount ++;
		}
	}
	
	sqlite3_finalize(stmt);
	
	pthread_mutex_unlock(&dbHandle->mMutex);

	if (NULL != count)
	{
		*count = areaCount;
	}

	return dataLen;
}

int JT808RoadArea_QueryEncodeData(JT808RoadArea_t* handle, unsigned int areaId, unsigned char* buff)
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
			if(!strcmp(name, "data"))
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

void* JT808RoadAreaMainTask(void* pArg)
{
	pthread_detach(pthread_self());

	JT808RoadArea_t* handle = (JT808RoadArea_t*)pArg;
	JT808Controller_t* controller = handle->mOwner;

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
			JT808RoadAreaItem_t* item = (JT808RoadAreaItem_t*)head->m_content;
			
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

			for (int i=0; i<item->mAreaPointCount-1; i++)
			{
				LatLng_t currLoc = {controller->mLocation.latitude*1000000, controller->mLocation.longitude*1000000};

				if (controller->mLocation.lat == 'S')
				{
					currLoc.m_lat = -currLoc.m_lat;
				}
				if (controller->mLocation.lon == 'W')
				{
					currLoc.m_lng = -currLoc.m_lng;
				}

				int distance = get_distance_point_to_line(item->mPoints[i].mInflectPointLongitude, item->mPoints[i].mInflectPointLatitude,
						item->mPoints[i+1].mInflectPointLongitude, item->mPoints[i+1].mInflectPointLatitude, currLoc);
				//PNT_LOGI("road: %d (%d-%d) distance %d", item->mAreaID, i, i+1, distance);
				if (distance <= item->mPoints[i].mRoadWidth/2)
				{
					if (!item->mIsInArea)
					{
						JT808MsgBody_8606_t areaInfo = { 0 };
						JT808RoadArea_GetItemInfo(handle, &areaInfo, item);
						
						if (item->mAttr & JT808_ROUTE_ATTRIBUTE_IN_REPORT2PLATFORM)
						{
							JT808MsgBody_0200Ex_t ext = { 0 };
							ext.mExtraInfoID = JT808_EXTRA_MSG_ID_ENTER_OR_EXIT_SPECIFIED_AREA_OR_ROUTE_WARNING;
							ext.mExtraInfoLen = 6;
							ext.mExtraInfoData[0] = 4;
							NetbufPushDWORD(ext.mExtraInfoData, item->mAreaID, 1);
							ext.mExtraInfoData[5] = 0;
							JT808Controller_Insert0200Ext(&ext);
							controller->mCurrentAlertFlag |= JT808_ALERT_FLAG_GETINOUTROAD;
						}
					}
					if (item->mIsInArea != (i+1))
					{
						if (item->mPoints[i].mRoadAttribute & JT808_ROAD_ATTRIBUTE_USE_SPEEDLIMIT)
						{
							JT808Overspeed_SetSpeedLimit(&controller->mOverspeed, item->mPoints[i].mSpeedLimit, 4, item->mAreaID, item->mPoints[i].mOverspeedContinueTime);
						}
					}
					item->mIsInArea = i+1;
					break;
				}
				else if (item->mIsInArea == (i+1))
				{
					if (item->mAttr & JT808_ROUTE_ATTRIBUTE_OUT_REPORT2PLATFORM)
					{
						JT808MsgBody_0200Ex_t ext = { 0 };
						ext.mExtraInfoID = JT808_EXTRA_MSG_ID_ENTER_OR_EXIT_SPECIFIED_AREA_OR_ROUTE_WARNING;
						ext.mExtraInfoLen = 6;
						ext.mExtraInfoData[0] = 4;
						NetbufPushDWORD(ext.mExtraInfoData, item->mAreaID, 1);
						ext.mExtraInfoData[5] = 1;
						JT808Controller_Insert0200Ext(&ext);
						controller->mCurrentAlertFlag |= JT808_ALERT_FLAG_GETINOUTROAD;
					}
					if (item->mPoints[i].mRoadAttribute & JT808_ROAD_ATTRIBUTE_USE_SPEEDLIMIT)
					{
						JT808Overspeed_SetSpeedLimit(&controller->mOverspeed, 0, 0, 0, 0);
					}
					
					item->mIsInArea = 0;
				}
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

void JT808RoadArea_Init(JT808RoadArea_t* handle, void* owner)
{
	memset(handle, 0, sizeof(JT808RoadArea_t));

	JT808AreaDb_t* dbHandle = &handle->mDB;
	
	pthread_mutex_init(&dbHandle->mMutex, NULL);
	
	queue_list_create(&handle->mList, 1000);
	queue_list_set_free(&handle->mList, JT808RoadPointFree);

	int ret = sqlite3_open_v2(JT808_DATABASE_ROADAREA, &dbHandle->mDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

	if (SQLITE_OK != ret)
	{
		PNT_LOGE("open/create DB %s failed.", JT808_DATABASE_ROADAREA);
		return;
	}

	PNT_LOGI("start to create session table");
	ExecSql(dbHandle->mDB, kSqlCreateAreaTable, NULL, NULL);
	ExecSql(dbHandle->mDB, kSqlCreateAreaIndex, NULL, NULL);

	handle->mOwner = owner;

	JT808RoadArea_LoadAllItems(handle);

	pthread_t pid;
	pthread_create(&pid, NULL, JT808RoadAreaMainTask, handle);
}




