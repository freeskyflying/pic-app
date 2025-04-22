#include <stdlib.h>

#include "algo_common.h"
#include "utils.h"
#include "pnt_log.h"
#include "jt808_activesafety.h"
#include "jt808_controller.h"
#include "media_storage.h"
#include "media_cache.h"
#include "media_snap.h"
#include "jt808_utils.h"

static JT808ActiveSafety_t* pActivesafety = NULL;

static DWORD dword_swap(DWORD a)
{
	BYTE b[4] = { 0 };

	b[0] = a >> 24;
	b[1] = a >> 16;
	b[2] = a >> 8;
	b[3] = a;

	return ((b[3]<<24)&0xFF000000) + ((b[2]<<16)&0xFF0000) + ((b[1]<<8)&0xFF00) + b[0];
}

static WORD word_swap(WORD a)
{
	BYTE b[2] = { 0 };

	b[0] = a >> 8;
	b[1] = a;

	return ((b[1]<<8)&0xFF00) + b[0];
}

void JT808ActiveSafety_Start(void* p808controller)
{
	JT808Controller_t* controller = (JT808Controller_t*)p808controller;

	pActivesafety = &controller->mActivesafety;
	
	memset(pActivesafety, 0, sizeof(JT808ActiveSafety_t));

	pActivesafety->dmsParam = JT808Database_GetJT808Param(&controller->mJT808DB, 0xF365, NULL);
	
	printf("speed_thd: %d\n", pActivesafety->dmsParam->m_speed_alert_threshold);
	printf("volume: %d\n", pActivesafety->dmsParam->m_alert_volume);
	printf("active_phone_cnt: %d\n", pActivesafety->dmsParam->m_active_photo_cnt);
	printf("iden_trigger: %d\n", pActivesafety->dmsParam->m_driver_iden_trigger);
	printf("alert_enable: %08X\n", dword_swap(pActivesafety->dmsParam->m_alerts_enable_set));
	
	pActivesafety->adasParam = JT808Database_GetJT808Param(&controller->mJT808DB, 0xF364, NULL);

	printf("speed_thd: %d\n", pActivesafety->adasParam->m_speed_alert_threshold);
	printf("volume: %d\n", pActivesafety->adasParam->m_alert_volume);
	printf("active_phone_cnt: %d\n", pActivesafety->adasParam->m_active_photo_cnt);
	printf("iden_trigger: %d\n", pActivesafety->adasParam->m_lmi_photo_time_interval);
	printf("alert_enable: %08X\n", dword_swap(pActivesafety->adasParam->m_alert_enables));
}

void JT808ActiveSafety_GetFilename(jt808_alert_flag_t* alertFlag, char* filename, char fileType)
{
	sprintf(filename, 
		ALARM_PATH"/%02X%02X%02X%02X%02X%02X_%02X%s",
		alertFlag->m_datetime[0],
		alertFlag->m_datetime[1],
		alertFlag->m_datetime[2],
		alertFlag->m_datetime[3],
		alertFlag->m_datetime[4],
		alertFlag->m_datetime[5],
		alertFlag->m_resv,
		(fileType==FILETYPE_PHOTO)?".jpg":".mp4"
	);
}

