/*
 *------------------------------------------------------------------------------
 * @File      :    dms_config.c
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

#include "dms_config.h"
#include "pnt_log.h"

DmsAlgoParams_t* gDmsAlgoParams = NULL;

void dms_al_para_init(void)
{
	DmsAlgoParams_t param;

	memset(&param, 0, sizeof(DmsAlgoParams_t));

	param.calib_pitch = 0;
	param.calib_yaw = 0;

	param.call_enable = 3;
	param.call_photo_count = 3;
	param.call_photo_intervel_ms = 200;
	param.call_video_time = 5;
	param.call_intervel = 180;
	param.call_speed_level2 = 50;
	
	param.smok_enable = 3;
	param.smok_photo_count = 3;
	param.smok_photo_intervel_ms = 200;
	param.smok_video_time = 5;
	param.smok_intervel = 180;
	param.smok_speed_level2 = 50;
	
	param.fati_enable = 3;
	param.fati_photo_count = 3;
	param.fati_photo_intervel_ms = 200;
	param.fati_video_time = 5;
	param.fati_intervel = 180;
	param.fati_speed_level2 = 50;
	
	param.dist_enable = 3;
	param.dist_photo_count = 3;
	param.dist_photo_intervel_ms = 200;
	param.dist_video_time = 5;
	param.dist_intervel = 180;
	param.dist_speed_level2 = 50;
	
	param.nodriver_enable = 3;
	param.nodriver_photo_count = 3;
	param.nodriver_photo_intervel_ms = 200;
	param.nodriver_video_time = 5;
	param.nodriver_intervel = 180;
	param.abnm_speed_level2 = 50;
	
	param.glass_enable = 3;
	param.glass_photo_count = 3;
	param.glass_photo_intervel_ms = 200;
	param.glass_video_time = 5;
	param.glass_intervel = 180;
	
	param.cover_enable = 3;
	param.cover_photo_count = 3;
	param.cover_photo_intervel_ms = 200;
	param.cover_video_time = 5;
	param.cover_intervel = 180;
	
	param.driverchg_enable = 3;
	param.driverchg_photo_count = 3;
	param.driverchg_photo_intervel_ms = 200;
	param.driverchg_video_time = 5;
	param.driverchg_intervel = 30;

	param.alarm_speed = 30;

	param.fati_sensitivity = 2;
	param.dist_sensitivity = 2;
	param.call_sensitivity = 2;
	param.smok_sensitivity = 2;
	param.belt_sensitivity = 2;

	param.fati_speed = 30;
	param.dist_speed = 30;
	param.call_speed = 30;
	param.smok_speed = 30;
	param.belt_speed = 30;
	
#if (defined INDIA)
	param.driver_rect_xs = 0;
	param.driver_rect_ys = 0;
	param.driver_rect_xe = 1280/2;
	param.driver_rect_ye = 720;
	
	param.passenger_enable = 0;
	param.passenger_photo_cnt = 1;
	
	param.call_voice_intervel = 30;
	param.smok_voice_intervel = 30;
	param.dist_voice_intervel = 30;
	param.fati_voice_intervel = 30;
	param.nobelt_voice_intervel = 30;
	
	param.nodriver_voice_intervel = 60;
	param.glass_voice_intervel = 60;
	param.cover_voice_intervel = 60;
#else
	param.driver_rect_xs = 0;
	param.driver_rect_ys = 0;
	param.driver_rect_xe = 1280;
	param.driver_rect_ye = 720;
	
	param.passenger_enable = 0;
	param.passenger_photo_cnt = 1;
	
	param.call_voice_intervel = param.call_intervel;
	param.smok_voice_intervel = param.smok_intervel;
	param.dist_voice_intervel = param.dist_intervel;
	param.fati_voice_intervel = param.fati_intervel;
	param.nobelt_voice_intervel = param.nodriver_intervel;
	param.nodriver_voice_intervel = param.nodriver_intervel;
	param.glass_voice_intervel = param.glass_intervel;
	param.cover_voice_intervel = param.cover_intervel;
#endif

#if (defined LOSANGLE)
	param.glass_enable = 0;
	param.belt_sensitivity = 0;
#endif

	param.active_rect_xs = 0;
	param.active_rect_ys = 0;
	param.active_rect_xe = 1280/2;
	param.active_rect_ye = 720;

	param.otherarea_cover = 0;

	param.driver_recgn = 0;
	param.facerecg_match_score = 0.55;

	param.resetDefault = 1;

	int fd = open(DMS_ALGO_PARAMS_FILE, O_RDWR | O_CREAT);
	if (fd < 0)
	{
		PNT_LOGE("create %s failed.\n", DMS_ALGO_PARAMS_FILE);
		return;
	}
	
	int size = lseek(fd, 0, SEEK_END);

	if (size != sizeof(DmsAlgoParams_t))
	{
		PNT_LOGE("%s resize %d to %d", DMS_ALGO_PARAMS_FILE, size, sizeof(DmsAlgoParams_t));

		if (0 < size)
		{
			lseek(fd, 0, SEEK_SET);
			read(fd, &param, size);
		}
		
#if (defined LOSANGLE)
		param.glass_enable = 0;
		param.belt_sensitivity = 0;
#endif

		if (size < sizeof(DmsAlgoParams_t))
		{
			ftruncate(fd, sizeof(DmsAlgoParams_t)-size);
		}
		else
		{
			close(fd);
			remove(DMS_ALGO_PARAMS_FILE);
			fd = open(DMS_ALGO_PARAMS_FILE, O_RDWR | O_CREAT);
		}
		lseek(fd, 0, SEEK_SET);
		write(fd, &param, sizeof(DmsAlgoParams_t));
	}

	gDmsAlgoParams = mmap(NULL, sizeof(DmsAlgoParams_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (MAP_FAILED == gDmsAlgoParams)
	{
		PNT_LOGE("mmap file %s failed.\n", DMS_ALGO_PARAMS_FILE);
		close(fd);
		return;
	}

	if (param.resetDefault != gDmsAlgoParams->resetDefault)
	{
		PNT_LOGE("reset to default parameters");
		memcpy(gDmsAlgoParams, &param, sizeof(param));
	}
	
	close(fd);
}

void dms_al_para_deinit(void)
{
	if (gDmsAlgoParams)
	{
		munmap(gDmsAlgoParams, sizeof(DmsAlgoParams_t));
	}
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

