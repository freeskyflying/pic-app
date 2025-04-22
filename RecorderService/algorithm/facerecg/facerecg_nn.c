#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <errno.h>
#include <sys/prctl.h>
#include <linux/falloc.h>
#include <math.h>

#include "algo_common.h"
#include "utils.h"
#include "pnt_log.h"
#include "facerecg_nn.h"
#include "turbojpeg.h" /* jpeg soft decode */
#include "dms_config.h"

Facerecg_nn_t gFacerecgNN = { 0 };

int sqlite_callback(void *para, int argc, char **argv, char **azColName)
{
	int i;
	
	for(i=0; i<argc; i++)
	{
		PNT_LOGI("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}

	return 0;
}
static int facerecg_createTable(sqlite3 *db, char *sql)
{
	int ret;
	char *zErrMsg = 0;

	/* Execute SQL statement */
	ret = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
	if (ret != SQLITE_OK)
	{
		PNT_LOGE("createTable SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		return 0;
	}
	else
	{
		PNT_LOGI("createTable successfully\n");
		return -1;
	}
}
static void facerecg_open_sqlitedb()
{
	char *sql;
	int rc;
	Facerecg_nn_t* handle = &gFacerecgNN;

	/* Open database */
	rc = sqlite3_open(FACE_DATABASE, &handle->drivers_db);

	if (rc)
	{
		PNT_LOGE("Can't open database: %s\n", sqlite3_errmsg(handle->drivers_db));
		return ;
	}
	else
	{
		PNT_LOGI("Opened database successfully\n");
	}

	facerecg_createTable(handle->drivers_db, FACEDB_CREATE);
}
void facerecg_close_sqlitedb()
{
	Facerecg_nn_t* handle = &gFacerecgNN;
	
	sqlite3_close(handle->drivers_db);
}

static int facerecg_insertData(float norm, int feature_len, signed char *feature, int driverID, char* driverName)
{
	sqlite3_stmt *stmt = 0;
	int ret = 0, replace = 0;
	char sql[512];
	Facerecg_nn_t* handle = &gFacerecgNN;

	for (int i=0; i<handle->m_drivers.size; i++)
	{
		if (!strcmp(handle->m_drivers.info[i].name, driverName))
		{
			driverID = i;
			replace = 1;
			break;
		}
	}

	if (replace)
	{
		PNT_LOGI("%s is in database, replase it.", driverName);
        memset(sql, 0, 512);
        sprintf(sql, FACEDB_DELBYNAME, driverName);
        ExecSql(handle->drivers_db, sql, NULL, NULL);
	}
	
	sprintf(sql, FACEDB_COUNT);
    if (sqlite3_prepare_v2(handle->drivers_db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 :%s error!", sql);
        return -1;
    }
    if (sqlite3_step(stmt) != SQLITE_ROW)
	{
        PNT_LOGE("sqlite3_step!");
        return -1;
    }
    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    stmt = NULL;
    if(count >= REGISTER_FACE_MAX)
	{
        memset(sql, 0, 512);
        sprintf(sql, FACEDB_DELETE);
        ExecSql(handle->drivers_db, sql, NULL, NULL);
    }

	PNT_LOGI("insert drivers: %d %s", driverID, driverName);

	sprintf(sql, FACEDB_INSERT, driverName, norm, feature_len);

	ret = sqlite3_prepare(handle->drivers_db, sql, strlen(sql), &stmt, 0);
	if (ret != SQLITE_OK)
	{
		PNT_LOGE("sqlite3_prepare filed, ret:%d\n", ret);
		return -1;
	}
	
	ret = sqlite3_bind_blob(stmt, 1, feature, feature_len, NULL);
	if (ret != SQLITE_OK)
	{
		PNT_LOGE("sqlite3_step filed, ret:%d\n", ret);
		return -1;
	}
	
	ret = sqlite3_step(stmt);
	if (ret != SQLITE_DONE)
	{
		PNT_LOGE("sqlite3_step filed, ret:%d\n", ret);
	}
	sqlite3_finalize(stmt);

	strcpy(handle->m_drivers.info[driverID].name, driverName);
	memcpy(handle->m_drivers.m_face_features.p[driverID].feature, feature, feature_len);
	handle->m_drivers.m_face_features.p[driverID].norm = norm;

	if (0 == replace && handle->m_drivers.size < REGISTER_FACE_MAX)
		handle->m_drivers.size ++;
	
	handle->m_drivers.m_face_features.size = handle->m_drivers.size;

	return 0;

}

static void facerecg_selectDb()
{
	sqlite3_stmt *stmt;
	Facerecg_nn_t* handle = &gFacerecgNN;
	
	char *sql = "select * from drives;" ;
	
	sqlite3_prepare(handle->drivers_db, sql, strlen(sql), &stmt, 0);
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int len;
		int index = handle->m_drivers.size % REGISTER_FACE_MAX;
		
		const unsigned char *pname = sqlite3_column_text(stmt, 1);
		len = sqlite3_column_bytes(stmt, 1);
		if (pname)
		{
			memcpy(handle->m_drivers.info[index].name, pname, len);
		}
		
		handle->m_drivers.m_face_features.p[index].norm = (float)sqlite3_column_double(stmt, 2);
		
		handle->m_drivers.m_face_features.p[index].feature_len = sqlite3_column_int(stmt, 3);
		
		const char *pfeature = sqlite3_column_blob(stmt, 4);
		len = sqlite3_column_bytes(stmt, 4);
		if (pfeature)
		{
			//memcpy(feature, pfeature, len);
			memcpy(handle->m_drivers.m_face_features.p[index].feature, pfeature, len);
		}
		
		PNT_LOGI("index:%d, name:%s\n", index, handle->m_drivers.info[index].name);
		
		handle->m_drivers.size += 1;
	}

	if (REGISTER_FACE_MAX < handle->m_drivers.size)
		handle->m_drivers.size = REGISTER_FACE_MAX;
	
	handle->m_drivers.m_face_features.size = handle->m_drivers.size;

	PNT_LOGI("drivers_db drivers count: %d", handle->m_drivers.size);
	
	sqlite3_finalize(stmt);
}


static int start_nn_facerecg_fun(void)
{
	Facerecg_nn_t* handle = &gFacerecgNN;

	if (handle->nn_hdl != NULL)
	{
		return 0;
	}

	handle->facerecg_cfg.fmt = YUV420_SP;
	handle->facerecg_cfg.img_width = handle->u32Width;
	handle->facerecg_cfg.img_height = handle->u32Height;
	handle->facerecg_cfg.model_index = 0;
	handle->facerecg_cfg.model_basedir = NN_FACERECG_AX_PATH;
	handle->facerecg_cfg.interest_roi[0] = 0;
	handle->facerecg_cfg.interest_roi[1] = 0;
	handle->facerecg_cfg.interest_roi[2] = handle->u32Width - 1;
	handle->facerecg_cfg.interest_roi[3] = handle->u32Height - 1;

	PNT_LOGI("start face recgnize.");

	/* open libdms_facerecg.so*/
	handle->nn_hdl = nn_facerecg_create(&handle->facerecg_cfg);
	if(handle->nn_hdl == NULL)
	{
		PNT_LOGE("nn_facerecg_create() failed!");
		return -1;
	}
	
	return 0;
}

static void stop_nn_facerecg_fun()
{
	Facerecg_nn_t* handle = &gFacerecgNN;
	
	nn_facerecg_release(handle->nn_hdl);

	PNT_LOGI("stop face recgnize.");

	handle->nn_hdl = NULL;
}

static int check_face_in_limit_rect(int box[])
{
	Facerecg_nn_t* handle = &gFacerecgNN;
	
	if ((box[0] > 0) &&
		(box[1] > 0)
		&& (box[2] < (handle->u32Width - 1))
		&& (box[3] < (handle->u32Height - 1))) {

		return 0;
	}

	return -1;
}

/* 人脸图像离边缘的距离要超过人脸图像对角线长度*dist_thresh,否则认为是图像边界 */
static int facerecg_at_image_border(int *box,  int *roi_box, float dist_thresh)
{
	int ret = 0;

	int width = box[2] - box[0];
	int height = box[3] - box[1];
	float ab = sqrtf(width * width + height * height);
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			int dist = 100000;
			if (i % 2 == 0)
			{
				dist = box[i] - roi_box[j * 2];
			}
			else
			{
				dist = box[i] - roi_box[j * 2 + 1];
			}
			float ratio = dist / ab;
			if (fabs(ratio) < dist_thresh)
			{
				ret = 1;
			}
		}
	}
	if(ret == 1) {
		PNT_LOGE("at border %d, %d, %d, %d\n", box[0], box[1], box[2], box[3]);
	}

	return ret;
}

int facerecg_nn_start(VIDEO_FRAME_INFO_S *frame)
{
	int get_feature_flag = 0;
	nn_facerecg_features_t features = { 0 };
	Facerecg_nn_t* handle = &gFacerecgNN;

	facerecg_nn_init(1280, 720);

	if (access("/tmp/recg", F_OK) == 0)
	{
		system("rm /tmp/recg");
		facerecg_driver_recgonize(NULL);
	}

	if (access("/tmp/register", F_OK) == 0)
	{
		char name[64] = { 0 };
		FILE* f = fopen("/tmp/register", "rb");
		fread(name, 1, sizeof(name)-1, f);
		fclose(f);
		
		facerecg_driver_register(name);
		system("rm /tmp/register");
	}

	if (!gDmsAlgoParams->driver_recgn)
	{
		return -1;
	}

	if (handle->action == RECOGNITION_FACE && handle->m_drivers.size == 0)
	{
		return -1;
	}

	if (NULL == handle->nn_hdl)
	{
		return -1;
	}

	if (ACTION_IDEL == handle->action || REGISTER_END == handle->action)
	{
		return -1;
	}

	int pool_idx = 0;
	for (pool_idx = 0; pool_idx < VBUF_POOL_MAX_NUM; pool_idx++)
	{
		if (frame->u32PoolId == gAlComParam.pool_info[pool_idx].Pool)
		{
			break;
		}
	}
	if (pool_idx >= VBUF_POOL_MAX_NUM)
	{
		return -1;
	}

    handle->recg_in.img.y_phy = (void *)frame->stVFrame.u64PlanePhyAddr[0];
    handle->recg_in.img.y = (void *)frame->stVFrame.ulPlaneVirAddr[0];
    handle->recg_in.img.uv_phy = (void *)frame->stVFrame.u64PlanePhyAddr[1];
    handle->recg_in.img.uv = (void *)frame->stVFrame.ulPlaneVirAddr[1];
    handle->recg_in.img.width = handle->u32Width;
    handle->recg_in.img.height = handle->u32Height;
    handle->recg_in.img.format = YUV420_SP;
	if (handle->action == REGISTER_FACE_CAM)
	{
		handle->recg_in.interest_roi[0] = 0;
		handle->recg_in.interest_roi[1] = 0;
		handle->recg_in.interest_roi[2] = handle->u32Width - 1;
		handle->recg_in.interest_roi[3] = handle->u32Height - 1;
	}
	else
	{
		handle->recg_in.interest_roi[0] = gDmsAlgoParams->driver_rect_xs;
		handle->recg_in.interest_roi[1] = gDmsAlgoParams->driver_rect_ys;
		handle->recg_in.interest_roi[2] = gDmsAlgoParams->driver_rect_xe - 1;
		handle->recg_in.interest_roi[3] = gDmsAlgoParams->driver_rect_ye - 1;
	}

	nn_facerecg_detect(handle->nn_hdl, &handle->recg_in, &handle->faces);
		
	PNT_LOGI("handle->faces.size %d", handle->faces.size);
			
	if (handle->action == REGISTER_FACE_CAM)
	{
		static int face_rect[4] = { 0 };

		if (handle->faces.size == 1)
		{
			int ck = check_face_in_limit_rect(handle->faces.p[0].box);

			if (ck == -1)
			{
				if (handle->nn_facerecg_det_too_big_count > 100)
				{
					handle->nn_facerecg_det_too_big_count = 0;
				}
				handle->nn_facerecg_det_too_big_count++;
				handle->nn_facerecg_det_count = 0;
				handle->nn_facerecg_det_too_small_count = 0;
			}
			else if (handle->nn_facerecg_det_count == 0)
			{
				face_rect[0] = handle->faces.p[0].box[0];
				face_rect[1] = handle->faces.p[0].box[1];
				face_rect[2] = handle->faces.p[0].box[2];
				face_rect[3] = handle->faces.p[0].box[3];
				handle->nn_facerecg_det_count++;
			}
			else
			{
				nn_facerecg_feature_t face_feature;
				int width = handle->faces.p[0].box[2] - handle->faces.p[0].box[0];
				int height = handle->faces.p[0].box[3] - handle->faces.p[0].box[1];

				if (abs(handle->faces.p[0].headpose[0]) >= 15.0 || abs(handle->faces.p[0].headpose[1]) >= 30.0 || abs(handle->faces.p[0].headpose[2]) >= 10.0)
				{
					if (handle->nn_facerecg_det_offhead_count == 0)
					{
						handle->nn_facerecg_det_offhead_count = 40;
						handle->nn_facerecg_offhead_times ++;
						if (handle->nn_facerecg_offhead_times >= 5)
						{
							playAudio("face_regn/driver_picture_error.pcm");
						}
						else
						{
							playAudio("face_regn/directly_face_to_lens.pcm");
						}
					}
					else
					{
						handle->nn_facerecg_det_offhead_count --;
					}

					handle->nn_facerecg_det_count = 0;
					
					PNT_LOGI("headpose : %0.3f,%0.3f,%0.3f", handle->faces.p[0].headpose[0], handle->faces.p[0].headpose[1], handle->faces.p[0].headpose[2]);
				}
				else
				{
	 				if ((width > FACE_REGISTER_MIN_PIX) && (width < FACE_REGISTER_MAX_PIX) && (height > FACE_REGISTER_MIN_PIX) && (height < FACE_REGISTER_MAX_PIX))
					{
						handle->nn_facerecg_det_too_big_count = 0;
						handle->nn_facerecg_det_too_small_count = 0;
						get_feature_flag = nn_facerecg_get_feature(handle->nn_hdl, &handle->recg_in, &handle->faces.p[0], &face_feature);
						if (get_feature_flag == 0)
						{
							int t1 = compare_distance((float)face_rect[0], (float)face_rect[2], (float)handle->faces.p[0].box[0], (float)handle->faces.p[0].box[2]);
							if (t1 == 1)
							{
								if(handle->nn_facerecg_det_count == COUNT_REGISTER_FACE_FRAME)
								{
									playAudio("face_regn/register_finish.pcm");
									
									features.size = handle->faces.size;
									features.p = &face_feature;
									facerecg_insertData(features.p[0].norm, features.p[0].feature_len, features.p[0].feature, handle->regDriverID, handle->regDriverName);
									handle->action = REGISTER_END;
									stop_nn_facerecg_fun();
								}
								else
								{
									handle->nn_facerecg_det_count ++;
								}
							}
							else
							{
								handle->nn_facerecg_det_count = 0;
								PNT_LOGE("compare_distance failed.");
							}
						}
						else
						{
							handle->nn_facerecg_det_count = 0;
							PNT_LOGE("get feature failed.");
						}
					}
					else
					{
						if (width <= FACE_REGISTER_MIN_PIX || height <= FACE_REGISTER_MIN_PIX)
						{
							if (handle->nn_facerecg_det_too_small_count > 100)
							{
								handle->nn_facerecg_det_too_small_count = 0;
							}
							handle->nn_facerecg_det_too_big_count = 0;
							handle->nn_facerecg_det_too_small_count++;
						}
						else
						{
							if (handle->nn_facerecg_det_too_big_count > 100)
							{
								handle->nn_facerecg_det_too_big_count = 0;
							}
							handle->nn_facerecg_det_too_small_count = 0;
							handle->nn_facerecg_det_too_big_count++;
						}
						handle->nn_facerecg_det_count = 0;
						PNT_LOGE("face size error: %d %d", width, height);
					}

					handle->nn_facerecg_det_offhead_count = 0;
					handle->nn_facerecg_offhead_times = 0;
				}
			} 
		}
		else
		{
			handle->nn_facerecg_det_count = 0;
		}	
	}
	else if (handle->action == RECOGNITION_FACE)
	{
		int match_index = 0;
		float match_score = 0.0;
		
		features.size = handle->faces.size;
		if (features.size > 0)
		{
			features.p = (nn_facerecg_feature_t *)malloc(sizeof(nn_facerecg_feature_t)*features.size);
		}
		else
		{
			handle->nn_facerecg_det_count = 0;
		}
		
		for (int i = 0; i<handle->faces.size; i++)
		{
			nn_facerecg_match_result_t match_temp;
			
			get_feature_flag = nn_facerecg_get_feature(handle->nn_hdl, &handle->recg_in, &handle->faces.p[i], &features.p[i]);
			
			if (get_feature_flag == 0)
			{
				nn_facerecg_match(handle->nn_hdl, &features.p[i], handle->match_features, &match_temp);
				
				for (int j=0; j<handle->match_features->size; j++)
				{
					if (match_temp.score[j] > match_score)
					{
						match_score = match_temp.score[j];
						match_index = match_temp.max_index;
					}
				}

				if (check_face_in_limit_rect(handle->faces.p[i].box) || abs(handle->faces.p[i].headpose[0]) >= 45.0 || abs(handle->faces.p[i].headpose[1]) >= 30.0 || abs(handle->faces.p[i].headpose[2]) >= 10.0)
				{
					PNT_LOGI("ignored dirver faces not correct position[%d,%d,%d,%d] pose[%f,%f,%f].", handle->faces.p[i].box[0],
							handle->faces.p[i].box[1], handle->faces.p[i].box[2], handle->faces.p[i].box[3],
							handle->faces.p[i].headpose[0], handle->faces.p[i].headpose[1], handle->faces.p[i].headpose[2]);
				}
				else
				{
					handle->recgResult[handle->nn_facerecg_det_count%FACE_RECG_FRAMES].score = match_score;
					handle->recgResult[handle->nn_facerecg_det_count%FACE_RECG_FRAMES].index = match_index;
					
					PNT_LOGI("drivers[%d] temp driver match [%f]: %s", handle->match_features->size, match_score, handle->m_drivers.info[match_index].name);
					
					handle->nn_facerecg_det_count ++;
				}
			}
			else
			{
				handle->nn_facerecg_det_count = 0;
			}
		}

		if (handle->nn_facerecg_det_count >= FACE_RECG_FRAMES)
		{
			int indexCounts[REGISTER_FACE_MAX] = { 0 };
			float scores[REGISTER_FACE_MAX] = { 0.0 };
			
			for (int idx=0; idx<REGISTER_FACE_MAX; idx++)
			{
				scores[idx] = 0.0;
			}
			for (int idx=0; idx<FACE_RECG_FRAMES; idx++)
			{
				indexCounts[handle->recgResult[idx].index] += 1;
				scores[handle->recgResult[idx].index] += handle->recgResult[idx].score;
			}
			
			match_index = 0;
			
			for (int idx=0; idx<REGISTER_FACE_MAX; idx++)
			{
				if (scores[idx] > scores[match_index])
				{
					match_index = idx;
				}
			}
			
			match_score = scores[match_index] / indexCounts[match_index];

			if ((indexCounts[match_index] > FACE_RECG_FRAMES/10) && (match_score > gDmsAlgoParams->facerecg_match_score))
			{
				PNT_LOGI("****************** driver match [%f]: %s", match_score, handle->m_drivers.info[match_index].name);
				playAudio("face_regn/driver_match_success.pcm");
			}
			else
			{
				playAudio("face_regn/driver_match_failed.pcm");
				al_warning_proc(LB_WARNING_DMS_DRIVER_CHANGE, 0, 0, 0);
			}
			
			handle->action = ACTION_IDEL;
			stop_nn_facerecg_fun();
		}		
		
		if (features.size > 0)
		{
			free(features.p);
		}
	}

	return 0;
}

void facerecg_driver_register(char* driver_name)
{
	Facerecg_nn_t* handle = &gFacerecgNN;
	
	if (!gDmsAlgoParams->driver_recgn)
	{
		return;
	}
	
	strcpy(handle->regDriverName, driver_name);
	handle->regDriverID = handle->m_drivers.size % REGISTER_FACE_MAX;

	if (ACTION_IDEL == handle->action || REGISTER_END == handle->action)
	{		
		start_nn_facerecg_fun();
	}
	
	playAudioBlock("face_regn/start_register_driver.pcm");

	handle->action = REGISTER_FACE_CAM;

	handle->nn_facerecg_det_offhead_count = 10;
	handle->register_start_time = getUptime();
	handle->nn_facerecg_offhead_times = 0;
	handle->nn_facerecg_det_count = 0;

	PNT_LOGI("start to register: %d %s", handle->regDriverID, handle->regDriverName);
}

void facerecg_driver_recgonize(char* driver_name)
{
	Facerecg_nn_t* handle = &gFacerecgNN;

	if (handle->action == RECOGNITION_FACE)
	{
		return;
	}

	if (!gDmsAlgoParams->driver_recgn)
	{
		return;
	}
	
	if (ACTION_IDEL == handle->action || REGISTER_END == handle->action)
	{
		start_nn_facerecg_fun();
	}

	handle->action = RECOGNITION_FACE;

	handle->nn_facerecg_det_count = 0;

	handle->match_features = &handle->m_drivers.m_face_features;
}

int facerecg_get_face_feature(tjhandle tjInstance, char* imagefile, char* result, int driverId, char* driverName)
{
	PNT_LOGI("drivers image:%s", imagefile);

	Facerecg_nn_t* handle = &gFacerecgNN;

	int imageSize = 0, ret = 0, pool_idx = 0, flags = 0, yuv_size = 0;
	int u32Width = 0, u32Height = 0, yuv_width = 0, yuv_height = 0;
	int inSubsamp = 0, inColorspace = 0;
	unsigned char* imageBuffer = NULL, *yuv_buf = NULL;
	VBUF_BUFFER Buffer = VBUF_INVALID_BUFFER;
	nn_facerecg_feature_t feature = { 0 };
	
	nn_facerecg_faces_t faces = { 0 };
    VIDEO_FRAME_INFO_S axFrame = {0};
	int try_cnt = 5;
	FILE* imageFd = NULL;

retry:

	memset(&feature, 0, sizeof(feature));
	memset(&axFrame, 0, sizeof(axFrame));

	imageFd = fopen(imagefile, "rb");

	if (imageFd == NULL)
	{
		PNT_LOGE("[%s] opening input file err", imagefile);
		return -1;
	}
	if (fseek(imageFd, 0, SEEK_END) < 0 || ((imageSize = ftell(imageFd)) <=0) || fseek(imageFd, 0, SEEK_SET) < 0)
	{
		PNT_LOGE("[%s] determining input file size err", imagefile);
		ret = -1;	goto exit;
	}

	if ((imageBuffer = (unsigned char *)tjAlloc(imageSize)) == NULL)
	{
		PNT_LOGE("[%s] allocating JPEG buffer err", imagefile);
		ret = -1;	goto exit;
	}
	
	if (fread(imageBuffer, imageSize, 1, imageFd) < 1)
	{
		PNT_LOGE("[%s] reading input file err", imagefile);
		ret = -1;	goto exit;
	}
	
	/* 获取JPEG文件的宽和高，用于后面的等比例缩放 */
	if (tjDecompressHeader3(tjInstance, imageBuffer, imageSize, &u32Width, &u32Height, &inSubsamp, &inColorspace) < 0)
	{
		PNT_LOGE("[%s] tjDecompressHeader3 err", imagefile);
		ret = -1;	goto exit;
	}

	double w_h_rate = 1.0;

	if (u32Width!=1280 || u32Height!=720)
	{ /* 如果大小不是720P，等比例缩放 */
			w_h_rate = (double)u32Width/(double)u32Height;
			if (w_h_rate>(double)1280/720)
			{
				yuv_height = 1280/(w_h_rate);
				if (yuv_height>720)
					yuv_height = 720;
				yuv_width = 1280;
			}
			else
			{
				yuv_width = 720*(w_h_rate);
				if (yuv_width>1280)
					yuv_width = 1280;
				yuv_height = 720;
			}
	}
	else
	{
		yuv_width = u32Width;
		yuv_height = u32Height;
	}

	yuv_width = (yuv_width+1)/2*2;
	yuv_height = (yuv_height+1)/2*2;
	
	PNT_LOGI("[%s] w_h_rate %.3f  yuv size: %d %d jpeg size: %d %d", imagefile, w_h_rate, yuv_width, yuv_height, u32Width, u32Height);
	
	yuv_size = tjBufSizeYUV2(yuv_width, 1, yuv_height, TJSAMP_420); 
	/* 分配解码出来的I420 大小*/
	yuv_buf = (unsigned char *)tjAlloc(yuv_size);
	if (NULL == yuv_buf)
	{
		PNT_LOGE("[%s] yuv_buf malloc failed.", imagefile);
		ret = -1;	goto exit;
	}
	
	/* 算法只能处理720P NV12格式，获取720P NV12大小 buf */
	Buffer = EI_MI_VBUF_GetBuffer(gAlComParam.pool_info[1].Pool, 10*1000);
	if (Buffer == VBUF_INVALID_BUFFER)
	{
		PNT_LOGE("[%s] EI_MI_VBUF_GetBuffer failed.", imagefile);
		ret = -1;	goto exit;
	}

	ret = EI_MI_VBUF_GetFrameInfo(Buffer, &axFrame);
	if (ret)
	{
		PNT_LOGE("[%s] EI_MI_VBUF_GetFrameInfo failed.", imagefile);
		ret = -1;	goto exit;
	}

	for (pool_idx = 0; pool_idx < VBUF_POOL_MAX_NUM; pool_idx++)
	{
		if (axFrame.u32PoolId == gAlComParam.pool_info[pool_idx].Pool) {
			break;
		}
	}
	if (pool_idx >= VBUF_POOL_MAX_NUM)
	{
		PNT_LOGE("[%s] EI_MI_VBUF_GetFrameInfo failed.", imagefile);
		ret = -1;	goto exit;
	}
	
	axFrame.stVFrame.ulPlaneVirAddr[0] = (void *)gAlComParam.pool_info[pool_idx].pstBufMap[axFrame.u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[0];
	axFrame.stVFrame.ulPlaneVirAddr[1] = (void *)gAlComParam.pool_info[pool_idx].pstBufMap[axFrame.u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[1];
	if (tjDecompressToYUV2(tjInstance, imageBuffer, imageSize, yuv_buf, yuv_width, 1, yuv_height, flags) < 0)
	{
		PNT_LOGE("[%s] decompressing JPEG image err\n", imagefile);
		ret = -1;	goto exit;
	}

	/* I420转720P NV12 */
	memset((void*)axFrame.stVFrame.ulPlaneVirAddr[0], 0, 1280*720);
	memset((void*)axFrame.stVFrame.ulPlaneVirAddr[1], 0, 1280*720/2);

#if 0
	for (int i = 0; i < yuv_height; i++)
	{
		memcpy(axFrame.stVFrame.ulPlaneVirAddr[0]+i*1280 + (1280-yuv_width)/2, yuv_buf+i*yuv_width, yuv_width);
	}
	for (int i = 0; i < yuv_height/2; i++)
	{
		for (int j = 0; j < yuv_width/2; j++)
		{
			memcpy(axFrame.stVFrame.ulPlaneVirAddr[1]+(i + (720-yuv_height)/2)*1280+j*2 + (1280-yuv_width)/2, yuv_buf+yuv_width*yuv_height+i*yuv_width/2+j, 1);
			memcpy(axFrame.stVFrame.ulPlaneVirAddr[1]+(i + (720-yuv_height)/2)*1280+j*2+1 + (1280-yuv_width)/2, yuv_buf+yuv_width*yuv_height+yuv_width*yuv_height/4+i*yuv_width/2+j, 1);
		}
	}
#else
	for (int i = 0; i < yuv_height; i++)
	{
		memcpy((void*)axFrame.stVFrame.ulPlaneVirAddr[0]+i*1280, yuv_buf+i*yuv_width, yuv_width);
	}
	for (int i = 0; i < yuv_height/2; i++)
	{
		for (int j = 0; j < yuv_width/2; j++)
		{
			memcpy((void*)axFrame.stVFrame.ulPlaneVirAddr[1]+(i)*1280+j*2, yuv_buf+yuv_width*yuv_height+i*yuv_width/2+j, 1);
			memcpy((void*)axFrame.stVFrame.ulPlaneVirAddr[1]+(i)*1280+j*2+1, yuv_buf+yuv_width*yuv_height+yuv_width*yuv_height/4+i*yuv_width/2+j, 1);
		}
	}
#endif
	handle->recg_in.img.y_phy = (void *)axFrame.stVFrame.u64PlanePhyAddr[0];
	handle->recg_in.img.y = (void *)gAlComParam.pool_info[pool_idx].pstBufMap[axFrame.u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[0];
	handle->recg_in.img.uv_phy = (void *)axFrame.stVFrame.u64PlanePhyAddr[1];
	handle->recg_in.img.uv = (void *)gAlComParam.pool_info[pool_idx].pstBufMap[axFrame.u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[1];
	handle->recg_in.img.width = 1280;
	handle->recg_in.img.height = 720;
	handle->recg_in.img.format = YUV420_SP;
	handle->recg_in.interest_roi[0] = 0;
	handle->recg_in.interest_roi[1] = 0;
	handle->recg_in.interest_roi[2] = handle->recg_in.img.width - 1;
	handle->recg_in.interest_roi[3] = handle->recg_in.img.height - 1;
	
	ret = -1;

	nn_facerecg_detect(handle->nn_hdl, &handle->recg_in, &faces);

	if (faces.size == 1)
	{
		int width = faces.p[0].box[2] - faces.p[0].box[0];
		int height = faces.p[0].box[3] - faces.p[0].box[1];
		
		PNT_LOGI("[%s] box %d %d %d %d", imagefile, faces.p[0].box[0], faces.p[0].box[1], faces.p[0].box[2], faces.p[0].box[3]);
		
		if (0 == facerecg_at_image_border(faces.p[0].box, handle->recg_in.interest_roi, 0.1))
		{
			if ((width > FACE_REGISTER_MIN_PIX) && (width < FACE_REGISTER_MAX_PIX) && (height > FACE_REGISTER_MIN_PIX) && (height < FACE_REGISTER_MAX_PIX))
			{
				ret = nn_facerecg_get_feature(handle->nn_hdl, &handle->recg_in, &faces.p[0], &feature);
				
				PNT_LOGI("feature_len:%d, norm:%f, ret %d", feature.feature_len, feature.norm, ret);
				
				facerecg_insertData(feature.norm, feature.feature_len, feature.feature, driverId, driverName);
				ret = 0;
			}
		}
	}
	else
	{
		PNT_LOGE("[%s] faces count error : %d\n", imagefile, faces.size);
	}

exit:

	if (NULL != imageBuffer)
	{
		tjFree(imageBuffer);	imageBuffer = NULL;
	}

	if (NULL != yuv_buf)
	{
		tjFree(yuv_buf);	yuv_buf = NULL;
	}

	if (NULL != imageFd)
	{
		fclose(imageFd);	imageFd = NULL;
	}

	if (Buffer != VBUF_INVALID_BUFFER)
	{
		EI_MI_VBUF_PutBuffer(Buffer);
		Buffer = VBUF_INVALID_BUFFER;
	}

	try_cnt --;
	if (try_cnt > 0 && -1 == ret)
	{
		PNT_LOGE("[%s] get feature failed, retry", imagefile);
		goto retry;
	}

	if (ret)
	{
		PNT_LOGE("[%s] picture error.", imagefile);
		playAudioBlock("face_regn/driver_picture_error.pcm");
	}

	return ret;
}

int facerecg_register_by_images(char* imagesPath)
{
	Facerecg_nn_t* handle = &gFacerecgNN;

	if (!access("/tmp/image_reg_finish", F_OK))
	{
		return -1;
	}

	facerecg_nn_init(1280, 720);

	if (!gDmsAlgoParams->driver_recgn)
	{
		return -1;
	}

    DIR *pDir;
    struct dirent *ptr;
    pDir = opendir(imagesPath);
    if (pDir == NULL)
    {
        PNT_LOGE("open %s failed.", imagesPath);
        return -1;
    }

	char result[512] = { 0 };
	int success = 0;

	tjhandle tjInstance = NULL;
	tjInstance = tjInitDecompress();
	if (tjInstance == NULL)
	{
		PNT_LOGE("turbojpeg init failed.");
		return -1;
	}
	
	start_nn_facerecg_fun();
		
    ExecSql(handle->drivers_db, FACEDB_DELALL, NULL, NULL);

	memset(handle->m_drivers.m_face_features.p,0,sizeof(nn_facerecg_feature_t)*REGISTER_FACE_MAX);
	memset(handle->m_drivers.info, 0, sizeof(handle->m_drivers.info));
	handle->m_drivers.m_face_features.size = 0;
	handle->m_drivers.size = 0;
	
	playAudioBlock("face_regn/start_register_driver.pcm");

	// 遍历司机图片文件夹
    while (1)
    {
        ptr = readdir(pDir);

        if (ptr == NULL)
        {
            break;
        }
		
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;

		if (strstr(ptr->d_name, ".jpg") || strstr(ptr->d_name, ".jpeg"))
		{
			sprintf(result, "%s/%s", imagesPath, ptr->d_name);

			char driverName[128] = { 0 };

			// 从图片名获取司机名称
			strcpy(driverName, ptr->d_name);
			int i = strlen(driverName);
			while (i>0)
			{
				if (driverName[i-1] =='.')
				{
					driverName[i-1] = 0;
					break;
				}
				i--;
			}
			
			if (facerecg_get_face_feature(tjInstance, result, result, handle->m_drivers.size%REGISTER_FACE_MAX, driverName))
			{
				PNT_LOGE(result);
				//success = -1;
				//break;
			}
			else
			{
				PNT_LOGI("%s register success.", ptr->d_name);
			}
		}
    }

	if (0 == success)
	{
		playAudio("face_regn/register_finish.pcm");
	}
	
    closedir(pDir);
	
	stop_nn_facerecg_fun();

	tjDestroy(tjInstance);

	system("touch /tmp/image_reg_finish");

	return 0;
}

void facerecg_nn_init(unsigned short frmWidth, unsigned short frmHeight)
{
	Facerecg_nn_t* handle = &gFacerecgNN;

	if (!gDmsAlgoParams->driver_recgn || handle->initFlag)
	{
		return;
	}

	memset(handle, 0, sizeof(Facerecg_nn_t));

	handle->u32Width = frmWidth;
	handle->u32Height = frmHeight;

	facerecg_open_sqlitedb();

	handle->m_drivers.size = 0;
	handle->m_drivers.m_face_features.p = (nn_facerecg_feature_t *)malloc(sizeof(nn_facerecg_feature_t)*REGISTER_FACE_MAX);
	memset(handle->m_drivers.m_face_features.p,0,sizeof(nn_facerecg_feature_t)*REGISTER_FACE_MAX);
	memset(handle->m_drivers.info, 0, sizeof(handle->m_drivers.info));

	facerecg_selectDb();

	handle->initFlag = 1;
}

int facerecg_nn_get_drvier(int idx, char* name)
{
	Facerecg_nn_t* handle = &gFacerecgNN;

	if (idx >= handle->m_drivers.size)
	{
		return -1;
	}

	if (0 == idx)
	{
		handle->m_drivers.size = 0;
		memset(handle->m_drivers.m_face_features.p,0,sizeof(nn_facerecg_feature_t)*REGISTER_FACE_MAX);
		memset(handle->m_drivers.info, 0, sizeof(handle->m_drivers.info));

		facerecg_selectDb();
	}
	
	strcpy(name, handle->m_drivers.info[idx].name);

	return 0;
}

int facerecg_nn_del_driver(int idx, char* name)
{
	Facerecg_nn_t* handle = &gFacerecgNN;

	char sql[512] = { 0 };

	if (idx > handle->m_drivers.size)
	{
		return -1;
	}

	if (0 == idx)
	{
    	ExecSql(handle->drivers_db, FACEDB_DELALL, NULL, NULL);
	}
	else
	{
		idx -= 1;
		
	    memset(sql, 0, 512);
	    sprintf(sql, FACEDB_DELBYNAME, handle->m_drivers.info[idx].name);

	    ExecSql(handle->drivers_db, sql, NULL, NULL);

		strcpy(name, handle->m_drivers.info[idx].name);
	}
	
	handle->m_drivers.size = 0;
	memset(handle->m_drivers.m_face_features.p,0,sizeof(nn_facerecg_feature_t)*REGISTER_FACE_MAX);
	memset(handle->m_drivers.info, 0, sizeof(handle->m_drivers.info));
	
	facerecg_selectDb();

	return 0;
}

