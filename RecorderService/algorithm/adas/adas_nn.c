
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "pnt_log.h"
#include "adas_config.h"
#include "adas_nn.h"

adas_nn_t gAdasNN = { 0 };
adas_calibrate_para_t m_adas_cali;

//#define AL_PRO_FRAMERATE_STATISTIC

static EI_S32 rgn_start(void)
{
	EI_S32 s32Ret = EI_FAILURE;

	gAdasNN.stRgnChn.enModId = EI_ID_IPPU;
	gAdasNN.stRgnChn.s32ChnId = 1;
	gAdasNN.stRgnChn.s32DevId = gAdasNN.dev;
	
	gAdasNN.Handle[0] = 10;
    gAdasNN.stRegion[0].enType = RECTANGLEEX_ARRAY_RGN;
    gAdasNN.stRegion[0].unAttr.u32RectExArrayMaxNum = ADAS_RGN_RECT_NUM;
	
    s32Ret = EI_MI_RGN_Create(gAdasNN.Handle[0], &gAdasNN.stRegion[0]);
    if(s32Ret)
	{
        PNT_LOGE("EI_MI_RGN_Create \n");
        goto exit;
    }
	gAdasNN.Handle[1] = 11;
    gAdasNN.stRegion[1].enType = LINEEX_ARRAY_RGN;
    gAdasNN.stRegion[1].unAttr.u32LineExArrayMaxNum = ADAS_RGN_LINE_NUM;
    s32Ret = EI_MI_RGN_Create(gAdasNN.Handle[1], &gAdasNN.stRegion[1]);
    if(s32Ret)
	{
        PNT_LOGE("EI_MI_RGN_Create \n");
        goto exit;
    }
	gAdasNN.stRgnChnAttr[0].bShow = EI_TRUE;
    gAdasNN.stRgnChnAttr[0].enType = RECTANGLEEX_ARRAY_RGN;
    gAdasNN.stRgnChnAttr[0].unChnAttr.stRectangleExArrayChn.u32MaxRectNum = ADAS_RGN_RECT_NUM;
    gAdasNN.stRgnChnAttr[0].unChnAttr.stRectangleExArrayChn.u32ValidRectNum = ADAS_RGN_RECT_NUM;
    gAdasNN.stRgnChnAttr[0].unChnAttr.stRectangleExArrayChn.pstRectExArraySubAttr = gAdasNN.stRectArray;
//    s32Ret = EI_MI_RGN_AddToChn(gAdasNN.Handle[0], &gAdasNN.stRgnChn, &gAdasNN.stRgnChnAttr[0]);
//	if(s32Ret)
//	{
//        PNT_LOGE("EI_MI_RGN_AddToChn \n");
//    }
	gAdasNN.stRgnChnAttr[1].bShow = EI_TRUE;
    gAdasNN.stRgnChnAttr[1].enType = LINEEX_ARRAY_RGN;
    gAdasNN.stRgnChnAttr[1].unChnAttr.stLineExArrayChn.u32MaxLineNum = ADAS_RGN_LINE_NUM;
    gAdasNN.stRgnChnAttr[1].unChnAttr.stLineExArrayChn.u32ValidLineNum = ADAS_RGN_LINE_NUM;
    gAdasNN.stRgnChnAttr[1].unChnAttr.stLineExArrayChn.pstLineExArraySubAttr = gAdasNN.stLineArray;
//    s32Ret = EI_MI_RGN_AddToChn(gAdasNN.Handle[1], &gAdasNN.stRgnChn, &gAdasNN.stRgnChnAttr[1]);
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

	for (i = 0;i < ADAS_RGN_ARRAY_NUM; i++)
	{
		if (gAlComParam.mAdasRender)
			EI_MI_RGN_DelFromChn(gAdasNN.Handle[i], &gAdasNN.stRgnChn);
		EI_MI_RGN_Destroy(gAdasNN.Handle[i]);
	}
}

