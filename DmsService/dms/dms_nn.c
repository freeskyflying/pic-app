
#include "dms_config.h"
#include "algo_common.h"

//#define AL_PRO_FRAMERATE_STATISTIC /* al static frame rate */
#define COUNT_CALIBRATE_NUM 20

static axframe_info_s g_dms_axframe_info;
static dms_calibrate_para_t m_dms_cali;

extern pool_info_t pool_info[VBUF_POOL_MAX_NUM];

#define DMS_DBG		//EI_PRINT
#define DMS_INFO	EI_PRINT
#define DMS_ERR		EI_PRINT

#define WARN_PROC		0

int dms_warning(nn_dms_out_t *data);

static int check_face_in_limit_rect(int box[])
{
    axframe_info_s* axframe_info = &g_dms_axframe_info;

	if ((box[0] > 0) &&
		(box[1] > 0)
		&& (box[2] < (axframe_info->u32Width - 1))
		&& (box[3] < (axframe_info->u32Height - 1)))
    {

		return 0;
	}
	return -1;
}

int dms_update_para(float yawn, float pitch)
{
    axframe_info_s* axframe_info = &g_dms_axframe_info;

	DMS_DBG("********dms_update()******");
	int ret = -1;
	if (axframe_info->nn_hdl == NULL)
    {
		return ret;
	}
	axframe_info->m_nn_dms_cfg.warn_cfg.calib_yaw = yawn;
	axframe_info->m_nn_dms_cfg.warn_cfg.calib_pitch = pitch;
	ret = nn_dms_cmd(axframe_info->nn_hdl, NN_DMS_SET_PARAM, &axframe_info->m_nn_dms_cfg);
	if( ret == -1)
    {
		DMS_ERR("NN_DMS_SET_PARAM, failed failed!!");
	}
	return ret;
}

static void dms_cali_fun(nn_dms_out_t *out)
{
	nn_dms_face_t *face = NULL;
	if (m_dms_cali.cali_flag == DMS_CALI_START)
    {
		if (out->faces.size > 0)
        {
			DMS_DBG("**************");
			if (out->faces.driver_idx < 0 || out->faces.driver_idx >= out->faces.size)
            {
				DMS_DBG("out->faces.driver_idx %d\n", out->faces.driver_idx);
				/* if driver location not found , don't calib */
				return ;
			}
            else
            {
				face = &out->faces.p[out->faces.driver_idx];
			}
			int ck = check_face_in_limit_rect(face->box);
			DMS_DBG("ck : %d %d %f %f\n", ck, m_dms_cali.count_face_cb, face->face_attr.headpose[0], face->face_attr.headpose[1]);
			if (ck == -1)
            {
				m_dms_cali.count_face_cb = 0;
				return ;
			}
			if (m_dms_cali.count_face_cb == 0)
            {
				m_dms_cali.m_face_rect[0] = face->box[0];
				m_dms_cali.m_face_rect[1] = face->box[1];
				m_dms_cali.m_face_rect[2] = face->box[2];
				m_dms_cali.m_face_rect[3] = face->box[3];
				m_dms_cali.calib_yaw = 0.0;
				m_dms_cali.calib_pitch = 0.0;
				m_dms_cali.count_face_cb ++;
			}
            else
            {
				int t1 = compare_distance((float)m_dms_cali.m_face_rect[0], (float)m_dms_cali.m_face_rect[2], (float)face->box[0], (float)face->box[2]);
				if (t1 == 1)
                {
					m_dms_cali.calib_yaw += face->face_attr.headpose[0];
					m_dms_cali.calib_pitch += face->face_attr.headpose[1];
					if(m_dms_cali.count_face_cb == COUNT_CALIBRATE_NUM)
                    {
						m_dms_cali.calib_yaw = m_dms_cali.calib_yaw/COUNT_CALIBRATE_NUM;
						m_dms_cali.calib_pitch = m_dms_cali.calib_pitch/COUNT_CALIBRATE_NUM;
						m_dms_cali.count_face_cb = 0;
						m_dms_cali.cali_flag = DMS_CALI_END;
						m_dms_cali.auto_cali_count = 0;
						gDmsAlgoParams->calib_yaw = 10*m_dms_cali.calib_yaw;
						gDmsAlgoParams->calib_pitch = 10*m_dms_cali.calib_pitch;
						dms_update_para(m_dms_cali.calib_yaw, m_dms_cali.calib_pitch);
						DMS_DBG("m_dms_cali.calib_yaw %f; m_dms_cali.calib_pitch %f\n", m_dms_cali.calib_yaw, m_dms_cali.calib_pitch);
						//al_warning_msg(LB_WARNING_DMS_CALIBRATE_SUCCESS);//osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_CALIBRATE_SUCCESS, NULL, 0);
					}
                    else 
                    {
						m_dms_cali.count_face_cb ++;
					}
				}
                else
                {
					m_dms_cali.count_face_cb = 0;
				}
			}
		}
        else
        {
			m_dms_cali.count_face_cb = 0;
		}
	}
}