void JT808ActiveSafety_AlarmReport(int type, jt808_alert_info_t* alertInfo)
{
	alertInfo->m_alert_id = pActivesafety->alertID ++;

	JT808MsgBody_0200Ex_t ext = { 0 };

	ext.mExtraInfoID = (type==ACTIVESAFETY_ALARM_ADAS)?0x64:0x65;

	ext.mExtraInfoLen = NetbufPushDWORD(ext.mExtraInfoData, alertInfo->m_alert_id, ext.mExtraInfoLen);
	ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, alertInfo->m_status_flag, ext.mExtraInfoLen);
	
	if (0x64 == ext.mExtraInfoID)
	{
		jsatl_alert_adas_t* adas = &alertInfo->m_type_info.m_adas_event;
		ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, adas->m_alert_type, ext.mExtraInfoLen);
		ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, adas->m_alert_level, ext.mExtraInfoLen);
		ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, adas->m_front_car_speed, ext.mExtraInfoLen);
		ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, adas->m_front_abs_distance, ext.mExtraInfoLen);
		ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, adas->m_departure_type, ext.mExtraInfoLen);
		ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, adas->m_road_flag_type, ext.mExtraInfoLen);
		ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, adas->m_road_flag_count, ext.mExtraInfoLen);
	}
	else if (0x65 == ext.mExtraInfoID)
	{
		jsatl_alert_dsm_t* dms = &alertInfo->m_type_info.m_dsm_event;
		ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, dms->m_alert_type, ext.mExtraInfoLen);
		ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, dms->m_alert_level, ext.mExtraInfoLen);
		ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, dms->m_fati_degree, ext.mExtraInfoLen);
		ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, dms->m_reserved[0], ext.mExtraInfoLen);
		ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, dms->m_reserved[0], ext.mExtraInfoLen);
		ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, dms->m_reserved[0], ext.mExtraInfoLen);
		ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, dms->m_reserved[0], ext.mExtraInfoLen);
	}
	
	ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, alertInfo->m_speed, ext.mExtraInfoLen);
	ext.mExtraInfoLen = NetbufPushWORD(ext.mExtraInfoData, alertInfo->m_altitude, ext.mExtraInfoLen);
	ext.mExtraInfoLen = NetbufPushDWORD(ext.mExtraInfoData, alertInfo->m_latitude, ext.mExtraInfoLen);
	ext.mExtraInfoLen = NetbufPushDWORD(ext.mExtraInfoData, alertInfo->m_longtitude, ext.mExtraInfoLen);
	ext.mExtraInfoLen = NetbufPushBuffer(ext.mExtraInfoData, alertInfo->m_datetime, ext.mExtraInfoLen, 6);
	ext.mExtraInfoLen = NetbufPushWORD(ext.mExtraInfoData, alertInfo->m_vehicel_status, ext.mExtraInfoLen);

	jt808_alert_flag_t* alertFlag = &alertInfo->m_alert_flag;
	ext.mExtraInfoLen = NetbufPushBuffer(ext.mExtraInfoData, alertFlag->m_dev_id, ext.mExtraInfoLen, 7);
	ext.mExtraInfoLen = NetbufPushBuffer(ext.mExtraInfoData, alertFlag->m_datetime, ext.mExtraInfoLen, 6);
	ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, alertFlag->m_event_id, ext.mExtraInfoLen);
	ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, alertFlag->m_files_cnt, ext.mExtraInfoLen);
	ext.mExtraInfoLen = NetbufPushBYTE(ext.mExtraInfoData, alertFlag->m_resv, ext.mExtraInfoLen);

	JT808Controller_Insert0200Ext(&ext);
}

