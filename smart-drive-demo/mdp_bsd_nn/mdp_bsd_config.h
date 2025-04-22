/*
* bsd_config.h - bsd config para.
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
float bsd_get_vanish_point_x();
float bsd_get_vanish_point_y();
float bsd_get_bottom_line_y();
int bsd_get_warn_roi_point(unsigned int  point, unsigned int  cordinate);
int bsd_save_warn_roi_point(int warn_area[16][2]);
int bsd_save_cali_parm(float vanish_point_x, float vanish_point_y);
int al_para_save(void);
int al_para_exit(void);

#endif