static int nn_dms_result_cb(int event, void *data)
{
    axframe_info_s* axframe_info = &g_dms_axframe_info;
    nn_dms_out_t *out = (nn_dms_out_t*)data;
	uint32_t index;
	nn_dms_face_t *face = NULL;
	//nn_dms_state_t *state = NULL;

	pthread_mutex_lock(&axframe_info->p_det_mutex);

	/* faces copy */
    if (axframe_info->p_det_info->faces.p)
    {
        free(axframe_info->p_det_info->faces.p);
        axframe_info->p_det_info->faces.p = NULL;
        axframe_info->p_det_info->faces.size = 0;
    }
    axframe_info->p_det_info->faces.p = malloc(out->faces.size * sizeof(nn_dms_face_t));
    if (axframe_info->p_det_info->faces.p)
    {
        memcpy(axframe_info->p_det_info->faces.p, out->faces.p, out->faces.size * sizeof(nn_dms_face_t));
        axframe_info->p_det_info->faces.size = out->faces.size;
		axframe_info->p_det_info->faces.driver_idx = out->faces.driver_idx;
    }
	/* persons copy */
	if (axframe_info->p_det_info->persons.p)
    {
        free(axframe_info->p_det_info->persons.p);
        axframe_info->p_det_info->persons.p = NULL;
        axframe_info->p_det_info->persons.size = 0;
    }
    axframe_info->p_det_info->persons.p = malloc(out->persons.size * sizeof(nn_dms_person_t));
    if (axframe_info->p_det_info->persons.p)
    {
        memcpy(axframe_info->p_det_info->persons.p, out->persons.p, out->persons.size * sizeof(nn_dms_person_t));
        axframe_info->p_det_info->persons.size = out->persons.size;
		axframe_info->p_det_info->persons.driver_idx = out->persons.driver_idx;
    }
	/* heads copy */
	if (axframe_info->p_det_info->heads.p)
    {
        free(axframe_info->p_det_info->heads.p);
        axframe_info->p_det_info->heads.p = NULL;
        axframe_info->p_det_info->heads.size = 0;
    }
    axframe_info->p_det_info->heads.p = malloc(out->heads.size * sizeof(nn_dms_head_t));
    if (axframe_info->p_det_info->heads.p)
    {
        memcpy(axframe_info->p_det_info->heads.p, out->heads.p, out->heads.size * sizeof(nn_dms_head_t));
        axframe_info->p_det_info->heads.size = out->heads.size;
		axframe_info->p_det_info->heads.driver_idx = out->heads.driver_idx;
    }
    axframe_info->p_det_info->cover_state = out->cover_state;
	axframe_info->p_det_info->driver_leave = out->driver_leave;
	axframe_info->p_det_info->no_face = out->no_face;
	axframe_info->p_det_info->warn_status = out->warn_status;
	/* obj copy */
    for (int i = 0; i < 3; i++)
    {
        if (axframe_info->p_det_info->obj_boxes[i].p)
        {
            free(axframe_info->p_det_info->obj_boxes[i].p);
            axframe_info->p_det_info->obj_boxes[i].p = NULL;
            axframe_info->p_det_info->obj_boxes[i].size = 0;
        }
        if (out->obj_boxes[i].size == 0)
            continue;
		DMS_DBG("out->obj_boxes[%d].size:%d\n", i, out->obj_boxes[i].size);
        axframe_info->p_det_info->obj_boxes[i].p = malloc(sizeof(nn_dmsobj_box_t) * out->obj_boxes[i].size);
        if (axframe_info->p_det_info->obj_boxes[i].p)
        {
            memcpy(axframe_info->p_det_info->obj_boxes[i].p, out->obj_boxes[i].p, sizeof(nn_dmsobj_box_t) * out->obj_boxes[i].size);
            axframe_info->p_det_info->obj_boxes[i].size = out->obj_boxes[i].size;
        }
    }

	pthread_mutex_unlock(&axframe_info->p_det_mutex);
	if (axframe_info->action == DMS_CALIB)
    {
		DMS_INFO("calibration\n");
		dms_cali_fun(out);

		if ((m_dms_cali.cali_flag != DMS_CALI_END) /*&& al_warning_proc(LB_WARNING_DMS_CALIBRATE_FAIL, 0)*/)
		{
		}
		else if (m_dms_cali.cali_flag == DMS_CALI_END)
		{
			axframe_info->action = DMS_RECOGNITION;
		}
	}
    else
    {
		if (out->faces.size > 0)
        {
			if (out->faces.driver_idx < 0 || out->faces.driver_idx >= out->faces.size)
            {
				DMS_DBG("out->faces.driver_idx %d\n", out->faces.driver_idx);
				face = &out->faces.p[0];
			}
            else
            {
				face = &out->faces.p[out->faces.driver_idx];
			}
			//state = &face->face_attr.dms_state;
			/*  注意: 要传要检测的人脸坐标和关键点信息*/
			axframe_info->face.roi.x0 = face->box[0];
			axframe_info->face.roi.y0 = face->box[1];
			axframe_info->face.roi.x1 = face->box[2];
			axframe_info->face.roi.y1 = face->box[3];
			if (axframe_info->livingdet_flag == LIVING_DET_SINGLE_RGB)
            {
				int ret;
				ezax_face_kpts_t *face_kpts = axframe_info->face.roi.in_ex_inform;
				/* face_kpts 是人脸的关键点信息，先要识别到人脸，再做活体检测 */
				for (int i=0; i < 10; i++)
                {
					face_kpts->kpts[i] = (float)face->pts[i];
				}
				ret = nna_livingdet_process(axframe_info->livingdet_hdl, NULL, &axframe_info->face, axframe_info->livingdet_out);
				if (!ret)
                {
					axframe_info->score  = (ezax_livingdet_rt_t *)axframe_info->livingdet_out->out_ex_inform;
					DMS_DBG("rgb live_score: %d\n", axframe_info->score->live_score);
				}
			}
            else if(axframe_info->livingdet_flag == LIVING_DET_SINGLE_NIR)
            {
				int ret;
				ezax_face_kpts_t *face_kpts = axframe_info->face.roi.in_ex_inform;
				/* face_kpts 是人脸的关键点信息，先要识别到人脸，再做活体检测 */
				for (int i=0; i < 10; i++)
                {
					face_kpts->kpts[i] = (float)face->pts[i];
				}
				ret = nna_livingdet_process(axframe_info->livingdet_hdl, &axframe_info->face, NULL, axframe_info->livingdet_out);
				if (!ret)
                {
					axframe_info->score  = (ezax_livingdet_rt_t *)axframe_info->livingdet_out->out_ex_inform;
					DMS_DBG("nir live_score: %d\n", axframe_info->score->live_score);
				}
			}

//			DMS_DBG("action: drink:%d,  call:%d, smoke:%d, yawn:%d, eyeclosed:%d\n", state->drink, state->call, state->smoke, state->yawn, state->eyeclosed);
//	        DMS_DBG("action: lookaround:%d, lookup:%d, lookdown:%d, red_resist_glasses:%d, cover_state:%d, no_face_mask:%d, headpose:%f %f %f\n",
//	                state->lookaround, state->lookup, state->lookdown, face->face_attr.red_resist_glasses, out->cover_state, face->face_attr.no_face_mask,
//	                face->face_attr.headpose[0], face->face_attr.headpose[1], face->face_attr.headpose[2]);
	    }
        else
        {
			DMS_DBG("cover_state:%d no_face: %d\n", out->cover_state, out->no_face);
		}
		dms_warning(out);
	}

	sem_post(&axframe_info->frame_del_sem);

    return 0;
}

