#include "jt808_database.h"
#include "queue_list.h"
#include "jt808_session.h"
#include "pnt_log.h"
#include "jt808_utils.h"
#include "system_param.h"

int JT808Database_GetVersion(sqlite3* db)
{
    if (db == NULL)
	{
        PNT_LOGE("Please open database!");
        return -1;
    }
	
    sqlite3_stmt *stmt = NULL;
	
    if (sqlite3_prepare_v2(db, kSqlGetVersion, strlen(kSqlGetVersion), &stmt, NULL) != SQLITE_OK)
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

void JT808Database_SetVersion(sqlite3* db, int version)
{
	char sql[1024] = {0};
	
	sprintf(sql, kSqlSetVersion, version);
	
	ExecSql(db, sql, NULL, NULL);
}


static int JT808Database_LoadSessionDBAll(JT808Database_t* jt808DB, queue_list_t* plist)
{
	sqlite3* mDB = jt808DB->mSessionDB;
	
	PNT_LOGI("start to store the old heritage session info data");
	if (mDB == NULL)
	{
		PNT_LOGE("fatal state error!!! the database do not open yet!!!");
		return RET_FAILED;
	}
	sqlite3_stmt* stmt = NULL;
	if (sqlite3_prepare_v2(mDB, kSqlQuerySession, strlen(kSqlQuerySession), &stmt, NULL) != SQLITE_OK)
	{
		PNT_LOGE("fatal state error!!! sqlite3_prepare_v2 error! %s", sqlite3_errmsg(mDB));
		return RET_FAILED;
	}

	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		JT808SessionData_t *sessionData = (JT808SessionData_t *) malloc(sizeof(JT808SessionData_t));
		if (sessionData != NULL)
		{
			memset(sessionData, 0, sizeof(JT808SessionData_t));

			int column = sqlite3_column_count(stmt);
			for (int i = 0; i < column; i++)
			{
				const char *name = sqlite3_column_name(stmt, i);
				// in the old session table, there are just 3 fields, so we need to to set
				// the 3 new filed to same value
				if (!strcmp(name, "address"))
				{
					const unsigned char *data = sqlite3_column_text(stmt, i);
					if (data != NULL)
					{
						memcpy(sessionData->mAddress, data, strlen((const char *) data));
					}
				}
				else if (!strcmp(name, "port"))
				{
					sessionData->mPort = (WORD) sqlite3_column_int(stmt, i);
				}
				else if (!strcmp(name, "code"))
				{
					const unsigned char *code = sqlite3_column_text(stmt, i);
					if (code != NULL)
					{
						memcpy(sessionData->mAuthenticode, code, strlen((const char *) code));
					}
				}
				else if (!strcmp(name, "phone_num"))
				{
					const unsigned char *phone_num = sqlite3_column_text(stmt, i);
					if (phone_num != NULL)
					{
						memcpy(sessionData->mPhoneNum, phone_num, strlen((const char *) phone_num));
					}
				}
				else if (!strcmp(name, "session_type"))
				{
					sessionData->mSessionType = sqlite3_column_int(stmt, i);
				}
				else if (!strcmp(name, "vehicle_id"))
				{
					int vehicle_id = sqlite3_column_int(stmt, i);
					sessionData->mVehicleId = vehicle_id;
				}
				else if (!strcmp(name, "warningFilesEnable"))
				{
					int warningFilesEnable = sqlite3_column_int(stmt, i);
					sessionData->mWarningFileEnable = warningFilesEnable;
				}
				else if (!strcmp(name, "realTimeVideoEnable"))
				{
					int realTimeVideoEnable = sqlite3_column_int(stmt, i);
					sessionData->mRealTimeVideo = realTimeVideoEnable;
				}
			}
			
			PNT_LOGI("store the heritage session info: %s:%d, authCode:%s", sessionData->mAddress,
				  sessionData->mPort, sessionData->mAuthenticode);

			queue_list_push(plist, sessionData);
		}
	}
	
	sqlite3_finalize(stmt);
	
	PNT_LOGI("the local stored heritage session info count are %d", queue_list_get_count(plist));

	return RET_SUCCESS;
}