static void adas_draw_location_rgn(nn_adas_out_t *p_det_info)
{
	int draw_rect_num, draw_line0_num, draw_line1_num;
	int index = 0;
	int loc_x0, loc_y0, loc_x1, loc_y1;
	EI_S32 s32Ret = EI_FAILURE;

	m_adas_cali.vanish_point_x = gAdasAlgoParams->vanish_point_x;
	m_adas_cali.vanish_point_y = gAdasAlgoParams->vanish_point_y;
	m_adas_cali.bottom_line_y = gAdasAlgoParams->bottom_line_y;

	if (gAdasNN.action == ADAS_CALIB)
	{
		for (int i = 2; i < 5; i++)
		{
			gAdasNN.stRgnChnAttr[1].bShow = EI_TRUE;
			gAdasNN.stLineArray[i].isShow = EI_TRUE;
			if (i == VANISH_Y_LINE)
			{
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[0].s32X = 0;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[0].s32Y = m_adas_cali.vanish_point_y/3;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[1].s32X = gAdasNN.u32scnWidth/3;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[1].s32Y = m_adas_cali.vanish_point_y/3;
		        gAdasNN.stLineArray[i].stLineAttr.u32Color = 0xffff0000;
			}
			else if (i == VANISH_X_LINE)
			{
				gAdasNN.stLineArray[i].stLineAttr.stPoints[0].s32X = m_adas_cali.vanish_point_x/3;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[0].s32Y = 0;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[1].s32X = m_adas_cali.vanish_point_x/3;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[1].s32Y = gAdasNN.u32scnHeight/3;
		        gAdasNN.stLineArray[i].stLineAttr.u32Color = 0xff00ff00;
			}
			else if (i == BOTTOM_Y_LINE)
			{
				gAdasNN.stLineArray[i].stLineAttr.stPoints[0].s32X = 0;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[0].s32Y = m_adas_cali.bottom_line_y/3;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[1].s32X = gAdasNN.u32scnWidth/3;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[1].s32Y = m_adas_cali.bottom_line_y/3;
		        gAdasNN.stLineArray[i].stLineAttr.u32Color = 0xff8000ff;
			}
			if (i==m_adas_cali.adas_calibline)
				gAdasNN.stLineArray[i].stLineAttr.u32LineSize = 4;
			else
	        	gAdasNN.stLineArray[i].stLineAttr.u32LineSize = 2;
			s32Ret = EI_MI_RGN_SetChnAttr(gAdasNN.Handle[1], &gAdasNN.stRgnChn, &gAdasNN.stRgnChnAttr[1]);
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
		if (draw_rect_num > ADAS_RGN_RECT_NUM || (draw_line0_num+draw_line1_num) > ADAS_RGN_LINE_NUM)
			return;

		if (gAdasNN.last_draw_rect_num > draw_rect_num)
		{
			for (int i = gAdasNN.last_draw_rect_num-1; i >= draw_rect_num; i--)
			{
				gAdasNN.stRectArray[i].isShow = EI_FALSE;
				s32Ret = EI_MI_RGN_SetChnAttr(gAdasNN.Handle[0], &gAdasNN.stRgnChn, &gAdasNN.stRgnChnAttr[0]);
				if(s32Ret)
				{
					PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
	    		}
			}
		}
		if (gAdasNN.last_draw_line0_num > draw_line0_num)
		{
				gAdasNN.stLineArray[0].isShow = EI_FALSE;
				s32Ret = EI_MI_RGN_SetChnAttr(gAdasNN.Handle[1], &gAdasNN.stRgnChn, &gAdasNN.stRgnChnAttr[1]);
				if(s32Ret)
				{
					PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
	    		}
		}
		if (gAdasNN.last_draw_line1_num > draw_line1_num)
		{
				gAdasNN.stLineArray[1].isShow = EI_FALSE;
				s32Ret = EI_MI_RGN_SetChnAttr(gAdasNN.Handle[1], &gAdasNN.stRgnChn, &gAdasNN.stRgnChnAttr[1]);
				if(s32Ret)
				{
					PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
	    		}
		}
	    if (p_det_info->cars.size)
		{
			for (int cnt_cars = 0; cnt_cars < p_det_info->cars.size; cnt_cars++)
			{
		        loc_x0 = p_det_info->cars.p[cnt_cars].box[0]/3;
		        loc_y0 = p_det_info->cars.p[cnt_cars].box[1]/3;
		        loc_x1 = p_det_info->cars.p[cnt_cars].box[2]/3;
		        loc_y1 = p_det_info->cars.p[cnt_cars].box[3]/3;
		        gAdasNN.stRectArray[cnt_cars].stRectAttr.stRect.s32X = loc_x0;
				gAdasNN.stRectArray[cnt_cars].stRectAttr.stRect.s32Y = loc_y0;
				gAdasNN.stRectArray[cnt_cars].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
				gAdasNN.stRectArray[cnt_cars].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
				gAdasNN.stRectArray[cnt_cars].stRectAttr.u32BorderSize = 2;
				if (p_det_info->cars.p[cnt_cars].region <= 1)
				{
					if (WARN_STATUS_FCW == p_det_info->cars.p[cnt_cars].status || WARN_STATUS_UFCW == p_det_info->cars.p[cnt_cars].status)
						gAdasNN.stRectArray[cnt_cars].stRectAttr.u32Color = 0xffff0000;
					else
						gAdasNN.stRectArray[cnt_cars].stRectAttr.u32Color = 0xffffff00;
				}
				else
					gAdasNN.stRectArray[cnt_cars].stRectAttr.u32Color = 0xff00ff00;
				gAdasNN.stRectArray[cnt_cars].isShow = EI_TRUE;
				gAdasNN.stRgnChnAttr[0].bShow = EI_TRUE;
				s32Ret = EI_MI_RGN_SetChnAttr(gAdasNN.Handle[0], &gAdasNN.stRgnChn, &gAdasNN.stRgnChnAttr[0]);
				if(s32Ret)
				{
					PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
	    		}
			}
	    }
		index = index + p_det_info->cars.size;
	    for (int cnt_peds = 0; cnt_peds < p_det_info->peds.size; cnt_peds++)
		{
	        loc_x0 = p_det_info->peds.p[cnt_peds].box[0]/3;
	        loc_y0 = p_det_info->peds.p[cnt_peds].box[1]/3;
	        loc_x1 = p_det_info->peds.p[cnt_peds].box[2]/3;
	        loc_y1 = p_det_info->peds.p[cnt_peds].box[3]/3;
	        gAdasNN.stRectArray[index+cnt_peds].stRectAttr.stRect.s32X = loc_x0;
			gAdasNN.stRectArray[index+cnt_peds].stRectAttr.stRect.s32Y = loc_y0;
			gAdasNN.stRectArray[index+cnt_peds].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
			gAdasNN.stRectArray[index+cnt_peds].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
			gAdasNN.stRectArray[index+cnt_peds].stRectAttr.u32BorderSize = 2;
			if (1 >= p_det_info->peds.p[cnt_peds].region)
			{
				if (WARN_STATUS_PCW <= p_det_info->peds.p[cnt_peds].status)
					gAdasNN.stRectArray[index+cnt_peds].stRectAttr.u32Color = 0xffff0000;
				else
					gAdasNN.stRectArray[index+cnt_peds].stRectAttr.u32Color = 0xffffff00;
			}
			else
			{
				gAdasNN.stRectArray[index+cnt_peds].stRectAttr.u32Color = 0xff00ff00;
			}
			gAdasNN.stRectArray[index+cnt_peds].isShow = EI_TRUE;
			gAdasNN.stRgnChnAttr[0].bShow = EI_TRUE;
			s32Ret = EI_MI_RGN_SetChnAttr(gAdasNN.Handle[0], &gAdasNN.stRgnChn, &gAdasNN.stRgnChnAttr[0]);
			if(s32Ret)
			{
				PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
	    	}
	    }
		gAdasNN.last_draw_rect_num = draw_rect_num;
	    for (int i = 0; i < 2; i++)
		{
	        if (p_det_info->lanes.lanes[i].exist)
			{
				gAdasNN.stRgnChnAttr[1].bShow = EI_TRUE;
				gAdasNN.stLineArray[i].isShow = EI_TRUE;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[0].s32X = p_det_info->lanes.lanes[i].pts[0][0]/3;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[0].s32Y = p_det_info->lanes.lanes[i].pts[0][1]/3;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[1].s32X = p_det_info->lanes.lanes[i].pts[5][0]/3;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[1].s32Y = p_det_info->lanes.lanes[i].pts[5][1]/3;
				if (p_det_info->lanes.lanes[i].press_lane_status || p_det_info->lanes.lanes[i].status)
					gAdasNN.stLineArray[i].stLineAttr.u32Color = 0xffff0000;
				else
		        	gAdasNN.stLineArray[i].stLineAttr.u32Color = 0xffffff80;
		        gAdasNN.stLineArray[i].stLineAttr.u32LineSize = 2;
				s32Ret = EI_MI_RGN_SetChnAttr(gAdasNN.Handle[1], &gAdasNN.stRgnChn, &gAdasNN.stRgnChnAttr[1]);
				if(s32Ret)
				{
					PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
	    		}
	        }
    	}
		gAdasNN.last_draw_line0_num = draw_line0_num;
		gAdasNN.last_draw_line1_num = draw_line1_num;
		for (int i = 2; i < 5; i++)
		{
			gAdasNN.stRgnChnAttr[1].bShow = EI_TRUE;
			gAdasNN.stLineArray[i].isShow = EI_TRUE;
			if (i == VANISH_Y_LINE)
			{
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[0].s32X = 0;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[0].s32Y = m_adas_cali.vanish_point_y/3;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[1].s32X = gAdasNN.u32scnWidth/3;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[1].s32Y = m_adas_cali.vanish_point_y/3;
		        gAdasNN.stLineArray[i].stLineAttr.u32Color = 0xffffff00;
			}
			else if (i == VANISH_X_LINE)
			{
				gAdasNN.stLineArray[i].stLineAttr.stPoints[0].s32X = m_adas_cali.vanish_point_x/3;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[0].s32Y = 0;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[1].s32X = m_adas_cali.vanish_point_x/3;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[1].s32Y = gAdasNN.u32scnHeight/3;
		        gAdasNN.stLineArray[i].stLineAttr.u32Color = 0xff00ff00;
			}
			else if (i == BOTTOM_Y_LINE)
			{
				gAdasNN.stLineArray[i].stLineAttr.stPoints[0].s32X = 0;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[0].s32Y = m_adas_cali.bottom_line_y/3;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[1].s32X = gAdasNN.u32scnWidth/3;
		        gAdasNN.stLineArray[i].stLineAttr.stPoints[1].s32Y = m_adas_cali.bottom_line_y/3;
		        gAdasNN.stLineArray[i].stLineAttr.u32Color = 0xff8000ff;
			}
	        gAdasNN.stLineArray[i].stLineAttr.u32LineSize = 2;
			s32Ret = EI_MI_RGN_SetChnAttr(gAdasNN.Handle[1], &gAdasNN.stRgnChn, &gAdasNN.stRgnChnAttr[1]);
			if(s32Ret)
			{
				PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
    		}
		}
	}
}

int adas_update_para(float vanish_point_x, float vanish_point_y, float bottom_line_y)
{
	int ret = -1;
	if (gAdasNN.nn_hdl == NULL)
	{
		return ret;
	}
	gAdasNN.m_nn_adas_cfg.camera_param.vanish_point[0] = vanish_point_x;
	gAdasNN.m_nn_adas_cfg.camera_param.vanish_point[1] = vanish_point_y;
	gAdasNN.m_nn_adas_cfg.camera_param.bottom_line = bottom_line_y;
	
	gAdasAlgoParams->vanish_point_x = vanish_point_x;
	gAdasAlgoParams->vanish_point_y = vanish_point_y;
	gAdasAlgoParams->bottom_line_y = bottom_line_y;
	//ret = nn_adas_cmd(gAdasNN.nn_hdl, NN_ADAS_SET_PARAM, &gAdasNN.m_nn_adas_cfg);

	return ret;
}

int nn_adas_start(VIDEO_FRAME_INFO_S *frame, void *nn_hdl)
{
    EI_S32 s32Ret;
    static nn_adas_in_t m_adas_in;
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
    m_adas_in.img.y_phy = (void *)frame->stVFrame.u64PlanePhyAddr[0];
    m_adas_in.img.y = (void *)gAlComParam.pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[0];
    m_adas_in.img.uv_phy = (void *)frame->stVFrame.u64PlanePhyAddr[1];
    m_adas_in.img.uv = (void *)gAlComParam.pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[1];
    m_adas_in.img.width = gAdasNN.u32Width;
    m_adas_in.img.height = gAdasNN.u32Height;
    m_adas_in.img.format = YUV420_SP;
    m_adas_in.gps = gAlComParam.mCurrentSpeed;
    m_adas_in.carped_en = 1;
    m_adas_in.lane_en = 1;
	m_adas_in.cb_param = NULL;

    if (nn_hdl != NULL)
	{
        s32Ret = nn_adas_cmd(nn_hdl, NN_ADAS_SET_DATA, &m_adas_in);
		if (s32Ret < 0)
		{
			PNT_LOGE("NN_ADAS_SET_DATA failed %d!\n", s32Ret);
		}
    }
	
#ifdef SAVE_AX_YUV_SP
		EI_S32 i;

		for (i = 0; i < frame->stVFrame.u32PlaneNum; i++)
		{
			if (gAdasNN.ax_fout) {
				if (m_adas_in.img.format == YUV420_SP) {
					if (i == 0) {
						fwrite((void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[i], 1,  m_adas_in.img.width*m_adas_in.img.height, axframe_info.ax_fout);
					} else {
						fwrite((void *)pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[i], 1,  m_adas_in.img.width*m_adas_in.img.height/2, axframe_info.ax_fout);
					}
				}
			}
		}
#endif

    sem_wait(&gAdasNN.frame_del_sem);

    return 0;
}

int adas_warning(nn_adas_out_t *data)
{
	int   status, index;

	if (gAdasNN.action == ADAS_CALIB)
	{
		return 0;
	}
	
	if (gAdasAlgoParams->fcw_sensitivity == 0 && gAdasAlgoParams->hmw_sensitivity == 0 && gAdasAlgoParams->fmw_sensitivity == 0 &&
		gAdasAlgoParams->pcw_sensitivity == 0 && gAdasAlgoParams->ldw_sensitivity == 0)
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
        		if (gAdasAlgoParams->fcw_sensitivity!=0)
			    	al_warning_proc(LB_WARNING_ADAS_COLLIDE, data->cars.p[cnt_cars].dist, data->cars.p[cnt_cars].relative_speed, data->cars.p[cnt_cars].ttc);
 			}
            else if (status >= WARN_STATUS_HMW_LEVEL1 && status <= WARN_STATUS_HMW_LEVEL4)
            {
        		if (gAdasAlgoParams->hmw_sensitivity!=0)
			    	al_warning_proc(LB_WARNING_ADAS_DISTANCE, data->cars.p[cnt_cars].dist, data->cars.p[cnt_cars].relative_speed, data->cars.p[cnt_cars].ttc);
			}
            else if (status == WARN_STATUS_FMW)
            {
        		if (gAdasAlgoParams->fmw_sensitivity!=0)
			    	al_warning_proc(LB_WARNING_ADAS_LAUNCH, data->cars.p[cnt_cars].dist, data->cars.p[cnt_cars].relative_speed, data->cars.p[cnt_cars].ttc);
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
        		if (gAdasAlgoParams->pcw_sensitivity!=0)
			    	al_warning_proc(LB_WARNING_ADAS_PEDS, data->peds.p[cnt_peds].dist, data->peds.p[cnt_peds].relative_speed, data->peds.p[cnt_peds].ttc);
			}
		}
	}

	for (int cnt_lanes = 0; cnt_lanes < 2; cnt_lanes++)
	{
		status = data->lanes.lanes[cnt_lanes].status;
		PNT_LOGD("cnt_lanes %d lanes status:%d\n", cnt_lanes, status);
		
		if (status == WARN_STATUS_PRESS_LANE)
        {
        	if (gAdasAlgoParams->ldw_sensitivity!=0)
				al_warning_proc(LB_WARNING_ADAS_PRESSURE, cnt_lanes+1, 0, 0);
		}
		if (status == WARN_STATUS_CHANGING_LANE)
        {
			al_warning_proc(LB_WARNING_ADAS_CHANGLANE, cnt_lanes+1, 0, 0);
		}
	}

	return 0;
}

