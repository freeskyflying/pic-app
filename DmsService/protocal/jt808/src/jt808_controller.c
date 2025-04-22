#include "jt808_controller.h"
#include "6zhentan.h"
#include "strnormalize.h"

JT808Controller_t gJT808Controller = { 0 };

static int LocationIPCIRcvDataProcess(char* buff, int len, int fd, void* arg)
{
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
			controller->mCurrentAlertFlag |= JT808_ALERT_FLAG_GNSSFAULT;
		}
		else
		{
			controller->mCurrentAlertFlag &= (~JT808_ALERT_FLAG_GNSSFAULT);
		}

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
	controller->mCurrentTermState = JT808_TERMINAL_STATE_ACC_STATE;

    LibPnt_IPCConnect(&controller->mLocationIPC, PNT_IPC_GPS_NAME, LocationIPCIRcvDataProcess, NULL, 256, controller);

	while (1)
	{
		// 实时获取报警信息，查看报警标志，有变化则下发到各个session进行上报

		// 实时获取车辆/终端状态，查看状态标志，有变化则下发各个session进行上报

		// 获取其他公共数据传递给session

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

	LibPnt_IPCClientRelease(&controller->mLocationIPC);
#if 0
	for (int i=0; i<JT808SESSION_COUNT_INTIME; i++)
	{
		JT808Session_t* session = controller->mSession[i];

		if (NULL != session)
		{
			JT808Controller_StopSession(i);
		}
	}
#endif
}

void JT808Controller_Start(void)
{
	JT808Controller_t* controller = &gJT808Controller;

	if (controller->mStartFlag)
	{
		return;
	}

	JT808Database_Init(&controller->mJT808DB, controller);

	JT808ActiveSafety_Start(controller);
	
	JT1078FileUpload_Init(&controller->m1078FileUpload, controller);
	
	JT808Overspeed_Init(&controller->mOverspeed, controller);

	JT808Database_t* jt808DB = &controller->mJT808DB;

	int sessionCount = 0;

	AppPlatformParams_t appPlatform = { 0 };
	getAppPlatformParams(&appPlatform, 0);

	if (0 != strlen(appPlatform.ip) && 0 != appPlatform.port)
	{
		JT808SessionData_t sessionData = { 0 };

		memcpy(controller->mDeviceID, appPlatform.phoneNum, sizeof(controller->mDeviceID));

		strcpy((char*)sessionData.mAddress, appPlatform.ip);
		strcpy((char*)sessionData.mPhoneNum, appPlatform.phoneNum);
		sessionData.mPort = appPlatform.port;
		sessionData.mSessionType = SESSION_TYPE_JT808;
		
		if (RET_SUCCESS == JT808Database_AddSessionData(&controller->mJT808DB, &sessionData))
		{
			while (queue_list_get_count(&controller->mJT808DB.mSessionDataList) > 1)
			{
				JT808SessionData_t* sessionData = queue_list_popup(&controller->mJT808DB.mSessionDataList);
				if (NULL != sessionData)
					free(sessionData);
				else
					break;
			}
		}
	}
	if (strlen(appPlatform.plateNum))
	{
		str_normalize_init();
		unsigned int to_len = strlen(appPlatform.plateNum) + 1;
		char* buffer = (char*)malloc(to_len);
		memset(buffer, 0, to_len);
		utf8_to_gbk(appPlatform.plateNum, strlen(appPlatform.plateNum), &buffer, &to_len);
		strncpy(controller->mPlateNum, buffer, sizeof(controller->mPlateNum)-1);
		free(buffer);
	}
	controller->mPlateColor = appPlatform.plateColor;
	controller->mProtocalType = appPlatform.protocalType;

	getAppPlatformParams(&appPlatform, 1);

	if (0 != strlen(appPlatform.ip) && 0 != appPlatform.port)
	{
		JT808SessionData_t sessionData = { 0 };

		strcpy((char*)sessionData.mAddress, appPlatform.ip);
		strcpy((char*)sessionData.mPhoneNum, appPlatform.phoneNum);
		sessionData.mPort = appPlatform.port;
		sessionData.mSessionType = SESSION_TYPE_JT808;
		
		if (RET_SUCCESS == JT808Database_AddSessionData(&controller->mJT808DB, &sessionData))
		{
			while (queue_list_get_count(&controller->mJT808DB.mSessionDataList) > 2)
			{
				JT808SessionData_t* sessionData = queue_list_popup(&controller->mJT808DB.mSessionDataList);
				if (NULL != sessionData)
					free(sessionData);
				else
					break;
			}
		}
	}
	
#if (LOAD_SESSIONDATA_TO_MEM == 1)
	pthread_mutex_lock(&jt808DB->mSessionDataList.m_mutex);
	queue_t* head = jt808DB->mSessionDataList.m_head;
	while(NULL != head)
	{
		JT808SessionData_t* sessionData = (JT808SessionData_t*)head->m_content;

		controller->mSession[sessionCount] = JT808Session_Create(sessionData, controller);

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

	pthread_t pid;
	pthread_create(&pid, NULL, JT808Contorller_MainTask, controller);

	controller->mStartFlag = 1;
}

void JT808Contorller_InsertFrame(int chnVenc, int sub, unsigned char* buff1, int len1, unsigned char* buff2, int len2)
{
	int i = 0;
	JT808Controller_t* controller = &gJT808Controller;

	for (int i=0; i<JT808SESSION_COUNT_INTIME; i++)
	{
		JT808Session_t* session = controller->mSession[i];

		if (NULL != session)
		{
			JT1078Rtsp_InsertFrame(&session->mRtspController, chnVenc+1, sub, buff1, len1, buff2, len2);
		}
	}
}

void JT808Contorller_InsertAudio(int chnVenc, unsigned char* buff, int len)
{
	int i = 0;
	JT808Controller_t* controller = &gJT808Controller;

	for (int i=0; i<JT808SESSION_COUNT_INTIME; i++)
	{
		JT808Session_t* session = controller->mSession[i];

		if (NULL != session)
		{
			JT1078Rtsp_InsertAudio(&session->mRtspController, chnVenc+1, buff, len);
		}
	}
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