int nn_dms_start(axframe_info_s* axframe_info, VIDEO_FRAME_INFO_S *frame, void *nn_hdl)
{
    EI_S32 s32Ret;
    static nn_dms_in_t m_dms_in;
	int pool_idx = 0;
	/* 获取保存的映射虚拟地址 */
	for (pool_idx = 0; pool_idx < VBUF_POOL_MAX_NUM; pool_idx++)
    {
		if (frame->u32PoolId == pool_info[pool_idx].Pool)
        {
			break;
		}
	}
	if (pool_idx >= VBUF_POOL_MAX_NUM)
		return 0;
    m_dms_in.img.y_phy = (void *)frame->stVFrame.u64PlanePhyAddr[0];
    m_dms_in.img.y = (void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[0];
    m_dms_in.img.uv_phy = (void *)frame->stVFrame.u64PlanePhyAddr[1];
    m_dms_in.img.uv = (void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[1];
    m_dms_in.img.width = 1280;
    m_dms_in.img.height = 720;
    m_dms_in.img.format = YUV420_SP;
    m_dms_in.cover_det_en = 1;
    m_dms_in.red_resist_glasses_en = 1;
	m_dms_in.auto_calib_en = 0;
	if (axframe_info->livingdet_flag == LIVING_DET_SINGLE_RGB ||
		axframe_info->livingdet_flag == LIVING_DET_SINGLE_NIR)
    {
		if (axframe_info->livingdet_hdl)
        {
			axframe_info->face.img_handle.fmt = EZAX_YUV420_SP;
			axframe_info->face.img_handle.w = axframe_info->u32Width;//输入人脸图像的大小
			axframe_info->face.img_handle.h = axframe_info->u32Height;
			axframe_info->face.img_handle.stride = axframe_info->u32Width;
			axframe_info->face.img_handle.c = 3; // chanel,reserved
			axframe_info->face.nUID = 0;// mark id,reserved
			axframe_info->face.img_handle.pVir = (void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[0];
			axframe_info->face.img_handle.pPhy = (unsigned int)frame->stVFrame.u64PlanePhyAddr[0];
			axframe_info->face.img_handle.pVir_UV = (void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[1];
			axframe_info->face.img_handle.pPhy_UV = (unsigned int)frame->stVFrame.u64PlanePhyAddr[1];
			axframe_info->face.roi.x0 = 0;//检测人脸的有效区域
			axframe_info->face.roi.y0 = 0;
			axframe_info->face.roi.x1 = axframe_info->u32Width-1;
			axframe_info->face.roi.y1 = axframe_info->u32Height-1;
		}
	}
    if (nn_hdl != NULL)
    {
        s32Ret = nn_dms_cmd(nn_hdl, NN_DMS_SET_DATA, &m_dms_in);
		if (s32Ret < 0)
        {
			DMS_ERR("NN_DMS_SET_DATA failed %d!\n", s32Ret);
		}
    }
    
    sem_wait(&axframe_info->frame_del_sem);

    return 0;
}

static EI_VOID *get_axframe_proc(EI_VOID *p)
{
    int ret;
    VIDEO_FRAME_INFO_S axFrame = {0};
    axframe_info_s *axframe_info;
    void *nn_hdl = NULL;

    axframe_info = (axframe_info_s *)p;
    DMS_DBG("get_axframe_proc bThreadStart = %d\n", axframe_info->bThreadStart);

#ifdef AL_PRO_FRAMERATE_STATISTIC
	int64_t time1 = 0LL, time2 = 0LL, time_GetChnFrame1 = 0LL, time_GetChnFrame2 = 0LL,
	time_AlProFrame1 = 0LL, time_AlProFrame2 = 0LL;
	int64_t frame_cnt = 0;
#endif
    DMS_DBG("********start_nn_dms******\n");
    memset(&axframe_info->m_nn_dms_cfg, 0, sizeof(nn_dms_cfg_t));
    /* init nn_dms, cfg */
    axframe_info->m_nn_dms_cfg.img_width = axframe_info->u32Width; //算法输入图像宽
    axframe_info->m_nn_dms_cfg.img_height = axframe_info->u32Height; //算法输入图像高
    axframe_info->m_nn_dms_cfg.format = YUV420_SP;
    axframe_info->m_nn_dms_cfg.model_basedir = NN_DMS_PATH;
    axframe_info->m_nn_dms_cfg.cb_func = nn_dms_result_cb;
    axframe_info->m_nn_dms_cfg.dms_behave_en = 1; //哈欠 闭眼 开关
    axframe_info->m_nn_dms_cfg.headpose_en = 1; //左顾右盼 抬头 低头探测开关
    axframe_info->m_nn_dms_cfg.dms_obj_det_en = 1; //喝水 打电话 吸烟探测开关
    axframe_info->m_nn_dms_cfg.cover_det_en = 1; //摄像头遮住探测开关
    axframe_info->m_nn_dms_cfg.red_resist_glasses_en = 1; //支持佩戴防红外眼镜检测使能
    axframe_info->m_nn_dms_cfg.face_mask_en = 1; //支持佩戴口罩检测使能
    axframe_info->m_nn_dms_cfg.person_det_en = 1; //支持人型检测使能
    axframe_info->m_nn_dms_cfg.headface_det_en = 1; //支持人脸和人头检测，如果置1，选择headfacedet.bin模型，否则使用dmsfacedet.bin相关模型
	axframe_info->m_nn_dms_cfg.headface_det_resolution = 1; //1是大分辨率检测，占用资源多，检测效果好，0是小分辨率检测，占用资源小，效果没有1好
	axframe_info->m_nn_dms_cfg.auto_calib_en = 0; //DMS 自动校准使能位
	if (axframe_info->action == DMS_CALIB)
    {
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_yaw = 0.0;
    	axframe_info->m_nn_dms_cfg.warn_cfg.calib_pitch = 0.0;
		//al_warning_msg(LB_WARNING_DMS_CALIBRATE_START); //osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_CALIBRATE_START, NULL, 0);
	}
    else
    {
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_yaw = (float)gDmsAlgoParams->calib_pitch/10;//dms_get_cali_yaw();
    	axframe_info->m_nn_dms_cfg.warn_cfg.calib_pitch = (float)gDmsAlgoParams->calib_yaw/10;//dms_get_cali_pitch();
		/* if calib_position not set, driver detcect from whole region,other , driver detect for calib_position region */
		#if 0
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[0] = 0;
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[1] = 0;
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[2] = 640;
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[3] = 720;
		#endif
	}
    EI_PRINT(" calib_yaw:%f, calib_pitch:%f \n", axframe_info->m_nn_dms_cfg.warn_cfg.calib_yaw, axframe_info->m_nn_dms_cfg.warn_cfg.calib_pitch);
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.yaw_thresh[0] = -40.0; //驾驶员分神状态，相对于校准点的水平偏离门限角度，左负右正
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.yaw_thresh[1] = 40.0; //驾驶员分神状态，相对于校准点的水平偏离门限角度，左负右正
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.warn_interval = 5.0; //报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.count = 1; // 触发一次报警状态的act 状态个数
    axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.time = 3.0;
// 3.0s  报警状态统计总时间
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.act.ratio = 0.7; // 0.7  百分比，单个act.time中检测到70%  act动作即触发一个act状态
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.act.time = 3.0; // 3.0s，单次act统计时间 

	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.pitch_thresh[0] = -20.0; //驾驶员分神状态，相对于校准点的垂直偏离门限角度，上正下负
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.pitch_thresh[1] = 30.0; //驾驶员分神状态，相对于校准点的垂直偏离门限角度，上正下负
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.warn_interval = 5.0; //报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.count = 1;// 触发一次报警状态的act 状态个数
    axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.time = 3.0;
// 3.0s   报警状态统计总时间
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.act.ratio = 0.7;// 0.7  百分比，单个act.time中检测到70%  act动作即触发一个act状态
	axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.act.time = 3.0;// 3.0s，单次act统计时间 

    axframe_info->m_nn_dms_cfg.warn_cfg.drink_cfg.warn_interval = 5.0;//驾驶员喝水报警时间间隔
    axframe_info->m_nn_dms_cfg.warn_cfg.drink_cfg.count = 1;
    axframe_info->m_nn_dms_cfg.warn_cfg.drink_cfg.time = 2.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.drink_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.drink_cfg.act.time = 2.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.drink_det_thresh = 0.7; //det level, low:0.6;mid:0.7;high:0.8

	axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.warn_interval = 5.0;//驾驶员打电话报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.count = 1;
    axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.time = 3.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.act.time = 3.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.call_det_thresh = 0.7; //det level, low:0.6;mid:0.7;high:0.8

	axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.warn_interval = 5.0; //驾驶员抽烟报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.count = 1;
    axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.time = 2.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.act.time = 2.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.smoke_det_thresh = 0.7; //det level, low:0.6;mid:0.7;high:0.8

	axframe_info->m_nn_dms_cfg.warn_cfg.red_resist_glasses_cfg.warn_interval = 5.0;//驾驶员佩戴防红外眼镜报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.red_resist_glasses_cfg.count = 1;
    axframe_info->m_nn_dms_cfg.warn_cfg.red_resist_glasses_cfg.time = 5.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.red_resist_glasses_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.red_resist_glasses_cfg.act.time = 5.0;

	axframe_info->m_nn_dms_cfg.warn_cfg.face_mask_cfg.warn_interval = 5.0;//驾驶员是否 佩戴口罩报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.face_mask_cfg.count = 1;
	axframe_info->m_nn_dms_cfg.warn_cfg.face_mask_cfg.time = 5.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.face_mask_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.face_mask_cfg.act.time = 5.0;

	axframe_info->m_nn_dms_cfg.warn_cfg.cover_cfg.warn_interval = 5.0;//遮挡摄像头报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.cover_cfg.count = 1;
    axframe_info->m_nn_dms_cfg.warn_cfg.cover_cfg.time = 5.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.cover_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.cover_cfg.act.time = 5.0;

	axframe_info->m_nn_dms_cfg.warn_cfg.driver_leave_cfg.warn_interval = 5.0;//驾驶员离开报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.driver_leave_cfg.count = 1;
    axframe_info->m_nn_dms_cfg.warn_cfg.driver_leave_cfg.time = 5.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.driver_leave_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.driver_leave_cfg.act.time = 5.0;
	/* no_face_cfg 用来单独检测人脸的*/
	axframe_info->m_nn_dms_cfg.warn_cfg.no_face_cfg.warn_interval = 5.0;//未检测到人脸报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.no_face_cfg.count = 1;
    axframe_info->m_nn_dms_cfg.warn_cfg.no_face_cfg.time = 5.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.no_face_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.no_face_cfg.act.time = 5.0;
	/* level 2 fragile */
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.warn_interval = 10.0;//驾驶员打哈欠报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.count = 1; // time 内检测到act 状态>= 3次，触发报警
    axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.time = 3;
// 300s ,报警状态统计总时间
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.act.ratio = 0.7;// 0.7  百分比，单个act.time中检测到70%  act动作即触发一个act状态
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.act.time = 3.0;// 2.0s，单次act统计时间 
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.yawn_headpose_range[0] = -60; //打哈欠报警的人脸头部姿态检测范围，左负右正，上正下负，数组索引顺序: 左右下上
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.yawn_headpose_range[1] = 60;
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.yawn_headpose_range[2] = -35;
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.yawn_headpose_range[3] = 45;
	/* level 4 fragile */
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[0].warn_interval = 10.0;//驾驶员眨眼报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[0].count = 3;// time 内检测到act 状态>=3次，触发报警
    axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[0].time = 2 * 60;
// 120s ,报警状态统计总时间
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[0].act.ratio = 0.7;// 0.5  百分比，单个act.time中检测到50%  act动作即触发一个act状态
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[0].act.time = 1.0;// 1.0s，单次act统计时间
	/* level 6 fragile */
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].warn_interval = 10.0;//驾驶员眨眼报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].count = 6;// time 内检测到act 状态>=6次，触发报警
    axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].time = 1 * 60;
// 60s ,报警状态统计总时间
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].act.ratio = 0.7;// 0.5 百分比，单个act.time中检测到50%  act动作即触发一个act状态
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].act.time = 1.0;// 1.0s，单次act统计时间
	/* level 8 fragile */
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].warn_interval = 10.0;//驾驶员闭眼报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].count = 1;// time 内检测到act 状态>=1次，触发报警
    axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].time = 2;
