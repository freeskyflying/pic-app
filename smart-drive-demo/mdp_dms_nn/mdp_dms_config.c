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

#include "mdp_dms_config.h"

#define CASE_ROOT_PATH	"/usr/share/out/"
#define SDCARD_MOUNT_PATH "/mnt/card/"
#define CFG_SAVE_PATH "/data/"

typedef struct tag_al_config_obj {
	cJSON *para_root;
	cJSON *car_dms;
	cJSON *yaw;
	cJSON *pitch;
} al_config_obj_t;

static al_config_obj_t al_config_obj;

static int dms_para_array_init(al_config_obj_t *obj, cJSON *para)
{
	int ret = 0;
	int i = 0;
	int js_size = 0;

	cJSON  *temp0 = NULL;
	cJSON  *temp1 = NULL;

	OSAL_ASSERT(obj != NULL);
	OSAL_ASSERT(para != NULL);

	js_size = cJSON_GetArraySize(para);
	if (js_size == 0) {
		OSAL_LOGE("failed\n");
		ret = -1;
		goto exit;
	}

	for (i = 0; i < js_size; i++) {
		temp0 = cJSON_GetArrayItem(para, i);
		OSAL_ASSERT(temp0 != NULL);
		if (cJSON_GetObjectItem(temp0, "yaw")) {	
			temp1 = cJSON_GetObjectItem(temp0, "yaw");
			obj->yaw = temp1;
		} else if (cJSON_GetObjectItem(temp0, "pitch")) {
			temp1 = cJSON_GetObjectItem(temp0, "pitch");
			obj->pitch = temp1;
		} 
	}

exit:
	return ret;
}

/**
 * cr_cfg_init - get config parameter from file
 *
 * This function use to init config parameter from config file
 *
 * Returns -1 if called when get error ; otherwise, return 0
 */
int al_para_init(void)
{
	int ret = 0;

	/* capture it from customer path first */
	al_config_obj.para_root = lb_open_cfg_file(CFG_SAVE_PATH"/al_config.cfg");
	if (al_config_obj.para_root == NULL) {
		/* capture it from original path again */
		al_config_obj.para_root = lb_open_cfg_file(CASE_ROOT_PATH"/etc/al_config.cfg");
		/* capture it failed only exit */
		if (al_config_obj.para_root == NULL) {
			OSAL_LOGE("failed\n");
			ret = -1;
			goto exit;
		}
	}
	al_config_obj.car_dms = cJSON_GetObjectItem(al_config_obj.para_root, "car_dms");
	OSAL_ASSERT(al_config_obj.car_dms != NULL);
	ret = dms_para_array_init(&al_config_obj, al_config_obj.car_dms);
	if (ret != 0) {
		OSAL_LOGE("failed\n");
		ret = -1;
		goto exit;
	}

	return ret;
exit:
	if (al_config_obj.para_root) {
		lb_exit_cfg_file(al_config_obj.para_root);
		/* para_root will release inside upper interface */
		al_config_obj.para_root = NULL;
		/* car_dms will release inside upper interface */
		al_config_obj.car_dms = NULL;
	}

	return ret;
}
float dms_get_cali_yaw()
{
	if (al_config_obj.yaw != NULL)
		return al_config_obj.yaw->valuedouble;
	else
		return 0.0;
}

float dms_get_cali_pitch()
{

	if (al_config_obj.pitch != NULL)
		return al_config_obj.pitch->valuedouble;
	else
		return 0.0;
}

int dms_save_cali_parm(float yaw, float pitch)
{
	int ret = 0;

	OSAL_ASSERT(al_config_obj.yaw != NULL);
	OSAL_ASSERT(al_config_obj.pitch != NULL);

	al_config_obj.yaw->valuedouble = (double)yaw;
	al_config_obj.pitch->valuedouble = (double)pitch;

	return ret;
}

/**
 * cr_cfg_save - save config parameter to file
 *
 * This function use to save config parameter for config file
 *
 * Returns -1 if called when get error ; otherwise, return 1
 */
int al_para_save(void)
{
	int ret = 0;

	OSAL_ASSERT(al_config_obj.para_root != NULL);

	/* we save it to json which is coming from customer path */
	ret = lb_save_cfg_file(CFG_SAVE_PATH"/al_config.cfg", al_config_obj.para_root);

	return ret;
}

/**
 * cr_cfg_exit - exit config file
 *
 * This function use to exit config file,if you want save_para,
 * you must call home_cfg_init first
 *
 * Returns  1
 */
int al_para_exit(void)
{
	int ret = 0;

	OSAL_ASSERT(al_config_obj.para_root != NULL);

	lb_exit_cfg_file(al_config_obj.para_root);
	/* para_root will release inside upper interface */
	al_config_obj.para_root = NULL;
	/* car_dms will release inside upper interface */
	al_config_obj.car_dms = NULL;
	/* car_pano will release inside upper interface */
	return ret;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

