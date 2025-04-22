/*
 *------------------------------------------------------------------------------
 * @File      :    adas_config.c
 * @Date      :    2021-9-24
 * @Author    :    lomboswer <lomboswer@lombotech.com>
 * @Brief     :    Source file for MDP(Media Development Platform).
 *
 * Copyright (C) 2020-2021, LomboTech Co.Ltd. All rights reserved.
 *------------------------------------------------------------------------------
 */
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <wait.h>

#include "adas_config.h"
#include "pnt_log.h"

AdasAlgoParams_t* gAdasAlgoParams = NULL;

void adas_al_para_init(void)
{
	AdasAlgoParams_t param;

	memset(&param, 0, sizeof(AdasAlgoParams_t));

	param.camera_to_ground = 120;
	param.car_head_to_camera = 120;
	param.car_width = 150;
	param.camera_to_middle = 0;
	param.auto_calib_en = 1;
	param.calib_finish = 1;

	param.ldw_en = 2;
	param.hmw_en = 2;
	param.fcw_en = 2;
	param.pcw_en = 2;
	param.fmw_en = 2;

	param.ldw_speed = 30;
	param.hmw_speed = 30;
	param.fcw_speed = 30;
	param.pcw_speed = 30;

	param.vanish_point_x = 960;
	param.vanish_point_y = 540;
	param.bottom_line_y = 700;

	int fd = open(ADAS_ALGO_PARAMS_FILE, O_RDWR | O_CREAT);
	if (fd < 0)
	{
		PNT_LOGE("create %s failed.\n", ADAS_ALGO_PARAMS_FILE);
		return;
	}
	
	int size = lseek(fd, 0, SEEK_END);

	if (size != sizeof(AdasAlgoParams_t))
	{
		PNT_LOGE("%s resize %d", ADAS_ALGO_PARAMS_FILE, size);
		
		if (0 < size)
		{
			lseek(fd, 0, SEEK_SET);
			read(fd, &param, size);
		}
		ftruncate(fd, sizeof(AdasAlgoParams_t)-size);
		lseek(fd, 0, SEEK_SET);
		write(fd, &param, sizeof(AdasAlgoParams_t));
	}

	gAdasAlgoParams = mmap(NULL, sizeof(AdasAlgoParams_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (MAP_FAILED == gAdasAlgoParams)
	{
		PNT_LOGE("mmap file %s failed.\n", ADAS_ALGO_PARAMS_FILE);
		close(fd);
		return;
	}
	
	close(fd);
}

void adas_al_para_deinit(void)
{
	if (gAdasAlgoParams)
	{
		munmap(gAdasAlgoParams, sizeof(AdasAlgoParams_t));
	}
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

