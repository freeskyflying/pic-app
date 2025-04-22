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

#include "mdp_adas_config.h"

#define CASE_ROOT_PATH	"/usr/share/out/"
#define SDCARD_MOUNT_PATH "/mnt/card/"
#define CFG_SAVE_PATH "/data/"

typedef struct tag_al_config_obj {
	cJSON *para_root;
	cJSON *car_adas;
	cJSON *vanish_point_x;
	cJSON *vanish_point_y;
	cJSON *bottom_line_y;
} al_config_obj_t;

static al_config_obj_t al_config_obj;

static int adas_para_array_init(al_config_obj_t *obj, cJSON *para)
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
		if (cJSON_GetObjectItem(temp0, "vanish_point_x")) {	
			temp1 = cJSON_GetObjectItem(temp0, "vanish_point_x");
			obj->vanish_point_x = temp1;
		} else if (cJSON_GetObjectItem(temp0, "vanish_point_y")) {
			temp1 = cJSON_GetObjectItem(temp0, "vanish_point_y");
			obj->vanish_point_y = temp1;
		} else if (cJSON_GetObjectItem(temp0, "bottom_line_y")) {
			temp1 = cJSON_GetObjectItem(temp0, "bottom_line_y");
			obj->bottom_line_y = temp1;
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
	al_config_obj.car_adas = cJSON_GetObjectItem(al_config_obj.para_root, "car_nn_adas");
	OSAL_ASSERT(al_config_obj.car_adas != NULL);
	ret = adas_para_array_init(&al_config_obj, al_config_obj.car_adas);
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
		/* car_adas will release inside upper interface */
		al_config_obj.car_adas = NULL;
	}

	return ret;
}
float adas_get_vanish_point_x()
{
	if (al_config_obj.vanish_point_x != NULL)
		return al_config_obj.vanish_point_x->valuedouble;
	else
		return 0.0;
}

float adas_get_vanish_point_y()
{

	if (al_config_obj.vanish_point_y != NULL)
		return al_config_obj.vanish_point_y->valuedouble;
	else
		return 0.0;
}

float adas_get_bottom_line_y()
{

	if (al_config_obj.bottom_line_y != NULL)
		return al_config_obj.bottom_line_y->valuedouble;
	else
		return 0.0;
}

int adas_save_cali_parm(float vanish_point_x, float vanish_point_y, float bottom_line_y)
{
	int ret = 0;

	OSAL_ASSERT(al_config_obj.vanish_point_x != NULL);
	OSAL_ASSERT(al_config_obj.vanish_point_y != NULL);
	OSAL_ASSERT(al_config_obj.bottom_line_y != NULL);

	al_config_obj.vanish_point_x->valuedouble = (double)vanish_point_x;
	al_config_obj.vanish_point_y->valuedouble = (double)vanish_point_y;
	al_config_obj.bottom_line_y->valuedouble = (double)bottom_line_y;

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

	if(al_config_obj.para_root != NULL){
		/* we save it to json which is coming from customer path */
		ret = lb_save_cfg_file(CFG_SAVE_PATH"/al_config.cfg", al_config_obj.para_root);
	}
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

	if(al_config_obj.para_root != NULL) {
		lb_exit_cfg_file(al_config_obj.para_root);
		/* para_root will release inside upper interface */
		al_config_obj.para_root = NULL;
		/* car_adas will release inside upper interface */
		al_config_obj.car_adas = NULL;
		/* car_pano will release inside upper interface */
	}
	return ret;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

