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
	unsigned char smok_enable;
	unsigned char dist_enable;
	unsigned char fati_enable;
	unsigned char nodriver_enable;
	unsigned char glass_enable;
	unsigned char cover_enable;

	unsigned char call_photo_count;
	unsigned char call_video_time;
	unsigned short call_photo_intervel_ms;
	unsigned short call_intervel;

	unsigned char smok_photo_count;
	unsigned char smok_video_time;
	unsigned short smok_photo_intervel_ms;
	unsigned short smok_intervel;

	unsigned char dist_photo_count;
	unsigned char dist_video_time;
	unsigned short dist_photo_intervel_ms;
	unsigned short dist_intervel;

	unsigned char fati_photo_count;
	unsigned char fati_video_time;
	unsigned short fati_photo_intervel_ms;
	unsigned short fati_intervel;

	unsigned char nodriver_photo_count;
	unsigned char nodriver_video_time;
	unsigned short nodriver_photo_intervel_ms;
	unsigned short nodriver_intervel;

	unsigned char glass_photo_count;
	unsigned char glass_video_time;
	unsigned short glass_photo_intervel_ms;
	unsigned short glass_intervel;

	unsigned char cover_photo_count;
	unsigned char cover_video_time;
	unsigned short cover_photo_intervel_ms;
	unsigned short cover_intervel;
	
	unsigned char alarm_speed;

} DmsAlgoParams_t;

void dms_al_para_init(void);

extern DmsAlgoParams_t* gDmsAlgoParams;

#endif
