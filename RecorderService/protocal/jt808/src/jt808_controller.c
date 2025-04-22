#include "jt808_controller.h"
#include "6zhentan.h"
#include "strnormalize.h"
#include "pnt_log.h"
#include "system_param.h"
#include "jt808_mediaevent.h"

JT808Controller_t gJT808Controller = { 0 };
extern volatile unsigned char gSOSKeyState;
extern volatile unsigned char gACCState;

static unsigned int Jt808Contorller_CalcDistance(GpsLocation_t* gps)
{
	static GpsLocation_t lastGpsLocation = { 0 };

	unsigned int distance = 0;

	if (0 == gps->valid)
	{
		return 0;
	}

	if (0 == lastGpsLocation.valid)
	{
		memcpy(&lastGpsLocation, gps, sizeof(GpsLocation_t));
		return 0;
	}

	distance = get_distance_by_lat_long(gps->longitude*1000000, gps->latitude*1000000, lastGpsLocation.longitude*1000000, lastGpsLocation.latitude*1000000);

	unsigned int time_delta = gps->timestamp - lastGpsLocation.timestamp;

	float speedKmh = distance*3600/(time_delta*1000);

	if (speedKmh > 160 || distance < 50)
	{
		return 0;
	}

	memcpy(&lastGpsLocation, gps, sizeof(GpsLocation_t));

	return distance;
}

static int LocationIPCIRcvDataProcess(char* buff, int len, int fd, void* arg)
{
	static int gpsFaultCount = 0;
	PNTIPC_t* ipc = (PNTIPC_t*)arg;
	JT808Controller_t* controller = (JT808Controller_t*)ipc->mOwner;

    if (len >= sizeof(GpsLocation_t))
    {
        memcpy(&controller->mLocation, buff, sizeof(GpsLocation_t));

		if (controller->mLocation.valid)
		{
			controller->mCurrentTermState |= JT808_TERMINAL_STATE_GPS_STATE;
		}
		else
		{
			controller->mCurrentTermState &= (~JT808_TERMINAL_STATE_GPS_STATE);
		}
		
		if (controller->mLocation.lat == 'S')
		{
			controller->mCurrentTermState |= JT808_TERMINAL_STATE_SOUTH_NORTH;
		}
		else
		{
			controller->mCurrentTermState &= (~JT808_TERMINAL_STATE_SOUTH_NORTH);
		}
		
		if (controller->mLocation.lon == 'W')
		{
			controller->mCurrentTermState |= JT808_TERMINAL_STATE_EAST_WEST;
		}
		else
		{
			controller->mCurrentTermState &= (~JT808_TERMINAL_STATE_EAST_WEST);
		}
		
		if (controller->mLocation.antenna == 1)
		{
			controller->mCurrentAlertFlag |= JT808_ALERT_FLAG_GNSSANTOPEN;
		}
		else if (controller->mLocation.antenna == 2)
		{
			controller->mCurrentAlertFlag |= JT808_ALERT_FLAG_GNSSANTCLOSE;
		}
		else
		{
			controller->mCurrentAlertFlag &= ~(JT808_ALERT_FLAG_GNSSANTOPEN|JT808_ALERT_FLAG_GNSSANTCLOSE);
		}

		if (controller->mLocation.timestamp == 0)
		{
			gpsFaultCount ++;
			if (gpsFaultCount > 3)
			{
				controller->mCurrentAlertFlag |= JT808_ALERT_FLAG_GNSSFAULT;
			}
		}
		else
		{
			gpsFaultCount = 0;
			controller->mCurrentAlertFlag &= (~JT808_ALERT_FLAG_GNSSFAULT);
		}

		controller->mVehicalDriveDistance += Jt808Contorller_CalcDistance(&controller->mLocation);

        return len;
    }
    else
    {
        return 0;
    }
}

