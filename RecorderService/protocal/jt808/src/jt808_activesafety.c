#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <errno.h>
#include <sys/prctl.h>
#include <linux/falloc.h>

#include "algo_common.h"
#include "utils.h"
#include "pnt_log.h"
#include "jt808_activesafety.h"
#include "jt808_controller.h"
#include "media_storage.h"
#include "media_cache.h"
#include "media_snap.h"
#include "jt808_utils.h"
#include "dms/dms_config.h"
#include "adas/adas_config.h"

static JT808ActiveSafety_t* pActivesafety = NULL;

static void JT808ActiveSafety_StoreParamsFromMem(unsigned int id, unsigned char* data)
{
	int index = 0;
	
	if (0xF364 == id)
	{
		jt808_setting_info_adas_t* param = &pActivesafety->adasParam;
		
		param->m_speed_alert_threshold = gAdasAlgoParams->alarm_speed;
		
		param->m_fcw_photo_cnt = gAdasAlgoParams->fcw_photo_count;
		param->m_fcw_photo_time_interval = gAdasAlgoParams->fcw_photo_intervel_ms/100;
		param->m_fcw_video_record_time = gAdasAlgoParams->fcw_video_time;
		param->m_fcw_speed_level = gAdasAlgoParams->fcw_speed_level2;
		
		param->m_ldw_photo_cnt = gAdasAlgoParams->ldw_photo_count;
		param->m_ldw_photo_time_interval = gAdasAlgoParams->ldw_photo_intervel_ms/100;
		param->m_ldw_video_record_time = gAdasAlgoParams->ldw_video_time;
		param->m_ldw_speed_level = gAdasAlgoParams->ldw_speed_level2;
		
		param->m_pcw_photo_cnt = gAdasAlgoParams->pcw_photo_count;
		param->m_pcw_photo_time_interval = gAdasAlgoParams->pcw_photo_intervel_ms/100;
		param->m_pcw_video_record_time = gAdasAlgoParams->pcw_video_time;
		param->m_pcw_speed_level = gAdasAlgoParams->pcw_speed_level2;
			
		param->m_hmw_photo_cnt = gAdasAlgoParams->hmw_photo_count;
		param->m_hmw_photo_time_interval = gAdasAlgoParams->hmw_photo_intervel_ms/100;
		param->m_hmw_video_record_time = gAdasAlgoParams->hmw_video_time;
		param->m_hmw_speed_level = gAdasAlgoParams->hmw_speed_level2;
		
		param->m_alert_enables = (gAdasAlgoParams->fcw_en<<6) & 0x00C0;
		param->m_alert_enables |= (gAdasAlgoParams->ldw_en<<4) & 0x0030;
		param->m_alert_enables |= (gAdasAlgoParams->hmw_en<<10) & 0x0C00;
		param->m_alert_enables |= (gAdasAlgoParams->pcw_en<<8) & 0x0300;
		
		index = NetbufPushBYTE(data, param->m_speed_alert_threshold, index);
		index = NetbufPushBYTE(data, param->m_alert_volume, index);
		index = NetbufPushBYTE(data, param->m_active_photo, index);
		index = NetbufPushWORD(data, param->m_active_photo_timer_interval, index);
		index = NetbufPushWORD(data, param->m_active_photo_dist_interval, index);
		index = NetbufPushBYTE(data, param->m_active_photo_cnt, index);
		index = NetbufPushBYTE(data, param->m_active_photo_time_interval, index);
		index = NetbufPushBYTE(data, param->m_photo_resolution, index);
		index = NetbufPushBYTE(data, param->m_video_resolution, index);
		index = NetbufPushDWORD(data, param->m_alert_enables, index);
		index = NetbufPushDWORD(data, param->m_event_enables, index);
		index = NetbufPushBYTE(data, param->m_reserved1, index);
		index = NetbufPushBYTE(data, param->m_abs_distance_threshold, index);
		index = NetbufPushBYTE(data, param->m_abs_speed_level, index);
		index = NetbufPushBYTE(data, param->m_abs_video_recod_time, index);
		index = NetbufPushBYTE(data, param->m_abs_photo_cnt, index);
		index = NetbufPushBYTE(data, param->m_abs_photo_time_interval, index);
		index = NetbufPushBYTE(data, param->m_lcw_check_time_interval, index);
		index = NetbufPushBYTE(data, param->m_lcw_check_times, index);
		index = NetbufPushBYTE(data, param->m_lcw_speed_level, index);
		index = NetbufPushBYTE(data, param->m_lcw_video_record_time, index);
		index = NetbufPushBYTE(data, param->m_lcw_photo_cnt, index);
		index = NetbufPushBYTE(data, param->m_lcw_photo_time_interval, index);
		index = NetbufPushBYTE(data, param->m_ldw_speed_level, index);
		index = NetbufPushBYTE(data, param->m_ldw_video_record_time, index);
		index = NetbufPushBYTE(data, param->m_ldw_photo_cnt, index);
		index = NetbufPushBYTE(data, param->m_ldw_photo_time_interval, index);
		index = NetbufPushBYTE(data, param->m_fcw_time_threshold, index);
		index = NetbufPushBYTE(data, param->m_fcw_speed_level, index);
		index = NetbufPushBYTE(data, param->m_fcw_video_record_time, index);
		index = NetbufPushBYTE(data, param->m_fcw_photo_cnt, index);
		index = NetbufPushBYTE(data, param->m_fcw_photo_time_interval, index);
		index = NetbufPushBYTE(data, param->m_pcw_time_threshold, index);
		index = NetbufPushBYTE(data, param->m_pcw_speed_level, index);
		index = NetbufPushBYTE(data, param->m_pcw_video_record_time, index);
		index = NetbufPushBYTE(data, param->m_pcw_photo_cnt, index);
		index = NetbufPushBYTE(data, param->m_pcw_photo_time_interval, index);
		index = NetbufPushBYTE(data, param->m_hmw_distance_threshold, index);
		index = NetbufPushBYTE(data, param->m_hmw_speed_level, index);
		index = NetbufPushBYTE(data, param->m_hmw_video_record_time, index);
		index = NetbufPushBYTE(data, param->m_hmw_photo_cnt, index);
		index = NetbufPushBYTE(data, param->m_hmw_photo_time_interval, index);
		index = NetbufPushBYTE(data, param->m_lmi_photo_cnt, index);
		index = NetbufPushBYTE(data, param->m_lmi_photo_time_interval, index);
	}
	else if (0xF365 == id)
	{
		jt808_setting_info_dsm_t* param = &pActivesafety->dmsParam;
		
		param->m_speed_alert_threshold = gDmsAlgoParams->alarm_speed;
		
		param->m_call_photo_cnt = gDmsAlgoParams->call_photo_count;
		param->m_call_photo_time_interval = gDmsAlgoParams->call_photo_intervel_ms/100;
		param->m_call_video_record_time = gDmsAlgoParams->call_video_time;
		param->m_call_check_time_interval = gDmsAlgoParams->call_intervel;
		param->m_call_speed_alert_threshold = gDmsAlgoParams->call_speed_level2;
		
		param->m_fati_photo_cnt = gDmsAlgoParams->fati_photo_count;
		param->m_fati_photo_time_interval = gDmsAlgoParams->fati_photo_intervel_ms/100;
		param->m_fati_video_record_time = gDmsAlgoParams->fati_video_time;
		param->m_fati_speed_alert_threshold = gDmsAlgoParams->fati_speed_level2;
		
		param->m_dist_photo_cnt = gDmsAlgoParams->dist_photo_count;
		param->m_dist_photo_time_interval = gDmsAlgoParams->dist_photo_intervel_ms/100;
		param->m_dist_video_record_time = gDmsAlgoParams->dist_video_time;
		param->m_dist_speed_alert_threshold = gDmsAlgoParams->dist_speed_level2;
		
		param->m_smok_photo_cnt = gDmsAlgoParams->smok_photo_count;
		param->m_smok_photo_time_interval = gDmsAlgoParams->smok_photo_intervel_ms/100;
		param->m_smok_video_record_time = gDmsAlgoParams->smok_video_time;
		param->m_smok_check_time_interval = gDmsAlgoParams->smok_intervel;
		param->m_smok_speed_alert_threshold = gDmsAlgoParams->smok_speed_level2;
		
		param->m_abnm_photo_cnt = gDmsAlgoParams->nodriver_photo_count;
		param->m_abnm_photo_time_interval = gDmsAlgoParams->nodriver_photo_intervel_ms/100;
		param->m_abnm_video_time = gDmsAlgoParams->nodriver_video_time;
		param->m_abnm_speed_alert_threshold = gDmsAlgoParams->abnm_speed_level2;
		
		param->m_alert_enables = (gDmsAlgoParams->fati_enable<<0) & 0x0003;
		param->m_alert_enables |= (gDmsAlgoParams->call_enable<<2) & 0x000C;
		param->m_alert_enables |= (gDmsAlgoParams->smok_enable<<4) & 0x0030;
		param->m_alert_enables |= (gDmsAlgoParams->dist_enable<<6) & 0x00C0;
		param->m_alert_enables |= (gDmsAlgoParams->nodriver_enable<<8) & 0x0300;
		
		index = NetbufPushBYTE(data, param->m_speed_alert_threshold, index);
		index = NetbufPushBYTE(data, param->m_alert_volume, index);
		index = NetbufPushBYTE(data, param->m_active_photo, index);
		index = NetbufPushWORD(data, param->m_active_photo_timer_interval, index);
		index = NetbufPushWORD(data, param->m_active_photo_dist_interval, index);
		index = NetbufPushBYTE(data, param->m_active_photo_cnt, index);
		index = NetbufPushBYTE(data, param->m_active_photo_time_interval, index);
		index = NetbufPushBYTE(data, param->m_photo_resolution, index);
		index = NetbufPushBYTE(data, param->m_video_resolution, index);
		index = NetbufPushDWORD(data, param->m_alert_enables, index);
		index = NetbufPushDWORD(data, param->m_event_enables, index);
		index = NetbufPushWORD(data, param->m_smok_check_time_interval, index);
		index = NetbufPushWORD(data, param->m_call_check_time_interval, index);
		index = NetbufPushBuffer(data, param->m_reserved1, index, 3);
		index = NetbufPushBYTE(data, param->m_fati_speed_alert_threshold, index);
		index = NetbufPushBYTE(data, param->m_fati_video_record_time, index);
		index = NetbufPushBYTE(data, param->m_fati_photo_cnt, index);
		index = NetbufPushBYTE(data, param->m_fati_photo_time_interval, index);
		index = NetbufPushBYTE(data, param->m_call_speed_alert_threshold, index);
		index = NetbufPushBYTE(data, param->m_call_video_record_time, index);
		index = NetbufPushBYTE(data, param->m_call_photo_cnt, index);
		index = NetbufPushBYTE(data, param->m_call_photo_time_interval, index);
		index = NetbufPushBYTE(data, param->m_smok_speed_alert_threshold, index);
		index = NetbufPushBYTE(data, param->m_smok_video_record_time, index);
		index = NetbufPushBYTE(data, param->m_smok_photo_cnt, index);
		index = NetbufPushBYTE(data, param->m_smok_photo_time_interval, index);
		index = NetbufPushBYTE(data, param->m_dist_speed_alert_threshold, index);
		index = NetbufPushBYTE(data, param->m_dist_video_record_time, index);
		index = NetbufPushBYTE(data, param->m_dist_photo_cnt, index);
		index = NetbufPushBYTE(data, param->m_dist_photo_time_interval, index);
		index = NetbufPushBYTE(data, param->m_abnm_speed_alert_threshold, index);
		index = NetbufPushBYTE(data, param->m_abnm_video_time, index);
		index = NetbufPushBYTE(data, param->m_abnm_photo_cnt, index);
		index = NetbufPushBYTE(data, param->m_abnm_photo_time_interval, index);
		index = NetbufPushBYTE(data, param->m_driver_iden_trigger, index);
	}

	char str[1024] = { 0 };
	for(int i=0; i<index; i++)
	{
		sprintf(str, "%s%02X", str, data[i]);
	}

	PNT_LOGI("store param[%04X]: %s", id, str);
}

