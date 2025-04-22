#include "algo_common.h"
#include "adas_config.h"

//#define AL_PRO_FRAMERATE_STATISTIC

static adasaxframe_info_s g_adas_axframe_info = { 0 };
static adas_calibrate_para_t m_adas_cali;

#define ADAS_DBG		//EI_PRINT
#define ADAS_INFO	    EI_PRINT
#define ADAS_ERR		EI_PRINT

extern pool_info_t pool_info[VBUF_POOL_MAX_NUM];
int gCurrentSpeed = 0;

int adas_warning(nn_adas_out_t *data)
{
	int64_t cur_time = osal_get_msec();
	int   status, index;
    adasaxframe_info_s* axframe_info = &g_adas_axframe_info;

	if (axframe_info->action == ADAS_CALIB)
    {
		return 0;
	}

	if (data->cars.size > 0)
    {
		for (int cnt_cars = 0; cnt_cars < data->cars.size; cnt_cars++)
        {
			status = data->cars.p[cnt_cars].status;
			ADAS_DBG("cnt_cars %d cars status:%d\n", cnt_cars, status);
			if (status == WARN_STATUS_FCW)
            {
			    al_warning_proc(LB_WARNING_ADAS_COLLIDE, data->cars.p[cnt_cars].dist, data->cars.p[cnt_cars].relative_speed);
 			}
            else if (status >= WARN_STATUS_HMW_LEVEL1 && status <= WARN_STATUS_HMW_LEVEL4)
            {
			    al_warning_proc(LB_WARNING_ADAS_DISTANCE, data->cars.p[cnt_cars].dist, data->cars.p[cnt_cars].relative_speed);
			}
            else if (status == WARN_STATUS_FMW)
            {
			    al_warning_proc(LB_WARNING_ADAS_LAUNCH, data->cars.p[cnt_cars].dist, data->cars.p[cnt_cars].relative_speed);
			}
		}
	}
	if (data->peds.size > 0)
    {
		for (int cnt_peds = 0; cnt_peds < data->peds.size; cnt_peds++)
        {
			status = data->peds.p[cnt_peds].status;
			ADAS_DBG("cnt_peds %d peds status:%d\n", cnt_peds, status);
			if (status)
            {
			    al_warning_proc(LB_WARNING_ADAS_PEDS, data->peds.p[cnt_peds].dist, data->peds.p[cnt_peds].relative_speed);
			}
		}
	}

	for (int cnt_lanes = 0; cnt_lanes < 2; cnt_lanes++)
    {
		status = data->lanes.lanes[cnt_lanes].status;
		ADAS_DBG("cnt_lanes %d lanes status:%d\n", cnt_lanes, status);
		if (status == WARN_STATUS_PRESS_LANE)
        {
			al_warning_proc(LB_WARNING_ADAS_PRESSURE, cnt_lanes, 0);
		}
		if (status == WARN_STATUS_CHANGING_LANE)
        {
			al_warning_proc(LB_WARNING_ADAS_CHANGLANE, cnt_lanes, 0);
		}
	}

	return 0;
}