void JT808ActiveSafety_ProcAlarm(int type, int extra, int extra2, int speed)
{
	unsigned int* plast_time = NULL;
	unsigned int intervel_time = 0, photo_intervel = 0, camChn = 0, last_timeval = 0;
	unsigned char speed_shreshod = 0, photo_cnt = 0, video_before = 0;
	unsigned char video_after = 0, speed_level2 = 0, alert_type = 0, enable_flag = 0;
	char audioPath[128] = { 0 };

	JT808Controller_t* controller = (JT808Controller_t*)&gJT808Controller;

	if (NULL == pActivesafety)
	{
		PNT_LOGE("pActivesafety not inited.");
		return;
	}

	switch (type)
	{
		case LB_WARNING_DMS_DRIVER_LEAVE:
			plast_time = &(pActivesafety->lasttime_abnomal);
			intervel_time = word_swap(pActivesafety->dmsParam->m_call_check_time_interval);
			speed_shreshod = pActivesafety->dmsParam->m_speed_alert_threshold;
			strcpy(audioPath, "dms_warning/weijiancedaojiashiyuan.pcm");
			photo_cnt = pActivesafety->dmsParam->m_abnm_photo_cnt;
			photo_intervel = pActivesafety->dmsParam->m_abnm_photo_time_interval;
			video_after = video_before = pActivesafety->dmsParam->m_abnm_video_time;
			speed_level2 = pActivesafety->dmsParam->m_abnm_speed_alert_threshold;
			camChn = 1;
			alert_type = DMS_ALERT_ABNOM;
			enable_flag = dword_swap(pActivesafety->dmsParam->m_alerts_enable_set) >> 8;
			break;
		case LB_WARNING_DMS_CALL:
			plast_time = &(pActivesafety->lasttime_call);
			intervel_time = word_swap(pActivesafety->dmsParam->m_call_check_time_interval);
			speed_shreshod = pActivesafety->dmsParam->m_speed_alert_threshold;
			strcpy(audioPath, "dms_warning/qingwudadianhua.pcm");
			photo_cnt = pActivesafety->dmsParam->m_call_photo_cnt;
			photo_intervel = pActivesafety->dmsParam->m_call_photo_time_interval;
			video_after = video_before = pActivesafety->dmsParam->m_call_video_record_time;
			speed_level2 = pActivesafety->dmsParam->m_call_speed_alert_threshold;
			camChn = 1;
			alert_type = DMS_ALERT_CALL;
			enable_flag = dword_swap(pActivesafety->dmsParam->m_alerts_enable_set) >> 2;
			break;
		case LB_WARNING_DMS_DRINK:
			plast_time = &(pActivesafety->lasttime_dist);
			intervel_time = word_swap(pActivesafety->dmsParam->m_call_check_time_interval);
			speed_shreshod = pActivesafety->dmsParam->m_speed_alert_threshold;
			strcpy(audioPath, "dms_warning/qingwuheshui.pcm");
			photo_cnt = pActivesafety->dmsParam->m_dist_photo_cnt;
			photo_intervel = pActivesafety->dmsParam->m_dist_photo_time_interval;
			video_after = video_before = pActivesafety->dmsParam->m_dist_video_record_time;
			speed_level2 = pActivesafety->dmsParam->m_dist_speed_alert_threshold;
			camChn = 1;
			alert_type = DMS_ALERT_DIST;
			enable_flag = dword_swap(pActivesafety->dmsParam->m_alerts_enable_set) >> 6;
			break;
		case LB_WARNING_DMS_SMOKE:
			plast_time = &(pActivesafety->lasttime_smok);
			intervel_time = word_swap(pActivesafety->dmsParam->m_call_check_time_interval);
			speed_shreshod = pActivesafety->dmsParam->m_speed_alert_threshold;
			strcpy(audioPath, "dms_warning/qingwuxiyan.pcm");
			photo_cnt = pActivesafety->dmsParam->m_smok_photo_cnt;
			photo_intervel = pActivesafety->dmsParam->m_smok_photo_time_interval;
			video_after = video_before = pActivesafety->dmsParam->m_smok_video_record_time;
			speed_level2 = pActivesafety->dmsParam->m_smok_speed_alert_threshold;
			camChn = 1;
			alert_type = DMS_ALERT_SMOK;
			enable_flag = dword_swap(pActivesafety->dmsParam->m_alerts_enable_set) >> 4;
			break;
		case LB_WARNING_DMS_ATTATION:
			plast_time = &(pActivesafety->lasttime_dist);
			intervel_time = word_swap(pActivesafety->dmsParam->m_call_check_time_interval);
			speed_shreshod = pActivesafety->dmsParam->m_speed_alert_threshold;
			strcpy(audioPath, "dms_warning/qingzhuanxinjiashi.pcm");
			photo_cnt = pActivesafety->dmsParam->m_dist_photo_cnt;
			photo_intervel = pActivesafety->dmsParam->m_dist_photo_time_interval;
			video_after = video_before = pActivesafety->dmsParam->m_dist_video_record_time;
			speed_level2 = pActivesafety->dmsParam->m_dist_speed_alert_threshold;
			camChn = 1;
			alert_type = DMS_ALERT_DIST;
			enable_flag = dword_swap(pActivesafety->dmsParam->m_alerts_enable_set) >> 6;
			break;
		case LB_WARNING_DMS_REST:
			plast_time = &(pActivesafety->lasttime_fati);
			intervel_time = word_swap(pActivesafety->dmsParam->m_call_check_time_interval);
			speed_shreshod = pActivesafety->dmsParam->m_speed_alert_threshold;
			strcpy(audioPath, "dms_warning/qingzhuyixiuxi.pcm");
			photo_cnt = pActivesafety->dmsParam->m_fati_photo_cnt;
			photo_intervel = pActivesafety->dmsParam->m_fati_photo_time_interval;
			video_after = video_before = pActivesafety->dmsParam->m_fati_video_record_time;
			speed_level2 = pActivesafety->dmsParam->m_fati_speed_alert_threshold;
			camChn = 1;
			alert_type = DMS_ALERT_FATI;
			enable_flag = dword_swap(pActivesafety->dmsParam->m_alerts_enable_set) >> 0;
			break;
		case LB_WARNING_DMS_CAMERA_COVER:
			plast_time = &(pActivesafety->lasttime_abnomal);
			intervel_time = word_swap(pActivesafety->dmsParam->m_call_check_time_interval);
			speed_shreshod = pActivesafety->dmsParam->m_speed_alert_threshold;
			strcpy(audioPath, "dms_warning/qingwuzhedangshexiangtou.pcm");
			photo_cnt = pActivesafety->dmsParam->m_abnm_photo_cnt;
			photo_intervel = pActivesafety->dmsParam->m_abnm_photo_time_interval;
			video_after = video_before = pActivesafety->dmsParam->m_abnm_video_time;
			speed_level2 = pActivesafety->dmsParam->m_abnm_speed_alert_threshold;
			camChn = 1;
			alert_type = DMS_ALERT_ABNOM;
			enable_flag = dword_swap(pActivesafety->dmsParam->m_alerts_enable_set) >> 8;
			break;
		case LB_WARNING_DMS_INFRARED_BLOCK:
			plast_time = &(pActivesafety->lasttime_abnomal);
			intervel_time = word_swap(pActivesafety->dmsParam->m_call_check_time_interval);
			speed_shreshod = pActivesafety->dmsParam->m_speed_alert_threshold;
			strcpy(audioPath, "dms_warning/qingwupeidaifanhongwaiyanjing.pcm");
			photo_cnt = pActivesafety->dmsParam->m_abnm_photo_cnt;
			photo_intervel = pActivesafety->dmsParam->m_abnm_photo_time_interval;
			video_after = video_before = pActivesafety->dmsParam->m_abnm_video_time;
			speed_level2 = pActivesafety->dmsParam->m_abnm_speed_alert_threshold;
			camChn = 1;
			alert_type = DMS_ALERT_ABNOM;
			enable_flag = dword_swap(pActivesafety->dmsParam->m_alerts_enable_set) >> 8;
			break;
            
		case LB_WARNING_ADAS_LAUNCH:
			//playAudio("adas_warning/alarm_launch.pcm");
			break;
		case LB_WARNING_ADAS_DISTANCE:
			plast_time = &(pActivesafety->lasttime_hmw);
			intervel_time = word_swap(pActivesafety->dmsParam->m_call_check_time_interval);
			speed_shreshod = pActivesafety->adasParam->m_speed_alert_threshold;
			strcpy(audioPath, "adas_warning/alarm_distance.pcm");
			photo_cnt = pActivesafety->adasParam->m_hmw_photo_cnt;
			photo_intervel = pActivesafety->adasParam->m_hmw_photo_time_interval;
			video_after = video_before = pActivesafety->adasParam->m_hmw_video_record_time;
			speed_level2 = pActivesafety->adasParam->m_hmw_speed_level;
			camChn = 0;
			alert_type = ADAS_ALERT_HMW;
			enable_flag = dword_swap(pActivesafety->adasParam->m_alert_enables) >> 10;
			break;
		case LB_WARNING_ADAS_COLLIDE:
			plast_time = &(pActivesafety->lasttime_fcw);
			intervel_time = word_swap(pActivesafety->dmsParam->m_call_check_time_interval);
			speed_shreshod = pActivesafety->adasParam->m_speed_alert_threshold;
			strcpy(audioPath, "adas_warning/alarm_collide.pcm");
			photo_cnt = pActivesafety->adasParam->m_fcw_photo_cnt;
			photo_intervel = pActivesafety->adasParam->m_fcw_photo_time_interval;
			video_after = video_before = pActivesafety->adasParam->m_fcw_video_record_time;
			speed_level2 = pActivesafety->adasParam->m_fcw_speed_level;
			camChn = 0;
			alert_type = ADAS_ALERT_FCW;
			enable_flag = dword_swap(pActivesafety->adasParam->m_alert_enables) >> 6;
			break;
		case LB_WARNING_ADAS_PRESSURE:
			plast_time = &(pActivesafety->lasttime_ldw);
			intervel_time = word_swap(pActivesafety->dmsParam->m_call_check_time_interval);
			speed_shreshod = pActivesafety->adasParam->m_speed_alert_threshold;
			strcpy(audioPath, "adas_warning/alarm_pressure.pcm");
			photo_cnt = pActivesafety->adasParam->m_ldw_photo_cnt;
			photo_intervel = pActivesafety->adasParam->m_ldw_photo_time_interval;
			video_after = video_before = pActivesafety->adasParam->m_ldw_video_record_time;
			speed_level2 = pActivesafety->adasParam->m_ldw_speed_level;
			camChn = 0;
			alert_type = ADAS_ALERT_LDW;
			enable_flag = dword_swap(pActivesafety->adasParam->m_alert_enables) >> 4;
			break;
		case LB_WARNING_ADAS_PEDS:
			plast_time = &(pActivesafety->lasttime_pcw);
			intervel_time = word_swap(pActivesafety->dmsParam->m_call_check_time_interval);
			speed_shreshod = pActivesafety->adasParam->m_speed_alert_threshold;
			strcpy(audioPath, "adas_warning/xiaoxinxingren.pcm");
			photo_cnt = pActivesafety->adasParam->m_pcw_photo_cnt;
			photo_intervel = pActivesafety->adasParam->m_pcw_photo_time_interval;
			video_after = video_before = pActivesafety->adasParam->m_pcw_video_record_time;
			speed_level2 = pActivesafety->adasParam->m_pcw_speed_level;
			camChn = 0;
			alert_type = ADAS_ALERT_PCW;
			enable_flag = dword_swap(pActivesafety->adasParam->m_alert_enables) >> 8;
			break;
		case LB_WARNING_ADAS_CHANGLANE:
			//playAudio("adas_warning/cheliangbiandao.pcm");
			break;

		default:
			break;
	}

	if (NULL == plast_time)
	{
		return;
	}

	speed = speed_shreshod = 0;
	if (speed < speed_shreshod)
	{
		PNT_LOGE("speed low %d < %d, ignore alarm [%d].", speed, speed_shreshod, type);
		return;
	}

	if (speed < speed_level2 && !(enable_flag & 0x01))
	{
		PNT_LOGE("level1 disabled, ignore alarm [%d].", type);
		return;
	}

	if (speed >= speed_level2 && !(enable_flag & 0x02))
	{
		PNT_LOGE("level2 disabled, ignore alarm [%d].", type);
		return;
	}

	last_timeval = (*plast_time);

	if (currentTimeMillis()/1000 - last_timeval >= intervel_time)
	{
		last_timeval = (currentTimeMillis()/1000);
		
		if (0 != getFlagStatus("tfcard") || 1660000000 > currentTimeMillis()/1000) // 未插tf卡或未校时
		{
			PNT_LOGE("no tfcard or time error, ignore alarm [%d].", type);
			return;
		}
		
		playAudio(audioPath);
		
		jt808_alert_info_t alertInfo = { 0 };

		char filename[128] = { 0 };

		getLocalBCDTime((char*)alertInfo.m_alert_flag.m_datetime);
		memcpy(alertInfo.m_alert_flag.m_dev_id, controller->mDeviceID, sizeof(alertInfo.m_alert_flag.m_dev_id));
		
		alertInfo.m_alert_flag.m_event_id = 0;
		alertInfo.m_alert_flag.m_resv = ((0==camChn)?ACTIVESAFETY_ALARM_ADAS:ACTIVESAFETY_ALARM_DMS) | alert_type;

		alertInfo.m_status_flag = 1;
		alertInfo.m_speed = speed;
		if (controller->mCurrentTermState & JT808_TERMINAL_STATE_ACC_STATE)
		{
			alertInfo.m_vehicel_status |= 0x0001;
		}
		if (controller->mCurrentTermState & JT808_TERMINAL_STATE_GPS_STATE)
		{
			alertInfo.m_vehicel_status |= 0x0400;
		}
		
		alertInfo.m_latitude = (DWORD)(controller->mLocation.latitude*1000000);
		alertInfo.m_longtitude = (DWORD)(controller->mLocation.longitude*1000000);
		alertInfo.m_altitude = (WORD)(controller->mLocation.altitude);
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

		if (0==camChn)
		{
			jsatl_alert_adas_t* adas = &alertInfo.m_type_info.m_adas_event;
			adas->m_alert_type = alert_type;
			adas->m_alert_level = (speed >= speed_level2)?2:1;
			adas->m_departure_type = extra;
			adas->m_front_car_speed = extra2;
			adas->m_front_abs_distance = extra;
			
		}
		else if (1 == camChn)
		{
			jsatl_alert_dsm_t* dms = &alertInfo.m_type_info.m_dsm_event;
			dms->m_alert_type = alert_type;
			dms->m_alert_level = (speed >= speed_level2)?2:1;
			dms->m_fati_degree = extra;
		}

		JT808ActiveSafety_AlarmReport(((0==camChn)?ACTIVESAFETY_ALARM_ADAS:ACTIVESAFETY_ALARM_DMS), &alertInfo);
	}
	else
	{
		unsigned int timeresv = intervel_time - ((currentTimeMillis()/1000) - last_timeval);
		PNT_LOGE("time not in intervel scale reserve [%d s] [%d].", timeresv, type);
	}

	*plast_time = last_timeval;
}