void JT808ActiveSafety_UpdateParamsFromMem(unsigned int id, unsigned char* data, int syncFlag)
{
	int index = 0;
	
	if (0xF364 == id)
	{
		jt808_setting_info_adas_t* param = &pActivesafety->adasParam;
		
		index = NetbufPopBYTE(data, &param->m_speed_alert_threshold, index);
		index = NetbufPopBYTE(data, &param->m_alert_volume, index);
		index = NetbufPopBYTE(data, &param->m_active_photo, index);
		index = NetbufPopWORD(data, &param->m_active_photo_timer_interval, index);
		index = NetbufPopWORD(data, &param->m_active_photo_dist_interval, index);
		index = NetbufPopBYTE(data, &param->m_active_photo_cnt, index);
		index = NetbufPopBYTE(data, &param->m_active_photo_time_interval, index);
		index = NetbufPopBYTE(data, &param->m_photo_resolution, index);
		index = NetbufPopBYTE(data, &param->m_video_resolution, index);
		index = NetbufPopDWORD(data, &param->m_alert_enables, index);
		index = NetbufPopDWORD(data, &param->m_event_enables, index);
		index = NetbufPopBYTE(data, &param->m_reserved1, index);
		index = NetbufPopBYTE(data, &param->m_abs_distance_threshold, index);
		index = NetbufPopBYTE(data, &param->m_abs_speed_level, index);
		index = NetbufPopBYTE(data, &param->m_abs_video_recod_time, index);
		index = NetbufPopBYTE(data, &param->m_abs_photo_cnt, index);
		index = NetbufPopBYTE(data, &param->m_abs_photo_time_interval, index);
		index = NetbufPopBYTE(data, &param->m_lcw_check_time_interval, index);
		index = NetbufPopBYTE(data, &param->m_lcw_check_times, index);
		index = NetbufPopBYTE(data, &param->m_lcw_speed_level, index);
		index = NetbufPopBYTE(data, &param->m_lcw_video_record_time, index);
		index = NetbufPopBYTE(data, &param->m_lcw_photo_cnt, index);
		index = NetbufPopBYTE(data, &param->m_lcw_photo_time_interval, index);
		index = NetbufPopBYTE(data, &param->m_ldw_speed_level, index);
		index = NetbufPopBYTE(data, &param->m_ldw_video_record_time, index);
		index = NetbufPopBYTE(data, &param->m_ldw_photo_cnt, index);
		index = NetbufPopBYTE(data, &param->m_ldw_photo_time_interval, index);
		index = NetbufPopBYTE(data, &param->m_fcw_time_threshold, index);
		index = NetbufPopBYTE(data, &param->m_fcw_speed_level, index);
		index = NetbufPopBYTE(data, &param->m_fcw_video_record_time, index);
		index = NetbufPopBYTE(data, &param->m_fcw_photo_cnt, index);
		index = NetbufPopBYTE(data, &param->m_fcw_photo_time_interval, index);
		index = NetbufPopBYTE(data, &param->m_pcw_time_threshold, index);
		index = NetbufPopBYTE(data, &param->m_pcw_speed_level, index);
		index = NetbufPopBYTE(data, &param->m_pcw_video_record_time, index);
		index = NetbufPopBYTE(data, &param->m_pcw_photo_cnt, index);
		index = NetbufPopBYTE(data, &param->m_pcw_photo_time_interval, index);
		index = NetbufPopBYTE(data, &param->m_hmw_distance_threshold, index);
		index = NetbufPopBYTE(data, &param->m_hmw_speed_level, index);
		index = NetbufPopBYTE(data, &param->m_hmw_video_record_time, index);
		index = NetbufPopBYTE(data, &param->m_hmw_photo_cnt, index);
		index = NetbufPopBYTE(data, &param->m_hmw_photo_time_interval, index);
		index = NetbufPopBYTE(data, &param->m_lmi_photo_cnt, index);
		index = NetbufPopBYTE(data, &param->m_lmi_photo_time_interval, index);

		if (syncFlag)
		{
			gAdasAlgoParams->alarm_speed = param->m_speed_alert_threshold;
			
			gAdasAlgoParams->fcw_photo_count = param->m_fcw_photo_cnt;
			gAdasAlgoParams->fcw_photo_intervel_ms = param->m_fcw_photo_time_interval*100;
			gAdasAlgoParams->fcw_video_time = param->m_fcw_video_record_time;
			gAdasAlgoParams->fcw_speed_level2 = param->m_fcw_speed_level;
			
			gAdasAlgoParams->ldw_photo_count = param->m_ldw_photo_cnt;
			gAdasAlgoParams->ldw_photo_intervel_ms = param->m_ldw_photo_time_interval*100;
			gAdasAlgoParams->ldw_video_time = param->m_ldw_video_record_time;
			gAdasAlgoParams->ldw_speed_level2 = param->m_ldw_speed_level;
			
			gAdasAlgoParams->hmw_photo_count = param->m_hmw_photo_cnt;
			gAdasAlgoParams->hmw_photo_intervel_ms = param->m_hmw_photo_time_interval*100;
			gAdasAlgoParams->hmw_video_time = param->m_hmw_video_record_time;
			gAdasAlgoParams->hmw_speed_level2 = param->m_hmw_speed_level;
			
			gAdasAlgoParams->pcw_photo_count = param->m_pcw_photo_cnt;
			gAdasAlgoParams->pcw_photo_intervel_ms = param->m_pcw_photo_time_interval*100;
			gAdasAlgoParams->pcw_video_time = param->m_pcw_video_record_time;
			gAdasAlgoParams->pcw_speed_level2 = param->m_pcw_speed_level;

			gAdasAlgoParams->fcw_en = (param->m_alert_enables>>6) & 0x03;
			gAdasAlgoParams->ldw_en = (param->m_alert_enables>>4) & 0x03;
			gAdasAlgoParams->hmw_en = (param->m_alert_enables>>10) & 0x03;
			gAdasAlgoParams->pcw_en = (param->m_alert_enables>>8) & 0x03;
		}
	}
	else if (0xF365 == id)
	{
		jt808_setting_info_dsm_t* param = &pActivesafety->dmsParam;
		
		index = NetbufPopBYTE(data, &param->m_speed_alert_threshold, index);
		index = NetbufPopBYTE(data, &param->m_alert_volume, index);
		index = NetbufPopBYTE(data, &param->m_active_photo, index);
		index = NetbufPopWORD(data, &param->m_active_photo_timer_interval, index);
		index = NetbufPopWORD(data, &param->m_active_photo_dist_interval, index);
		index = NetbufPopBYTE(data, &param->m_active_photo_cnt, index);
		index = NetbufPopBYTE(data, &param->m_active_photo_time_interval, index);
		index = NetbufPopBYTE(data, &param->m_photo_resolution, index);
		index = NetbufPopBYTE(data, &param->m_video_resolution, index);
		index = NetbufPopDWORD(data, &param->m_alert_enables, index);
		index = NetbufPopDWORD(data, &param->m_event_enables, index);
		index = NetbufPopWORD(data, &param->m_smok_check_time_interval, index);
		index = NetbufPopWORD(data, &param->m_call_check_time_interval, index);
		index = NetbufPopBuffer(data, param->m_reserved1, index, 3);
		index = NetbufPopBYTE(data, &param->m_fati_speed_alert_threshold, index);
		index = NetbufPopBYTE(data, &param->m_fati_video_record_time, index);
		index = NetbufPopBYTE(data, &param->m_fati_photo_cnt, index);
		index = NetbufPopBYTE(data, &param->m_fati_photo_time_interval, index);
		index = NetbufPopBYTE(data, &param->m_call_speed_alert_threshold, index);
		index = NetbufPopBYTE(data, &param->m_call_video_record_time, index);
		index = NetbufPopBYTE(data, &param->m_call_photo_cnt, index);
		index = NetbufPopBYTE(data, &param->m_call_photo_time_interval, index);
		index = NetbufPopBYTE(data, &param->m_smok_speed_alert_threshold, index);
		index = NetbufPopBYTE(data, &param->m_smok_video_record_time, index);
		index = NetbufPopBYTE(data, &param->m_smok_photo_cnt, index);
		index = NetbufPopBYTE(data, &param->m_smok_photo_time_interval, index);
		index = NetbufPopBYTE(data, &param->m_dist_speed_alert_threshold, index);
		index = NetbufPopBYTE(data, &param->m_dist_video_record_time, index);
		index = NetbufPopBYTE(data, &param->m_dist_photo_cnt, index);
		index = NetbufPopBYTE(data, &param->m_dist_photo_time_interval, index);
		index = NetbufPopBYTE(data, &param->m_abnm_speed_alert_threshold, index);
		index = NetbufPopBYTE(data, &param->m_abnm_video_time, index);
		index = NetbufPopBYTE(data, &param->m_abnm_photo_cnt, index);
		index = NetbufPopBYTE(data, &param->m_abnm_photo_time_interval, index);
		index = NetbufPopBYTE(data, &param->m_driver_iden_trigger, index);
		
		if (syncFlag)
		{
			gDmsAlgoParams->alarm_speed = param->m_speed_alert_threshold;
			
			gDmsAlgoParams->call_photo_count = param->m_call_photo_cnt;
			gDmsAlgoParams->call_photo_intervel_ms = param->m_call_photo_time_interval*100;
			gDmsAlgoParams->call_video_time = param->m_call_video_record_time;
			gDmsAlgoParams->call_intervel = param->m_call_check_time_interval;
			gDmsAlgoParams->call_speed_level2 = param->m_call_speed_alert_threshold;
			
			gDmsAlgoParams->smok_photo_count = param->m_smok_photo_cnt;
			gDmsAlgoParams->smok_photo_intervel_ms = param->m_smok_photo_time_interval*100;
			gDmsAlgoParams->smok_video_time = param->m_smok_video_record_time;
			gDmsAlgoParams->smok_intervel = param->m_smok_check_time_interval;
			gDmsAlgoParams->smok_speed_level2 = param->m_smok_speed_alert_threshold;
			
			gDmsAlgoParams->fati_photo_count = param->m_fati_photo_cnt;
			gDmsAlgoParams->fati_photo_intervel_ms = param->m_fati_photo_time_interval*100;
			gDmsAlgoParams->fati_video_time = param->m_fati_video_record_time;
			gDmsAlgoParams->fati_speed_level2 = param->m_fati_speed_alert_threshold;
			
			gDmsAlgoParams->dist_photo_count = param->m_dist_photo_cnt;
			gDmsAlgoParams->dist_photo_intervel_ms = param->m_dist_photo_time_interval*100;
			gDmsAlgoParams->dist_video_time = param->m_dist_video_record_time;
			gDmsAlgoParams->dist_speed_level2 = param->m_dist_speed_alert_threshold;
			
			gDmsAlgoParams->nodriver_photo_count = param->m_abnm_photo_cnt;
			gDmsAlgoParams->nodriver_photo_intervel_ms = param->m_abnm_photo_time_interval*100;
			gDmsAlgoParams->nodriver_video_time = param->m_abnm_video_time;
			gDmsAlgoParams->abnm_speed_level2 = param->m_abnm_speed_alert_threshold;
			
			gDmsAlgoParams->call_enable = (param->m_alert_enables>>2) & 0x03;
			gDmsAlgoParams->smok_enable = (param->m_alert_enables>>4) & 0x03;
			gDmsAlgoParams->fati_enable = (param->m_alert_enables) & 0x03;
			gDmsAlgoParams->dist_enable = (param->m_alert_enables>>6) & 0x03;
			gDmsAlgoParams->nodriver_enable = (param->m_alert_enables>>8) & 0x03;
		}
	}
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

int JT808ActiveSafety_ProcAlarm(int type, int extra, int extra2, float ttc, int speed)
{
	unsigned int* plast_time = NULL, *plast_voice_time = NULL;
	unsigned int intervel_time = 0, photo_intervel = 0, camChn = 0, last_timeval = 0;
	unsigned int intervel_voice_time = 0, last_voice_timeval = 0;
	unsigned char speed_shreshod = 0, photo_cnt = 0, video_before = 0, photo_res = 0;
	unsigned char video_after = 0, speed_level2 = 0, alert_type = 0, enable_flag = 0;
	char audioPath[128] = { 0 };

	int ret = 0;

	JT808Controller_t* controller = (JT808Controller_t*)&gJT808Controller;

	const int resolutions[] =
	{
		1280,720,
		352, 288,
		704, 288,
		704, 576,
		640, 480,
		1280,720,
		1920,1080,
	};

	if (NULL == pActivesafety)
	{
		PNT_LOGE("pActivesafety not inited.");
		return ret;
	}
	
	switch (type)
	{
		case LB_WARNING_DMS_DRIVER_LEAVE:
			plast_time = &(pActivesafety->lasttime_abnomal);
			intervel_time = gDmsAlgoParams->nodriver_intervel;
			plast_voice_time = &(pActivesafety->lasttime_voice_abnomal);
			intervel_voice_time = gDmsAlgoParams->nodriver_voice_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/weijiancedaojiashiyuan.pcm");
			photo_cnt = gDmsAlgoParams->nodriver_photo_count;
			photo_intervel = gDmsAlgoParams->nodriver_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->nodriver_video_time;
			speed_level2 = gDmsAlgoParams->abnm_speed_level2;
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALERT_ABNOM;
			enable_flag = gDmsAlgoParams->nodriver_enable;
			photo_res = pActivesafety->dmsParam.m_photo_resolution;
			break;
		case LB_WARNING_DMS_CALL:
			plast_time = &(pActivesafety->lasttime_call);
			intervel_time = gDmsAlgoParams->call_intervel;
			plast_voice_time = &(pActivesafety->lasttime_voice_call);
			intervel_voice_time = gDmsAlgoParams->call_voice_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingwudadianhua.pcm");
			photo_cnt = gDmsAlgoParams->call_photo_count;
			photo_intervel = gDmsAlgoParams->call_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->call_video_time;
			speed_level2 = gDmsAlgoParams->call_speed_level2;
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALERT_CALL;
			enable_flag = gDmsAlgoParams->call_enable;
			photo_res = pActivesafety->dmsParam.m_photo_resolution;
			break;
		case LB_WARNING_DMS_DRINK:
			plast_time = &(pActivesafety->lasttime_dist);
			intervel_time = gDmsAlgoParams->dist_intervel;
			plast_voice_time = &(pActivesafety->lasttime_voice_dist);
			intervel_voice_time = gDmsAlgoParams->dist_voice_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingwuheshui.pcm");
			photo_cnt = gDmsAlgoParams->dist_photo_count;
			photo_intervel = gDmsAlgoParams->dist_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->dist_video_time;
			speed_level2 = gDmsAlgoParams->dist_speed_level2;
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALERT_DIST;
			enable_flag = gDmsAlgoParams->dist_enable;
			photo_res = pActivesafety->dmsParam.m_photo_resolution;
			break;
		case LB_WARNING_DMS_SMOKE:
			plast_time = &(pActivesafety->lasttime_smok);
			intervel_time = gDmsAlgoParams->smok_intervel;
			plast_voice_time = &(pActivesafety->lasttime_voice_smok);
			intervel_voice_time = gDmsAlgoParams->smok_voice_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingwuxiyan.pcm");
			photo_cnt = gDmsAlgoParams->smok_photo_count;
			photo_intervel = gDmsAlgoParams->smok_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->smok_video_time;
			speed_level2 = gDmsAlgoParams->smok_speed_level2;
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALERT_SMOK;
			enable_flag = gDmsAlgoParams->smok_enable;
			photo_res = pActivesafety->dmsParam.m_photo_resolution;
			break;
		case LB_WARNING_DMS_ATTATION:
			plast_time = &(pActivesafety->lasttime_dist);
			intervel_time = gDmsAlgoParams->dist_intervel;
			plast_voice_time = &(pActivesafety->lasttime_voice_dist);
			intervel_voice_time = gDmsAlgoParams->dist_voice_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingzhuanxinjiashi.pcm");
			photo_cnt = gDmsAlgoParams->dist_photo_count;
			photo_intervel = gDmsAlgoParams->dist_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->dist_video_time;
			speed_level2 = gDmsAlgoParams->dist_speed_level2;
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALERT_DIST;
			enable_flag = gDmsAlgoParams->dist_enable;
			photo_res = pActivesafety->dmsParam.m_photo_resolution;
			break;
		case LB_WARNING_DMS_REST:
			plast_time = &(pActivesafety->lasttime_fati);
			intervel_time = gDmsAlgoParams->fati_intervel;
			plast_voice_time = &(pActivesafety->lasttime_voice_fati);
			intervel_voice_time = gDmsAlgoParams->fati_voice_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingzhuyixiuxi.pcm");
			photo_cnt = gDmsAlgoParams->fati_photo_count;
			photo_intervel = gDmsAlgoParams->fati_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->fati_video_time;
			speed_level2 = gDmsAlgoParams->fati_speed_level2;
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALERT_FATI;
			enable_flag = gDmsAlgoParams->fati_enable;
			photo_res = pActivesafety->dmsParam.m_photo_resolution;
			break;
		case LB_WARNING_DMS_CAMERA_COVER:
			plast_time = &(pActivesafety->lasttime_abnomal);
			intervel_time = gDmsAlgoParams->cover_intervel;
			plast_voice_time = &(pActivesafety->lasttime_voice_cover);
			intervel_voice_time = gDmsAlgoParams->cover_voice_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingwuzhedangshexiangtou.pcm");
			photo_cnt = gDmsAlgoParams->cover_photo_count;
			photo_intervel = gDmsAlgoParams->cover_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->cover_video_time;
			speed_level2 = gDmsAlgoParams->abnm_speed_level2;
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALERT_ABNOM;
			enable_flag = gDmsAlgoParams->cover_enable;
			photo_res = pActivesafety->dmsParam.m_photo_resolution;
			break;
		case LB_WARNING_DMS_INFRARED_BLOCK:
			plast_time = &(pActivesafety->lasttime_abnomal);
			intervel_time = gDmsAlgoParams->glass_intervel;
			plast_voice_time = &(pActivesafety->lasttime_voice_glass);
			intervel_voice_time = gDmsAlgoParams->glass_voice_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			strcpy(audioPath, "dms_warning/qingwupeidaifanhongwaiyanjing.pcm");
			photo_cnt = gDmsAlgoParams->glass_photo_count;
			photo_intervel = gDmsAlgoParams->glass_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->glass_video_time;
			speed_level2 = gDmsAlgoParams->abnm_speed_level2;
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALERT_ABNOM;
			enable_flag = gDmsAlgoParams->glass_enable;
			photo_res = pActivesafety->dmsParam.m_photo_resolution;
			break;
		case LB_WARNING_DMS_NO_BELT:
			plast_time = &(pActivesafety->lasttime_nobelt);
			intervel_time = gDmsAlgoParams->nodriver_intervel;
			plast_voice_time = &(pActivesafety->lasttime_voice_nobelt);
			intervel_voice_time = gDmsAlgoParams->nobelt_voice_intervel;
			speed_shreshod = gDmsAlgoParams->alarm_speed;
			speed_level2 = gDmsAlgoParams->abnm_speed_level2;
			photo_cnt = 3;
			video_after = video_before = 5;
			enable_flag = 3;
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALERT_SEATBELT;
			strcpy(audioPath, "dms_warning/qingxihaoanquandai.pcm");
			photo_res = pActivesafety->dmsParam.m_photo_resolution;
			break;
		case LB_WARNING_DMS_ACTIVE_PHOTO:
			plast_time = &(pActivesafety->lasttime_dactive);
			intervel_time = extra2;
			photo_intervel = 5;
			speed_shreshod = 0;
			speed_level2 = 50;
			photo_cnt = extra;
			video_after = video_before = 0;
			enable_flag = 3;
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALERT_ACTIVE_PHOTO;
			photo_res = pActivesafety->dmsParam.m_photo_resolution;
			break;

		case LB_WARNING_DMS_DRIVER_CHANGE:
			plast_time = &(pActivesafety->lasttime_driverchg);
			intervel_time = gDmsAlgoParams->driverchg_intervel;
			speed_shreshod = 0;//gDmsAlgoParams->alarm_speed;
			photo_cnt = gDmsAlgoParams->driverchg_photo_count;
			photo_intervel = gDmsAlgoParams->driverchg_photo_intervel_ms;
			video_after = video_before = gDmsAlgoParams->driverchg_video_time;
			speed_level2 = 50;
			camChn = DMS_CAM_CHANNEL;
			alert_type = DMS_ALERT_DRIVERCHANGE;
			enable_flag = gDmsAlgoParams->driverchg_enable;
			photo_res = pActivesafety->dmsParam.m_photo_resolution;
			break;
            
		case LB_WARNING_ADAS_LAUNCH:
			playAudio("adas_warning/alarm_launch.pcm");
			break;
		case LB_WARNING_ADAS_DISTANCE:
			plast_time = &(pActivesafety->lasttime_hmw);
			intervel_time = gAdasAlgoParams->hmw_intervel;
			plast_voice_time = &(pActivesafety->lasttime_voice_hmw);
			intervel_voice_time = gAdasAlgoParams->hmw_voice_intervel;
			speed_shreshod = gAdasAlgoParams->alarm_speed;
			strcpy(audioPath, "adas_warning/alarm_distance.pcm");
			photo_cnt =gAdasAlgoParams->hmw_photo_count;
			photo_intervel = gAdasAlgoParams->hmw_photo_intervel_ms;
			video_after = video_before = gAdasAlgoParams->hmw_video_time;
			speed_level2 = gAdasAlgoParams->hmw_speed_level2;
			camChn = ADAS_CAM_CHANNEL;
			alert_type = ADAS_ALERT_HMW;
			enable_flag = gAdasAlgoParams->hmw_en;
			photo_res = pActivesafety->adasParam.m_photo_resolution;
			break;
		case LB_WARNING_ADAS_COLLIDE:
			plast_time = &(pActivesafety->lasttime_fcw);
			intervel_time = gAdasAlgoParams->fcw_intervel;
			plast_voice_time = &(pActivesafety->lasttime_voice_fcw);
			intervel_voice_time = gAdasAlgoParams->fcw_voice_intervel;
			speed_shreshod = gAdasAlgoParams->alarm_speed;
			strcpy(audioPath, "adas_warning/alarm_collide.pcm");
			photo_cnt =gAdasAlgoParams->fcw_photo_count;
			photo_intervel = gAdasAlgoParams->fcw_photo_intervel_ms;
			video_after = video_before = gAdasAlgoParams->fcw_video_time;
			speed_level2 = gAdasAlgoParams->fcw_speed_level2;
			camChn = ADAS_CAM_CHANNEL;
			alert_type = ADAS_ALERT_FCW;
			enable_flag = gAdasAlgoParams->fcw_en;
			photo_res = pActivesafety->adasParam.m_photo_resolution;
			break;
		case LB_WARNING_ADAS_PRESSURE:
			plast_time = &(pActivesafety->lasttime_ldw);
			intervel_time = gAdasAlgoParams->ldw_intervel;
			plast_voice_time = &(pActivesafety->lasttime_voice_ldw);
			intervel_voice_time = gAdasAlgoParams->ldw_voice_intervel;
			speed_shreshod = gAdasAlgoParams->alarm_speed;
			strcpy(audioPath, "adas_warning/alarm_pressure.pcm");
			photo_cnt =gAdasAlgoParams->ldw_photo_count;
			photo_intervel = gAdasAlgoParams->ldw_photo_intervel_ms;
			video_after = video_before = gAdasAlgoParams->ldw_video_time;
			speed_level2 = gAdasAlgoParams->ldw_speed_level2;
			camChn = ADAS_CAM_CHANNEL;
			alert_type = ADAS_ALERT_LDW;
			enable_flag = gAdasAlgoParams->ldw_en;
			photo_res = pActivesafety->adasParam.m_photo_resolution;
			break;
		case LB_WARNING_ADAS_PEDS:
			plast_time = &(pActivesafety->lasttime_pcw);
			intervel_time = gAdasAlgoParams->pcw_intervel;
			plast_voice_time = &(pActivesafety->lasttime_voice_pcw);
			intervel_voice_time = gAdasAlgoParams->pcw_voice_intervel;
			speed_shreshod = gAdasAlgoParams->alarm_speed;
			strcpy(audioPath, "adas_warning/xiaoxinxingren.pcm");
			photo_cnt =gAdasAlgoParams->pcw_photo_count;
			photo_intervel = gAdasAlgoParams->pcw_photo_intervel_ms;
			video_after = video_before = gAdasAlgoParams->pcw_video_time;
			speed_level2 = gAdasAlgoParams->pcw_speed_level2;
			camChn = ADAS_CAM_CHANNEL;
			alert_type = ADAS_ALERT_PCW;
			enable_flag = gAdasAlgoParams->pcw_en;
			photo_res = pActivesafety->adasParam.m_photo_resolution;
			break;
		case LB_WARNING_ADAS_CHANGLANE:
			//playAudio("adas_warning/cheliangbiandao.pcm");
			break;
		case LB_WARNING_ADAS_ACTIVE_PHOTO:
			plast_time = &(pActivesafety->lasttime_aactive);
			intervel_time = extra2;
			speed_shreshod = 0;
			speed_level2 = 50;
			photo_cnt = extra;
			video_after = video_before = 0;
			enable_flag = 3;
			camChn = DMS_CAM_CHANNEL;
			alert_type = ADAS_ALERT_ACTIVE_PHOTO;
			photo_res = pActivesafety->adasParam.m_photo_resolution;
			break;

		default:
			break;
	}

	if (NULL == plast_time)
	{
		return ret;
	}

	if (speed < speed_shreshod)
	{
		PNT_LOGE("speed low %d < %d, ignore alarm [%d].", speed, speed_shreshod, type);
		return ret;
	}

	if (speed < speed_level2 && !(enable_flag & 0x01))
	{
		PNT_LOGE("level1 disabled, ignore alarm [%d].", type);
		return ret;
	}

	if (speed >= speed_level2 && !(enable_flag & 0x02))
	{
		PNT_LOGE("level2 disabled, ignore alarm [%d].", type);
		return ret;
	}

	if (plast_voice_time)
	{
		last_voice_timeval = (*plast_voice_time);
	}
	
	if (0 != intervel_voice_time && currentTimeMillis()/1000 - last_voice_timeval >= intervel_voice_time)
	{
		if (plast_voice_time)
		{
			*plast_voice_time = (currentTimeMillis()/1000);
		}

		playAudio(audioPath);

		extern int KWatchReportAlarm(int level, int type, int extra, int extra2, float ttc);
		KWatchReportAlarm((speed >= speed_level2)?2:1, type, extra, extra2, ttc);
	}
		
	last_timeval = (*plast_time);

	if (currentTimeMillis()/1000 - last_timeval >= intervel_time)
	{
		last_timeval = (currentTimeMillis()/1000);
		
		if (0 != checkTFCard(NULL)  || 1660000000 > currentTimeMillis()/1000) // 未插tf卡或未校�?
		{
			PNT_LOGE("no tfcard or time error, ignore alarm [%d].", type);
		}
		else
		{
			jt808_alert_info_t alertInfo = { 0 };

			char filename[128] = { 0 };

			getLocalBCDTime((char*)alertInfo.m_alert_flag.m_datetime);
			memcpy(alertInfo.m_alert_flag.m_dev_id, controller->mDeviceID, sizeof(alertInfo.m_alert_flag.m_dev_id));
			
			alertInfo.m_alert_flag.m_event_id = 0;
			alertInfo.m_alert_flag.m_resv = ((ADAS_CAM_CHANNEL==camChn)?ACTIVESAFETY_ALARM_ADAS:ACTIVESAFETY_ALARM_DMS) | alert_type;

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

			if (photo_cnt > 0)
			{
				JT808ActiveSafety_GetFilename(&alertInfo.m_alert_flag, filename, FILETYPE_PHOTO);
				
				if (photo_res > ActiveSafePhotoRes_1920x1080)
				{
					photo_res = ActiveSafePhotoRes_1280x720;
				}
				if (RET_SUCCESS == MediaSnap_CreateSnap(camChn, photo_cnt, resolutions[photo_res*2], resolutions[photo_res*2+1], photo_intervel*100, filename))
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

			if (ADAS_CAM_CHANNEL==camChn)
			{
				jsatl_alert_adas_t* adas = &alertInfo.m_type_info.m_adas_event;
				adas->m_alert_type = alert_type;
				adas->m_alert_level = (speed >= speed_level2)?2:1;
				adas->m_departure_type = extra;
				adas->m_front_car_speed = extra2;
				adas->m_front_abs_distance = extra;
				
			}
			else if (DMS_CAM_CHANNEL == camChn)
			{
				jsatl_alert_dsm_t* dms = &alertInfo.m_type_info.m_dsm_event;
				dms->m_alert_type = alert_type;
				dms->m_alert_level = (speed >= speed_level2)?2:1;
				dms->m_fati_degree = extra;
			}

			if (alertInfo.m_alert_flag.m_files_cnt > 0)
			{
				JT808ActiveSafety_AlarmReport(((ADAS_CAM_CHANNEL==camChn)?ACTIVESAFETY_ALARM_ADAS:ACTIVESAFETY_ALARM_DMS), &alertInfo);
			}
		}

		ret = 1;

		*plast_time = last_timeval;
	}
	else
	{
		unsigned int timeresv = intervel_time - ((currentTimeMillis()/1000) - last_timeval);
		PNT_LOGE("time not in intervel scale reserve [%d s] [%d].", timeresv, type);
	}

	return ret;
}

void* JT808ActiveSafetyActivePhoto_Task(void* pArg)
{
	pthread_detach(pthread_self());

	const int resolutions[] =
	{
		1280,720,
		352, 288,
		704, 288,
		704, 576,
		640, 480,
		1280,720,
		1920,1080,
	};
	
	JT808ActivesafetyActivePhoto_t* handle = (JT808ActivesafetyActivePhoto_t*)pArg;
	JT808Controller_t* controller = (JT808Controller_t*)handle->owner;

	unsigned char* p_active_photo = NULL, *p_active_photo_cnt = NULL, *p_active_photos_intervel = NULL;
	unsigned short* p_active_photo_dist_intervel = NULL, *p_active_photo_time_intervel = NULL;
	unsigned char* p_photo_resolution = NULL;

	if (ACTIVESAFETY_ALARM_ADAS == handle->chnType)
	{
		p_active_photo = &controller->mActivesafety.adasParam.m_active_photo;
		p_active_photo_cnt = &controller->mActivesafety.adasParam.m_active_photo_cnt;
		p_active_photos_intervel = &controller->mActivesafety.adasParam.m_active_photo_time_interval;
		p_active_photo_dist_intervel = &controller->mActivesafety.adasParam.m_active_photo_dist_interval;
		p_active_photo_time_intervel = &controller->mActivesafety.adasParam.m_active_photo_timer_interval;
		p_photo_resolution = &controller->mActivesafety.adasParam.m_photo_resolution;
	}
	else if (ACTIVESAFETY_ALARM_DMS == handle->chnType)
	{
		p_active_photo = &controller->mActivesafety.dmsParam.m_active_photo;
		p_active_photo_cnt = &controller->mActivesafety.dmsParam.m_active_photo_cnt;
		p_active_photos_intervel = &controller->mActivesafety.dmsParam.m_active_photo_time_interval;
		p_active_photo_dist_intervel = &controller->mActivesafety.dmsParam.m_active_photo_dist_interval;
		p_active_photo_time_intervel = &controller->mActivesafety.dmsParam.m_active_photo_timer_interval;
		p_photo_resolution = &controller->mActivesafety.dmsParam.m_photo_resolution;
	}
	else
	{
		return NULL;
	}

	unsigned int lastPhotoTime = currentTimeMillis()/1000;
	unsigned int lastPhotoDist = controller->mVehicalDriveDistance;
	unsigned int currentTime = 0, photoFlag = 0;

	while (controller->mStartFlag)
	{
		photoFlag = 0;

		if (*p_active_photo_cnt > 0)
		{
			if (0x01 == *p_active_photo)
			{
				currentTime = currentTimeMillis()/1000;
				if (currentTime - lastPhotoTime > *p_active_photo_time_intervel)
				{
					photoFlag = 1;
					lastPhotoTime = currentTime;
				}
			}
			else if (0x02 == *p_active_photo)
			{
				if (controller->mVehicalDriveDistance - lastPhotoDist > *p_active_photo_dist_intervel)
				{
					photoFlag = 1;
					lastPhotoDist = controller->mVehicalDriveDistance;
				}
			}
			else if (0x03 == *p_active_photo)
			{
				
			}
		}

		if (photoFlag)
		{	
			jt808_alert_info_t alertInfo = { 0 };
			char filename[128] = { 0 };
			getLocalBCDTime((char*)alertInfo.m_alert_flag.m_datetime);
			memcpy(alertInfo.m_alert_flag.m_dev_id, controller->mDeviceID, sizeof(alertInfo.m_alert_flag.m_dev_id));
			
			alertInfo.m_alert_flag.m_event_id = 0;
			alertInfo.m_alert_flag.m_resv = handle->chnType | ((ACTIVESAFETY_ALARM_ADAS == handle->chnType)?ADAS_ALERT_ACTIVE_PHOTO:DMS_ALERT_ACTIVE_PHOTO);
			alertInfo.m_status_flag = 1;
			alertInfo.m_speed = controller->mLocation.speed;
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

			if (*p_photo_resolution > ActiveSafePhotoRes_1920x1080)
			{
				*p_photo_resolution = ActiveSafePhotoRes_1280x720;
			}

			JT808ActiveSafety_GetFilename(&alertInfo.m_alert_flag, filename, FILETYPE_PHOTO);
			if (RET_SUCCESS == MediaSnap_CreateSnap(((ACTIVESAFETY_ALARM_ADAS == handle->chnType)?ADAS_CAM_CHANNEL:DMS_CAM_CHANNEL), *p_active_photo_cnt, 
				resolutions[(*p_photo_resolution)*2], resolutions[(*p_photo_resolution)*2+1], (*p_active_photos_intervel)*100, filename))
			{
				alertInfo.m_alert_flag.m_files_cnt += *p_active_photo_cnt;
			}

			if (ACTIVESAFETY_ALARM_ADAS == handle->chnType)
			{
				jsatl_alert_adas_t* adas = &alertInfo.m_type_info.m_adas_event;
				adas->m_alert_type = ADAS_ALERT_ACTIVE_PHOTO;
				adas->m_alert_level = 1;
			}
			else if (ACTIVESAFETY_ALARM_DMS == handle->chnType)
			{
				jsatl_alert_dsm_t* dms = &alertInfo.m_type_info.m_dsm_event;
				dms->m_alert_type = DMS_ALERT_ACTIVE_PHOTO;
				dms->m_alert_level = 1;
			}

			JT808ActiveSafety_AlarmReport(handle->chnType, &alertInfo);
		}
		
		usleep(100*1000);
	}

	return NULL;
}

char removeFileList[100][32];
time_t removeFileTimeList[100];
void JT808ActiveSafety_DeleteOldestFiles(char* dir, int count, int duty)
{
    struct dirent *dp;
	char filename[512] = { 0 };
	struct stat sta = { 0 };
	unsigned int dirSize = 0;
    DIR *dirp = opendir(dir);
	unsigned int filecount = 0;

	if (NULL == dirp)
	{
		return;
	}

	if (count > 100)
	{
		count = 100;
	}

	memset(removeFileList, 0, 100*32);
	memset(removeFileTimeList, 0, sizeof(removeFileTimeList));
	
	while ((dp = readdir(dirp)) != NULL)
	{
		if(strcmp(dp->d_name,".")==0 || strcmp(dp->d_name,"..")==0)    ///current dir OR parrent dir
        {
        	continue;
        }

		sprintf(filename, "%s/%s", dir, dp->d_name);
		
		memset(&sta, 0, sizeof(struct stat));
		stat(filename, &sta);

		dirSize += sta.st_size;

		if (0 == removeFileTimeList[0] || removeFileTimeList[0] > sta.st_mtime)
		{
			for (int i=count-1; i>=0; i--)
			{
				if (0 == removeFileTimeList[i] || removeFileTimeList[i] > sta.st_mtime)
				{
					if (0 != i && 0 != removeFileTimeList[i])
					{
						memcpy(removeFileList, removeFileList[1], i*32); // 往前移
						memcpy(removeFileTimeList, &removeFileTimeList[1], i*sizeof(time_t));
					}

					removeFileTimeList[i] = sta.st_mtime;
					strcpy(removeFileList[i], dp->d_name);
					break;
				}
			}
		}
		usleep(1000);
		filecount ++;
    }
	
    (void) closedir(dirp);

	SDCardInfo_t info = { 0 };
	storage_info_get(SDCARD_PATH, &info);

	int dirSizeMax = info.total*duty/100;

	if (dirSize/1024 >= dirSizeMax || ((info.total > 0) && (info.free < SDCARD_RESV)))
	{
		for (int i=0; i<count; i++)
		{
			if (0 != removeFileTimeList[i])
			{
				char cmd[128] = { 0 };
				sprintf(cmd, "rm -rf %s/%s &", dir, removeFileList[i]);
				PNT_LOGE(cmd);
				system(cmd);
			}
		}
	}
}

void* JT808ActiveSafety_MediasRecycleTask(void* arg)
{
	pthread_detach(pthread_self());

	JT808Controller_t* controller = (JT808Controller_t*)arg;

    time_t nowtime = time(NULL);
    struct tm p;
    localtime_r(&nowtime,&p);
    time_t oldtime = nowtime;

    while (controller->mStartFlag)
    {
        nowtime = time(NULL);
        localtime_r(&nowtime,&p);

        if((nowtime-oldtime) >= 60)
        {
            oldtime = nowtime;

			JT808ActiveSafety_DeleteOldestFiles(ALARM_PATH, 100, SDCARD_ALARM_DUTY);
        }
		
        usleep(1000*100);
    }

	return NULL;
}

void JT808ActiveSafety_Start(void* p808controller)
{
	JT808Controller_t* controller = (JT808Controller_t*)p808controller;

	pActivesafety = &controller->mActivesafety;
	
	memset(pActivesafety, 0, sizeof(JT808ActiveSafety_t));

	JT808Param_t* item = NULL;
	
	JT808Database_GetJT808Param(&controller->mJT808DB, 0xF365, &item);

	if (NULL != item && NULL != (unsigned char*)item->mParamData)
	{
		PNT_LOGE("update dms alarm param.");
		JT808ActiveSafety_UpdateParamsFromMem(0xF365, (unsigned char*)item->mParamData, 0);

		JT808ActiveSafety_StoreParamsFromMem(0xF365, (unsigned char*)item->mParamData);
	}
	else
	{
		PNT_LOGE("get dms alarm param db failed..");
	}
	
	item = NULL;
	JT808Database_GetJT808Param(&controller->mJT808DB, 0xF364, &item);

	if (NULL != item && NULL != (unsigned char*)item->mParamData)
	{
		PNT_LOGE("update adas alarm param.");
		JT808ActiveSafety_UpdateParamsFromMem(0xF364, (unsigned char*)item->mParamData, 0);
		
		JT808ActiveSafety_StoreParamsFromMem(0xF364, (unsigned char*)item->mParamData);
	}
	else
	{
		PNT_LOGE("get adas alarm param db failed..");
	}

	pthread_t pid;
	pActivesafety->dmsActivePhoto.chnType = ACTIVESAFETY_ALARM_DMS;
	pActivesafety->dmsActivePhoto.owner = p808controller;
	pthread_create(&pid, NULL, JT808ActiveSafetyActivePhoto_Task, &pActivesafety->dmsActivePhoto);
	
	pActivesafety->adasActivePhoto.chnType = ACTIVESAFETY_ALARM_ADAS;
	pActivesafety->adasActivePhoto.owner = p808controller;
	pthread_create(&pid, NULL, JT808ActiveSafetyActivePhoto_Task, &pActivesafety->adasActivePhoto);

	pthread_create(&pid, NULL, JT808ActiveSafety_MediasRecycleTask, p808controller);
}