// 2s ,报警状态统计总时间
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].act.ratio = 0.7;// 0.7  百分比，单个act.time中检测到70%  act动作即触发一个act状态
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].act.time = 2.0;// 2.0s，单次act统计时间 
	/* level 10 fragile */
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].warn_interval = 10.0;//驾驶员闭眼报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].count = 1;// time 内检测到act 状态>=1次，触发报警
    axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].time = 4.0;
// 4s ,报警状态统计总时间
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].act.ratio = 0.7;// 0.7  百分比，单个act.time中检测到70%  act动作即触发一个act状态
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].act.time = 4.0;// 4.0s，单次act统计时间 

	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.eyeclosed_headpose_range[0] = -45; //闭眼报警的人脸头部姿态检测范围，左负右正，上正下负，数组索引顺序: 左右下上
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.eyeclosed_headpose_range[1] = 45;
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.eyeclosed_headpose_range[2] = -15;
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.eyeclosed_headpose_range[3] = 45;
	axframe_info->m_nn_dms_cfg.dump_cfg = 1;//dump 配置参数

    axframe_info->m_nn_dms_cfg.interest_box[0] = 0;//算法检测区域左上角X坐标
    axframe_info->m_nn_dms_cfg.interest_box[1] = 0;//算法检测区域左上角y坐标
    axframe_info->m_nn_dms_cfg.interest_box[2] = axframe_info->u32Width - 1;//算法检测区域右下角x坐标
    axframe_info->m_nn_dms_cfg.interest_box[3] = axframe_info->u32Height - 1;//算法检测区域右下角y坐标
    DMS_DBG("interest_box x1:%d,y1:%d,x2:%d,y2:%d \n", axframe_info->m_nn_dms_cfg.interest_box[0], axframe_info->m_nn_dms_cfg.interest_box[1],
        axframe_info->m_nn_dms_cfg.interest_box[2], axframe_info->m_nn_dms_cfg.interest_box[3]);

    /* open libdms.so*/
    nn_hdl = nn_dms_open(&axframe_info->m_nn_dms_cfg);
    if (nn_hdl == NULL)
    {
        DMS_ERR("nn_dms_open() failed!");
        return NULL;
    }
	axframe_info->nn_hdl = nn_hdl;
	DMS_DBG("axframe_info->livingdet_flag %d %s %d!\n", axframe_info->livingdet_flag, __FILE__, __LINE__);
	if (axframe_info->livingdet_flag == LIVING_DET_SINGLE_RGB)
    {
		axframe_info->livingdet_cfg.model_rootpath = LIVINGDET_PATH;
		axframe_info->livingdet_cfg.freq.id = 0;// nu 模块
		axframe_info->livingdet_cfg.freq.freq = 300;
		axframe_info->livingdet_cfg.single_rgb_model = 0;// use single rgb， only 0 valid
		axframe_info->livingdet_cfg.living_color_enable = 1;// support rgb color
		axframe_info->livingdet_cfg.mask_en = 0;
		axframe_info->livingdet_cfg.mask_living_enable = 0;
		axframe_info->livingdet_cfg.living_confidence_thr = 0;
		axframe_info->livingdet_cfg.livingdet_mode = 1; // 0:rgb+nir(live) 1:rgb(live) 2:nir(live)
		axframe_info->livingdet_cfg.rgb_exposure_time_rate = 0;
		axframe_info->livingdet_cfg.rgb_exposure_dark_rate = 0;
		axframe_info->livingdet_cfg.nir_avg_lum_rate = 0;
		axframe_info->livingdet_hdl = nna_livingdet_open(&axframe_info->livingdet_cfg);
		DMS_DBG("axframe_info->livingdet_hdl %p %s %d\n", axframe_info->livingdet_hdl, __FILE__, __LINE__);
	}
    else if (axframe_info->livingdet_flag == LIVING_DET_SINGLE_NIR)
    {
		axframe_info->livingdet_cfg.model_rootpath = LIVINGDET_PATH;
		axframe_info->livingdet_cfg.freq.id = 0;// nu 模块
		axframe_info->livingdet_cfg.freq.freq = 300;
		axframe_info->livingdet_cfg.single_rgb_model = 0;// use single rgb， only 0 valid
		axframe_info->livingdet_cfg.living_color_enable = 0;// 1: support rgb color 0: not support
		axframe_info->livingdet_cfg.mask_en = 0;
		axframe_info->livingdet_cfg.mask_living_enable = 0;
		axframe_info->livingdet_cfg.living_confidence_thr = 0;
		axframe_info->livingdet_cfg.livingdet_mode = 3; // 0:rgb+nir(live) 1:rgb(live) 2:nir+nir(live) 3:nir(live)
		axframe_info->livingdet_cfg.rgb_exposure_time_rate = 0;
		axframe_info->livingdet_cfg.rgb_exposure_dark_rate = 0;
		axframe_info->livingdet_cfg.nir_avg_lum_rate = 0;
		axframe_info->livingdet_hdl = nna_livingdet_open(&axframe_info->livingdet_cfg);
		DMS_DBG("axframe_info->livingdet_hdl %p %s %d\n", axframe_info->livingdet_hdl, __FILE__, __LINE__);
	}
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time1 = osal_get_msec();
#endif

    while (EI_TRUE == axframe_info->bThreadStart)
    {
        memset(&axFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time_GetChnFrame1 = osal_get_msec();
#endif
        ret = EI_MI_VISS_GetChnFrame(axframe_info->dev, axframe_info->chn, &axFrame, 1000);
        if (ret != EI_SUCCESS)
        {
            DMS_DBG("chn %d get frame error 0x%x\n", axframe_info->chn, ret);
            usleep(100 * 1000);
            continue;
        }
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time_GetChnFrame2 = osal_get_msec();
#endif
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time_AlProFrame1 = osal_get_msec();
#endif
        nn_dms_start(axframe_info, &axFrame, nn_hdl);
#ifdef AL_PRO_FRAMERATE_STATISTIC
        time_AlProFrame2 = osal_get_msec();
        DMS_DBG("time_GetChnFrame %lld ms, time_AlProFrame %lld ms\n",
            time_GetChnFrame2-time_GetChnFrame1, time_AlProFrame2-time_AlProFrame1);
#endif
#ifdef AL_PRO_FRAMERATE_STATISTIC
		frame_cnt++;
		time2 = osal_get_msec();
		if ((time2 - time1) >= 3000)
        {
			DMS_INFO("algo process %lld fps\n", (frame_cnt * 1000) / (time2 - time1));
			time1 = time2;
			frame_cnt  = 0;
		}
#endif
#if 0
		nn_dms_cmd(nn_hdl, NN_DMS_GET_VERSION, dmslib_ver);
		EI_PRINT("dmslib_ver %s\n", dmslib_ver);
		if (axframe_info->m_nn_dms_cfg.warn_cfg.face_mask_cfg.warn_interval != 20) {
			axframe_info->m_nn_dms_cfg.warn_cfg.face_mask_cfg.warn_interval = 20;
			nn_dms_cmd(nn_hdl, NN_DMS_SET_PARAM, &axframe_info->m_nn_dms_cfg);
			memset(&nn_dms_config_test,0,sizeof(nn_dms_cfg_t));
			nn_dms_cmd(nn_hdl, NN_DMS_GET_PARAM, &nn_dms_config_test);
			EI_PRINT("get face_mask_cfg %f %f\n", nn_dms_config_test.warn_cfg.face_mask_cfg.warn_interval, nn_dms_config_test.warn_cfg.face_mask_cfg.time);
		}
#endif
        EI_MI_VISS_ReleaseChnFrame(axframe_info->dev, axframe_info->chn, &axFrame);
    }

    EI_MI_VISS_DisableChn(axframe_info->dev, axframe_info->chn);
	if (axframe_info->livingdet_flag == LIVING_DET_SINGLE_RGB ||
		axframe_info->livingdet_flag == LIVING_DET_SINGLE_NIR)
    {
		if (axframe_info->livingdet_hdl)
        {
			nna_livingdet_close(axframe_info->livingdet_hdl);
		}
	}
    nn_dms_close(nn_hdl);
    
    return NULL;
}

