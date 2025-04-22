#include "algo_common.h"
#include "pnt_log.h"
#include "dms/dms_config.h"
#include "jt808_activesafety.h"
#include "media_snap.h"
#include "media_cache.h"

#define ALARM_INTERVAL  10*1000 /* same warning message interval time unit:ms */
#define WARN_PROC       1

typedef struct
{

	DWORD lasttime_call;
	DWORD lasttime_smok;
	DWORD lasttime_fati;
	DWORD lasttime_dist;
	DWORD lasttime_abnomal;

} dms_alarm_t;

#define WARNING_DBG        //printf
#define WARNING_INFO       PNT_LOGE//printf

dms_alarm_t gDmsAlarm = { 0 };

int al_warning_init(void)
{

    return 0;
}

int al_warning_proc(WARNING_MSG_TYPE warnType, int extra, int extra2)
{
	extern int gCurrentSpeed;

	unsigned int* plast_time = NULL;
	unsigned int intervel_time = 0, photo_intervel = 0, camChn = 0, last_timeval = 0;
	unsigned char speed_shreshod = 0, photo_cnt = 0, video_before = 0;
	unsigned char video_after = 0, speed_level2 = 0, alert_type = 0, enable_flag = 0;
	char audioPath[128] = { 0 };

	switch (warnType)
	{
		case LB_WARNING_DMS_DRIVER_LEAVE:
			plast_time = &(gDmsAlarm.lasttime_abnomal);
			intervel_time = gDmsAlgoParams->nodriver_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/weijiancedaojiashiyuan.pcm");
			photo_cnt = gDmsAlgoParams->nodriver_photo_count;
			photo_intervel = gDmsAlgoParams->nodriver_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->nodriver_video_time;
			alert_type = DMS_ALERT_ABNOM;
			enable_flag = gDmsAlgoParams->nodriver_enable;
			break;
		case LB_WARNING_DMS_CALL:
			plast_time = &(gDmsAlarm.lasttime_call);
			intervel_time = gDmsAlgoParams->call_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingwudadianhua.pcm");
			photo_cnt = gDmsAlgoParams->call_photo_count;
			photo_intervel = gDmsAlgoParams->call_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->call_video_time;
			alert_type = DMS_ALERT_CALL;
			enable_flag = gDmsAlgoParams->call_enable;
			break;
		case LB_WARNING_DMS_DRINK:
			plast_time = &(gDmsAlarm.lasttime_dist);
			intervel_time = gDmsAlgoParams->dist_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingwuheshui.pcm");
			photo_cnt = gDmsAlgoParams->dist_photo_count;
			photo_intervel = gDmsAlgoParams->dist_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->dist_video_time;
			alert_type = DMS_ALERT_DIST;
			enable_flag = gDmsAlgoParams->dist_enable;
			break;
		case LB_WARNING_DMS_SMOKE:
			plast_time = &(gDmsAlarm.lasttime_smok);
			intervel_time = gDmsAlgoParams->smok_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingwuxiyan.pcm");
			photo_cnt = gDmsAlgoParams->smok_photo_count;
			photo_intervel = gDmsAlgoParams->smok_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->smok_video_time;
			alert_type = DMS_ALERT_SMOK;
			enable_flag = gDmsAlgoParams->smok_enable;
			break;
		case LB_WARNING_DMS_ATTATION:
			plast_time = &(gDmsAlarm.lasttime_dist);
			intervel_time = gDmsAlgoParams->dist_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingzhuanxinjiashi.pcm");
			photo_cnt = gDmsAlgoParams->dist_photo_count;
			photo_intervel = gDmsAlgoParams->dist_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->dist_video_time;
			alert_type = DMS_ALERT_DIST;
			enable_flag = gDmsAlgoParams->dist_enable;
			break;
		case LB_WARNING_DMS_REST:
			plast_time = &(gDmsAlarm.lasttime_fati);
			intervel_time = gDmsAlgoParams->fati_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingzhuyixiuxi.pcm");
			photo_cnt = gDmsAlgoParams->fati_photo_count;
			photo_intervel = gDmsAlgoParams->fati_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->fati_video_time;
			alert_type = DMS_ALERT_FATI;
			enable_flag = gDmsAlgoParams->fati_enable;
			break;
		case LB_WARNING_DMS_CAMERA_COVER:
			plast_time = &(gDmsAlarm.lasttime_abnomal);
			intervel_time = gDmsAlgoParams->cover_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingwuzhedangshexiangtou.pcm");
			photo_cnt = gDmsAlgoParams->cover_photo_count;
			photo_intervel = gDmsAlgoParams->cover_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->cover_video_time;
			alert_type = DMS_ALERT_ABNOM;
			enable_flag = gDmsAlgoParams->cover_enable;
			break;
		case LB_WARNING_DMS_INFRARED_BLOCK:
			plast_time = &(gDmsAlarm.lasttime_abnomal);
			intervel_time = gDmsAlgoParams->glass_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingwupeidaifanhongwaiyanjing.pcm");
			photo_cnt = gDmsAlgoParams->glass_photo_count;
			photo_intervel = gDmsAlgoParams->glass_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->glass_video_time;
			alert_type = DMS_ALERT_ABNOM;
			enable_flag = gDmsAlgoParams->glass_enable;
			break;

		default:
			break;
	}
	
	camChn = 0;

	if (NULL == plast_time)
	{
		return 0;
	}

	if (gCurrentSpeed < speed_shreshod)
	{
		PNT_LOGE("speed low %d < %d, ignore alarm [%d].", gCurrentSpeed, speed_shreshod, warnType);
		return 0;
	}

	if (!enable_flag)
	{
		PNT_LOGE("alarm disabled, ignore alarm [%d].", warnType);
		return 0;
	}

	last_timeval = (*plast_time);

	if (currentTimeMillis()/1000 - last_timeval >= intervel_time)
	{
		last_timeval = (currentTimeMillis()/1000);
		
		if (0 != getFlagStatus("tfcard")) // 未插tf卡或未校时
		{
			PNT_LOGE("no tfcard or time error, ignore alarm [%d].", warnType);
			return 0;
		}
		
		playAudio(audioPath);
		
		jt808_alert_info_t alertInfo = { 0 };

		char filename[128] = { 0 };

		getLocalBCDTime((char*)alertInfo.m_alert_flag.m_datetime);
		memcpy(alertInfo.m_alert_flag.m_dev_id, "1234567897", sizeof(alertInfo.m_alert_flag.m_dev_id));
		
		alertInfo.m_alert_flag.m_event_id = 0;
		alertInfo.m_alert_flag.m_resv = ((0==camChn)?ACTIVESAFETY_ALARM_ADAS:ACTIVESAFETY_ALARM_DMS) | alert_type;

		alertInfo.m_status_flag = 1;
		alertInfo.m_speed = gCurrentSpeed;
		
		memcpy(alertInfo.m_datetime, alertInfo.m_alert_flag.m_datetime, 6);

		MediaStorage_AlarmRecycle();

		if (photo_cnt > 0)
		{
			JT808ActiveSafety_GetFilename(&alertInfo.m_alert_flag, filename, FILETYPE_PHOTO);
			
			if (RET_SUCCESS == MediaSnap_CreateSnap(camChn, photo_cnt, 1280, 720, photo_intervel*100, filename))
			{
				alertInfo.m_alert_flag.m_files_cnt += photo_cnt;
			}
		}
		if (video_after > 0 || video_before > 0)
		{
			JT808ActiveSafety_GetFilename(&alertInfo.m_alert_flag, filename, FILETYPE_VIDEO);
			
			if (RET_SUCCESS == MediaCache_VideoCreate(video_before, video_after, camChn, filename))
			{
				alertInfo.m_alert_flag.m_files_cnt += 1;
			}
		}
	}
	else
	{
		unsigned int timeresv = intervel_time - ((currentTimeMillis()/1000) - last_timeval);
		PNT_LOGE("time not in intervel scale reserve [%d s] [%d].", timeresv, warnType);
	}

	*plast_time = last_timeval;

	return 0;
}

void al_warning_msg(int msg)
{
}