static void JT808Database_RestoreSessionDB(JT808Database_t* jt808DB, queue_list_t* plist)
{
	PNT_LOGI("start to insert the heritage session data into the new create table");
	int ret;
	int flag = TRUE;
	sqlite3* mDB = jt808DB->mSessionDB;

	sqlite3_exec(mDB, "begin", 0, 0, NULL);
	
	while (1)
	{
		JT808SessionData_t *sessionData = (JT808SessionData_t*)queue_list_popup(plist);
		if (NULL == sessionData)
		{
			break;
		}
		
		PNT_LOGI("restore session: %s:%d auth code:%s, phone num:%s, session type:%d, vehicle id:%d to new database",
			  sessionData->mAddress, sessionData->mPort, sessionData->mAuthenticode,
			  sessionData->mPhoneNum, sessionData->mSessionType, sessionData->mVehicleId);

		sqlite3_stmt *stmt;
		ret = sqlite3_prepare_v2(mDB, kSqlInsertSession, strlen(kSqlInsertSession), &stmt, NULL);
		if (ret != SQLITE_OK)
		{
			PNT_LOGE("sqlite3_prepare_v2 error! %d %s", ret, sqlite3_errmsg(mDB));
			break;
		}

		ret = sqlite3_bind_text(stmt, 1, (const char *) sessionData->mAddress, -1, SQLITE_TRANSIENT);
		if (ret != SQLITE_OK)
		{
			PNT_LOGE("sqlite3_bind_text error!%d", ret);
			flag = FALSE;
		}

		ret = sqlite3_bind_int(stmt, 2, sessionData->mPort);
		if (ret != SQLITE_OK)
		{
			PNT_LOGE("sqlite3_bind_text error!%d", ret);
			flag = FALSE;
		}

		// we do not need to add the auth field into local database, as after we perform database upgrade action
		// just let this session register again, the new auth code will persist into local database

		ret = sqlite3_bind_text(stmt, 4, (const char *) sessionData->mPhoneNum, -1, SQLITE_TRANSIENT);
		if (ret != SQLITE_OK)
		{
			PNT_LOGE("sqlite3_bind_text bind phone number %s error!%d", sessionData->mPhoneNum, ret);
			flag = FALSE;
		}

		ret = sqlite3_bind_int(stmt, 5, sessionData->mSessionType);
		if (ret != SQLITE_OK)
		{
			PNT_LOGE("sqlite3_bind_int bind session type %d error!%d", sessionData->mSessionType, ret);
			flag = FALSE;
		}

		ret = sqlite3_bind_int(stmt, 6, sessionData->mVehicleId);
		if (ret != SQLITE_OK)
		{
			PNT_LOGE("sqlite3_bind_int bind vehicleId %d error!%d", sessionData->mVehicleId, ret);
			flag = FALSE;
		}

		ret = sqlite3_bind_int(stmt,7,sessionData->mWarningFileEnable);
		if (ret != SQLITE_OK)
		{
			PNT_LOGE("SessionDBHelper --> sqlite3_bind_int bind warningFileEnable is %d error!%s,%d",sessionData->mWarningFileEnable,sqlite3_errstr(ret),ret);
			flag = FALSE;
		}

		ret = sqlite3_bind_int(stmt,8,sessionData->mRealTimeVideo);
		if (ret != SQLITE_OK)
		{
			PNT_LOGE("SessionDBHelper --> sqlite3_bind_int bind realTimeVideo is %d,error !%s,%d",sessionData->mRealTimeVideo,sqlite3_errstr(ret),ret);
			flag = FALSE;
		}

		ret = sqlite3_step(stmt);
		if (ret != SQLITE_DONE)
		{
			PNT_LOGE("sqlite3_step error!%d", ret);
			flag = FALSE;
		}

		if (!flag)
		{
			PNT_LOGE("fatal state error!!! cannot insert the session record of %s:%d auth code:%s sessionType:%d phone num:%s vehicle id:%d to database",
				  sessionData->mAddress, sessionData->mPort, sessionData->mAuthenticode,
				  sessionData->mSessionType, sessionData->mPhoneNum, sessionData->mVehicleId);
			sqlite3_exec(mDB, "rollback", 0, 0, NULL);
		}
		
#if (LOAD_SESSIONDATA_TO_MEM == 1)
		queue_list_push(&jt808DB->mSessionDataList, sessionData);
#else
		free(sessionData);
#endif
		sqlite3_finalize(stmt);
	}

	sqlite3_exec(mDB, "commit", 0, 0, NULL);
}

