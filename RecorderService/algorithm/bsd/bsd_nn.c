
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "pnt_log.h"
//#include "bsd_config.h"
#include "bsd_nn.h"

bsd_nn_t gBsdNN = { 0 };
bsd_calibrate_para_t m_bsd_cali;

static EI_S32 rgn_start(void)
{
	EI_S32 s32Ret = EI_FAILURE;

	gBsdNN.stRgnChn.enModId = EI_ID_VISS;
	gBsdNN.stRgnChn.s32DevId = gBsdNN.dev;
	gBsdNN.stRgnChn.s32ChnId = gBsdNN.chn - VISS_MAX_PHY_CHN_NUM;
	gBsdNN.Handle[0] = 10;
    gBsdNN.stRegion[0].enType = RECTANGLEEX_ARRAY_RGN;
    gBsdNN.stRegion[0].unAttr.u32RectExArrayMaxNum = BSD_RGN_RECT_NUM;
	
    s32Ret = EI_MI_RGN_Create(gBsdNN.Handle[0], &gBsdNN.stRegion[0]);
    if(s32Ret)
	{
        PNT_LOGE("EI_MI_RGN_Create \n");
        goto exit;
    }
	gBsdNN.Handle[1] = 11;
    gBsdNN.stRegion[1].enType = LINEEX_ARRAY_RGN;
    gBsdNN.stRegion[1].unAttr.u32LineExArrayMaxNum = BSD_RGN_LINE_NUM;
    s32Ret = EI_MI_RGN_Create(gBsdNN.Handle[1], &gBsdNN.stRegion[1]);
    if(s32Ret)
	{
        PNT_LOGE("EI_MI_RGN_Create \n");
        goto exit;
    }
	gBsdNN.stRgnChnAttr[0].bShow = EI_TRUE;
    gBsdNN.stRgnChnAttr[0].enType = RECTANGLEEX_ARRAY_RGN;
    gBsdNN.stRgnChnAttr[0].unChnAttr.stRectangleExArrayChn.u32MaxRectNum = BSD_RGN_RECT_NUM;
    gBsdNN.stRgnChnAttr[0].unChnAttr.stRectangleExArrayChn.u32ValidRectNum = BSD_RGN_RECT_NUM;
    gBsdNN.stRgnChnAttr[0].unChnAttr.stRectangleExArrayChn.pstRectExArraySubAttr = gBsdNN.stRectArray;
//    s32Ret = EI_MI_RGN_AddToChn(gBsdNN.Handle[0], &gBsdNN.stRgnChn, &gBsdNN.stRgnChnAttr[0]);
//	if(s32Ret)
//	{
//        PNT_LOGE("EI_MI_RGN_AddToChn \n");
//    }
	gBsdNN.stRgnChnAttr[1].bShow = EI_TRUE;
    gBsdNN.stRgnChnAttr[1].enType = LINEEX_ARRAY_RGN;
    gBsdNN.stRgnChnAttr[1].unChnAttr.stLineExArrayChn.u32MaxLineNum = BSD_RGN_LINE_NUM;
    gBsdNN.stRgnChnAttr[1].unChnAttr.stLineExArrayChn.u32ValidLineNum = BSD_RGN_LINE_NUM;
    gBsdNN.stRgnChnAttr[1].unChnAttr.stLineExArrayChn.pstLineExArraySubAttr = gBsdNN.stLineArray;
//    s32Ret = EI_MI_RGN_AddToChn(gBsdNN.Handle[1], &gBsdNN.stRgnChn, &gBsdNN.stRgnChnAttr[1]);
//	if(s32Ret)
//	{
//        PNT_LOGE("EI_MI_RGN_AddToChn \n");
//    }

	return EI_SUCCESS;
exit:
	return s32Ret;
}

static void rgn_stop(void)
{
	int i;

	for (i = 0;i < BSD_RGN_ARRAY_NUM; i++)
	{
		EI_MI_RGN_DelFromChn(gBsdNN.Handle[i], &gBsdNN.stRgnChn);
		EI_MI_RGN_Destroy(gBsdNN.Handle[i]);
	}
}

