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
#include "cJSON.h"
#include "lb_cfg_file.h"
#include "osal.h"

int al_para_init(void);
float adas_get_vanish_point_x();
float adas_get_vanish_point_y();
float adas_get_bottom_line_y();
int adas_save_cali_parm(float vanish_point_x, float vanish_point_y, float bottom_line_y);
float dms_get_cali_yaw();
float dms_get_cali_pitch();
int dms_save_cali_parm(float yaw, float pitch);
int al_para_save(void);
int al_para_exit(void);

#endif