static int nn_adas_result_cb(int event, void *data)
{
	if (event == NN_ADAS_DET_DONE)
	{
		nn_adas_out_t *curr_data = (nn_adas_out_t *)data;
    	int i;
		pthread_mutex_lock(&gAdasNN.p_det_mutex);
	    /* cars data */
	    if (gAdasNN.p_det_info->cars.p)
		{
	        free(gAdasNN.p_det_info->cars.p);
	        gAdasNN.p_det_info->cars.p = NULL;
	        gAdasNN.p_det_info->cars.size = 0;
	    }
	    gAdasNN.p_det_info->cars.p = malloc(curr_data->cars.size * sizeof(nn_adas_car_t));
	    if (gAdasNN.p_det_info->cars.p)
		{
	        memcpy(gAdasNN.p_det_info->cars.p, curr_data->cars.p, curr_data->cars.size * sizeof(nn_adas_car_t));
	        gAdasNN.p_det_info->cars.size = curr_data->cars.size;
	    }

	    /* peds data */
	    if (gAdasNN.p_det_info->peds.p)
		{
	        free(gAdasNN.p_det_info->peds.p);
	        gAdasNN.p_det_info->peds.p = NULL;
	        gAdasNN.p_det_info->peds.size = 0;
	    }
	    gAdasNN.p_det_info->peds.p = malloc(curr_data->peds.size * sizeof(nn_adas_ped_t));
	    if (gAdasNN.p_det_info->peds.p)
		{
	        memcpy(gAdasNN.p_det_info->peds.p, curr_data->peds.p, curr_data->peds.size * sizeof(nn_adas_ped_t));
	        gAdasNN.p_det_info->peds.size = curr_data->peds.size;
	    }
		
	    /* lanes data */
	    memcpy(&gAdasNN.p_det_info->lanes, &curr_data->lanes, sizeof(nn_adas_lanes_t));
		pthread_mutex_unlock(&gAdasNN.p_det_mutex);
		
		if (gAdasNN.action != ADAS_CALIB)
		{
		    if (curr_data->cars.size)
			{
		        PNT_LOGD("nn_adas_result_cb curr_data->cars.size = %d\n", curr_data->cars.size);
		        for (i=0; i<curr_data->cars.size; i++)
				{
		            PNT_LOGD("curr_data->cars.p[%d].status = %d\n", i, curr_data->cars.p[i].status);
		        }
		    }
		    if (curr_data->peds.size)
			{
		        PNT_LOGD("nn_adas_result_cb curr_data->peds.size = %d\n", curr_data->peds.size);
		        for (i=0; i<curr_data->peds.size; i++)
				{
		            PNT_LOGD("curr_data->peds.p[%d].status = %d\n", i, curr_data->peds.p[i].status);
		        }
		    }
		    if (curr_data->lanes.lanes[0].exist)
		        PNT_LOGD("nn_adas_result_cb curr_data->lanes.lanes[0].exist = %d\n", curr_data->lanes.lanes[0].exist);
		    if (curr_data->lanes.lanes[1].exist)
		        PNT_LOGD("nn_adas_result_cb curr_data->lanes.lanes[1].exist = %d\n", curr_data->lanes.lanes[1].exist);
		    if (curr_data->lanes.lanes[0].status)
		        PNT_LOGD("nn_adas_result_cb curr_data->lanes.lanes[0].exist = %d\n", curr_data->lanes.lanes[0].status);
		    if (curr_data->lanes.lanes[1].status)
		        PNT_LOGD("nn_adas_result_cb curr_data->lanes.lanes[1].exist = %d\n", curr_data->lanes.lanes[1].status);
		}

		adas_warning(curr_data);

		sem_post(&gAdasNN.frame_del_sem);
    } 
	else if (event == NN_ADAS_SELF_CALIBRATE)
	{
		PNT_LOGD("NN_ADAS_SELF_CALIBRATE\n");
		nn_adas_cfg_t *curr_data = (nn_adas_cfg_t *)data;
		
		PNT_LOGI("curr_data->camera_param.vanish_point: %d %d %d %f\n",curr_data->camera_param.vanish_point[0], curr_data->camera_param.vanish_point[1],
			curr_data->camera_param.bottom_line, curr_data->camera_param.camera_to_middle);

		adas_update_para(curr_data->camera_param.vanish_point[0], curr_data->camera_param.vanish_point[1], curr_data->camera_param.bottom_line);
	}
    
    return 0;


}


