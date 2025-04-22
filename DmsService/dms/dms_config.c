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

	param.call_enable = 1;
	param.call_photo_count = 3;
	param.call_photo_intervel_ms = 200;
	param.call_video_time = 5;
	param.call_intervel = 180;
	
	param.smok_enable = 1;
	param.smok_photo_count = 3;
	param.smok_photo_intervel_ms = 200;
	param.smok_video_time = 5;
	param.smok_intervel = 180;
	
	param.fati_enable = 1;
	param.fati_photo_count = 3;
	param.fati_photo_intervel_ms = 200;
	param.fati_video_time = 5;
	param.fati_intervel = 180;
	
	param.dist_enable = 1;
	param.dist_photo_count = 3;
	param.dist_photo_intervel_ms = 200;
	param.dist_video_time = 5;
	param.dist_intervel = 180;
	
	param.nodriver_enable = 1;
	param.nodriver_photo_count = 3;
	param.nodriver_photo_intervel_ms = 200;
	param.nodriver_video_time = 5;
	param.nodriver_intervel = 180;
	
	param.glass_enable = 1;
	param.glass_photo_count = 3;
	param.glass_photo_intervel_ms = 200;
	param.glass_video_time = 5;
	param.glass_intervel = 180;
	
	param.cover_enable = 1;
	param.cover_photo_count = 3;
	param.cover_photo_intervel_ms = 200;
	param.cover_video_time = 5;
	param.cover_intervel = 180;

	param.alarm_speed = 0;

	int fd = open(DMS_ALGO_PARAMS_FILE, O_RDWR | O_CREAT);
	if (fd < 0)
	{
		PNT_LOGE("create %s failed.\n", DMS_ALGO_PARAMS_FILE);
		return;
	}
	
	int size = lseek(fd, 0, SEEK_END);

	if (size != sizeof(DmsAlgoParams_t))
	{
		PNT_LOGE("%s resize %d", DMS_ALGO_PARAMS_FILE, size);
		
		if (0 < size)
		{
			lseek(fd, 0, SEEK_SET);
			read(fd, &param, size);
		}
		ftruncate(fd, sizeof(DmsAlgoParams_t)-size);
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