void* JT808Contorller_MainTask(void* arg)
{
	JT808Controller_t* controller = (JT808Controller_t*)arg;

	controller->mCurrentAlertFlag = 0;

    LibPnt_IPCConnect(&controller->mLocationIPC, PNT_IPC_GPS_NAME, LocationIPCIRcvDataProcess, NULL, 256, controller);

	JT808ActiveSafety_Start(controller);
	
	JT1078FileUpload_Init(&controller->m1078FileUpload, controller);
	
	JT808Overspeed_Init(&controller->mOverspeed, controller);

	JT808FatigueDrive_Init(&controller->mFatigueDrive, controller);

	JT808CircleArea_Init(&controller->mCircleArea, controller);

	JT808RectArea_Init(&controller->mRectArea, controller);
	
	JT808PolygonArea_Init(&controller->mPolygonArea, controller);
	
	JT808RoadArea_Init(&controller->mRoadArea, controller);

	unsigned char lastSosKeyState = gSOSKeyState;

	while (controller->mStartFlag)
	{
		// 实时获取报警信息，查看报警标志，有变化则下发到各个session进行上报
		if (gSOSKeyState && 0 == gGlobalSysParam->mSOSDisable)
		{
			if (lastSosKeyState != gSOSKeyState)
			{
				for (int i=0; i<JT808SESSION_COUNT_INTIME; i++)
				{
					JT808Session_t* session = controller->mSession[i];

					if (NULL != session)
					{
						for (int i=0; i<MAX_VIDEO_NUM; i++)
						{
							JT808MediaEvent_MediaPhotoEvent(&session->mMediaEvent, JT808MediaEventType_EmergeAlarm, i+1, 3, 1, 1, MediaPhotoResolution_800x600);
						}
					}
				}
			}
			controller->mCurrentAlertFlag |= JT808_ALERT_FLAG_SOS;
		}
		else
		{
//			controller->mCurrentAlertFlag &= (~JT808_ALERT_FLAG_SOS);
		}
		lastSosKeyState = gSOSKeyState;

		if (gACCState)
		{
			controller->mCurrentTermState |= JT808_TERMINAL_STATE_ACC_STATE;
		}
		else
		{
			controller->mCurrentTermState &= (~JT808_TERMINAL_STATE_ACC_STATE);
		}
		
		// 实时获取车辆/终端状态，查看状态标志，有变化则下发各个session进行上报

		// 获取其他公共数据传递给session
		
		MediaStorage_UpdateVideoAlertFlag(controller->mCurrentAlertFlag, 0);

		SleepMs(100);
	}

	return NULL;
}

void JT808Controller_StopSession(int id)
{
	JT808Controller_t* controller = &gJT808Controller;
	
	JT808Session_Release(controller->mSession[id]);

	if (0 == id)
	{
		free(controller->mSession[id]->mSessionData);
		controller->mSession[id]->mSessionData = NULL;
	}
	free(controller->mSession[id]);

	controller->mSession[id] = NULL;
}

void JT808Controller_StartSession(int id)
{
	JT808Controller_t* controller = &gJT808Controller;

	if (0 == id)
	{
		JT808SessionData_t* mainSessionData = (JT808SessionData_t*)malloc(sizeof(JT808SessionData_t));

		controller->mSession[0] = JT808Session_Create(mainSessionData, controller);

		JT808Session_Start(controller->mSession[0]);
	}
}

void JT808Controller_Stop(void)
{
	JT808Controller_t* controller = &gJT808Controller;

	if (!controller->mStartFlag)
	{
		return;
	}

	for (int i=0; i<JT808SESSION_COUNT_INTIME; i++)
	{
		JT808Session_t* session = controller->mSession[i];

		if (NULL != session)
		{
			//JT808Controller_StopSession(i);
			JT808Session_SendMsg(session, 0x0003, NULL, 0, 0);
		}
	}

	controller->mStartFlag = 0;

	LibPnt_IPCClientRelease(&controller->mLocationIPC);
}