static EI_VOID *get_axframe_proc(EI_VOID *p)
{
    int ret;
    VIDEO_FRAME_INFO_S axFrame = {0};
    adas_nn_t *axframe_info;
    void *nn_hdl = NULL;
	char adaslib_ver[64]={0};

    axframe_info = (adas_nn_t *)p;
    PNT_LOGE("get_axframe_proc bThreadStart = %d\n", axframe_info->bThreadStart);
	
#ifdef SAVE_AX_YUV_SP
		char srcfilename[FULLNAME_LEN_MAX];
		sprintf(srcfilename, "%s/adas_raw_ch%d-%d.yuv", PATH_ROOT, axframe_info->chn);
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

    PNT_LOGE("********start_nn_adas******\n");
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
    axframe_info->m_nn_adas_cfg.camera_param.vanish_point[0] = gAdasAlgoParams->vanish_point_x;//adas_get_vanish_point_x(); //天际线和车中线交叉点X坐标 单位像素
    axframe_info->m_nn_adas_cfg.camera_param.vanish_point[1] = gAdasAlgoParams->vanish_point_y;//adas_get_vanish_point_y(); //天际线和车中线交叉点Y坐标 单位像素
    axframe_info->m_nn_adas_cfg.camera_param.bottom_line = gAdasAlgoParams->bottom_line_y;//adas_get_bottom_line_y(); //引擎盖线
    axframe_info->m_nn_adas_cfg.camera_param.camera_angle[0] = 80; //摄像头水平角度
    axframe_info->m_nn_adas_cfg.camera_param.camera_angle[1] = 50; //摄像头垂直角度
    axframe_info->m_nn_adas_cfg.camera_param.camera_to_ground = (float)gAdasAlgoParams->camera_to_ground/100; //与地面距离，单位米
    axframe_info->m_nn_adas_cfg.camera_param.car_head_to_camera= (float)gAdasAlgoParams->car_head_to_camera/100; //与车头距离 单位米
    axframe_info->m_nn_adas_cfg.camera_param.car_width= (float)gAdasAlgoParams->car_width/100; //车宽，单位米
    axframe_info->m_nn_adas_cfg.camera_param.camera_to_middle= (float)gAdasAlgoParams->camera_to_middle/100; //与车中线距离，单位米，建议设备安装在车的中间，左负右正
	axframe_info->m_nn_adas_cfg.calib_param.auto_calib_en = 1;//自动校准使能
	axframe_info->m_nn_adas_cfg.calib_param.calib_finish = 1;//设置为1，在自动校准前也会按默认参数报警, 设置为0，则在自动校准前不报警
 
	m_adas_cali.vanish_point_x = axframe_info->m_nn_adas_cfg.camera_param.vanish_point[0];
	m_adas_cali.vanish_point_y = axframe_info->m_nn_adas_cfg.camera_param.vanish_point[1];
	m_adas_cali.bottom_line_y = axframe_info->m_nn_adas_cfg.camera_param.bottom_line;
	
	axframe_info->m_nn_adas_cfg.ldw_en = 1;//车道偏移和变道报警
	axframe_info->m_nn_adas_cfg.hmw_en = 1;//保持车距报警
	axframe_info->m_nn_adas_cfg.ufcw_en = 0;//低速下前车碰撞报警，reserved备用
	axframe_info->m_nn_adas_cfg.fcw_en = 1;//前车碰撞报警
	axframe_info->m_nn_adas_cfg.pcw_en = 1;//行人碰撞报警
	axframe_info->m_nn_adas_cfg.fmw_en = 1;//前车启动报警，必须有GPS，且本车车速为0
	
	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.min_warn_dist = 0;//最小报警距离，单位米
	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.max_warn_dist = 100;//最大报警距离，单位米
	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.min_warn_gps = gAdasAlgoParams->hmw_speed;//最低报警车速，单位KM/H
	axframe_info->m_nn_adas_cfg.warn_param.hmw_param.max_warn_gps = 200;//最高报警车速，单位KM/H

	if (1 >= gAdasAlgoParams->hmw_sensitivity)
	{
		axframe_info->m_nn_adas_cfg.warn_param.hmw_param.gps_ttc_thresh[0] = 0.9f;//与前车碰撞小于这个时间报警(假设前车不动)，单位秒,距离/本车GPS速度，对应WARN_STATUS_HMW_LEVEL1报警状态
		axframe_info->m_nn_adas_cfg.warn_param.hmw_param.gps_ttc_thresh[1] = 0.3f;//与前车碰撞小于这个时间报警(假设前车不动)，单位秒,距离/本车GPS速度，对应WARN_STATUS_HMW_LEVEL2报警状态
	}
	else if (3 == gAdasAlgoParams->hmw_sensitivity)
	{
		axframe_info->m_nn_adas_cfg.warn_param.hmw_param.gps_ttc_thresh[0] = 1.5f;//与前车碰撞小于这个时间报警(假设前车不动)，单位秒,距离/本车GPS速度，对应WARN_STATUS_HMW_LEVEL1报警状态
		axframe_info->m_nn_adas_cfg.warn_param.hmw_param.gps_ttc_thresh[1] = 0.9f;//与前车碰撞小于这个时间报警(假设前车不动)，单位秒,距离/本车GPS速度，对应WARN_STATUS_HMW_LEVEL2报警状态
	}
	else
	{
		axframe_info->m_nn_adas_cfg.warn_param.hmw_param.gps_ttc_thresh[0] = 1.2f;//与前车碰撞小于这个时间报警(假设前车不动)，单位秒,距离/本车GPS速度，对应WARN_STATUS_HMW_LEVEL1报警状态
		axframe_info->m_nn_adas_cfg.warn_param.hmw_param.gps_ttc_thresh[1] = 0.6f;//与前车碰撞小于这个时间报警(假设前车不动)，单位秒,距离/本车GPS速度，对应WARN_STATUS_HMW_LEVEL2报警状态
	}

	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.min_warn_dist = 0;
	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.max_warn_dist = 100;
	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.min_warn_gps = gAdasAlgoParams->fcw_speed;
	axframe_info->m_nn_adas_cfg.warn_param.fcw_param.max_warn_gps = 200;
	if (1 >= gAdasAlgoParams->fcw_sensitivity)
	{
		axframe_info->m_nn_adas_cfg.warn_param.fcw_param.gps_ttc_thresh = 1.7f; //与前车碰撞报警时间(假设前车不动)，单位秒,距离/本车GPS速度
		axframe_info->m_nn_adas_cfg.warn_param.fcw_param.ttc_thresh = 2.2f; //与前车碰撞报警时间，单位秒,距离/相对车速，以上4个条件同时满足
	}
	else if (3 == gAdasAlgoParams->fcw_sensitivity)
	{
		axframe_info->m_nn_adas_cfg.warn_param.fcw_param.gps_ttc_thresh = 2.3f; //与前车碰撞报警时间(假设前车不动)，单位秒,距离/本车GPS速度
		axframe_info->m_nn_adas_cfg.warn_param.fcw_param.ttc_thresh = 2.8f; //与前车碰撞报警时间，单位秒,距离/相对车速，以上4个条件同时满足
	}
	else
	{
		axframe_info->m_nn_adas_cfg.warn_param.fcw_param.gps_ttc_thresh = 2.0f; //与前车碰撞报警时间(假设前车不动)，单位秒,距离/本车GPS速度
		axframe_info->m_nn_adas_cfg.warn_param.fcw_param.ttc_thresh = 2.5f; //与前车碰撞报警时间，单位秒,距离/相对车速，以上4个条件同时满足
	}

	axframe_info->m_nn_adas_cfg.warn_param.ufcw_param.min_warn_dist = 0;
	axframe_info->m_nn_adas_cfg.warn_param.ufcw_param.max_warn_dist = 100;
	axframe_info->m_nn_adas_cfg.warn_param.ufcw_param.min_warn_gps = 0;
	axframe_info->m_nn_adas_cfg.warn_param.ufcw_param.max_warn_gps = 30;
	axframe_info->m_nn_adas_cfg.warn_param.ufcw_param.relative_speed_thresh = -3.0f; //与前车碰撞相对车速，前车车速-本车车速小于这个速度

	if (1 >= gAdasAlgoParams->fmw_sensitivity)
	{
		axframe_info->m_nn_adas_cfg.warn_param.fmw_param.min_static_time = 8.0f;//前车静止的时间需大于多少，单位秒
		axframe_info->m_nn_adas_cfg.warn_param.fmw_param.warn_car_inside_dist = 4.0f;//与前车的距离在多少米内，单位米
		axframe_info->m_nn_adas_cfg.warn_param.fmw_param.warn_dist_leave = 1.2f;//前车启动大于多远后报警，单位米
	}
	else if (3 == gAdasAlgoParams->fmw_sensitivity)
	{
		axframe_info->m_nn_adas_cfg.warn_param.fmw_param.min_static_time = 2.0f;//前车静止的时间需大于多少，单位秒
		axframe_info->m_nn_adas_cfg.warn_param.fmw_param.warn_car_inside_dist = 8.0f;//与前车的距离在多少米内，单位米
		axframe_info->m_nn_adas_cfg.warn_param.fmw_param.warn_dist_leave = 0.3f;//前车启动大于多远后报警，单位米
	}
	else
	{
		axframe_info->m_nn_adas_cfg.warn_param.fmw_param.min_static_time = 5.0f;//前车静止的时间需大于多少，单位秒
		axframe_info->m_nn_adas_cfg.warn_param.fmw_param.warn_car_inside_dist = 6.0f;//与前车的距离在多少米内，单位米
		axframe_info->m_nn_adas_cfg.warn_param.fmw_param.warn_dist_leave = 0.6f;//前车启动大于多远后报警，单位米
	}

	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.min_warn_dist = 0;
	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.max_warn_dist = 100;
	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.min_warn_gps = gAdasAlgoParams->pcw_speed;
	axframe_info->m_nn_adas_cfg.warn_param.pcw_param.max_warn_gps = 200;
	if (1 >= gAdasAlgoParams->pcw_sensitivity)
	{
		axframe_info->m_nn_adas_cfg.warn_param.pcw_param.gps_ttc_thresh = 1.2f;
		axframe_info->m_nn_adas_cfg.warn_param.pcw_param.ttc_thresh = 3.0f;
	}
	else if (3 == gAdasAlgoParams->pcw_sensitivity)
	{
		axframe_info->m_nn_adas_cfg.warn_param.pcw_param.gps_ttc_thresh = 1.8f;
		axframe_info->m_nn_adas_cfg.warn_param.pcw_param.ttc_thresh = 7.0f;
	}
	else
	{
		axframe_info->m_nn_adas_cfg.warn_param.pcw_param.gps_ttc_thresh = 1.5f;
		axframe_info->m_nn_adas_cfg.warn_param.pcw_param.ttc_thresh = 5.0f;
	}

	axframe_info->m_nn_adas_cfg.warn_param.ldw_param.min_warn_gps = gAdasAlgoParams->ldw_speed;//最低报警车速，单位KM/H
	if (1 >= gAdasAlgoParams->ldw_sensitivity)
	{
		axframe_info->m_nn_adas_cfg.warn_param.ldw_param.ldw_warn_thresh = 0.8f; //在lane_warn_dist距离内持续时间，单位秒
		axframe_info->m_nn_adas_cfg.warn_param.ldw_param.lane_warn_dist = 0.0f;//与车道线距离，车道线内
		axframe_info->m_nn_adas_cfg.warn_param.ldw_param.change_lane_dist = 0.4f; //变道侧车道线离车的中点距离，单位米
	}
	else if (3 == gAdasAlgoParams->ldw_sensitivity)
	{
		axframe_info->m_nn_adas_cfg.warn_param.ldw_param.ldw_warn_thresh = 0.3f; //在lane_warn_dist距离内持续时间，单位秒
		axframe_info->m_nn_adas_cfg.warn_param.ldw_param.lane_warn_dist = 0.0f;//与车道线距离，车道线内
		axframe_info->m_nn_adas_cfg.warn_param.ldw_param.change_lane_dist = 0.9f; //变道侧车道线离车的中点距离，单位米
	}
	else
	{
		axframe_info->m_nn_adas_cfg.warn_param.ldw_param.ldw_warn_thresh = 0.5f; //在lane_warn_dist距离内持续时间，单位秒
		axframe_info->m_nn_adas_cfg.warn_param.ldw_param.lane_warn_dist = 0.0f;//与车道线距离，车道线内
		axframe_info->m_nn_adas_cfg.warn_param.ldw_param.change_lane_dist = 0.6f; //变道侧车道线离车的中点距离，单位米
	}

    /* open libadas.so*/
    nn_hdl = nn_adas_open(&axframe_info->m_nn_adas_cfg);
    if (nn_hdl == NULL)
	{
        PNT_LOGE("nn_adas_open() failed!");
        return NULL;
    }
	axframe_info->nn_hdl = nn_hdl;
	
#ifdef AL_PRO_FRAMERATE_STATISTIC
	time1 = osal_get_msec();
#endif

	sleep(5);

    while (EI_TRUE == axframe_info->bThreadStart) 
	{
        memset(&axFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
		
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time_GetChnFrame1 = osal_get_msec();
#endif
		if (!gACCState || gCPUTemprature >= 95)
		{
			usleep(1000*1000);
			continue;
		}
		
		ret = EI_MI_IPPU_GetChnFrame(axframe_info->dev, axframe_info->chn, &axFrame, 1000);
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

        nn_adas_start(&axFrame, nn_hdl);

#ifdef AL_PRO_FRAMERATE_STATISTIC
	time_AlProFrame2 = osal_get_msec();
	PNT_LOGD("time_GetChnFrame %lld ms, time_AlProFrame %lld ms\n", time_GetChnFrame2-time_GetChnFrame1, time_AlProFrame2-time_AlProFrame1);
#endif

#ifdef AL_PRO_FRAMERATE_STATISTIC
		frame_cnt++;
		time2 = osal_get_msec();
		if ((time2 - time1) >= 3000) {
			PNT_LOGD("algo process %lld fps\n", (frame_cnt * 1000) / (time2 - time1));
			time1 = time2;
			frame_cnt  = 0;
		}
#endif

		EI_MI_IPPU_ReleaseChnFrame(axframe_info->dev, axframe_info->chn, &axFrame);
    }

	PNT_LOGE("axframe_info->dev %d %d\n", axframe_info->dev, axframe_info->chn);
	PNT_LOGE("nn_adas_close 0\n");
	
    nn_adas_close(nn_hdl);
	
	PNT_LOGE("nn_adas_close 1\n");

    return NULL;
}

static EI_VOID *rgn_draw_proc(EI_VOID *p)
{
    adas_nn_t *axframe_info = &gAdasNN;

	unsigned char lastState = gAlComParam.mAdasRender;

	rgn_start();

    while (EI_TRUE == axframe_info->bThreadStart)
	{
		if (lastState != gAlComParam.mAdasRender)
		{
			lastState = gAlComParam.mAdasRender;
			
			if (gAlComParam.mAdasRender)
			{
				if(EI_MI_RGN_AddToChn(gAdasNN.Handle[0], &gAdasNN.stRgnChn, &gAdasNN.stRgnChnAttr[0]))
				{
					PNT_LOGE("EI_MI_RGN_AddToChn \n");
				}

				if(EI_MI_RGN_AddToChn(gAdasNN.Handle[1], &gAdasNN.stRgnChn, &gAdasNN.stRgnChnAttr[1]))
				{
					PNT_LOGE("EI_MI_RGN_AddToChn \n");
				}
			}
			else
			{
				EI_MI_RGN_DelFromChn(gAdasNN.Handle[0], &gAdasNN.stRgnChn);
				EI_MI_RGN_DelFromChn(gAdasNN.Handle[1], &gAdasNN.stRgnChn);
			}
		}
		else
		{
			if (gAlComParam.mAdasRender)
			{
				pthread_mutex_lock(&axframe_info->p_det_mutex);

				adas_draw_location_rgn(axframe_info->p_det_info);

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

static int start_get_axframe(adas_nn_t *axframe_info)
{
    int ret;

    axframe_info->bThreadStart = EI_TRUE;
    ret = pthread_create(&(axframe_info->gs_framePid), NULL, get_axframe_proc, (EI_VOID *)axframe_info);

	pthread_t pid;
    ret = pthread_create(&pid, NULL, rgn_draw_proc, (EI_VOID *)axframe_info);

    PNT_LOGD("start_get_axframe success %#x! phyChn:%d chn :%d dev:%d\n", ret, axframe_info->phyChn, axframe_info->chn, axframe_info->dev);

    return EI_SUCCESS;
}

int adas_nn_init(int vissDev, int vissChn, int chn, int width, int height)
{
    adas_nn_t *axframe_info = &gAdasNN;

	if (gAdasAlgoParams->fcw_sensitivity == 0 && gAdasAlgoParams->hmw_sensitivity == 0 && gAdasAlgoParams->fmw_sensitivity == 0 &&
		gAdasAlgoParams->pcw_sensitivity == 0 && gAdasAlgoParams->ldw_sensitivity == 0)
	{
		return 0;
	}

    pthread_mutex_init(&axframe_info->p_det_mutex, NULL);
	
    axframe_info->p_det_info = malloc(sizeof(nn_adas_out_t));
	memset(axframe_info->p_det_info, 0, sizeof(nn_adas_out_t));
	
    if (!axframe_info->p_det_info)
    {
		PNT_LOGE("axframe_info->p_det_info malloc failed!\n");
		return -1;
    }
	
	if (axframe_info->action == ADAS_CALIB)
    {
		m_adas_cali.cali_flag = ADAS_CALI_START;
		m_adas_cali.adas_calibline = VANISH_Y_LINE;
		m_adas_cali.adas_calibstep = 5;
		
		PNT_LOGI("m_adas_cali.vanish_point_x %f %f %f\n", m_adas_cali.vanish_point_x, m_adas_cali.vanish_point_y, m_adas_cali.bottom_line_y);
	}

	axframe_info->dev = vissDev;
	axframe_info->chn = chn;
	axframe_info->phyChn = vissChn;
    axframe_info->frameRate = 25;
    axframe_info->u32Width = width;
    axframe_info->u32Height = height;

    sem_init(&axframe_info->frame_del_sem, 0, 0);

	gAdasNN.u32scnWidth = width;
	gAdasNN.u32scnHeight = height;

	adas_al_para_init();

    start_get_axframe(axframe_info);

    return 0;
}

void adas_nn_stop(void)
{
    adas_nn_t *axframe_info = &gAdasNN;

	if (axframe_info->bThreadStart)
	{
		axframe_info->bThreadStart = EI_FALSE;

		usleep(200*1000);

		rgn_stop();
	}
}




