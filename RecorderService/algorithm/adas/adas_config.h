/*
* adas_config.h - adas config para.
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

#ifndef _ADAS_CONFIG_H_
#define _ADAS_CONFIG_H_
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define ADAS_ALGO_PARAMS_FILE			"/data/etc/AdasAlgoParam.bin"

typedef struct
{

	unsigned short camera_to_ground;
	unsigned short car_head_to_camera;
	unsigned short car_width;
	short camera_to_middle;
	unsigned char auto_calib_en;
	unsigned char calib_finish;

	unsigned char hmw_speed;
	unsigned char fcw_speed;
	unsigned char pcw_speed;
	unsigned char ldw_speed;

	unsigned short vanish_point_x;
	unsigned short vanish_point_y;
	unsigned short bottom_line_y;
	
	unsigned char ldw_en;
	unsigned char ldw_photo_count;
	unsigned char ldw_video_time;
	unsigned char ldw_speed_level2;
	unsigned short ldw_photo_intervel_ms;
	unsigned short ldw_intervel;
	
	unsigned char hmw_en;
	unsigned char hmw_photo_count;
	unsigned char hmw_video_time;
	unsigned char hmw_speed_level2;
	unsigned short hmw_photo_intervel_ms;
	unsigned short hmw_intervel;
	
	unsigned char fcw_en;
	unsigned char fcw_photo_count;
	unsigned char fcw_video_time;
	unsigned char fcw_speed_level2;
	unsigned short fcw_photo_intervel_ms;
	unsigned short fcw_intervel;
	
	unsigned char pcw_en;
	unsigned char pcw_photo_count;
	unsigned char pcw_video_time;
	unsigned char pcw_speed_level2;
	unsigned short pcw_photo_intervel_ms;
	unsigned short pcw_intervel;
	
	unsigned char fmw_en;
	unsigned char fmw_photo_count;
	unsigned char fmw_video_time;
	unsigned char fmw_speed_level2;
	unsigned short fmw_photo_intervel_ms;
	unsigned short fmw_intervel;
	
	unsigned char alarm_speed;
	
	unsigned char ldw_sensitivity; // 0-off 1-low 2-mid 3-high
	unsigned char hmw_sensitivity;
	unsigned char fcw_sensitivity;
	unsigned char pcw_sensitivity;
	unsigned char fmw_sensitivity; // Ç°³µÆô¶¯
	
	unsigned char fmw_speed;
	
	unsigned short ldw_voice_intervel;
	unsigned short hmw_voice_intervel;
	unsigned short fcw_voice_intervel;
	unsigned short pcw_voice_intervel;
	unsigned short fmw_voice_intervel;
	
} AdasAlgoParams_t;

#ifdef __cplusplus
extern "C" {
#endif

void adas_al_para_init(void);
void adas_al_para_deinit(void);

#ifdef __cplusplus
}
#endif

extern AdasAlgoParams_t* gAdasAlgoParams;

#endif
