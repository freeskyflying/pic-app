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
	unsigned short camera_to_middle;
	unsigned char auto_calib_en;
	unsigned char calib_finish;

	unsigned char ldw_en;
	unsigned char hmw_en;
	unsigned char fcw_en;
	unsigned char pcw_en;
	unsigned char fmw_en;

	unsigned char hmw_speed;
	unsigned char fcw_speed;
	unsigned char pcw_speed;
	unsigned char ldw_speed;

	unsigned short vanish_point_x;
	unsigned short vanish_point_y;
	unsigned short bottom_line_y;
	
} AdasAlgoParams_t;

void adas_al_para_init(void);

extern AdasAlgoParams_t* gAdasAlgoParams;

#endif