static int start_get_axframe(axframe_info_s *axframe_info)
{
    int ret;
    static VISS_EXT_CHN_ATTR_S stExtChnAttr;

    stExtChnAttr.s32BindChn = axframe_info->phyChn;
    stExtChnAttr.u32Depth = 1;
    stExtChnAttr.stSize.u32Width = axframe_info->u32Width;
    stExtChnAttr.stSize.u32Height = axframe_info->u32Height;
    stExtChnAttr.u32Align = 32;
    stExtChnAttr.enPixFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stExtChnAttr.stFrameRate.s32SrcFrameRate = axframe_info->frameRate;
    stExtChnAttr.stFrameRate.s32DstFrameRate = AL_SEND_FRAMERATE;
    stExtChnAttr.s32Position = 1;

    ret = EI_MI_VISS_SetExtChnAttr(axframe_info->dev, axframe_info->chn, &stExtChnAttr);
    if (ret != EI_SUCCESS)
    {
        DMS_ERR("EI_MI_VISS_SetExtChnAttr failed with %#x!\n", ret);
        return EI_FAILURE;
    }
    ret = EI_MI_VISS_EnableChn(axframe_info->dev, axframe_info->chn);
    if (ret != EI_SUCCESS)
    {
        DMS_ERR("EI_MI_VISS_EnableChn failed with %#x!\n", ret);
        return EI_FAILURE;
    }

    axframe_info->bThreadStart = EI_TRUE;
    ret = pthread_create(&(axframe_info->gs_framePid), NULL, get_axframe_proc, (EI_VOID *)axframe_info);
    if (ret)
        DMS_ERR("errno=%d, reason(%s)\n", errno, strerror(errno));
    DMS_DBG("start_get_axframe success %#x! phyChn:%d chn :%d dev:%d\n", ret, axframe_info->phyChn, axframe_info->chn, axframe_info->dev);

    return EI_SUCCESS;
}