static void bsd_draw_location_rgn(nn_bsd_out_t *p_det_info)
{
	int draw_rect_num, draw_line0_num, draw_line1_num;
	int index = 0;
	int loc_x0, loc_y0, loc_x1, loc_y1;
	EI_S32 s32Ret = EI_FAILURE;

	m_bsd_cali.vanish_point_x = gBsdAlgoParams->vanish_point_x;
	m_bsd_cali.vanish_point_y = gBsdAlgoParams->vanish_point_y;
	m_bsd_cali.bottom_line_y = gBsdAlgoParams->bottom_line_y;

	if (gBsdNN.action == BSD_CALIB)
	{
		for (int i = 2; i < 5; i++)
		{
			gBsdNN.stRgnChnAttr[1].bShow = EI_TRUE;
			gBsdNN.stLineArray[i].isShow = EI_TRUE;
			if (i == VANISH_Y_LINE)
			{
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[0].s32X = 0;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[0].s32Y = m_bsd_cali.vanish_point_y;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[1].s32X = gBsdNN.u32scnWidth;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[1].s32Y = m_bsd_cali.vanish_point_y;
		        gBsdNN.stLineArray[i].stLineAttr.u32Color = 0xffff0000;
			}
			else if (i == VANISH_X_LINE)
			{
				gBsdNN.stLineArray[i].stLineAttr.stPoints[0].s32X = m_bsd_cali.vanish_point_x;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[0].s32Y = 0;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[1].s32X = m_bsd_cali.vanish_point_x;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[1].s32Y = gBsdNN.u32scnHeight;
		        gBsdNN.stLineArray[i].stLineAttr.u32Color = 0xff00ff00;
			}
			else if (i == BOTTOM_Y_LINE)
			{
				gBsdNN.stLineArray[i].stLineAttr.stPoints[0].s32X = 0;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[0].s32Y = m_bsd_cali.bottom_line_y;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[1].s32X = gBsdNN.u32scnWidth;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[1].s32Y = m_bsd_cali.bottom_line_y;
		        gBsdNN.stLineArray[i].stLineAttr.u32Color = 0xff8000ff;
			}
			if (i==m_bsd_cali.bsd_calibline)
				gBsdNN.stLineArray[i].stLineAttr.u32LineSize = 8;
			else
	        	gBsdNN.stLineArray[i].stLineAttr.u32LineSize = 4;
			s32Ret = EI_MI_RGN_SetChnAttr(gBsdNN.Handle[1], &gBsdNN.stRgnChn, &gBsdNN.stRgnChnAttr[1]);
			if(s32Ret) {
				PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
	    	}
    	}
	} 
	else
	{
		draw_rect_num = p_det_info->cars.size + p_det_info->peds.size;
		draw_line0_num = p_det_info->lanes.lanes[0].exist;
		draw_line1_num = p_det_info->lanes.lanes[1].exist;
		if (draw_rect_num > BSD_RGN_RECT_NUM || (draw_line0_num+draw_line1_num) > BSD_RGN_LINE_NUM)
			return;

		if (gBsdNN.last_draw_rect_num > draw_rect_num)
		{
			for (int i = gBsdNN.last_draw_rect_num-1; i >= draw_rect_num; i--)
			{
				gBsdNN.stRectArray[i].isShow = EI_FALSE;
				s32Ret = EI_MI_RGN_SetChnAttr(gBsdNN.Handle[0], &gBsdNN.stRgnChn, &gBsdNN.stRgnChnAttr[0]);
				if(s32Ret)
				{
					PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
	    		}
			}
		}
		if (gBsdNN.last_draw_line0_num > draw_line0_num)
		{
				gBsdNN.stLineArray[0].isShow = EI_FALSE;
				s32Ret = EI_MI_RGN_SetChnAttr(gBsdNN.Handle[1], &gBsdNN.stRgnChn, &gBsdNN.stRgnChnAttr[1]);
				if(s32Ret)
				{
					PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
	    		}
		}
		if (gBsdNN.last_draw_line1_num > draw_line1_num)
		{
				gBsdNN.stLineArray[1].isShow = EI_FALSE;
				s32Ret = EI_MI_RGN_SetChnAttr(gBsdNN.Handle[1], &gBsdNN.stRgnChn, &gBsdNN.stRgnChnAttr[1]);
				if(s32Ret)
				{
					PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
	    		}
		}
	    if (p_det_info->cars.size)
		{
			for (int cnt_cars = 0; cnt_cars < p_det_info->cars.size; cnt_cars++)
			{
		        loc_x0 = p_det_info->cars.p[cnt_cars].box[0];
		        loc_y0 = p_det_info->cars.p[cnt_cars].box[1];
		        loc_x1 = p_det_info->cars.p[cnt_cars].box[2];
		        loc_y1 = p_det_info->cars.p[cnt_cars].box[3];
		        gBsdNN.stRectArray[cnt_cars].stRectAttr.stRect.s32X = loc_x0;
				gBsdNN.stRectArray[cnt_cars].stRectAttr.stRect.s32Y = loc_y0;
				gBsdNN.stRectArray[cnt_cars].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
				gBsdNN.stRectArray[cnt_cars].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
				gBsdNN.stRectArray[cnt_cars].stRectAttr.u32BorderSize = 4;
				if (p_det_info->cars.p[cnt_cars].region <= 1)
					gBsdNN.stRectArray[cnt_cars].stRectAttr.u32Color = 0xffffff00;
				else
					gBsdNN.stRectArray[cnt_cars].stRectAttr.u32Color = 0xff00ff00;
				gBsdNN.stRectArray[cnt_cars].isShow = EI_TRUE;
				gBsdNN.stRgnChnAttr[0].bShow = EI_TRUE;
				s32Ret = EI_MI_RGN_SetChnAttr(gBsdNN.Handle[0], &gBsdNN.stRgnChn, &gBsdNN.stRgnChnAttr[0]);
				if(s32Ret)
				{
					PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
	    		}
			}
	    }
		index = index + p_det_info->cars.size;
	    for (int cnt_peds = 0; cnt_peds < p_det_info->peds.size; cnt_peds++)
		{
	        loc_x0 = p_det_info->peds.p[cnt_peds].box[0];
	        loc_y0 = p_det_info->peds.p[cnt_peds].box[1];
	        loc_x1 = p_det_info->peds.p[cnt_peds].box[2];
	        loc_y1 = p_det_info->peds.p[cnt_peds].box[3];
	        gBsdNN.stRectArray[index+cnt_peds].stRectAttr.stRect.s32X = loc_x0;
			gBsdNN.stRectArray[index+cnt_peds].stRectAttr.stRect.s32Y = loc_y0;
			gBsdNN.stRectArray[index+cnt_peds].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
			gBsdNN.stRectArray[index+cnt_peds].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
			gBsdNN.stRectArray[index+cnt_peds].stRectAttr.u32BorderSize = 4;
			gBsdNN.stRectArray[index+cnt_peds].stRectAttr.u32Color = 0xffff0000;
			gBsdNN.stRectArray[index+cnt_peds].isShow = EI_TRUE;
			gBsdNN.stRgnChnAttr[0].bShow = EI_TRUE;
			s32Ret = EI_MI_RGN_SetChnAttr(gBsdNN.Handle[0], &gBsdNN.stRgnChn, &gBsdNN.stRgnChnAttr[0]);
			if(s32Ret)
			{
				PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
	    	}
	    }
		gBsdNN.last_draw_rect_num = draw_rect_num;
	    for (int i = 0; i < 2; i++)
		{
	        if (p_det_info->lanes.lanes[i].exist)
			{
				gBsdNN.stRgnChnAttr[1].bShow = EI_TRUE;
				gBsdNN.stLineArray[i].isShow = EI_TRUE;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[0].s32X = p_det_info->lanes.lanes[i].pts[0][0];
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[0].s32Y = p_det_info->lanes.lanes[i].pts[0][1];
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[1].s32X = p_det_info->lanes.lanes[i].pts[5][0];
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[1].s32Y = p_det_info->lanes.lanes[i].pts[5][1];
		        gBsdNN.stLineArray[i].stLineAttr.u32Color = 0xffffff80;
		        gBsdNN.stLineArray[i].stLineAttr.u32LineSize = 4;
				s32Ret = EI_MI_RGN_SetChnAttr(gBsdNN.Handle[1], &gBsdNN.stRgnChn, &gBsdNN.stRgnChnAttr[1]);
				if(s32Ret)
				{
					PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
	    		}
	        }
    	}
		gBsdNN.last_draw_line0_num = draw_line0_num;
		gBsdNN.last_draw_line1_num = draw_line1_num;
		for (int i = 2; i < 5; i++)
		{
			gBsdNN.stRgnChnAttr[1].bShow = EI_TRUE;
			gBsdNN.stLineArray[i].isShow = EI_TRUE;
			if (i == VANISH_Y_LINE)
			{
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[0].s32X = 0;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[0].s32Y = m_bsd_cali.vanish_point_y;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[1].s32X = gBsdNN.u32scnWidth;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[1].s32Y = m_bsd_cali.vanish_point_y;
		        gBsdNN.stLineArray[i].stLineAttr.u32Color = 0xffff0000;
			}
			else if (i == VANISH_X_LINE)
			{
				gBsdNN.stLineArray[i].stLineAttr.stPoints[0].s32X = m_bsd_cali.vanish_point_x;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[0].s32Y = 0;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[1].s32X = m_bsd_cali.vanish_point_x;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[1].s32Y = gBsdNN.u32scnHeight;
		        gBsdNN.stLineArray[i].stLineAttr.u32Color = 0xff00ff00;
			}
			else if (i == BOTTOM_Y_LINE)
			{
				gBsdNN.stLineArray[i].stLineAttr.stPoints[0].s32X = 0;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[0].s32Y = m_bsd_cali.bottom_line_y;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[1].s32X = gBsdNN.u32scnWidth;
		        gBsdNN.stLineArray[i].stLineAttr.stPoints[1].s32Y = m_bsd_cali.bottom_line_y;
		        gBsdNN.stLineArray[i].stLineAttr.u32Color = 0xff8000ff;
			}
	        gBsdNN.stLineArray[i].stLineAttr.u32LineSize = 4;
			s32Ret = EI_MI_RGN_SetChnAttr(gBsdNN.Handle[1], &gBsdNN.stRgnChn, &gBsdNN.stRgnChnAttr[1]);
			if(s32Ret)
			{
				PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
    		}
		}
	}
}

