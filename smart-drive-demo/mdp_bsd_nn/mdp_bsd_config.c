/*
 *------------------------------------------------------------------------------
 * @File      :    bsd_config.c
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

#include "mdp_bsd_config.h"
#include "osal.h"

#define CASE_ROOT_PATH	"/usr/share/out/"
#define SDCARD_MOUNT_PATH "/mnt/card/"
#define CFG_SAVE_PATH "/data/"

typedef struct _nn_bsd_warn_area_obj_
{
	cJSON *para_root;
	cJSON *lt_up_x;
	cJSON *lt_up_y;
	cJSON *rt_up_x;
	cJSON *rt_up_y;
	cJSON *lt_dn_x;
	cJSON *lt_dn_y;
	cJSON *rt_dn_x;
	cJSON *rt_dn_y;
}nn_bsd_warn_area_obj_t;

typedef struct tag_al_config_obj {
	cJSON *para_root;
	cJSON *car_bsd;
	cJSON *vanish_point_x;
	cJSON *vanish_point_y;
	cJSON *bottom_line_y;
	nn_bsd_warn_area_obj_t left_warn_area;
	nn_bsd_warn_area_obj_t right_warn_area;
	int	warn_area[16][2];
} al_config_obj_t;

static al_config_obj_t al_config_obj;

void nn_bsd_warn_area_init(cJSON *para, int flag)
{
	int i = 0;
	for (i = 0; i < cJSON_GetArraySize(para); i++) {
		cJSON *temp;
		temp = cJSON_GetArrayItem(para, i);
		if (temp && temp->type == cJSON_Object) {
			if (cJSON_GetObjectItem(temp, "lt_up_x")) {
				al_config_obj.left_warn_area.lt_up_x = cJSON_GetObjectItem(temp, "lt_up_x");
				al_config_obj.warn_area[0+flag*4][0] = al_config_obj.left_warn_area.lt_up_x->valueint;
				OSAL_LOGD(" al_config_obj.warn_roi[%d][0]:%d", (flag*4+0), al_config_obj.warn_area[0+flag*4][0]);
			} else if (cJSON_GetObjectItem(temp, "lt_up_y")) {
				al_config_obj.left_warn_area.lt_up_y = cJSON_GetObjectItem(temp, "lt_up_y");
				al_config_obj.warn_area[0+flag*4][1] = al_config_obj.left_warn_area.lt_up_y->valueint;
				OSAL_LOGD(" al_config_obj.warn_area[%d][1]:%d", (flag*4+0), al_config_obj.warn_area[0+flag*4][1]);
			} else if (cJSON_GetObjectItem(temp, "rt_up_x")) {
				al_config_obj.left_warn_area.rt_up_x = cJSON_GetObjectItem(temp, "rt_up_x");
				al_config_obj.warn_area[1+flag*4][0] = al_config_obj.left_warn_area.rt_up_x->valueint;
				OSAL_LOGD(" al_config_obj.warn_area[%d][0]:%d", (flag*4+1), al_config_obj.warn_area[1+flag*4][0]);
			} else if (cJSON_GetObjectItem(temp, "rt_up_y")) {
				al_config_obj.left_warn_area.rt_up_y = cJSON_GetObjectItem(temp, "rt_up_y");
				al_config_obj.warn_area[1+flag*4][1] = al_config_obj.left_warn_area.rt_up_y->valueint;
				OSAL_LOGD(" al_config_obj.warn_area[%d][1]:%d", (flag*4+1), al_config_obj.warn_area[1+flag*4][1]);
			} else if (cJSON_GetObjectItem(temp, "rt_dn_x")) {
				al_config_obj.left_warn_area.rt_dn_x = cJSON_GetObjectItem(temp, "rt_dn_x");
				al_config_obj.warn_area[2+flag*4][0] = al_config_obj.left_warn_area.rt_dn_x->valueint;
				OSAL_LOGD(" al_config_obj.warn_area[%d][0]:%d", (flag*4+2), al_config_obj.warn_area[2+flag*4][0]);
			} else if (cJSON_GetObjectItem(temp, "rt_dn_y")) {
				al_config_obj.left_warn_area.rt_dn_y = cJSON_GetObjectItem(temp, "rt_dn_y");
				al_config_obj.warn_area[2+flag*4][1] = al_config_obj.left_warn_area.rt_dn_y->valueint;
				OSAL_LOGD(" al_config_obj.warn_area[%d][1]:%d", (flag*4+2), al_config_obj.warn_area[2+flag*4][1]);
			} else if (cJSON_GetObjectItem(temp, "lt_dn_x")) {
				al_config_obj.left_warn_area.lt_dn_x = cJSON_GetObjectItem(temp, "lt_dn_x");
				al_config_obj.warn_area[3+flag*4][0] = al_config_obj.left_warn_area.lt_dn_x->valueint;
				OSAL_LOGD(" al_config_obj.warn_area[%d][0]:%d", (flag*4+3), al_config_obj.warn_area[3+flag*4][0]);
			} else if (cJSON_GetObjectItem(temp, "lt_dn_y")) {
				al_config_obj.left_warn_area.lt_dn_y = cJSON_GetObjectItem(temp, "lt_dn_y");
				al_config_obj.warn_area[3+flag*4][1] = al_config_obj.left_warn_area.lt_dn_y->valueint;
				OSAL_LOGD(" al_config_obj.warn_area[%d][1]:%d", (flag*4+3), al_config_obj.warn_area[3+flag*4][1]);
			}
		}
	}

}

static int bsd_para_array_init(al_config_obj_t *obj, cJSON *para)
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
		printf("failed\n");
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
		} else {
			temp1 = cJSON_GetObjectItem(temp0, "warnarea_lt");
			if (temp1 == NULL)
				continue;
			obj->left_warn_area.para_root = temp1;
			nn_bsd_warn_area_init(obj->left_warn_area.para_root, 0);

			temp1 = cJSON_GetObjectItem(temp0, "warnarea_rt");
			if (temp1 == NULL)
				continue;
			obj->right_warn_area.para_root = temp1;
			nn_bsd_warn_area_init(obj->left_warn_area.para_root, 1);
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
			printf("failed\n");
			ret = -1;
			goto exit;
		}
	}
	al_config_obj.car_bsd = cJSON_GetObjectItem(al_config_obj.para_root, "car_nn_bsd");
	OSAL_ASSERT(al_config_obj.car_bsd != NULL);
	ret = bsd_para_array_init(&al_config_obj, al_config_obj.car_bsd);
	if (ret != 0) {
		printf("failed\n");
		ret = -1;
		goto exit;
	}

	return ret;
exit:
	if (al_config_obj.para_root) {
		lb_exit_cfg_file(al_config_obj.para_root);
		/* para_root will release inside upper interface */
		al_config_obj.para_root = NULL;
		/* car_bsd will release inside upper interface */
		al_config_obj.car_bsd = NULL;
	}

	return ret;
}
float bsd_get_vanish_point_x()
{
	if (al_config_obj.vanish_point_x != NULL)
		return al_config_obj.vanish_point_x->valuedouble;
	else
		return 0.0;
}

