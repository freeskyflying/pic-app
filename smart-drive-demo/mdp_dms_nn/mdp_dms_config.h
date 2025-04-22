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
#include "cJSON.h"
#include "lb_cfg_file.h"
#include "osal.h"

int al_para_init(void);
float dms_get_cali_yaw();
float dms_get_cali_pitch();
int dms_save_cali_parm(float yaw, float pitch);
int al_para_save(void);
int al_para_exit(void);

#endif