void JT808Controller_AddSession(char* ip, int port, char* phoneNum, int sessionType)
{
	JT808Controller_t* controller = &gJT808Controller;
	
	JT808SessionData_t sessionData = { 0 };
	
	strcpy((char*)sessionData.mAddress, ip);
	strcpy((char*)sessionData.mPhoneNum, phoneNum);
	sessionData.mPort = port;
	sessionData.mSessionType = sessionType;
	
	if (RET_SUCCESS == JT808Database_AddSessionData(&controller->mJT808DB, &sessionData))
	{
		PNT_LOGI("add session: %s,%d %s %d", ip, port, phoneNum, sessionType);
	}
	else
	{
		PNT_LOGI("update session: %s,%d %s %d", ip, port, phoneNum, sessionType);
	}
}

void JT808Controller_Init(void)
{
	JT808Controller_t* controller = &gJT808Controller;

	memset(controller, 0, sizeof(JT808Controller_t));
	
	JT808Database_Init(&controller->mJT808DB, controller);

	JT808DataBase_ClearSessionData(&controller->mJT808DB);

	JT808Param_t param = { 0 };
	param.mDataType = 0x03;
	param.mParamID = 0x55;
	param.mParamDataLen = 4;
	param.mParamData = gGlobalSysParam->limitSpeed;
	JT808Database_SetJT808Param(&controller->mJT808DB, &param);
	
	memcpy(controller->mDeviceID, &gGlobalSysParam->serialNum[strlen(gGlobalSysParam->serialNum)-sizeof(controller->mDeviceID)], sizeof(controller->mDeviceID));
}

void JT808Controller_Start(void)
{
	JT808Controller_t* controller = &gJT808Controller;

	if (controller->mStartFlag)
	{
		return;
	}

	if (0 == queue_list_get_count(&controller->mJT808DB.mSessionDataList))
	{
		return;
	}

	JT808Database_t* jt808DB = &controller->mJT808DB;

	int sessionCount = 0;

#if (LOAD_SESSIONDATA_TO_MEM == 1)
	pthread_mutex_lock(&jt808DB->mSessionDataList.m_mutex);
	queue_t* head = jt808DB->mSessionDataList.m_head;
	while(NULL != head)
	{
		JT808SessionData_t* sessionData = (JT808SessionData_t*)head->m_content;

		controller->mSession[sessionCount] = JT808Session_Create(sessionData, controller);

		PNT_LOGI("start session: %s,%d %s %d", sessionData->mAddress, sessionData->mPort, sessionData->mPhoneNum, sessionData->mSessionType);

		JT808Session_Start(controller->mSession[sessionCount]);
		
		sessionCount ++;

		if (sessionCount >= JT808SESSION_COUNT_INTIME)
		{
			break;
		}
		head = head->next;
	}
	pthread_mutex_unlock(&jt808DB->mSessionDataList.m_mutex);
#endif

	controller->mStartFlag = 1;

	pthread_t pid;
	pthread_create(&pid, NULL, JT808Contorller_MainTask, controller);

}

void JT808Controller_Insert0200Ext(JT808MsgBody_0200Ex_t* ext)
{
	int i = 0;
	JT808Controller_t* controller = &gJT808Controller;

	for (int i=0; i<JT808SESSION_COUNT_INTIME; i++)
	{
		JT808Session_t* session = controller->mSession[i];

		if (NULL != session)
		{
			JT808Session_InsertLocationExtra(session, ext);
		}
	}
}

void JT808Controller_ReportPassengers(uint32_t start_time, int pg_count_on, int pg_count_off)
{
	JT808MsgBody_1005_t data = { 0 };

	convertSysTimeToBCD(start_time, data.mStartBcdTime);
	getLocalBCDTime((char*)data.mEndBcdTime);
	data.mGetOnCount = pg_count_on;
	data.mGetOffCount = pg_count_off;
	
	int i = 0;
	JT808Controller_t* controller = &gJT808Controller;

	for (int i=0; i<JT808SESSION_COUNT_INTIME; i++)
	{
		JT808Session_t* session = controller->mSession[i];

		if (NULL != session)
		{
			JT808Session_ReportPassengers(session, &data);
		}
	}
}


