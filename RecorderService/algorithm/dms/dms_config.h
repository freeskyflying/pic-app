/*
* dms_config.h - dms config para.
*
* Copyright (C) 2016-2018, LomboTech Co.Ltd.
* Author: lomboswer <lomboswer@lombotech.com>
*
* Common code for LomboTech Socs
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*/

#ifndef _DMS_CONFIG_H_
#define _DMS_CONFIG_H_
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define DMS_ALGO_PARAMS_FILE			"/data/etc/DmsAlgoParam.bin"

typedef struct
{

	unsigned short calib_yaw;
	unsigned short calib_pitch;

	unsigned char call_enable;
	unsigned char call_photo_count;
	unsigned char call_video_time;
	unsigned char call_speed_level2;
	unsigned short call_photo_intervel_ms;
	unsigned short call_intervel;

	unsigned char smok_enable;
	unsigned char smok_photo_count;
	unsigned char smok_video_time;
	unsigned char smok_speed_level2;
	unsigned short smok_photo_intervel_ms;
	unsigned short smok_intervel;

	unsigned char dist_enable;
	unsigned char dist_photo_count;
	unsigned char dist_video_time;
	unsigned char dist_speed_level2;
	unsigned short dist_photo_intervel_ms;
	unsigned short dist_intervel;

	unsigned char fati_enable;
	unsigned char fati_photo_count;
	unsigned char fati_video_time;
	unsigned char fati_speed_level2;
	unsigned short fati_photo_intervel_ms;
	unsigned short fati_intervel;

	unsigned char nodriver_enable;
	unsigned char abnm_speed_level2;
	unsigned char nodriver_photo_count;
	unsigned char nodriver_video_time;
	unsigned short nodriver_photo_intervel_ms;
	unsigned short nodriver_intervel;

	unsigned char glass_enable;
	unsigned char glass_photo_count;
	unsigned char glass_video_time;
	unsigned char resv1;
	unsigned short glass_photo_intervel_ms;
	unsigned short glass_intervel;

	unsigned char cover_enable;
	unsigned char cover_photo_count;
	unsigned char cover_video_time;
	unsigned char resv2;
	unsigned short cover_photo_intervel_ms;
	unsigned short cover_intervel;
	
	unsigned char alarm_speed;

	unsigned char no_belt_en;
	
	unsigned char fati_sensitivity; // 0-off 1-low 2-mid 3-high
	unsigned char dist_sensitivity;
	unsigned char smok_sensitivity;
	unsigned char call_sensitivity;
	unsigned char belt_sensitivity;
	
	unsigned char fati_speed;
	unsigned char dist_speed;
	unsigned char smok_speed;
	unsigned char call_speed;
	unsigned char belt_speed;

	unsigned short driver_rect_xs;
	unsigned short driver_rect_ys;
	unsigned short driver_rect_xe;
	unsigned short driver_rect_ye;

	unsigned int passenger_enable;
	unsigned char passenger_photo_cnt;

	unsigned short call_voice_intervel;
	unsigned short smok_voice_intervel;
	unsigned short dist_voice_intervel;
	unsigned short fati_voice_intervel;
	unsigned short nodriver_voice_intervel;
	unsigned short glass_voice_intervel;
	unsigned short cover_voice_intervel;
	unsigned short nobelt_voice_intervel;

	unsigned short active_rect_xs;
	unsigned short active_rect_ys;
	unsigned short active_rect_xe;
	unsigned short active_rect_ye;
	unsigned int otherarea_cover;

	float facerecg_match_score;
	unsigned char driver_recgn;

	unsigned char driverchg_enable;
	unsigned char driverchg_photo_count;
	unsigned char driverchg_video_time;
	unsigned short driverchg_photo_intervel_ms;
	unsigned short driverchg_intervel;

	unsigned int resetDefault;

} DmsAlgoParams_t;

#ifdef __cplusplus
extern "C" {
#endif

void dms_al_para_init(void);
void dms_al_para_deinit(void);

extern DmsAlgoParams_t* gDmsAlgoParams;

#ifdef __cplusplus
}
#endif

#endif