int nn_adas_start(VIDEO_FRAME_INFO_S *frame, void *nn_hdl)
{
    adasaxframe_info_s* axframe_info = &g_adas_axframe_info;

    EI_S32 s32Ret;
    static nn_adas_in_t m_adas_in;
	int pool_idx = 0;
	for (pool_idx = 0; pool_idx < VBUF_POOL_MAX_NUM; pool_idx++) {
		if (frame->u32PoolId == pool_info[pool_idx].Pool) {
			break;
		}
	}
	if (pool_idx >= VBUF_POOL_MAX_NUM)
		return 0;
	/* 获取保存的映射虚拟地址 */
    m_adas_in.img.y_phy = (void *)frame->stVFrame.u64PlanePhyAddr[0];
    m_adas_in.img.y = (void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[0];
    m_adas_in.img.uv_phy = (void *)frame->stVFrame.u64PlanePhyAddr[1];
    m_adas_in.img.uv = (void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[1];
    m_adas_in.img.width = 1920;
    m_adas_in.img.height = 1080;
    m_adas_in.img.format = YUV420_SP;
    m_adas_in.gps = gCurrentSpeed;
    m_adas_in.carped_en = 1;
    m_adas_in.lane_en = 1;
	m_adas_in.cb_param = NULL;

    if (nn_hdl != NULL) {
        s32Ret = nn_adas_cmd(nn_hdl, NN_ADAS_SET_DATA, &m_adas_in);
		if (s32Ret < 0) {
			EI_PRINT("NN_ADAS_SET_DATA failed %d!\n", s32Ret);
		}
    }

    sem_wait(&axframe_info->frame_del_sem);

    return 0;
}

static int nn_adas_result_cb(int event, void *data)
{
    adasaxframe_info_s* axframe_info = &g_adas_axframe_info;

	if (event == NN_ADAS_DET_DONE)
    {
		nn_adas_out_t *curr_data = (nn_adas_out_t *)data;
    	int i;
		pthread_mutex_lock(&axframe_info->p_det_mutex);
	    /* cars data */
	    if (axframe_info->p_det_info->cars.p)
        {
	        free(axframe_info->p_det_info->cars.p);
	        axframe_info->p_det_info->cars.p = NULL;
	        axframe_info->p_det_info->cars.size = 0;
	    }
	    axframe_info->p_det_info->cars.p = malloc(curr_data->cars.size * sizeof(nn_adas_car_t));
	    if (axframe_info->p_det_info->cars.p)
        {
	        memcpy(axframe_info->p_det_info->cars.p, curr_data->cars.p, curr_data->cars.size * sizeof(nn_adas_car_t));
	        axframe_info->p_det_info->cars.size = curr_data->cars.size;
	    }

	    /* peds data */
	    if (axframe_info->p_det_info->peds.p)
        {
	        free(axframe_info->p_det_info->peds.p);
	        axframe_info->p_det_info->peds.p = NULL;
	        axframe_info->p_det_info->peds.size = 0;
	    }
	    axframe_info->p_det_info->peds.p = malloc(curr_data->peds.size * sizeof(nn_adas_ped_t));
	    if (axframe_info->p_det_info->peds.p)
        {
	        memcpy(axframe_info->p_det_info->peds.p, curr_data->peds.p, curr_data->peds.size * sizeof(nn_adas_ped_t));
	        axframe_info->p_det_info->peds.size = curr_data->peds.size;
	    }
	    /* lanes data */
	    memcpy(&axframe_info->p_det_info->lanes, &curr_data->lanes, sizeof(nn_adas_lanes_t));
		pthread_mutex_unlock(&axframe_info->p_det_mutex);
		if (axframe_info->action != ADAS_CALIB)
        {
		    if (curr_data->cars.size)
            {
		        ADAS_DBG("nn_adas_result_cb curr_data->cars.size = %d\n", curr_data->cars.size);
		        for (i=0; i<curr_data->cars.size; i++)
                {
		            ADAS_DBG("curr_data->cars.p[%d].status = %d\n", i, curr_data->cars.p[i].status);
		        }
		    }
		    if (curr_data->peds.size)
            {
		        ADAS_DBG("nn_adas_result_cb curr_data->peds.size = %d\n", curr_data->peds.size);
		        for (i=0; i<curr_data->peds.size; i++)
                {
		            ADAS_DBG("curr_data->peds.p[%d].status = %d\n", i, curr_data->peds.p[i].status);
		        }
		    }
		    if (curr_data->lanes.lanes[0].exist)
		        ADAS_DBG("nn_adas_result_cb curr_data->lanes.lanes[0].exist = %d\n", curr_data->lanes.lanes[0].exist);
		    if (curr_data->lanes.lanes[1].exist)
		        ADAS_DBG("nn_adas_result_cb curr_data->lanes.lanes[1].exist = %d\n", curr_data->lanes.lanes[1].exist);
		    if (curr_data->lanes.lanes[0].status)
		        ADAS_DBG("nn_adas_result_cb curr_data->lanes.lanes[0].exist = %d\n", curr_data->lanes.lanes[0].status);
		    if (curr_data->lanes.lanes[1].status)
		        ADAS_DBG("nn_adas_result_cb curr_data->lanes.lanes[1].exist = %d\n", curr_data->lanes.lanes[1].status);
		}
		adas_warning(curr_data);
		sem_post(&axframe_info->frame_del_sem);
    }
    else if (event == NN_ADAS_SELF_CALIBRATE)
    {
    	ADAS_DBG("NN_ADAS_SELF_CALIBRATE\n");
		nn_adas_cfg_t *curr_data = (nn_adas_cfg_t *)data;
		ADAS_DBG("curr_data->camera_param.vanish_point: %d %d %d %f\n",
			curr_data->camera_param.vanish_point[0], curr_data->camera_param.vanish_point[1],
			curr_data->camera_param.bottom_line, curr_data->camera_param.camera_to_middle);

		gAdasAlgoParams->vanish_point_x = curr_data->camera_param.vanish_point[0];
		gAdasAlgoParams->vanish_point_y = curr_data->camera_param.vanish_point[1];
		gAdasAlgoParams->bottom_line_y = curr_data->camera_param.bottom_line;
		
        if (!m_adas_cali.auto_calib_flag)
        { /*only warning for first time */
			//al_warning_msg(LB_WARNING_ADAS_CALIBRATE_SUCCESS); //osal_mq_send(warning_info.mq_id, LB_WARNING_ADAS_CALIBRATE_SUCCESS, NULL, 0);
		}
		m_adas_cali.auto_calib_flag = 1;
		/* save calibrate param used for next power on */
	}
    
    return 0;
}

static EI_VOID *get_axframe_proc(EI_VOID *p)
{
    int ret;
    VIDEO_FRAME_INFO_S axFrame = {0};
    adasaxframe_info_s *axframe_info = &g_adas_axframe_info;
    void *nn_hdl = NULL;
	char adaslib_ver[64]={0};

#ifdef AL_PRO_FRAMERATE_STATISTIC
	int64_t time1 = 0LL, time2 = 0LL, time_GetChnFrame1 = 0LL, time_GetChnFrame2 = 0LL,
	time_AlProFrame1 = 0LL, time_AlProFrame2 = 0LL;
	int64_t frame_cnt = 0;
#endif
    ADAS_DBG("********start_nn_adas******\n");
    memset(&axframe_info->m_nn_adas_cfg, 0, sizeof(axframe_info->m_nn_adas_cfg));
    /*init nn_adas, cfg*/
    axframe_info->m_nn_adas_cfg.img_width = axframe_info->u32Width; //算法输入图像宽
    axframe_info->m_nn_adas_cfg.img_height = axframe_info->u32Height; //算法输入图像高
    axframe_info->m_nn_adas_cfg.interest_box[0] = 0; //算法检测区域左上角X坐标
    axframe_info->m_nn_adas_cfg.interest_box[1] = 0; //算法检测区域左上角y坐标
    axframe_info->m_nn_adas_cfg.interest_box[2] = axframe_info->u32Width-1; //算法检测区域右下角x坐标
    axframe_info->m_nn_adas_cfg.interest_box[3] = axframe_info->u32Height-1; //算法检测区域右下角y坐标
    axframe_info->m_nn_adas_cfg.format = YUV420_SP;
    axframe_info->m_nn_adas_cfg.model_basedir = NN_ADAS_PATH;
    axframe_info->m_nn_adas_cfg.cb_func = nn_adas_result_cb;
    axframe_info->m_nn_adas_cfg.camera_param.vanish_point[0] = gAdasAlgoParams->vanish_point_x; //天际线和车中线交叉点X坐标 单位像素
    axframe_info->m_nn_adas_cfg.camera_param.vanish_point[1] = gAdasAlgoParams->vanish_point_y; //天际线和车中线交叉点Y坐标 单位像素
    axframe_info->m_nn_adas_cfg.camera_param.bottom_line = gAdasAlgoParams->bottom_line_y; //引擎盖线
    axframe_info->m_nn_adas_cfg.camera_param.camera_angle[0] = 80; //摄像头水平角度
    axframe_info->m_nn_adas_cfg.camera_param.camera_angle[1] = 50; //摄像头垂直角度
    axframe_info->m_nn_adas_cfg.camera_param.camera_to_ground = (float)gAdasAlgoParams->camera_to_ground/100.0; //与地面距离，单位米
    axframe_info->m_nn_adas_cfg.camera_param.car_head_to_camera= (float)gAdasAlgoParams->car_head_to_camera/100.0; //与车头距离 单位米
    axframe_info->m_nn_adas_cfg.camera_param.car_width= (float)gAdasAlgoParams->car_width/100.0; //车宽，单位米
    axframe_info->m_nn_adas_cfg.camera_param.camera_to_middle= (float)gAdasAlgoParams->camera_to_middle/100.0; //与车中线距离，单位米，建议设备安装在车的中间，左负右正
	axframe_info->m_nn_adas_cfg.calib_param.auto_calib_en = gAdasAlgoParams->auto_calib_en;//自动校准使能
	axframe_info->m_nn_adas_cfg.calib_param.calib_finish = gAdasAlgoParams->calib_finish;//设置为1，在自动校准前也会按默认参数报警, 设置为0，则在自动校准前不报警
 
	axframe_info->m_nn_adas_cfg.ldw_en = (gAdasAlgoParams->ldw_en>0?1:0);//车道偏移和变道报警
	axframe_info->m_nn_adas_cfg.hmw_en = (gAdasAlgoParams->hmw_en>0?1:0);//保持车距报警
	axframe_info->m_nn_adas_cfg.ufcw_en = 0;//低速下前车碰撞报警，reserved备用
	axframe_info->m_nn_adas_cfg.fcw_en = (gAdasAlgoParams->fcw_en>0?1:0);//前车碰撞报警
	axframe_info->m_nn_adas_cfg.pcw_en = (gAdasAlgoParams->pcw_en>0?1:0);//行人碰撞报警
	axframe_info->m_nn_adas_cfg.fmw_en = (gAdasAlgoParams->fmw_en>0?1:0);//前车启动报警，必须有GPS，且本车车速为0

	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.min_warn_dist = 0;//最小报警距离，单位米
	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.max_warn_dist = 100;//最大报警距离，单位米
	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.min_warn_gps = gAdasAlgoParams->hmw_speed;//最低报警车速，单位KM/H
	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.max_warn_gps = 200;//最高报警车速，单位KM/H
	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.gps_ttc_thresh[0] = 1.2f;//与前车碰撞小于这个时间报警(假设前车不动)，单位秒,距离/本车GPS速度，对应WARN_STATUS_HMW_LEVEL1报警状态
	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.gps_ttc_thresh[1] = 0.6f;//与前车碰撞小于这个时间报警(假设前车不动)，单位秒,距离/本车GPS速度，对应WARN_STATUS_HMW_LEVEL2报警状态

	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.min_warn_dist = 0;
	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.max_warn_dist = 100;
	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.min_warn_gps = gAdasAlgoParams->fcw_speed;
	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.max_warn_gps = 200;
	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.gps_ttc_thresh = 2.0f; //与前车碰撞报警时间(假设前车不动)，单位秒,距离/本车GPS速度
	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.ttc_thresh = 2.5f; //与前车碰撞报警时间，单位秒,距离/相对车速，以上4个条件同时满足

	axframe_info->m_nn_adas_cfg.warn_param.ufcw_param.min_warn_dist = 0;
	axframe_info->m_nn_adas_cfg.warn_param.ufcw_param.max_warn_dist = 100;
	axframe_info->m_nn_adas_cfg.warn_param.ufcw_param.min_warn_gps = 0;
	axframe_info->m_nn_adas_cfg.warn_param.ufcw_param.max_warn_gps = 30;
	axframe_info->m_nn_adas_cfg.warn_param.ufcw_param.relative_speed_thresh = -3.0f; //与前车碰撞相对车速，前车车速-本车车速小于这个速度

	axframe_info->m_nn_adas_cfg.warn_param.fmw_param.min_static_time = 5.0f;//前车静止的时间需大于多少，单位秒
	axframe_info->m_nn_adas_cfg.warn_param.fmw_param.warn_car_inside_dist = 6.0f;//与前车的距离在多少米内，单位米
	axframe_info->m_nn_adas_cfg.warn_param.fmw_param.warn_dist_leave = 0.6f;//前车启动大于多远后报警，单位米

	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.min_warn_dist = 0;
	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.max_warn_dist = 100;
	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.min_warn_gps = gAdasAlgoParams->pcw_speed;
	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.max_warn_gps = 200;
	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.gps_ttc_thresh = 1.5f;
	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.ttc_thresh = 5.0f;

	axframe_info->m_nn_adas_cfg.warn_param.ldw_param.min_warn_gps = gAdasAlgoParams->ldw_speed;//最低报警车速，单位KM/H
	axframe_info->m_nn_adas_cfg.warn_param.ldw_param.ldw_warn_thresh = 0.5f; //在lane_warn_dist距离内持续时间，单位秒
	axframe_info->m_nn_adas_cfg.warn_param.ldw_param.lane_warn_dist = 0.0f;//与车道线距离，车道线内
	axframe_info->m_nn_adas_cfg.warn_param.ldw_param.change_lane_dist = 0.6f; //变道侧车道线离车的中点距离，单位米

    /* open libadas.so*/
    nn_hdl = nn_adas_open(&axframe_info->m_nn_adas_cfg);
    if (nn_hdl == NULL)
    {
        ADAS_ERR("nn_adas_open() failed!");
        return NULL;
    }

	axframe_info->nn_hdl = nn_hdl;
#ifdef AL_PRO_FRAMERATE_STATISTIC
	time1 = osal_get_msec();
#endif

    while (EI_TRUE == axframe_info->bThreadStart)
    {
        memset(&axFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time_GetChnFrame1 = osal_get_msec();
#endif
        //ret = EI_MI_VISS_GetChnFrame(axframe_info->dev, axframe_info->chn, &axFrame, 1000);
        ret = EI_MI_IPPU_GetChnFrame(axframe_info->dev, axframe_info->chn, &axFrame, 1000);
        if (ret != EI_SUCCESS)
        {
            ADAS_ERR("chn %d get frame error 0x%x\n", axframe_info->chn, ret);
            usleep(100 * 1000);
            continue;
        }
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time_GetChnFrame2 = osal_get_msec();
#endif
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time_AlProFrame1 = osal_get_msec();
#endif
        nn_adas_start(&axFrame, nn_hdl);
#ifdef AL_PRO_FRAMERATE_STATISTIC
	time_AlProFrame2 = osal_get_msec();
	ADAS_DBG("time_GetChnFrame %lld ms, time_AlProFrame %lld ms\n",
		time_GetChnFrame2-time_GetChnFrame1, time_AlProFrame2-time_AlProFrame1);
#endif
#ifdef AL_PRO_FRAMERATE_STATISTIC
		frame_cnt++;
		time2 = osal_get_msec();
		if ((time2 - time1) >= 3000)
        {
			ADAS_ERR("algo process %lld fps\n", (frame_cnt * 1000) / (time2 - time1));
			time1 = time2;
			frame_cnt  = 0;
		}
#endif
		nn_adas_cmd(nn_hdl, NN_ADAS_GET_VERSION, adaslib_ver);

        //EI_MI_VISS_ReleaseChnFrame(axframe_info->dev, axframe_info->chn, &axFrame);
        EI_MI_IPPU_ReleaseChnFrame(axframe_info->dev, axframe_info->chn, &axFrame);
    }

//    EI_MI_VISS_DisableChn(axframe_info->dev, axframe_info->chn);

    nn_adas_close(nn_hdl);

    return NULL;
}

static int start_get_axframe(adasaxframe_info_s *axframe_info)
{
    int ret;
#if 0
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
        ADAS_ERR("EI_MI_VISS_SetExtChnAttr failed with %#x!\n", ret);
        return EI_FAILURE;
    }
    ret = EI_MI_VISS_EnableChn(axframe_info->dev, axframe_info->chn);
    if (ret != EI_SUCCESS)
    {
        ADAS_ERR("EI_MI_VISS_EnableChn failed with %#x!\n", ret);
        return EI_FAILURE;
    }
#endif
    axframe_info->bThreadStart = EI_TRUE;
    ret = pthread_create(&(axframe_info->gs_framePid), NULL, get_axframe_proc, (EI_VOID *)axframe_info);
    if (ret)
        ADAS_ERR("errno=%d, reason(%s)\n", errno, strerror(errno));
    ADAS_DBG("start_get_axframe success %#x! phyChn:%d chn :%d dev:%d\n", ret, axframe_info->phyChn, axframe_info->chn, axframe_info->dev);

    return EI_SUCCESS;
}

int adas_nn_init(int vissDev, int vissChn, int width, int height)
{
    adasaxframe_info_s *axframe_info = &g_adas_axframe_info;

	adas_al_para_init();

    pthread_mutex_init(&axframe_info->p_det_mutex, NULL);
    axframe_info->p_det_info = malloc(sizeof(nn_adas_out_t));
    if (!axframe_info->p_det_info)
    {
		ADAS_ERR("axframe_info->p_det_info malloc failed!\n");
		return -1;
    }
	if (axframe_info->action == ADAS_CALIB)
    {
		m_adas_cali.cali_flag = ADAS_CALI_START;
		m_adas_cali.vanish_point_x = gAdasAlgoParams->vanish_point_x;
		m_adas_cali.vanish_point_y = gAdasAlgoParams->vanish_point_y;
		m_adas_cali.bottom_line_y = gAdasAlgoParams->bottom_line_y;
		m_adas_cali.adas_calibline = VANISH_Y_LINE;
		m_adas_cali.adas_calibstep = 5;
		
		ADAS_INFO("m_adas_cali.vanish_point_x %f %f %f\n", m_adas_cali.vanish_point_x, m_adas_cali.vanish_point_y, m_adas_cali.bottom_line_y);
	}

    memset(axframe_info->p_det_info, 0x00, sizeof(nn_adas_out_t));
    axframe_info->dev = 0;//vissDev;
    axframe_info->chn = 1;//VISS_MAX_PHY_CHN_NUM;
    axframe_info->phyChn = vissChn;
    axframe_info->frameRate = 25;
    axframe_info->u32Width = width;
    axframe_info->u32Height = height;
	axframe_info->ax_fout = NULL;
    sem_init(&axframe_info->frame_del_sem, 0, 0);

    start_get_axframe(axframe_info);

    return 0;
}

void adas_nn_stop(void)
{
    adasaxframe_info_s *axframe_info = &g_adas_axframe_info;
	axframe_info->bThreadStart = EI_FALSE;
	usleep(200*1000);
}

