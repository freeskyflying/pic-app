#include <stdio.h>
#include <sys/errno.h>
#include <pthread.h>

#include "jt808_overspeed.h"
#include "jt808_controller.h"
#include "jt808_utils.h"
#include "jt808_database.h"
#include "utils.h"
#include "system_param.h"
#include "pnt_log.h"

void JT808Overspeed_ParamUpdate(JT808Overspeed_t* handle, unsigned short paramID)
{
	if (paramID == 0x005B || 0x0055 == paramID || 0x0056 == paramID)
	{
		handle->mParamUpdate = 1;
	}
}

void* JT808OverspeedTask(void* pArg)
{
	pthread_detach(pthread_self());
	JT808Overspeed_t* handle = (JT808Overspeed_t*)pArg;
	JT808Controller_t* controller = (JT808Controller_t*)handle->mOnwer;

	unsigned int reportTime = 0;

	unsigned int overSpeedStartTime = 0;
	unsigned int releaseSpeedStartTime = 0;
	unsigned short* ealyWarnSpeedDelta = (unsigned short*)JT808Database_GetJT808Param(&controller->mJT808DB, 0x005B, NULL);
//	unsigned int* normalSpeedLimit = (unsigned int*)JT808Database_GetJT808Param(&controller->mJT808DB, 0x0055, NULL);
	unsigned int* normalTimeThreashold = (unsigned int*)JT808Database_GetJT808Param(&controller->mJT808DB, 0x0056, NULL);

	while (controller->mStartFlag)
	{
		if (handle->mParamUpdate)
		{
			ealyWarnSpeedDelta = (unsigned short*)JT808Database_GetJT808Param(&controller->mJT808DB, 0x005B, NULL);
//			normalSpeedLimit = (unsigned int*)JT808Database_GetJT808Param(&controller->mJT808DB, 0x0055, NULL);
			normalTimeThreashold = (unsigned int*)JT808Database_GetJT808Param(&controller->mJT808DB, 0x0056, NULL);
			handle->mParamUpdate = 0;
		}
		
		if (0 == handle->mAreaType)
		{
			handle->mSpeedLimit = gGlobalSysParam->limitSpeed;//(*normalSpeedLimit);
			handle->mTimeThreshold = (NULL!=normalTimeThreashold?(*normalTimeThreashold):10);
		}
	
		unsigned int ealyWarnSpeed = handle->mSpeedLimit - ((NULL!=ealyWarnSpeedDelta?(*ealyWarnSpeedDelta):100))/10;

		if (handle->mSpeedLimit < controller->mLocation.speed)
		{
			if (0 == overSpeedStartTime)
			{
				overSpeedStartTime = currentTimeMillis()/1000;
			}
			else
			{
				if (currentTimeMillis()/1000 - releaseSpeedStartTime >= 2)
				{
					releaseSpeedStartTime = 0;
				}
				if (currentTimeMillis()/1000 - overSpeedStartTime >= handle->mTimeThreshold)
				{
					if (!(controller->mCurrentAlertFlag & JT808_ALERT_FLAG_OVERSPEED))
					{
						playAudio("xiansu/yichaosu.pcm");
						reportTime = currentTimeMillis()/1000;
					}
					else if ((gGlobalSysParam->overspeed_report_intv > 0) && (currentTimeMillis()/1000 - reportTime >= gGlobalSysParam->overspeed_report_intv))
					{
						playAudio("xiansu/yichaosu.pcm");
						reportTime = currentTimeMillis()/1000;
					}
					controller->mCurrentAlertFlag |= JT808_ALERT_FLAG_OVERSPEED;
					JT808MsgBody_0200Ex_t ext = { 0 };
					ext.mExtraInfoID = JT808_EXTRA_MSG_ID_SPEEDING;
					if (0 == handle->mAreaType)
					{
						ext.mExtraInfoLen = 1;
						ext.mExtraInfoData[0] = 0;
					}
					else
					{
						ext.mExtraInfoLen = 5;
						ext.mExtraInfoData[0] = handle->mAreaType;
						NetbufPushDWORD(ext.mExtraInfoData, handle->mAreaID, 1);
					}
					JT808Controller_Insert0200Ext(&ext);
				}
			}
		}
		else
		{
			if (0 == releaseSpeedStartTime)
			{
				releaseSpeedStartTime = currentTimeMillis()/1000;
			}
			else
			{
				if (currentTimeMillis()/1000 - releaseSpeedStartTime >= 2)
				{
					overSpeedStartTime = 0;
					if (controller->mCurrentAlertFlag & JT808_ALERT_FLAG_OVERSPEED)
					{
					}
					controller->mCurrentAlertFlag &= (~JT808_ALERT_FLAG_OVERSPEED);
				}
			}
		}

		if (controller->mCurrentAlertFlag & JT808_ALERT_FLAG_OVERSPEED)
		{
			controller->mCurrentAlertFlag &= (~JT808_ALERT_FLAG_PRE_OVERSPEED);
		}
		else
		{
			if (ealyWarnSpeed < controller->mLocation.speed)
			{
				if (!(controller->mCurrentAlertFlag & JT808_ALERT_FLAG_PRE_OVERSPEED))
				{
					playAudio("xiansu/jijiangchaosu.pcm");
					char audio_name[32] = { 0 };
					int speed = (handle->mSpeedLimit)/10*10;
					speed = speed < 30 ? 30 : speed;
					speed = speed > 120 ? 120 : speed;
					sprintf(audio_name, "xiansu/xiansu%d.pcm", speed);
					playAudio(audio_name);
				}
				controller->mCurrentAlertFlag |= JT808_ALERT_FLAG_PRE_OVERSPEED;
			}
			else
			{
				controller->mCurrentAlertFlag &= (~JT808_ALERT_FLAG_PRE_OVERSPEED);
			}
		}

		sleep(1);
	}

	return NULL;
}

void JT808Overspeed_SetSpeedLimit(JT808Overspeed_t* overspeed, unsigned int speedLimit, unsigned char areaType, unsigned int areaId, unsigned int limitThreashold)
{
	overspeed->mAreaID = areaId;
	overspeed->mAreaType = areaType;
	overspeed->mSpeedLimit = speedLimit;

	if (0 != limitThreashold)
	{
		overspeed->mTimeThreshold = limitThreashold;
	}

	if (0 != areaId)
	{
		PNT_LOGE("set area speedlimit: %d km/h, %d s by %d,%d", speedLimit, limitThreashold, areaType, areaId);
	}
}

void JT808Overspeed_Init(JT808Overspeed_t* overspeed, void* controller)
{
	memset(overspeed, 0, sizeof(JT808Overspeed_t));

	overspeed->mOnwer = controller;

	pthread_t pid;
	if (pthread_create(&pid, NULL, JT808OverspeedTask, overspeed))
	{
		PNT_LOGE("task create failed %s.", strerror(errno));
	}
}