int dms_warning(nn_dms_out_t *data)
{
	nn_dms_warn_status_t *warn_status = NULL;
	int64_t cur_time = osal_get_msec();
	uint32_t index;

	warn_status = &data->warn_status;

	if (data->faces.size > 0)
    {
		if (warn_status->call)
        {
			al_warning_proc(LB_WARNING_DMS_CALL, 0, 0);
		}
		if (warn_status->smoke)
        {
			al_warning_proc(LB_WARNING_DMS_SMOKE, 0, 0);
		}
		
		if (warn_status->fatigue_level == 2)
        {
			al_warning_proc(LB_WARNING_DMS_REST, warn_status->fatigue_level, 0);
		}
        else if (warn_status->fatigue_level == 4)
        {
			al_warning_proc(LB_WARNING_DMS_REST, warn_status->fatigue_level, 0);
		}
        else if (warn_status->fatigue_level == 6)
        {
			al_warning_proc(LB_WARNING_DMS_REST, warn_status->fatigue_level, 0);
		}
        else if (warn_status->fatigue_level == 8)
        {
			al_warning_proc(LB_WARNING_DMS_REST, warn_status->fatigue_level, 0);
		}
        else if (warn_status->fatigue_level == 10)
        {
			al_warning_proc(LB_WARNING_DMS_REST, warn_status->fatigue_level, 0);
		}
		if (warn_status->distracted)
        {
			al_warning_proc(LB_WARNING_DMS_ATTATION, 0, 0);
		}
		
		if (warn_status->red_resist_glasses)
        {
			al_warning_proc(LB_WARNING_DMS_INFRARED_BLOCK, 0, 0);
		}
	}
    else
    {
		if (warn_status->cover)
        {
			al_warning_proc(LB_WARNING_DMS_CAMERA_COVER, LB_WARNING_DMS_DRIVER_LEAVE, 0);
		}
		if (warn_status->driver_leave)
        {
			al_warning_proc(LB_WARNING_DMS_DRIVER_LEAVE, LB_WARNING_DMS_CAMERA_COVER, 0);
		}
	}
	return 0;
}