void JT808Database_LoadAllSessionData(JT808Database_t* jt808DB, queue_list_t* plist)
{
    if(jt808DB->mSessionDB == NULL)
	{
        PNT_LOGE("Please open database!");
        return;
    }
	
    sqlite3_stmt* stmt = NULL;
	
    if(sqlite3_prepare_v2(jt808DB->mSessionDB, kSqlQuerySession, strlen(kSqlQuerySession), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s", sqlite3_errmsg(jt808DB->mSessionDB));
        return;
    }
    while(sqlite3_step(stmt) == SQLITE_ROW)
	{
        JT808SessionData_t* sessionData = (JT808SessionData_t*)malloc(sizeof(JT808SessionData_t));
        if(sessionData != NULL)
		{
            memset(sessionData, 0, sizeof(JT808SessionData_t));
			
            int column = sqlite3_column_count(stmt);
			
            for(int i = 0; i < column; i++)
			{
                const char* name = sqlite3_column_name(stmt, i);
                if(!strcmp(name, "address"))
				{
                    const unsigned char* data = sqlite3_column_text(stmt, i);
                    if(data != NULL)
					{
                        memcpy(sessionData->mAddress, data, strlen((const char*)data));
                    }
                }
				else if(!strcmp(name, "port"))
				{
                    sessionData->mPort = (WORD)sqlite3_column_int(stmt, i);
                }
				else if(!strcmp(name, "code"))
				{
                    const unsigned char* code = sqlite3_column_text(stmt, i);
                    if(code != NULL)
					{
                        memcpy(sessionData->mAuthenticode, code, strlen((const char*)code));
                    }
                }
				else if(!strcmp(name, "phone_num"))
				{
                    const unsigned char *phoneNum = sqlite3_column_text(stmt, i);
                    if (phoneNum != NULL)
					{
                        memcpy(sessionData->mPhoneNum, phoneNum, strlen((const char*)phoneNum));
                    }
                }
				else if(!strcmp(name, "session_type"))
				{
                    sessionData->mSessionType = (BYTE)sqlite3_column_int(stmt, i);
                }
				else if(!strcmp(name, "vehicle_id"))
				{
                    sessionData->mVehicleId = (DWORD)sqlite3_column_int(stmt, i);
                }
				else if (!strcmp(name,"warningFilesEnable"))
				{
                    sessionData->mWarningFileEnable = (DWORD)sqlite3_column_int(stmt, i);
                }
				else if (!strcmp(name,"realTimeVideoEnable"))
				{
                    sessionData->mRealTimeVideo = (DWORD)sqlite3_column_int(stmt, i);
                }
            }

            PNT_LOGI("session [%s:%d] with phone num:%s, session type:%d vehicle id:%d authCode:%s to queue,warning files enable is %d,"
                  "real time enable is %d", sessionData->mAddress, sessionData->mPort, sessionData->mPhoneNum, sessionData->mSessionType,
                  sessionData->mVehicleId, sessionData->mAuthenticode,sessionData->mWarningFileEnable,sessionData->mRealTimeVideo);

#if (LOAD_SESSIONDATA_TO_MEM == 1)
			queue_list_push(plist, sessionData);
#else
			free(sessionData);
#endif
        }
    }
	
    sqlite3_finalize(stmt);
}

int JT808Database_DeleteSessionData(JT808Database_t* jt808DB, JT808SessionData_t* data)
{
	pthread_mutex_lock(&jt808DB->mMutexSessionDB);
	
    char sql[1024] = {0};
    memset(sql, 0, 1024);
    sprintf(sql, kSqlDeleteSession, data->mAddress, data->mPort);
    ExecSql(jt808DB->mSessionDB, sql, NULL, NULL);

	pthread_mutex_unlock(&jt808DB->mMutexSessionDB);

#if (LOAD_SESSIONDATA_TO_MEM == 1)
	pthread_mutex_lock(&jt808DB->mSessionDataList.m_mutex);
	queue_t* head = jt808DB->mSessionDataList.m_head;
	while(NULL != head)
	{
		JT808SessionData_t* sessionData = (JT808SessionData_t*)head->m_content;
		if (!strcmp((char*)sessionData->mAddress, (char*)data->mAddress) && sessionData->mPort == data->mPort)
		{
			PNT_LOGI("delete [%s:%d] sessionData.", data->mAddress, data->mPort);
			if (head == jt808DB->mSessionDataList.m_head)
			{
				jt808DB->mSessionDataList.m_head = NULL;
			}
			else
			{
				head->prev->next = head->next;
			}
			free(head->m_content);
			free(head);
			break;
		}
		head = head->next;
	}
	pthread_mutex_unlock(&jt808DB->mSessionDataList.m_mutex);
#endif

	return RET_SUCCESS;
}

int JT808Database_AddSessionData(JT808Database_t* jt808DB, JT808SessionData_t* data)
{
	int hasSameData = FALSE;

	pthread_mutex_lock(&jt808DB->mMutexSessionDB);
	
#if (LOAD_SESSIONDATA_TO_MEM == 1)

	pthread_mutex_lock(&jt808DB->mSessionDataList.m_mutex);
	queue_t* head = jt808DB->mSessionDataList.m_head;
	JT808SessionData_t* sessionData = NULL;
	while(NULL != head)
	{
		sessionData = (JT808SessionData_t*)head->m_content;
		if (!strcmp((char*)sessionData->mAddress, (char*)data->mAddress) && sessionData->mPort == data->mPort)
		{
			hasSameData = TRUE;
			break;
		}
		head = head->next;
	}
	pthread_mutex_unlock(&jt808DB->mSessionDataList.m_mutex);

#endif

	if (!hasSameData)
	{
	    JT808SessionData_t* sessionData = (JT808SessionData_t*)malloc(sizeof(JT808SessionData_t));
	    if(sessionData != NULL)
		{
#if (LOAD_SESSIONDATA_TO_MEM == 1)
	        memcpy(sessionData, data, sizeof(JT808SessionData_t));

			queue_list_push(&jt808DB->mSessionDataList, sessionData);
#endif
	    }
		
        int ret;
        bool_t flag = TRUE;
        sqlite3_stmt* stmt;
		
        ret = sqlite3_prepare_v2(jt808DB->mSessionDB, kSqlInsertSession, strlen(kSqlInsertSession), &stmt, NULL);
        if(ret != SQLITE_OK)
		{
			pthread_mutex_unlock(&jt808DB->mMutexSessionDB);
            PNT_LOGE("SessionDBHelper --> sqlite3_prepare_v2 error! %d %s", ret, sqlite3_errmsg(jt808DB->mSessionDB));
            return RET_FAILED;
        }
		
		sqlite3_exec(jt808DB->mSessionDB, "begin", 0, 0, NULL);
		
        ret = sqlite3_bind_text(stmt, 1, (const char*)sessionData->mAddress, -1, SQLITE_TRANSIENT);
        if(ret != SQLITE_OK)
		{
            PNT_LOGE("SessionDBHelper --> sqlite3_bind_text error!%d", ret);
            flag = FALSE;
        }

        ret = sqlite3_bind_int(stmt, 2, sessionData->mPort);
        if(ret != SQLITE_OK)
		{
            PNT_LOGE("SessionDBHelper --> sqlite3_bind_text error!%d", ret);
            flag = FALSE;
        }

        ret = sqlite3_bind_text(stmt, 4, (const char*)sessionData->mPhoneNum, -1, SQLITE_TRANSIENT);
        if (ret != SQLITE_OK)
		{
            PNT_LOGE("SessionDBHelper --> sqlite3_bind_text bind phone number %s error!%d", sessionData->mPhoneNum, ret);
            flag = FALSE;
        }

        ret = sqlite3_bind_int(stmt, 5, sessionData->mSessionType);
        if (ret != SQLITE_OK)
		{
            PNT_LOGE("SessionDBHelper --> sqlite3_bind_int bind session type %d error!%d", sessionData->mSessionType, ret);
            flag = FALSE;
        }

        ret = sqlite3_bind_int(stmt, 6, sessionData->mVehicleId);
        if (ret != SQLITE_OK)
		{
            PNT_LOGE("SessionDBHelper --> sqlite3_bind_int bind vehicleId %d error!%d", sessionData->mVehicleId, ret);
            flag = FALSE;
        }

        ret = sqlite3_bind_int(stmt,7,sessionData->mWarningFileEnable);
        if (ret != SQLITE_OK)
		{
            PNT_LOGE("SessionDBHelper --> sqlite3_bind_int bind warningFileEnable is %d error!%s,%d",sessionData->mWarningFileEnable,sqlite3_errstr(ret),ret);
            flag = FALSE;
        }

        ret = sqlite3_bind_int(stmt,8,sessionData->mRealTimeVideo);
        if (ret != SQLITE_OK)
		{
            PNT_LOGE("SessionDBHelper --> sqlite3_bind_int bind realTimeVideo is %d,error !%s,%d",sessionData->mRealTimeVideo,sqlite3_errstr(ret),ret);
            flag = FALSE;
        }

        ret = sqlite3_step(stmt);
        if(ret != SQLITE_DONE)
		{
            PNT_LOGE("SessionDBHelper --> sqlite3_step error!%d", ret);
            flag = FALSE;
        }

        if(!flag)
		{
			sqlite3_exec(jt808DB->mSessionDB, "rollback", 0, 0, NULL);
        }
		else
		{
			sqlite3_exec(jt808DB->mSessionDB, "commit", 0, 0, NULL);
        }

        sqlite3_finalize(stmt);
	}
	else
	{
		pthread_mutex_unlock(&jt808DB->mMutexSessionDB);
		
		if (memcmp(data->mPhoneNum, sessionData->mPhoneNum, sizeof(data->mPhoneNum)))
		{
			JT808Database_UpdateSessionData(jt808DB, data);
		}
		return RET_FAILED;
	}
	
	pthread_mutex_unlock(&jt808DB->mMutexSessionDB);

	return RET_SUCCESS;
}

int JT808Database_UpdateSessionData(JT808Database_t* jt808DB, JT808SessionData_t* data)
{
	pthread_mutex_lock(&jt808DB->mMutexSessionDB);
	
    char sql[1024] = {0};
    memset(sql, 0, 1024);
    sprintf(sql, kSqlUpdateSession, data->mAuthenticode, data->mPhoneNum, data->mSessionType, data->mVehicleId, data->mRealTimeVideo, data->mWarningFileEnable, data->mAddress, data->mPort);
    ExecSql(jt808DB->mSessionDB, sql, NULL, NULL);

	pthread_mutex_unlock(&jt808DB->mMutexSessionDB);
	
#if (LOAD_SESSIONDATA_TO_MEM == 1)

	pthread_mutex_lock(&jt808DB->mSessionDataList.m_mutex);
	queue_t* head = jt808DB->mSessionDataList.m_head;
	while(NULL != head)
	{
		JT808SessionData_t* sessionData = (JT808SessionData_t*)head->m_content;
		if (!strcmp((char*)sessionData->mAddress, (char*)data->mAddress) && sessionData->mPort == data->mPort)
		{
			PNT_LOGI("update [%s:%d] sessionData.", data->mAddress, data->mPort);
			memcpy(sessionData, data, sizeof(JT808SessionData_t));
			break;
		}
		head = head->next;
	}
	pthread_mutex_unlock(&jt808DB->mSessionDataList.m_mutex);

#endif

	return RET_SUCCESS;
}

void* JT808Database_GetJT808Param(JT808Database_t* jt808DB, DWORD paramID, JT808Param_t** paramOut)
{
	void* ret = 0;

	pthread_mutex_lock(&jt808DB->mSessionParams.m_mutex);
	queue_t* head = jt808DB->mSessionParams.m_head;
	while(NULL != head)
	{
		JT808Param_t* param = (JT808Param_t*)head->m_content;
		
		if (param->mParamID == paramID)
		{
			if (0x03 >= param->mDataType)
			{
				ret = &param->mParamData;
			}
			else
			{
				ret = (void*)param->mParamData;
			}
			if (NULL != paramOut)
			{
				*paramOut = param;
			}
			break;
		}
		
		head = head->next;
	}
	pthread_mutex_unlock(&jt808DB->mSessionParams.m_mutex);

	return ret;
}

int JT808Database_SetJT808Param(JT808Database_t* jt808DB, JT808Param_t* param)
{
    char sql[1024] = {0};

	char data[255] = { 0 };

	if (0x03 >= param->mDataType)
	{
		sprintf(data, "%d", param->mParamData);
	}
	else if (0x04 == param->mDataType || 0x06 == param->mDataType)
	{
		char* buff = (char*)(param->mParamData);
		for(int i=0; i<param->mParamDataLen; i++)
		{
			sprintf(data, "%s%02X", data, buff[i]);
		}
	}
	else if (0x05 == param->mDataType)
	{
		strcpy(data, (char*)param->mParamData);
	}
	
    sprintf(sql, kSqlInsertOrUpdateSessionParam, param->mParamID, param->mParamDataLen, param->mDataType, data);
	
	pthread_mutex_lock(&jt808DB->mMutexSessionDB);

    ExecSql(jt808DB->mSessionDB, sql, NULL, NULL);
	
	pthread_mutex_unlock(&jt808DB->mMutexSessionDB);

	pthread_mutex_lock(&jt808DB->mSessionParams.m_mutex);
	queue_t* head = jt808DB->mSessionParams.m_head;

	while(NULL != head)
	{
		JT808Param_t* paramT = (JT808Param_t*)head->m_content;
		if (paramT->mParamID == param->mParamID)
		{
			if (0x03 < param->mDataType)
			{
				free((void*)paramT->mParamData);
				
				paramT->mParamDataLen = param->mParamDataLen;
				paramT->mParamData = (DWORD)malloc(param->mParamDataLen);
				memcpy((char*)paramT->mParamData, (char*)param->mParamData, param->mParamDataLen);
			}
			else
			{
				paramT->mParamData = param->mParamData;
			}
			PNT_LOGI("JT808Param Set %08X %d %d %s.", param->mParamID, param->mParamDataLen, param->mDataType, data);
			break;
		}
		else if (NULL == head->next) // 新增
		{
			queue_t* tail = (queue_t*)malloc(sizeof(queue_t));
			memset(tail, 0, sizeof(queue_t));
			JT808Param_t* paramTT = malloc(sizeof(JT808Param_t));
			paramTT->mDataType = param->mDataType;
			paramTT->mParamDataLen = param->mParamDataLen;
			paramTT->mParamID = param->mParamID;
			if (0x03 < param->mDataType)
			{
				paramTT->mParamData = (DWORD)malloc(paramTT->mParamDataLen);
				memcpy((char*)paramTT->mParamData, (char*)param->mParamData, paramTT->mParamDataLen);
			}
			else
			{
				paramTT->mParamData = param->mParamData;
			}
			tail->m_content = paramTT;
			head->next = tail;
			PNT_LOGI("JT808Param Add and Set %08X %d %d %s.", param->mParamID, param->mParamDataLen, param->mDataType, data);
			break;
		}
		head = head->next;
	}
	pthread_mutex_unlock(&jt808DB->mSessionParams.m_mutex);

	return RET_SUCCESS;
}

int JT808Database_GetAllJT808Param(JT808Database_t* jt808DB, queue_list_t* plist)
{
	int paramCount = 0;

    if(jt808DB->mSessionDB == NULL)
	{
        PNT_LOGE("Please open database!");
        return paramCount;
    }

	if (jt808DB->mSessionParams.m_init)
	{		
		queue_list_create(plist, 250);
		
		pthread_mutex_lock(&jt808DB->mSessionParams.m_mutex);
		queue_t* head = jt808DB->mSessionParams.m_head;

		while(NULL != head)
		{
			JT808Param_t* param = (JT808Param_t*)head->m_content;
			if (NULL != param)
			{
				JT808Param_t* item = malloc(sizeof(JT808Param_t));
				item->mDataType = param->mDataType;
				item->mParamDataLen = param->mParamDataLen;
				item->mParamID = param->mParamID;
				if (0x03 < param->mDataType)
				{
					item->mParamData = (DWORD)malloc(item->mParamDataLen);
					memcpy((char*)item->mParamData, (char*)param->mParamData, item->mParamDataLen);
				}
				else
				{
					item->mParamData = param->mParamData;
				}
				if (0x55 == item->mParamID)
				{
					item->mParamData = gGlobalSysParam->limitSpeed;
				}
				queue_list_push(plist, item);
			}
			
			head = head->next;
			paramCount ++;
		}
		pthread_mutex_unlock(&jt808DB->mSessionParams.m_mutex);
		
		return paramCount;
	}

	queue_list_create(plist, 250);
	
    sqlite3_stmt* stmt = NULL;
	
	pthread_mutex_lock(&jt808DB->mMutexSessionDB);
	
    if(sqlite3_prepare_v2(jt808DB->mSessionDB, "SELECT * FROM SessionParam", strlen("SELECT * FROM SessionParam"), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s", sqlite3_errmsg(jt808DB->mSessionDB));
		pthread_mutex_unlock(&jt808DB->mMutexSessionDB);
        return paramCount;
    }
    while(sqlite3_step(stmt) == SQLITE_ROW)
	{
		JT808Param_t param = { 0 };
		const unsigned char* data = NULL;
        int column = sqlite3_column_count(stmt);
        for(int i = 0; i < column; i++)
		{
            const char* name = sqlite3_column_name(stmt, i);
            if(!strcmp(name, "paramID"))
			{
                param.mParamID = (DWORD)sqlite3_column_int(stmt, i);
            }
			else if(!strcmp(name, "paramLength"))
			{
                param.mParamDataLen = (BYTE)sqlite3_column_int(stmt, i);
            }
			else if(!strcmp(name, "paramType"))
			{
                param.mDataType = (BYTE)sqlite3_column_int(stmt, i);
            }
			else if(!strcmp(name, "paramData"))
			{
                data = sqlite3_column_text(stmt, i);
            }
        }

		if (strlen((char*)data)) // 没有设置初始值表示当前不支持此参数项
		{
			JT808Param_t* paramT = (JT808Param_t*)malloc(sizeof(JT808Param_t));
			memset(paramT, 0, sizeof(JT808Param_t));

			paramT->mDataType = param.mDataType;
			paramT->mParamDataLen = param.mParamDataLen;
			paramT->mParamID = param.mParamID;
			
			if (0x03 >= paramT->mDataType)
			{
				paramT->mParamData = atoi((char*)data);
			}
			else if (0x04 == paramT->mDataType || 0x06 == paramT->mDataType)
			{
				paramT->mParamDataLen = strlen((char*)data)/2;
				paramT->mParamData = (DWORD)malloc(paramT->mParamDataLen);
				ConvertHexStrToBCD((BYTE*)paramT->mParamData, (char*)data);
			}
			else if (0x05 == paramT->mDataType)
			{
				paramT->mParamDataLen = strlen((char*)data)+1;
				paramT->mParamData = (DWORD)malloc(paramT->mParamDataLen);
				strcpy((char*)paramT->mParamData, (char*)data);
			}

			queue_list_push(plist, paramT);

			paramCount ++;
		}
    }
	
    sqlite3_finalize(stmt);
	
	pthread_mutex_unlock(&jt808DB->mMutexSessionDB);

	return paramCount;
}

void JT808DataBase_ClearSessionData(JT808Database_t* jt808DB)
{
#if (LOAD_SESSIONDATA_TO_MEM == 1)
	while(1)
	{
		JT808SessionData_t* sessionData = (JT808SessionData_t*)queue_list_popup(&jt808DB->mSessionDataList);
		if (NULL != sessionData)
		{
			pthread_mutex_lock(&jt808DB->mMutexSessionDB);
			
		    char sql[1024] = {0};
		    memset(sql, 0, 1024);
		    sprintf(sql, kSqlDeleteSession, sessionData->mAddress, sessionData->mPort);
		    ExecSql(jt808DB->mSessionDB, sql, NULL, NULL);

			free(sessionData);

			pthread_mutex_unlock(&jt808DB->mMutexSessionDB);
		}
		else
		{
			break;
		}
	}
#endif
}

void JT808Database_ShowJT808Param(JT808Database_t* jt808DB)
{
    if(jt808DB->mSessionDB == NULL)
	{
        PNT_LOGE("Please open database!");
        return;
    }
	
    sqlite3_stmt* stmt = NULL;

	JT808Param_t param = { 0 };
	
	pthread_mutex_lock(&jt808DB->mMutexSessionDB);
	
    if(sqlite3_prepare_v2(jt808DB->mSessionDB, "SELECT * FROM SessionParam", strlen("SELECT * FROM SessionParam"), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s", sqlite3_errmsg(jt808DB->mSessionDB));
		pthread_mutex_unlock(&jt808DB->mMutexSessionDB);
        return;
    }
    while(sqlite3_step(stmt) == SQLITE_ROW)
	{
		const unsigned char* data = NULL;
        int column = sqlite3_column_count(stmt);
        for(int i = 0; i < column; i++)
		{
            const char* name = sqlite3_column_name(stmt, i);
            if(!strcmp(name, "paramID"))
			{
                param.mParamID = (DWORD)sqlite3_column_int(stmt, i);
            }
			else if(!strcmp(name, "paramLength"))
			{
                param.mParamDataLen = (BYTE)sqlite3_column_int(stmt, i);
            }
			else if(!strcmp(name, "paramType"))
			{
                param.mDataType = (BYTE)sqlite3_column_int(stmt, i);
            }
			else if(!strcmp(name, "paramData"))
			{
                data = sqlite3_column_text(stmt, i);
            }
        }

		PNT_LOGI("JT808Param %08X %d %d %s.", param.mParamID, param.mParamDataLen, param.mDataType, data);
    }
	
    sqlite3_finalize(stmt);
	
	pthread_mutex_unlock(&jt808DB->mMutexSessionDB);
}

int JT808Database_Init(JT808Database_t* jt808DB, void* owner)
{
	pthread_mutex_init(&jt808DB->mMutexSessionDB, NULL);

	int hasUpdate = 0, isInit = 0;
	queue_list_t backList = { 0 };

	memset(jt808DB, 0, sizeof(JT808Database_t));

	pthread_mutex_lock(&jt808DB->mMutexSessionDB);
	
#if (LOAD_SESSIONDATA_TO_MEM == 1)
	queue_list_create(&jt808DB->mSessionDataList, 1000);
#endif
	queue_list_create(&jt808DB->mSessionParams, 1000);

	if (access(JT808_DATABASE_SESSION, F_OK) == -1)
	{
		isInit = 1;
	}
	
	sqlite3_open_v2(JT808_DATABASE_SESSION, &jt808DB->mSessionDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

	int oldVersion = JT808Database_GetVersion(jt808DB->mSessionDB);

	if (oldVersion != -1 && oldVersion < JT808_DATABASE_SESSION_VER)
	{
		queue_list_create(&backList, 1000);
	
        PNT_LOGI("the %s old version:%d is less than current version %d", JT808_DATABASE_SESSION, oldVersion, JT808_DATABASE_SESSION_VER);
		// 版本更新，先将旧版本数据全部读出，然后再删除旧表及数据
		JT808Database_LoadSessionDBAll(jt808DB, &backList);
		
        PNT_LOGI("now start to drop the old table of %s", JT808_DATABASE_SESSION);
        ExecSql(jt808DB->mSessionDB, kSqlDropSessionTable, NULL, NULL);
        ExecSql(jt808DB->mSessionDB, kSqlDropSessionParamTable, NULL, NULL);
        ExecSql(jt808DB->mSessionDB, kSqlDropSessionParamIndex, NULL, NULL);
		
        // update the database version
        JT808Database_SetVersion(jt808DB->mSessionDB, JT808_DATABASE_SESSION_VER);
		
		hasUpdate = 1;
	}
	
	PNT_LOGI("start to create session table");
	ExecSql(jt808DB->mSessionDB, kSqlCreateSessionTable, NULL, NULL);
	ExecSql(jt808DB->mSessionDB, kSqlCreateSessionParamTable, NULL, NULL);
	ExecSql(jt808DB->mSessionDB, kSqlCreateSessionParamIndex, NULL, NULL);

	if (isInit)
	{
		ExecSql(jt808DB->mSessionDB, kSqlInitSessionParams, NULL, NULL);
		ExecSql(jt808DB->mSessionDB, kSqlAddSessionParams, NULL, NULL);
	}

	if (hasUpdate)
	{
		JT808Database_RestoreSessionDB(jt808DB, &backList);
	}
	else
	{
#if (LOAD_SESSIONDATA_TO_MEM == 1)
		JT808Database_LoadAllSessionData(jt808DB, &jt808DB->mSessionDataList);
#endif
	}
	
	jt808DB->mOwner = owner;
	
	pthread_mutex_unlock(&jt808DB->mMutexSessionDB);

	memset(&jt808DB->mSessionParams, 0, sizeof(jt808DB->mSessionParams));
	JT808Database_GetAllJT808Param(jt808DB, &jt808DB->mSessionParams);
	
	return RET_SUCCESS;
}