int bsd_update_para(float vanish_point_x, float vanish_point_y, float bottom_line_y)
{
	int ret = -1;
	if (gBsdNN.nn_hdl == NULL)
	{
		return ret;
	}
	gBsdNN.m_nn_bsd_cfg.camera_param.vanish_point[0] = vanish_point_x;
	gBsdNN.m_nn_bsd_cfg.camera_param.vanish_point[1] = vanish_point_y;
	gBsdNN.m_nn_bsd_cfg.camera_param.bottom_line = bottom_line_y;
	ret = nn_bsd_cmd(gBsdNN.nn_hdl, NN_BSD_SET_PARAM, &gBsdNN.m_nn_bsd_cfg);

	return ret;
}

int nn_bsd_start(VIDEO_FRAME_INFO_S *frame, void *nn_hdl)
{
    EI_S32 s32Ret;
    static nn_bsd_in_t m_bsd_in;
	int pool_idx = 0;
	for (pool_idx = 0; pool_idx < VBUF_POOL_MAX_NUM; pool_idx++)
	{
		if (frame->u32PoolId == gAlComParam.pool_info[pool_idx].Pool)
		{
			break;
		}
	}
	if (pool_idx >= VBUF_POOL_MAX_NUM)
		return 0;
	/* 获取保存的映射虚拟地址 */
    m_bsd_in.img.y_phy = (void *)frame->stVFrame.u64PlanePhyAddr[0];
    m_bsd_in.img.y = (void *)gAlComParam.pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[0];
    m_bsd_in.img.uv_phy = (void *)frame->stVFrame.u64PlanePhyAddr[1];
    m_bsd_in.img.uv = (void *)gAlComParam.pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[1];
    m_bsd_in.img.width = gBsdNN.u32Width;
    m_bsd_in.img.height = gBsdNN.u32Height;
    m_bsd_in.img.format = YUV420_SP;
    m_bsd_in.gps = gAlComParam.mCurrentSpeed;
    m_bsd_in.carped_en = 1;
    m_bsd_in.lane_en = 1;
	m_bsd_in.cb_param = NULL;

    if (nn_hdl != NULL)
	{
        s32Ret = nn_bsd_cmd(nn_hdl, NN_BSD_SET_DATA, &m_bsd_in);
		if (s32Ret < 0)
		{
			PNT_LOGE("NN_BSD_SET_DATA failed %d!\n", s32Ret);
		}
    }
	
#ifdef SAVE_AX_YUV_SP
		EI_S32 i;

		for (i = 0; i < frame->stVFrame.u32PlaneNum; i++)
		{
			if (gBsdNN.ax_fout) {
				if (m_bsd_in.img.format == YUV420_SP) {
					if (i == 0) {
						fwrite((void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[i], 1,  m_bsd_in.img.width*m_bsd_in.img.height, axframe_info.ax_fout);
					} else {
						fwrite((void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[i], 1,  m_bsd_in.img.width*m_bsd_in.img.height/2, axframe_info.ax_fout);
					}
				}
			}
		}
#endif

    sem_wait(&gBsdNN.frame_del_sem);

    return 0;
}

int bsd_warning(nn_bsd_out_t *data)
{
	int   status, index;

	if (gBsdNN.action == BSD_CALIB)
	{
		return 0;
	}

	if (data->cars.size > 0)
	{
		for (int cnt_cars = 0; cnt_cars < data->cars.size; cnt_cars++)
		{
			status = data->cars.p[cnt_cars].status;
			PNT_LOGD("cnt_cars %d cars status:%d\n", cnt_cars, status);

			if (status == WARN_STATUS_FCW)
            {
			    al_warning_proc(LB_WARNING_BSD_COLLIDE, data->cars.p[cnt_cars].dist, data->cars.p[cnt_cars].relative_speed);
 			}
            else if (status >= WARN_STATUS_HMW_LEVEL1 && status <= WARN_STATUS_HMW_LEVEL4)
            {
			    al_warning_proc(LB_WARNING_BSD_DISTANCE, data->cars.p[cnt_cars].dist, data->cars.p[cnt_cars].relative_speed);
			}
            else if (status == WARN_STATUS_FMW)
            {
			    al_warning_proc(LB_WARNING_BSD_LAUNCH, data->cars.p[cnt_cars].dist, data->cars.p[cnt_cars].relative_speed);
			}
		}
	}
	if (data->peds.size > 0)
	{
		for (int cnt_peds = 0; cnt_peds < data->peds.size; cnt_peds++)
		{
			status = data->peds.p[cnt_peds].status;
			PNT_LOGD("cnt_peds %d peds status:%d\n", cnt_peds, status);
			
			if (status)
            {
			    al_warning_proc(LB_WARNING_BSD_PEDS, data->peds.p[cnt_peds].dist, data->peds.p[cnt_peds].relative_speed);
			}
		}
	}

	for (int cnt_lanes = 0; cnt_lanes < 2; cnt_lanes++)
	{
		status = data->lanes.lanes[cnt_lanes].status;
		PNT_LOGD("cnt_lanes %d lanes status:%d\n", cnt_lanes, status);
		
		if (status == WARN_STATUS_PRESS_LANE)
        {
			al_warning_proc(LB_WARNING_BSD_PRESSURE, cnt_lanes, 0);
		}
		if (status == WARN_STATUS_CHANGING_LANE)
        {
			al_warning_proc(LB_WARNING_BSD_CHANGLANE, cnt_lanes, 0);
		}
	}

	return 0;
}

static int nn_bsd_result_cb(int event, void *data)
{
	if (event == NN_BSD_DET_DONE)
	{
		nn_bsd_out_t *curr_data = (nn_bsd_out_t *)data;
    	int i;
		pthread_mutex_lock(&gBsdNN.p_det_mutex);
	    /* cars data */
	    if (gBsdNN.p_det_info->cars.p)
		{
	        free(gBsdNN.p_det_info->cars.p);
	        gBsdNN.p_det_info->cars.p = NULL;
	        gBsdNN.p_det_info->cars.size = 0;
	    }
	    gBsdNN.p_det_info->cars.p = malloc(curr_data->cars.size * sizeof(nn_bsd_car_t));
	    if (gBsdNN.p_det_info->cars.p)
		{
	        memcpy(gBsdNN.p_det_info->cars.p, curr_data->cars.p, curr_data->cars.size * sizeof(nn_bsd_car_t));
	        gBsdNN.p_det_info->cars.size = curr_data->cars.size;
	    }

	    /* peds data */
	    if (gBsdNN.p_det_info->peds.p)
		{
	        free(gBsdNN.p_det_info->peds.p);
	        gBsdNN.p_det_info->peds.p = NULL;
	        gBsdNN.p_det_info->peds.size = 0;
	    }
	    gBsdNN.p_det_info->peds.p = malloc(curr_data->peds.size * sizeof(nn_bsd_ped_t));
	    if (gBsdNN.p_det_info->peds.p)
		{
	        memcpy(gBsdNN.p_det_info->peds.p, curr_data->peds.p, curr_data->peds.size * sizeof(nn_bsd_ped_t));
	        gBsdNN.p_det_info->peds.size = curr_data->peds.size;
	    }
		
	    /* lanes data */
	    memcpy(&gBsdNN.p_det_info->lanes, &curr_data->lanes, sizeof(nn_bsd_lanes_t));
		pthread_mutex_unlock(&gBsdNN.p_det_mutex);
		
		if (gBsdNN.action != BSD_CALIB)
		{
		    if (curr_data->cars.size)
			{
		        PNT_LOGD("nn_bsd_result_cb curr_data->cars.size = %d\n", curr_data->cars.size);
		        for (i=0; i<curr_data->cars.size; i++)
				{
		            PNT_LOGD("curr_data->cars.p[%d].status = %d\n", i, curr_data->cars.p[i].status);
		        }
		    }
		    if (curr_data->peds.size)
			{
		        PNT_LOGD("nn_bsd_result_cb curr_data->peds.size = %d\n", curr_data->peds.size);
		        for (i=0; i<curr_data->peds.size; i++)
				{
		            PNT_LOGD("curr_data->peds.p[%d].status = %d\n", i, curr_data->peds.p[i].status);
		        }
		    }
		    if (curr_data->lanes.lanes[0].exist)
		        PNT_LOGD("nn_bsd_result_cb curr_data->lanes.lanes[0].exist = %d\n", curr_data->lanes.lanes[0].exist);
		    if (curr_data->lanes.lanes[1].exist)
		        PNT_LOGD("nn_bsd_result_cb curr_data->lanes.lanes[1].exist = %d\n", curr_data->lanes.lanes[1].exist);
		    if (curr_data->lanes.lanes[0].status)
		        PNT_LOGD("nn_bsd_result_cb curr_data->lanes.lanes[0].exist = %d\n", curr_data->lanes.lanes[0].status);
		    if (curr_data->lanes.lanes[1].status)
		        PNT_LOGD("nn_bsd_result_cb curr_data->lanes.lanes[1].exist = %d\n", curr_data->lanes.lanes[1].status);
		}

		bsd_warning(curr_data);

		sem_post(&gBsdNN.frame_del_sem);
    } 
	else if (event == NN_BSD_SELF_CALIBRATE)
	{
		PNT_LOGD("NN_BSD_SELF_CALIBRATE\n");
		nn_bsd_cfg_t *curr_data = (nn_bsd_cfg_t *)data;
		
		PNT_LOGD("curr_data->camera_param.vanish_point: %d %d %d %f\n",curr_data->camera_param.vanish_point[0], curr_data->camera_param.vanish_point[1],
			curr_data->camera_param.bottom_line, curr_data->camera_param.camera_to_middle);

		gBsdAlgoParams->vanish_point_x = curr_data->camera_param.vanish_point[0];
		gBsdAlgoParams->vanish_point_y = curr_data->camera_param.vanish_point[1];
		gBsdAlgoParams->bottom_line_y = curr_data->camera_param.bottom_line;
	}
    
    return 0;


}


static EI_VOID *get_axframe_proc(EI_VOID *p)
{
    int ret;
    VIDEO_FRAME_INFO_S axFrame = {0};
    bsd_nn_t *axframe_info;
    void *nn_hdl = NULL;
	char bsdlib_ver[64]={0};

    axframe_info = (bsd_nn_t *)p;
    PNT_LOGE("get_axframe_proc bThreadStart = %d\n", axframe_info->bThreadStart);
	
#ifdef SAVE_AX_YUV_SP
		char srcfilename[FULLNAME_LEN_MAX];
		sprintf(srcfilename, "%s/bsd_raw_ch%d-%d.yuv", PATH_ROOT, axframe_info->chn);
		axframe_info->ax_fout = fopen(srcfilename, "wb+");
		if (axframe_info->ax_fout == NULL) {
			OSAL_LOGE("file open error1\n");
		}
#endif

#ifdef AL_PRO_FRAMERATE_STATISTIC
	int64_t time1 = 0LL, time2 = 0LL, time_GetChnFrame1 = 0LL, time_GetChnFrame2 = 0LL,
	time_AlProFrame1 = 0LL, time_AlProFrame2 = 0LL;
	int64_t frame_cnt = 0;
#endif

    PNT_LOGE("********start_nn_bsd******\n");
    memset(&axframe_info->m_nn_bsd_cfg, 0, sizeof(axframe_info->m_nn_bsd_cfg));
    /*init nn_bsd, cfg*/
    axframe_info->m_nn_bsd_cfg.img_width = axframe_info->u32Width; //算法输入图像宽
    axframe_info->m_nn_bsd_cfg.img_height = axframe_info->u32Height; //算法输入图像高
    axframe_info->m_nn_bsd_cfg.interest_box[0] = 0; //算法检测区域左上角X坐标
    axframe_info->m_nn_bsd_cfg.interest_box[1] = 0; //算法检测区域左上角y坐标
    axframe_info->m_nn_bsd_cfg.interest_box[2] = axframe_info->u32Width-1; //算法检测区域右下角x坐标
    axframe_info->m_nn_bsd_cfg.interest_box[3] = axframe_info->u32Height-1; //算法检测区域右下角y坐标
    axframe_info->m_nn_bsd_cfg.format = YUV420_SP;
    axframe_info->m_nn_bsd_cfg.model_basedir = NN_BSD_PATH;
    axframe_info->m_nn_bsd_cfg.cb_func = nn_bsd_result_cb;
    axframe_info->m_nn_bsd_cfg.camera_param.vanish_point[0] = gBsdAlgoParams->vanish_point_x;//bsd_get_vanish_point_x(); //天际线和车中线交叉点X坐标 单位像素
    axframe_info->m_nn_bsd_cfg.camera_param.vanish_point[1] = gBsdAlgoParams->vanish_point_y;//bsd_get_vanish_point_y(); //天际线和车中线交叉点Y坐标 单位像素
    axframe_info->m_nn_bsd_cfg.camera_param.bottom_line = gBsdAlgoParams->bottom_line_y;//bsd_get_bottom_line_y(); //引擎盖线
    axframe_info->m_nn_bsd_cfg.camera_param.camera_angle[0] = 80; //摄像头水平角度
    axframe_info->m_nn_bsd_cfg.camera_param.camera_angle[1] = 50; //摄像头垂直角度
    axframe_info->m_nn_bsd_cfg.camera_param.camera_to_ground = (float)gBsdAlgoParams->camera_to_ground/100; //与地面距离，单位米
    axframe_info->m_nn_bsd_cfg.camera_param.car_head_to_camera= (float)gBsdAlgoParams->car_head_to_camera/100; //与车头距离 单位米
    axframe_info->m_nn_bsd_cfg.camera_param.car_width= (float)gBsdAlgoParams->car_width/100; //车宽，单位米
    axframe_info->m_nn_bsd_cfg.camera_param.camera_to_middle= (float)gBsdAlgoParams->camera_to_middle/100; //与车中线距离，单位米，建议设备安装在车的中间，左负右正
	axframe_info->m_nn_bsd_cfg.calib_param.auto_calib_en = 1;//自动校准使能
	axframe_info->m_nn_bsd_cfg.calib_param.calib_finish = 1;//设置为1，在自动校准前也会按默认参数报警, 设置为0，则在自动校准前不报警
 
	m_bsd_cali.vanish_point_x = axframe_info->m_nn_bsd_cfg.camera_param.vanish_point[0];
	m_bsd_cali.vanish_point_y = axframe_info->m_nn_bsd_cfg.camera_param.vanish_point[1];
	m_bsd_cali.bottom_line_y = axframe_info->m_nn_bsd_cfg.camera_param.bottom_line;
	
	axframe_info->m_nn_bsd_cfg.ldw_en = 1;//车道偏移和变道报警
	axframe_info->m_nn_bsd_cfg.hmw_en = 1;//保持车距报警
	axframe_info->m_nn_bsd_cfg.ufcw_en = 0;//低速下前车碰撞报警，reserved备用
	axframe_info->m_nn_bsd_cfg.fcw_en = 1;//前车碰撞报警
	axframe_info->m_nn_bsd_cfg.pcw_en = 1;//行人碰撞报警
	axframe_info->m_nn_bsd_cfg.fmw_en = 1;//前车启动报警，必须有GPS，且本车车速为0
	axframe_info->m_nn_bsd_cfg.warn_param.hmw_param.min_warn_dist = 0;//最小报警距离，单位米
	axframe_info->m_nn_bsd_cfg.warn_param.hmw_param.max_warn_dist = 100;//最大报警距离，单位米
	axframe_info->m_nn_bsd_cfg.warn_param.hmw_param.min_warn_gps = 30;//最低报警车速，单位KM/H
	axframe_info->m_nn_bsd_cfg.warn_param.hmw_param.max_warn_gps = 200;//最高报警车速，单位KM/H
	axframe_info->m_nn_bsd_cfg.warn_param.hmw_param.gps_ttc_thresh[0] = 1.2f;//与前车碰撞小于这个时间报警(假设前车不动)，单位秒,距离/本车GPS速度，对应WARN_STATUS_HMW_LEVEL1报警状态
	axframe_info->m_nn_bsd_cfg.warn_param.hmw_param.gps_ttc_thresh[1] = 0.6f;//与前车碰撞小于这个时间报警(假设前车不动)，单位秒,距离/本车GPS速度，对应WARN_STATUS_HMW_LEVEL2报警状态

	axframe_info->m_nn_bsd_cfg.warn_param.fcw_param.min_warn_dist = 0;
	axframe_info->m_nn_bsd_cfg.warn_param.fcw_param.max_warn_dist = 100;
	axframe_info->m_nn_bsd_cfg.warn_param.fcw_param.min_warn_gps = 30;
	axframe_info->m_nn_bsd_cfg.warn_param.fcw_param.max_warn_gps = 200;
	axframe_info->m_nn_bsd_cfg.warn_param.fcw_param.gps_ttc_thresh = 2.0f; //与前车碰撞报警时间(假设前车不动)，单位秒,距离/本车GPS速度
	axframe_info->m_nn_bsd_cfg.warn_param.fcw_param.ttc_thresh = 2.5f; //与前车碰撞报警时间，单位秒,距离/相对车速，以上4个条件同时满足

	axframe_info->m_nn_bsd_cfg.warn_param.ufcw_param.min_warn_dist = 0;
	axframe_info->m_nn_bsd_cfg.warn_param.ufcw_param.max_warn_dist = 100;
	axframe_info->m_nn_bsd_cfg.warn_param.ufcw_param.min_warn_gps = 0;
	axframe_info->m_nn_bsd_cfg.warn_param.ufcw_param.max_warn_gps = 30;
	axframe_info->m_nn_bsd_cfg.warn_param.ufcw_param.relative_speed_thresh = -3.0f; //与前车碰撞相对车速，前车车速-本车车速小于这个速度

	axframe_info->m_nn_bsd_cfg.warn_param.fmw_param.min_static_time = 5.0f;//前车静止的时间需大于多少，单位秒
	axframe_info->m_nn_bsd_cfg.warn_param.fmw_param.warn_car_inside_dist = 6.0f;//与前车的距离在多少米内，单位米
	axframe_info->m_nn_bsd_cfg.warn_param.fmw_param.warn_dist_leave = 0.6f;//前车启动大于多远后报警，单位米

	axframe_info->m_nn_bsd_cfg.warn_param.pcw_param.min_warn_dist = 0;
	axframe_info->m_nn_bsd_cfg.warn_param.pcw_param.max_warn_dist = 100;
	axframe_info->m_nn_bsd_cfg.warn_param.pcw_param.min_warn_gps = 30;
	axframe_info->m_nn_bsd_cfg.warn_param.pcw_param.max_warn_gps = 200;
	axframe_info->m_nn_bsd_cfg.warn_param.pcw_param.gps_ttc_thresh = 1.5f;
	axframe_info->m_nn_bsd_cfg.warn_param.pcw_param.ttc_thresh = 5.0f;

	axframe_info->m_nn_bsd_cfg.warn_param.ldw_param.min_warn_gps = 30.0f;//最低报警车速，单位KM/H
	axframe_info->m_nn_bsd_cfg.warn_param.ldw_param.ldw_warn_thresh = 0.5f; //在lane_warn_dist距离内持续时间，单位秒
	axframe_info->m_nn_bsd_cfg.warn_param.ldw_param.lane_warn_dist = 0.0f;//与车道线距离，车道线内
	axframe_info->m_nn_bsd_cfg.warn_param.ldw_param.change_lane_dist = 0.6f; //变道侧车道线离车的中点距离，单位米

    /* open libbsd.so*/
    nn_hdl = nn_bsd_open(&axframe_info->m_nn_bsd_cfg);
    if (nn_hdl == NULL)
	{
        PNT_LOGE("nn_bsd_open() failed!");
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

        ret = EI_MI_VISS_GetChnFrame(axframe_info->dev, axframe_info->chn, &axFrame, 1000);
        if (ret != EI_SUCCESS)
		{
            PNT_LOGE("chn %d get frame error 0x%x\n", axframe_info->chn, ret);
            usleep(100 * 1000);
            continue;
        }
		
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time_GetChnFrame2 = osal_get_msec();
#endif

#ifdef AL_PRO_FRAMERATE_STATISTIC
		time_AlProFrame1 = osal_get_msec();
#endif

        nn_bsd_start(&axFrame, nn_hdl);

#ifdef AL_PRO_FRAMERATE_STATISTIC
	time_AlProFrame2 = osal_get_msec();
	PNT_LOGE("time_GetChnFrame %lld ms, time_AlProFrame %lld ms\n", time_GetChnFrame2-time_GetChnFrame1, time_AlProFrame2-time_AlProFrame1);
#endif

#ifdef AL_PRO_FRAMERATE_STATISTIC
		frame_cnt++;
		time2 = osal_get_msec();
		if ((time2 - time1) >= 3000) {
			PNT_LOGE("algo process %lld fps\n", (frame_cnt * 1000) / (time2 - time1));
			time1 = time2;
			frame_cnt  = 0;
		}
#endif

//		nn_bsd_cmd(nn_hdl, NN_BSD_GET_VERSION, bsdlib_ver);

//		PNT_LOGE("bsdlib_ver %s\n", bsdlib_ver);

        EI_MI_VISS_ReleaseChnFrame(axframe_info->dev, axframe_info->chn, &axFrame);
    }
	
    EI_MI_VISS_DisableChn(axframe_info->dev, axframe_info->chn);
	
	PNT_LOGE("axframe_info->dev %d %d\n", axframe_info->dev, axframe_info->chn);
	PNT_LOGE("nn_bsd_close 0\n");
	
    nn_bsd_close(nn_hdl);
	
	PNT_LOGE("nn_bsd_close 1\n");

    return NULL;
}

static EI_VOID *rgn_draw_proc(EI_VOID *p)
{
    bsd_nn_t *axframe_info = &gBsdNN;

	unsigned char lastState = gAlComParam.mBsdRender;

	rgn_start();

    while (EI_TRUE == axframe_info->bThreadStart)
	{
		if (lastState != gAlComParam.mBsdRender)
		{
			lastState = gAlComParam.mBsdRender;
			
			if (gAlComParam.mBsdRender)
			{
				if(EI_MI_RGN_AddToChn(gBsdNN.Handle[0], &gBsdNN.stRgnChn, &gBsdNN.stRgnChnAttr[0]))
				{
					PNT_LOGE("EI_MI_RGN_AddToChn \n");
				}

				if(EI_MI_RGN_AddToChn(gBsdNN.Handle[1], &gBsdNN.stRgnChn, &gBsdNN.stRgnChnAttr[1]))
				{
					PNT_LOGE("EI_MI_RGN_AddToChn \n");
				}
			}
			else
			{
				EI_MI_RGN_DelFromChn(gBsdNN.Handle[0], &gBsdNN.stRgnChn);
				EI_MI_RGN_DelFromChn(gBsdNN.Handle[1], &gBsdNN.stRgnChn);
			}
		}
		else
		{
			if (gAlComParam.mBsdRender)
			{
				pthread_mutex_lock(&axframe_info->p_det_mutex);

				bsd_draw_location_rgn(axframe_info->p_det_info);

				pthread_mutex_unlock(&axframe_info->p_det_mutex);
			}
			else
			{
				sleep(1);
			}
		}
    }
	
    return NULL;
}

static int start_get_axframe(bsd_nn_t *axframe_info)
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
    stExtChnAttr.stFrameRate.s32DstFrameRate = 10;
    stExtChnAttr.s32Position = 1;

    ret = EI_MI_VISS_SetExtChnAttr(axframe_info->dev, axframe_info->chn, &stExtChnAttr);
    if (ret != EI_SUCCESS)
    {
        PNT_LOGE("EI_MI_VISS_SetExtChnAttr failed with %#x!\n", ret);
        return EI_FAILURE;
    }
    ret = EI_MI_VISS_EnableChn(axframe_info->dev, axframe_info->chn);
    if (ret != EI_SUCCESS)
    {
        PNT_LOGE("EI_MI_VISS_EnableChn failed with %#x!\n", ret);
        return EI_FAILURE;
    }

    axframe_info->bThreadStart = EI_TRUE;
    ret = pthread_create(&(axframe_info->gs_framePid), NULL, get_axframe_proc, (EI_VOID *)axframe_info);

	pthread_t pid;
    ret = pthread_create(&pid, NULL, rgn_draw_proc, (EI_VOID *)axframe_info);

    PNT_LOGD("start_get_axframe success %#x! phyChn:%d chn :%d dev:%d\n", ret, axframe_info->phyChn, axframe_info->chn, axframe_info->dev);

    return EI_SUCCESS;
}

int bsd_nn_init(int vissDev, int vissChn, int chn, int width, int height)
{
    bsd_nn_t *axframe_info = &gBsdNN;

    pthread_mutex_init(&axframe_info->p_det_mutex, NULL);
	
    axframe_info->p_det_info = malloc(sizeof(nn_bsd_out_t));
	
    if (!axframe_info->p_det_info)
    {
		PNT_LOGE("axframe_info->p_det_info malloc failed!\n");
		return -1;
    }
	
	if (axframe_info->action == BSD_CALIB)
	{
		m_bsd_cali.cali_flag = BSD_CALI_START;
	}

    axframe_info->dev = vissDev;
    axframe_info->chn = chn;
    axframe_info->phyChn = vissChn;
    axframe_info->frameRate = 25;
    axframe_info->u32Width = width;
    axframe_info->u32Height = height;

    sem_init(&axframe_info->frame_del_sem, 0, 0);

	gBsdNN.u32scnWidth = width;
	gBsdNN.u32scnHeight = height;

//	bsd_al_para_init();

    start_get_axframe(axframe_info);

    return 0;
}

void bsd_nn_stop(void)
{
    bsd_nn_t *axframe_info = &gBsdNN;
	
	axframe_info->bThreadStart = EI_FALSE;

	usleep(200*1000);

	rgn_stop();
}