int dms_nn_init(int vissDev, int vissChn, int width, int height)
{
	axframe_info_s* axframe_info = &g_dms_axframe_info;

	dms_al_para_init();

    pthread_mutex_init(&axframe_info->p_det_mutex, NULL);

    axframe_info->p_det_info = malloc(sizeof(nn_dms_out_t));
    if (!axframe_info->p_det_info)
    {
		DMS_ERR("axframe_info->p_det_info malloc failed!\n");
		return -1;
    }
	if (axframe_info->action == DMS_CALIB)
    {
		m_dms_cali.cali_flag = DMS_CALI_START;
	}

    memset(axframe_info->p_det_info, 0x00, sizeof(nn_dms_out_t));

	if (axframe_info->livingdet_flag == LIVING_DET_SINGLE_RGB ||
		axframe_info->livingdet_flag == LIVING_DET_SINGLE_NIR)
    {
		axframe_info->livingdet_out = (ezax_rt_t *)calloc(1, sizeof(ezax_rt_t));
		if (!axframe_info->livingdet_out)
        {
			DMS_ERR("axframe_info->livingdet_out malloc failed!\n");
			return -1;
		}
		/* 活体的输出结果，live_score大于0为活体，小于0为假体*/
		axframe_info->livingdet_out->out_ex_inform = (ezax_livingdet_rt_t *)calloc(1, sizeof(ezax_livingdet_rt_t));
		if (!axframe_info->livingdet_out->out_ex_inform)
        {
			DMS_ERR("axframe_info->livingdet_out->out_ex_inform malloc failed!\n");
			return -1;
		}
		memset(&axframe_info->face, 0x00, sizeof(axframe_info->face));
		/* 人脸的关键点信息，先要识别到人脸，再做活体检测*/
		axframe_info->face.roi.in_ex_inform = (ezax_face_kpts_t *)calloc(1, sizeof(ezax_face_kpts_t));
		if (!axframe_info->face.roi.in_ex_inform)
        {
			DMS_ERR("axframe_info->face.roi.in_ex_inform malloc failed!\n");
			return -1;
		}
	}

    axframe_info->dev = vissDev;
    axframe_info->chn = VISS_MAX_PHY_CHN_NUM;
    axframe_info->phyChn = vissChn;
    axframe_info->frameRate = 25;
    axframe_info->u32Width = width;
    axframe_info->u32Height = height;
	axframe_info->ax_fout = NULL;
    sem_init(&axframe_info->frame_del_sem, 0, 0);

    start_get_axframe(axframe_info);

	return 0;
}

void dms_nn_stop(void)
{
    axframe_info_s *axframe_info = &g_dms_axframe_info;
	axframe_info->bThreadStart = EI_FALSE;
	usleep(200*1000);
}