float bsd_get_vanish_point_y()
{

	if (al_config_obj.vanish_point_y != NULL)
		return al_config_obj.vanish_point_y->valuedouble;
	else
		return 0.0;
}

float bsd_get_bottom_line_y()
{

	if (al_config_obj.bottom_line_y != NULL)
		return al_config_obj.bottom_line_y->valuedouble;
	else
		return 0.0;
}

int bsd_get_warn_roi_point(unsigned int point, unsigned int cordinate)
{
		if (point >=16 || cordinate >= 2)
			return -1;
		return al_config_obj.warn_area[point][cordinate];
}

int bsd_save_warn_roi_point(int warn_area[16][2])
{
		OSAL_ASSERT(al_config_obj.left_warn_area.lt_up_x != NULL);
		OSAL_ASSERT(al_config_obj.left_warn_area.lt_up_y != NULL);
		OSAL_ASSERT(al_config_obj.left_warn_area.rt_up_x != NULL);
		OSAL_ASSERT(al_config_obj.left_warn_area.rt_up_y != NULL);
		OSAL_ASSERT(al_config_obj.left_warn_area.lt_dn_x != NULL);
		OSAL_ASSERT(al_config_obj.left_warn_area.lt_dn_y != NULL);
		OSAL_ASSERT(al_config_obj.left_warn_area.rt_dn_x != NULL);
		OSAL_ASSERT(al_config_obj.left_warn_area.rt_dn_y != NULL);
		OSAL_ASSERT(al_config_obj.right_warn_area.lt_up_x != NULL);
		OSAL_ASSERT(al_config_obj.right_warn_area.lt_up_y != NULL);
		OSAL_ASSERT(al_config_obj.right_warn_area.rt_up_x != NULL);
		OSAL_ASSERT(al_config_obj.right_warn_area.rt_up_y != NULL);
		OSAL_ASSERT(al_config_obj.right_warn_area.lt_dn_x != NULL);
		OSAL_ASSERT(al_config_obj.right_warn_area.lt_dn_y != NULL);
		OSAL_ASSERT(al_config_obj.right_warn_area.rt_dn_x != NULL);
		OSAL_ASSERT(al_config_obj.right_warn_area.rt_dn_y != NULL);
		al_config_obj.left_warn_area.lt_up_x->valueint = warn_area[0][0];
		al_config_obj.left_warn_area.lt_up_x->valuedouble = warn_area[0][0];
		al_config_obj.left_warn_area.lt_up_y->valueint = warn_area[0][1];
		al_config_obj.left_warn_area.lt_up_y->valuedouble = warn_area[0][1];
		al_config_obj.left_warn_area.rt_up_x->valueint = warn_area[1][0];
		al_config_obj.left_warn_area.rt_up_x->valuedouble = warn_area[1][0];
		al_config_obj.left_warn_area.rt_up_y->valueint = warn_area[1][1];
		al_config_obj.left_warn_area.rt_up_y->valuedouble = warn_area[1][1];
		al_config_obj.left_warn_area.lt_dn_x->valueint = warn_area[2][0];
		al_config_obj.left_warn_area.lt_dn_x->valuedouble = warn_area[2][0];
		al_config_obj.left_warn_area.lt_dn_y->valueint = warn_area[2][1];
		al_config_obj.left_warn_area.lt_dn_y->valuedouble = warn_area[2][1];
		al_config_obj.left_warn_area.rt_dn_x->valueint = warn_area[3][0];
		al_config_obj.left_warn_area.rt_dn_x->valuedouble = warn_area[3][0];
		al_config_obj.left_warn_area.rt_dn_y->valueint = warn_area[3][1];
		al_config_obj.left_warn_area.rt_dn_y->valuedouble = warn_area[3][1];
		al_config_obj.right_warn_area.lt_up_x->valueint = warn_area[4][0];
		al_config_obj.right_warn_area.lt_up_x->valuedouble = warn_area[4][0];
		al_config_obj.right_warn_area.lt_up_y->valueint = warn_area[4][1];
		al_config_obj.right_warn_area.lt_up_y->valuedouble = warn_area[4][1];
		al_config_obj.right_warn_area.rt_up_x->valueint = warn_area[5][0];
		al_config_obj.right_warn_area.rt_up_x->valuedouble = warn_area[5][0];
		al_config_obj.right_warn_area.rt_up_y->valueint = warn_area[5][1];
		al_config_obj.right_warn_area.rt_up_y->valuedouble = warn_area[5][1];
		al_config_obj.right_warn_area.lt_dn_x->valueint = warn_area[6][0];
		al_config_obj.right_warn_area.lt_dn_x->valuedouble = warn_area[6][0];
		al_config_obj.right_warn_area.lt_dn_y->valueint = warn_area[6][1];
		al_config_obj.right_warn_area.lt_dn_y->valuedouble = warn_area[6][1];
		al_config_obj.right_warn_area.rt_dn_x->valueint = warn_area[7][0];
		al_config_obj.right_warn_area.rt_dn_x->valuedouble = warn_area[7][0];
		al_config_obj.right_warn_area.rt_dn_y->valueint = warn_area[7][1];
		al_config_obj.right_warn_area.rt_dn_y->valuedouble = warn_area[7][1];

		return 0;
}

int bsd_save_cali_parm(float vanish_point_x, float vanish_point_y)
{
	int ret = 0;

	OSAL_ASSERT(al_config_obj.vanish_point_x != NULL);
	OSAL_ASSERT(al_config_obj.vanish_point_y != NULL);

	al_config_obj.vanish_point_x->valuedouble = (double)vanish_point_x;
	al_config_obj.vanish_point_y->valuedouble = (double)vanish_point_y;

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
		al_config_obj.car_bsd = NULL;
		/* car_pano will release inside upper interface */
	}
	return ret;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

