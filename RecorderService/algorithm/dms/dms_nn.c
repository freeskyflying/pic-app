
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "pnt_log.h"
#include "dms_nn.h"
#include "dms_config.h"
#include "media_utils.h"
#include "facerecg_nn.h"

dms_nn_t gDmsNN = { 0 };
dms_calibrate_para_t m_dms_cali;

#define AL_PRO_FRAMERATE_STATISTIC

#define DRIVERMOSAIC_HANDLE		20

static EI_S32 rgn_start(void)
{
	EI_S32 s32Ret = EI_FAILURE;

	MDP_CHN_S* pStRgnChn = &gDmsNN.stRgnChn;
	
	pStRgnChn->enModId = EI_ID_VISS;
	pStRgnChn->s32ChnId = gDmsNN.phyChn;
	pStRgnChn->s32DevId = gDmsNN.dev;
	
	RGN_ATTR_S* pStRgnAttr = &gDmsNN.stRegion[0];
    pStRgnAttr->enType = RECTANGLEEX_ARRAY_RGN;
    pStRgnAttr->unAttr.u32RectExArrayMaxNum = DMS_RGN_RECT_NUM;
	
	gDmsNN.Handle[0] = 8;
    s32Ret = EI_MI_RGN_Create(gDmsNN.Handle[0], pStRgnAttr);
    if(s32Ret)
	{
        PNT_LOGE("EI_MI_RGN_Create \n");
        goto exit;
    }

	pStRgnAttr = &gDmsNN.stRegion[1];
    pStRgnAttr->enType = LINEEX_ARRAY_RGN;
    pStRgnAttr->unAttr.u32LineExArrayMaxNum = DMS_RGN_LINE_NUM;
	
	gDmsNN.Handle[1] = 9;
    s32Ret = EI_MI_RGN_Create(gDmsNN.Handle[1], pStRgnAttr);
    if(s32Ret)
	{
        PNT_LOGE("EI_MI_RGN_Create \n");
        goto exit;
    }
	
	RGN_CHN_ATTR_S* pStRgnChnAttr = &gDmsNN.stRgnChnAttr[0];
	
	pStRgnChnAttr->bShow = EI_TRUE;
    pStRgnChnAttr->enType = RECTANGLEEX_ARRAY_RGN;
    pStRgnChnAttr->unChnAttr.stRectangleExArrayChn.u32MaxRectNum = DMS_RGN_RECT_NUM;
    pStRgnChnAttr->unChnAttr.stRectangleExArrayChn.u32ValidRectNum = DMS_RGN_RECT_NUM;
    pStRgnChnAttr->unChnAttr.stRectangleExArrayChn.pstRectExArraySubAttr = gDmsNN.stRectArray;

	pStRgnChnAttr = &gDmsNN.stRgnChnAttr[1];
	pStRgnChnAttr->bShow = EI_TRUE;
    pStRgnChnAttr->enType = LINEEX_ARRAY_RGN;
    pStRgnChnAttr->unChnAttr.stLineExArrayChn.u32MaxLineNum = DMS_RGN_LINE_NUM;
    pStRgnChnAttr->unChnAttr.stLineExArrayChn.u32ValidLineNum = DMS_RGN_LINE_NUM;
    pStRgnChnAttr->unChnAttr.stLineExArrayChn.pstLineExArraySubAttr = gDmsNN.stLineArray;

	return EI_SUCCESS;
exit:
	return s32Ret;
}

static void rgn_stop(void)
{
	int i;

	for (i = 0;i < DMS_RGN_ARRAY_NUM; i++)
	{
		if (gAlComParam.mDmsRender)
			EI_MI_RGN_DelFromChn(gDmsNN.Handle[i], &gDmsNN.stRgnChn);
		EI_MI_RGN_Destroy(gDmsNN.Handle[i]);
	}
}

static void dms_draw_location_rgn(nn_dms_out_t *p_det_info)
{
	int draw_num;
	int index = 0;
	int loc_x0, loc_y0, loc_x1, loc_y1;
	EI_S32 s32Ret = EI_FAILURE;

	if (gDmsNN.action == DMS_CALIB) 
	{
		if (p_det_info->faces.size)
		{
			if (p_det_info->faces.driver_idx < 0 || p_det_info->faces.driver_idx >= p_det_info->faces.size)
			{
				//PNT_LOGE("p_det_info->faces.driver_idx %d\n", p_det_info->faces.driver_idx);
				/* if driver location not found , use idx 0 */
				loc_x0 = p_det_info->faces.p[0].box_smooth[0];
		        loc_y0 = p_det_info->faces.p[0].box_smooth[1];
		        loc_x1 = p_det_info->faces.p[0].box_smooth[2];
		        loc_y1 = p_det_info->faces.p[0].box_smooth[3];
			}
			else
			{
		        loc_x0 = p_det_info->faces.p[p_det_info->faces.driver_idx].box_smooth[0];
		        loc_y0 = p_det_info->faces.p[p_det_info->faces.driver_idx].box_smooth[1];
		        loc_x1 = p_det_info->faces.p[p_det_info->faces.driver_idx].box_smooth[2];
		        loc_y1 = p_det_info->faces.p[p_det_info->faces.driver_idx].box_smooth[3];
			}
	        gDmsNN.stRectArray[0].stRectAttr.stRect.s32X = loc_x0;
			gDmsNN.stRectArray[0].stRectAttr.stRect.s32Y = loc_y0;
			gDmsNN.stRectArray[0].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
			gDmsNN.stRectArray[0].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
			gDmsNN.stRectArray[0].stRectAttr.u32BorderSize = 4;
			if (m_dms_cali.cali_flag == DMS_CALI_END)
				gDmsNN.stRectArray[0].stRectAttr.u32Color = 0xff00ff00;
			else
				gDmsNN.stRectArray[0].stRectAttr.u32Color = 0xffff0000;
			gDmsNN.stRectArray[0].isShow = EI_TRUE;
			gDmsNN.stRgnChnAttr[0].bShow = EI_TRUE;
			s32Ret = EI_MI_RGN_SetChnAttr(gDmsNN.Handle[0], &gDmsNN.stRgnChn, &gDmsNN.stRgnChnAttr[0]);
			if(s32Ret)
			{
				PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
			}
	    } 
		else
		{
			gDmsNN.stRectArray[0].isShow = EI_FALSE;
			s32Ret = EI_MI_RGN_SetChnAttr(gDmsNN.Handle[0], &gDmsNN.stRgnChn, &gDmsNN.stRgnChnAttr[0]);
			if(s32Ret)
			{
				PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
			}
		}

	} 
	else
	{
		draw_num = p_det_info->faces.size + p_det_info->obj_boxes[0].size + p_det_info->obj_boxes[1].size + p_det_info->obj_boxes[2].size +
			p_det_info->obj_boxes[3].size + p_det_info->heads.size + p_det_info->persons.size + 1;
		if (draw_num > DMS_RGN_RECT_NUM)
			return;
		if (gDmsNN.last_draw_num > draw_num) 
		{
			for (int i = gDmsNN.last_draw_num-1; i >= draw_num; i--)
			{
				gDmsNN.stRectArray[i].isShow = EI_FALSE;
				s32Ret = EI_MI_RGN_SetChnAttr(gDmsNN.Handle[0], &gDmsNN.stRgnChn, &gDmsNN.stRgnChnAttr[0]);
				if(s32Ret)
				{
					PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
				}
			}
		}
		
		gDmsNN.stRectArray[index].stRectAttr.stRect.s32X = gDmsAlgoParams->driver_rect_xs;
		gDmsNN.stRectArray[index].stRectAttr.stRect.s32Y = gDmsAlgoParams->driver_rect_ys;
		gDmsNN.stRectArray[index].stRectAttr.stRect.u32Height = gDmsAlgoParams->driver_rect_ye - gDmsAlgoParams->driver_rect_ys;
		gDmsNN.stRectArray[index].stRectAttr.stRect.u32Width = gDmsAlgoParams->driver_rect_xe - gDmsAlgoParams->driver_rect_xs;
		gDmsNN.stRectArray[index].stRectAttr.u32BorderSize = 4;
		gDmsNN.stRectArray[index].stRectAttr.u32Color = 0xffff00ff;
		gDmsNN.stRectArray[index].isShow = EI_TRUE;
		gDmsNN.stRgnChnAttr[0].bShow = EI_TRUE;
		EI_MI_RGN_SetChnAttr(gDmsNN.Handle[0], &gDmsNN.stRgnChn, &gDmsNN.stRgnChnAttr[0]);
		
		index = index + 1;
	    if (p_det_info->faces.size)
		{
			//PNT_LOGE("p_det_info->faces.size %d \n", p_det_info->faces.size);
			for (int cnt_faces = 0; cnt_faces < p_det_info->faces.size; cnt_faces++)
			{
		        loc_x0 = p_det_info->faces.p[cnt_faces].box_smooth[0];
		        loc_y0 = p_det_info->faces.p[cnt_faces].box_smooth[1];
		        loc_x1 = p_det_info->faces.p[cnt_faces].box_smooth[2];
		        loc_y1 = p_det_info->faces.p[cnt_faces].box_smooth[3];
		        gDmsNN.stRectArray[index+cnt_faces].stRectAttr.stRect.s32X = loc_x0;
				gDmsNN.stRectArray[index+cnt_faces].stRectAttr.stRect.s32Y = loc_y0;
				gDmsNN.stRectArray[index+cnt_faces].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
				gDmsNN.stRectArray[index+cnt_faces].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
				gDmsNN.stRectArray[index+cnt_faces].stRectAttr.u32BorderSize = 4;
				if (cnt_faces == p_det_info->faces.driver_idx)
				{
					gDmsNN.stRectArray[index+cnt_faces].stRectAttr.u32Color = 0xff8000ff;
				}
				else
				{
					gDmsNN.stRectArray[index+cnt_faces].stRectAttr.u32Color = 0xffffff80;
				}
				gDmsNN.stRectArray[index+cnt_faces].isShow = EI_TRUE;
				gDmsNN.stRgnChnAttr[0].bShow = EI_TRUE;
				s32Ret = EI_MI_RGN_SetChnAttr(gDmsNN.Handle[0], &gDmsNN.stRgnChn, &gDmsNN.stRgnChnAttr[0]);
				if(s32Ret)
				{
					PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
				}
			}
	    }
		index = index + p_det_info->faces.size;

		index = index + p_det_info->obj_boxes[0].size;
	    for (int cnt_phones = 0; cnt_phones < p_det_info->obj_boxes[1].size; cnt_phones++)
		{
	        loc_x0 = p_det_info->obj_boxes[1].p[cnt_phones].box[0];
	        loc_y0 = p_det_info->obj_boxes[1].p[cnt_phones].box[1];
	        loc_x1 = p_det_info->obj_boxes[1].p[cnt_phones].box[2];
	        loc_y1 = p_det_info->obj_boxes[1].p[cnt_phones].box[3];
	        gDmsNN.stRectArray[index+cnt_phones].stRectAttr.stRect.s32X = loc_x0;
			gDmsNN.stRectArray[index+cnt_phones].stRectAttr.stRect.s32Y = loc_y0;
			gDmsNN.stRectArray[index+cnt_phones].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
			gDmsNN.stRectArray[index+cnt_phones].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
			gDmsNN.stRectArray[index+cnt_phones].stRectAttr.u32BorderSize = 4;
			gDmsNN.stRectArray[index+cnt_phones].stRectAttr.u32Color = 0xffff0000;
			gDmsNN.stRectArray[index+cnt_phones].isShow = EI_TRUE;
			gDmsNN.stRgnChnAttr[0].bShow = EI_TRUE;
			s32Ret = EI_MI_RGN_SetChnAttr(gDmsNN.Handle[0], &gDmsNN.stRgnChn, &gDmsNN.stRgnChnAttr[0]);
			if(s32Ret)
			{
				PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
	    	}
	    }
		index = index + p_det_info->obj_boxes[1].size;
	    for (int cnt_smokes = 0; cnt_smokes < p_det_info->obj_boxes[2].size; cnt_smokes++)
		{
	        loc_x0 = p_det_info->obj_boxes[2].p[cnt_smokes].box[0];
	        loc_y0 = p_det_info->obj_boxes[2].p[cnt_smokes].box[1];
	        loc_x1 = p_det_info->obj_boxes[2].p[cnt_smokes].box[2];
	        loc_y1 = p_det_info->obj_boxes[2].p[cnt_smokes].box[3];
	        gDmsNN.stRectArray[index+cnt_smokes].stRectAttr.stRect.s32X = loc_x0;
			gDmsNN.stRectArray[index+cnt_smokes].stRectAttr.stRect.s32Y = loc_y0;
			gDmsNN.stRectArray[index+cnt_smokes].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
			gDmsNN.stRectArray[index+cnt_smokes].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
			gDmsNN.stRectArray[index+cnt_smokes].stRectAttr.u32BorderSize = 4;
			gDmsNN.stRectArray[index+cnt_smokes].stRectAttr.u32Color = 0xff0000ff;
			gDmsNN.stRectArray[index+cnt_smokes].isShow = EI_TRUE;
			gDmsNN.stRgnChnAttr[0].bShow = EI_TRUE;
			s32Ret = EI_MI_RGN_SetChnAttr(gDmsNN.Handle[0], &gDmsNN.stRgnChn, &gDmsNN.stRgnChnAttr[0]);
			if(s32Ret)
			{
				PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
	    	}
	    }
		index = index + p_det_info->obj_boxes[2].size;
		for (int cnt_belt = 0; cnt_belt < p_det_info->obj_boxes[3].size; cnt_belt++)
		{
			loc_x0 = p_det_info->obj_boxes[3].p[cnt_belt].box[0];
			loc_y0 = p_det_info->obj_boxes[3].p[cnt_belt].box[1];
			loc_x1 = p_det_info->obj_boxes[3].p[cnt_belt].box[2];
			loc_y1 = p_det_info->obj_boxes[3].p[cnt_belt].box[3];
			gDmsNN.stRectArray[index+cnt_belt].stRectAttr.stRect.s32X = loc_x0;
			gDmsNN.stRectArray[index+cnt_belt].stRectAttr.stRect.s32Y = loc_y0;
			gDmsNN.stRectArray[index+cnt_belt].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
			gDmsNN.stRectArray[index+cnt_belt].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
			gDmsNN.stRectArray[index+cnt_belt].stRectAttr.u32BorderSize = 4;
			gDmsNN.stRectArray[index+cnt_belt].stRectAttr.u32Color = 0xffff00ff;
			gDmsNN.stRectArray[index+cnt_belt].isShow = EI_TRUE;
			gDmsNN.stRgnChnAttr[0].bShow = EI_TRUE;
			s32Ret = EI_MI_RGN_SetChnAttr(gDmsNN.Handle[0], &gDmsNN.stRgnChn, &gDmsNN.stRgnChnAttr[0]);
			if(s32Ret)
			{
				PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
			}
		}
		index = index + p_det_info->obj_boxes[3].size;
		if (p_det_info->heads.size)
		{
			for (int cnt_heads = 0; cnt_heads < p_det_info->heads.size; cnt_heads++)
			{
			        loc_x0 = p_det_info->heads.p[cnt_heads].box_smooth[0];
			        loc_y0 = p_det_info->heads.p[cnt_heads].box_smooth[1];
			        loc_x1 = p_det_info->heads.p[cnt_heads].box_smooth[2];
			        loc_y1 = p_det_info->heads.p[cnt_heads].box_smooth[3];
					//PNT_LOGE("heads %d %d %d %d \n", loc_x0, loc_y0, loc_x1, loc_y1);
			        gDmsNN.stRectArray[index+cnt_heads].stRectAttr.stRect.s32X = loc_x0;
					gDmsNN.stRectArray[index+cnt_heads].stRectAttr.stRect.s32Y = loc_y0;
					gDmsNN.stRectArray[index+cnt_heads].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
					gDmsNN.stRectArray[index+cnt_heads].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
					gDmsNN.stRectArray[index+cnt_heads].stRectAttr.u32BorderSize = 4;
					if (cnt_heads == p_det_info->heads.driver_idx)
					{
						gDmsNN.stRectArray[index+cnt_heads].stRectAttr.u32Color = 0xffffffff;
					} 
					else
					{
						//PNT_LOGE("p_det_info->heads.driver_idx %d \n", p_det_info->heads.driver_idx);
						gDmsNN.stRectArray[index+cnt_heads].stRectAttr.u32Color = 0xff000000;
					}
					gDmsNN.stRectArray[index+cnt_heads].isShow = EI_TRUE;
					gDmsNN.stRgnChnAttr[0].bShow = EI_TRUE;
					s32Ret = EI_MI_RGN_SetChnAttr(gDmsNN.Handle[0], &gDmsNN.stRgnChn, &gDmsNN.stRgnChnAttr[0]);
					if(s32Ret)
					{
						PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
					}
				}
		}
		index = index + p_det_info->heads.size;
		if (p_det_info->persons.size)
		{
			//PNT_LOGE("p_det_info->persons.size %d \n", p_det_info->persons.size);
			for (int cnt_persons = 0; cnt_persons < p_det_info->persons.size; cnt_persons++)
			{
			        loc_x0 = p_det_info->persons.p[cnt_persons].box_smooth[0];
			        loc_y0 = p_det_info->persons.p[cnt_persons].box_smooth[1];
			        loc_x1 = p_det_info->persons.p[cnt_persons].box_smooth[2];
			        loc_y1 = p_det_info->persons.p[cnt_persons].box_smooth[3];
					//PNT_LOGE("persons %d %d %d %d \n", loc_x0, loc_y0, loc_x1, loc_y1);
			        gDmsNN.stRectArray[index+cnt_persons].stRectAttr.stRect.s32X = loc_x0;
					gDmsNN.stRectArray[index+cnt_persons].stRectAttr.stRect.s32Y = loc_y0;
					gDmsNN.stRectArray[index+cnt_persons].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
					gDmsNN.stRectArray[index+cnt_persons].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
					gDmsNN.stRectArray[index+cnt_persons].stRectAttr.u32BorderSize = 4;
					if (cnt_persons == p_det_info->persons.driver_idx)
					{
						gDmsNN.stRectArray[index+cnt_persons].stRectAttr.u32Color = 0xff00ff00;
					}
					else
					{
						gDmsNN.stRectArray[index+cnt_persons].stRectAttr.u32Color = 0xffff00ff;
					}
					gDmsNN.stRectArray[index+cnt_persons].isShow = EI_TRUE;
					gDmsNN.stRgnChnAttr[0].bShow = EI_TRUE;
					s32Ret = EI_MI_RGN_SetChnAttr(gDmsNN.Handle[0], &gDmsNN.stRgnChn, &gDmsNN.stRgnChnAttr[0]);
					if(s32Ret)
					{
						PNT_LOGE("EI_MI_RGN_SetChnAttr \n");
					}
				}
		}
		gDmsNN.last_draw_num = draw_num;
	}
}

int nn_dms_start(VIDEO_FRAME_INFO_S *frame, void *nn_hdl)
{
    EI_S32 s32Ret;
    static nn_dms_in_t m_dms_in;
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
	
    m_dms_in.img.y_phy = (void *)frame->stVFrame.u64PlanePhyAddr[0];
    m_dms_in.img.y = (void *)gAlComParam.pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[0];
	if (gDmsNN.gray_flag == 1)
	{
		m_dms_in.img.uv_phy = (void *)gDmsNN.gray_uv_phy_addr;
		m_dms_in.img.uv = (void *)gDmsNN.gray_uv_vir_addr;
	}
	else
	{
		m_dms_in.img.uv_phy = (void *)frame->stVFrame.u64PlanePhyAddr[1];
		m_dms_in.img.uv = (void *)gAlComParam.pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[1];
	}
    m_dms_in.img.width = gDmsNN.u32Width;
    m_dms_in.img.height = gDmsNN.u32Height;
    m_dms_in.img.format = YUV420_SP;
    m_dms_in.cover_det_en = 1;
    m_dms_in.red_resist_glasses_en = 1;
	m_dms_in.auto_calib_en = 0;
	if (gDmsNN.livingdet_flag == LIVING_DET_SINGLE_RGB || gDmsNN.livingdet_flag == LIVING_DET_SINGLE_NIR)
	{
		if (gDmsNN.livingdet_hdl)
		{
			gDmsNN.face.img_handle.fmt = EZAX_YUV420_SP;
			gDmsNN.face.img_handle.w = gDmsNN.u32Width;//输入人脸图像的大小
			gDmsNN.face.img_handle.h = gDmsNN.u32Height;
			gDmsNN.face.img_handle.stride = gDmsNN.u32Width;
			gDmsNN.face.img_handle.c = 3; // chanel,reserved
			gDmsNN.face.nUID = 0;// mark id,reserved
			gDmsNN.face.img_handle.pVir = (void *)gAlComParam.pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[0];
			gDmsNN.face.img_handle.pPhy = (unsigned int)frame->stVFrame.u64PlanePhyAddr[0];
			gDmsNN.face.img_handle.pVir_UV = (void *)gAlComParam.pool_info[pool_idx].pstBufMap[frame->u32Idx].stVfrmInfo.stVFrame.ulPlaneVirAddr[1];
			gDmsNN.face.img_handle.pPhy_UV = (unsigned int)frame->stVFrame.u64PlanePhyAddr[1];
			gDmsNN.face.roi.x0 = 0;//检测人脸的有效区域
			gDmsNN.face.roi.y0 = 0;
			gDmsNN.face.roi.x1 = gDmsNN.u32Width-1;
			gDmsNN.face.roi.y1 = gDmsNN.u32Height-1;
		}
	}
		
    if (nn_hdl != NULL)
	{
        s32Ret = nn_dms_cmd(nn_hdl, NN_DMS_SET_DATA, &m_dms_in);
		if (s32Ret < 0)
		{
			PNT_LOGE("NN_DMS_SET_DATA failed %d!\n", s32Ret);
		}
    }
	
#ifdef SAVE_AX_YUV_SP
		if (gDmsNN.ax_fout) 
		{
			if (m_dms_in.img.format == YUV420_SP)
			{
				fwrite((void *)m_dms_in.img.y, 1, m_dms_in.img.width*m_dms_in.img.height, gDmsNN.ax_fout);
				fwrite((void *)m_dms_in.img.uv, 1, m_dms_in.img.width*m_dms_in.img.height/2, gDmsNN.ax_fout);
			}
			fclose(gDmsNN.ax_fout);
			gDmsNN.ax_fout = NULL;
		}
#endif

    sem_wait(&gDmsNN.frame_del_sem);

    return 0;
}

#if 0

int nn_dms_passenger_detect(nn_dms_out_t *data)
{
	static uint8_t face_inc_flag = 0, face_dec_flag = 0, last_face_cnt = 0;
	static uint8_t frame_count = 0;

	uint8_t face_head_cnt = data->faces.size;

	if (data->heads.size > face_head_cnt)
	{
		face_head_cnt = data->heads.size;
	}

	if (gAlComParam.mCurrentSpeed > 20 || 0 == gDmsAlgoParams->passenger_enable)
	{
		face_inc_flag = 0;
		face_dec_flag = 0;
		last_face_cnt = face_head_cnt;
		return 0;
	}

	if (face_head_cnt > last_face_cnt && face_head_cnt > 1)
	{
		if (0 == face_inc_flag)
		{
			face_inc_flag = 1;
			face_dec_flag = 0;
			frame_count = 0;
		}
		else
		{
			frame_count ++;
			if (frame_count > 10)
			{
				face_inc_flag = 0;
				frame_count = 0;
				PNT_LOGI("detect increase faces [%d]>[%d].", face_head_cnt, last_face_cnt);
				last_face_cnt = face_head_cnt;
				return 1;
			}
		}
	}
	else if (face_head_cnt < last_face_cnt)
	{
		if (0 == face_dec_flag)
		{
			face_inc_flag = 0;
			face_dec_flag = 1;
			frame_count = 0;
		}
		else
		{
			frame_count ++;
			if (frame_count > 30)
			{
				face_dec_flag = 0;
				frame_count = 0;
				PNT_LOGI("detect decrease faces [%d]<[%d].", face_head_cnt, last_face_cnt);
				last_face_cnt = face_head_cnt;
				return 1;
			}
		}
	}

	return 0;
}
#else

#define GET_MAX(a,b)		(a>b?a:b)

static uint32_t passengerReportTime = 0;
extern void JT808Controller_ReportPassengers(uint32_t start_time, int pg_count_on, int pg_count_off);
int nn_dms_passenger_detect(nn_dms_out_t *data)
{
	static uint32_t lastTime = 0;
	static uint8_t min_face_count = 0xff, max_face_count = 0, getoff_pg_count = 0;
	int ret = 0;

	if (0 == gDmsAlgoParams->passenger_enable)
	{
		return ret;
	}

	if (getUptime()-lastTime <= 0)
	{
		return ret;
	}

	lastTime = getUptime();

	uint8_t face_head_cnt = GET_MAX(data->faces.size, data->heads.size);

	if (gAlComParam.mCurrentSpeed <= 10)
	{
		if (min_face_count > face_head_cnt)
		{
			min_face_count = face_head_cnt;
			
			if (max_face_count > min_face_count)
			{
				getoff_pg_count = max_face_count - min_face_count - (min_face_count>0?0:1);
				
				PNT_LOGI("report geton count:%d getoff count:%d", max_face_count-min_face_count-(min_face_count>0?0:1), getoff_pg_count);
			
				if (getoff_pg_count > 0)
				{
					JT808Controller_ReportPassengers(passengerReportTime, 0, getoff_pg_count);
					max_face_count = min_face_count;
					getoff_pg_count = 0;
					passengerReportTime = currentTimeMillis()/1000;
					ret = 1;
				}
			}
		}
	}
	else if (gAlComParam.mCurrentSpeed >= 30)
	{
		if (max_face_count < face_head_cnt)
		{
			max_face_count = face_head_cnt;
			
			if (max_face_count > min_face_count)
			{
				PNT_LOGI("report geton count:%d getoff count:%d", max_face_count-min_face_count-(min_face_count>0?0:1), getoff_pg_count);
				if (max_face_count-min_face_count-(min_face_count>0?0:1) > 0 || getoff_pg_count > 0)
				{
					JT808Controller_ReportPassengers(passengerReportTime, max_face_count-min_face_count-(min_face_count>0?0:1), getoff_pg_count);
					ret = 1;
				}
				min_face_count = max_face_count;
				getoff_pg_count = 0;
				passengerReportTime = currentTimeMillis()/1000;
			}
		}
	}

	PNT_LOGI("update max count:%d last min count:%d", max_face_count, min_face_count);

	return ret;
}

#endif

static int gLastCoverNodriver = 0;
static int gFacerecgDriverState = 0x00;
int dms_warning(nn_dms_out_t *data)
{
	nn_dms_warn_status_t *warn_status = NULL;
	int64_t cur_time = osal_get_msec();
	uint32_t index;
	
#ifdef ALARM_THINING
	nn_dms_face_t *face = NULL;
	nn_dms_state_t *state = NULL;

	if (data->faces.driver_idx < 0 || data->faces.driver_idx >= data->faces.size) {
		PNT_LOGE("data->faces.driver_idx %d\n", data->faces.driver_idx);
		/* if driver location not found , use idx 0 */
		face = &data->faces.p[0];
	} else {
		face = &data->faces.p[data->faces.driver_idx];
	}
	state = &face->face_attr.dms_state;
#endif

	if (gDmsAlgoParams->belt_sensitivity==0 && gDmsAlgoParams->smok_sensitivity==0 && 
		gDmsAlgoParams->call_sensitivity==0 && gDmsAlgoParams->dist_sensitivity==0 &&
		gDmsAlgoParams->fati_sensitivity==0)
	{
		return 0;
	}

	warn_status = &data->warn_status;

	PNT_LOGD("dms warn_status drink-%d call-%d smoke-%d distracted-%d fatigue_level-%d red_resist_glasses-%d cover-%d driver_leave-%d no_face_mask-%d no_face-%d no_belt-%d\n",
			warn_status->drink, warn_status->call, warn_status->smoke,
			warn_status->distracted, warn_status->fatigue_level,
			warn_status->red_resist_glasses, warn_status->cover,
			warn_status->driver_leave, warn_status->no_face_mask, warn_status->no_face, warn_status->no_belt);
	PNT_LOGD("dms current frame warn_status cover_state:%d driver_leave:%d no_face:%d no_belt:%d\n",
		data->cover_state, data->driver_leave, data->no_face, data->no_belt);
	PNT_LOGD("yawn_count-%d, eyeclose_count-%d %d %d %d\n", data->debug_info.yawn_count,
		data->debug_info.eyeclose_count[0], data->debug_info.eyeclose_count[1], data->debug_info.eyeclose_count[2],  data->debug_info.eyeclose_count[3]);

	if (data->faces.size > 0)
	{
		if (warn_status->drink)
		{
			al_warning_proc(LB_WARNING_DMS_DRINK, 0, 0, 0);
		}
		if (warn_status->call)
		{
			if (gDmsAlgoParams->call_sensitivity!=0)
				al_warning_proc(LB_WARNING_DMS_CALL, 0, 0, 0);
		}
		if (warn_status->smoke)
		{
			if (gDmsAlgoParams->smok_sensitivity!=0)
				al_warning_proc(LB_WARNING_DMS_SMOKE, 0, 0, 0);
		}
	#ifdef ALARM_THINING
		if (warn_status->fatigue_level == 2)
		{
			al_warning_proc(LB_WARNING_DMS_REST_LEVEL2, warn_status->fatigue_level, 0, 0);
		} 
		else if (warn_status->fatigue_level == 4)
		{
			al_warning_proc(LB_WARNING_DMS_REST_LEVEL4, warn_status->fatigue_level, 0, 0);

		} 
		else if (warn_status->fatigue_level == 6)
		{
			al_warning_proc(LB_WARNING_DMS_REST_LEVEL6, warn_status->fatigue_level, 0, 0);
		} 
		else if (warn_status->fatigue_level == 8) 
		{
			al_warning_proc(LB_WARNING_DMS_REST_LEVEL8, warn_status->fatigue_level, 0, 0);
		}
		else if (warn_status->fatigue_level == 10)
		{
			al_warning_proc(LB_WARNING_DMS_REST_LEVEL10, warn_status->fatigue_level, 0, 0);
		}
		if (warn_status->distracted) 
		{
			if (state->lookaround) 
			{
				al_warning_proc(LB_WARNING_DMS_ATTATION_LOOKAROUND, 0, 0, 0);
			}
			else if (state->lookup)
			{
				al_warning_proc(LB_WARNING_DMS_ATTATION_LOOKUP, 0, 0, 0);
			}
			else if (state->lookdown)
			{
				al_warning_proc(LB_WARNING_DMS_ATTATION_LOOKDOWN, 0, 0, 0);
			}
		}
	#else
		if (warn_status->fatigue_level == 2 || warn_status->fatigue_level == 4 || warn_status->fatigue_level == 6 ||
			warn_status->fatigue_level == 8 || warn_status->fatigue_level == 10)
		{
			if (gDmsAlgoParams->fati_sensitivity!=0)
				al_warning_proc(LB_WARNING_DMS_REST, warn_status->fatigue_level, data->debug_info.yawn_count, 0);
		}
		if (warn_status->distracted)
		{
			if (gDmsAlgoParams->dist_sensitivity!=0)
				al_warning_proc(LB_WARNING_DMS_ATTATION, 0, 0, 0);
		}
	#endif
		if (warn_status->red_resist_glasses)
		{
			al_warning_proc(LB_WARNING_DMS_INFRARED_BLOCK, 0, 0, 0);
		}
		if (warn_status->no_belt)
		{
			if (gDmsAlgoParams->belt_sensitivity!=0)
				al_warning_proc(LB_WARNING_DMS_NO_BELT, 0, 0, 0);
		}

		gDmsNN.lastNodriverCoverCnt ++;

		if (gDmsNN.lastNodriverCoverCnt > 100)
		{
			if (!gFacerecgDriverState && gAlComParam.mCurrentSpeed < 10)
			{
				facerecg_driver_recgonize(NULL);
				gFacerecgDriverState = 1;
			}
			gLastCoverNodriver = 0;
			gDmsNN.lastNodriverCoverCnt = 0;
		}
	} 

	if (nn_dms_passenger_detect(data))
	{
		al_warning_proc(LB_WARNING_DMS_ACTIVE_PHOTO, gDmsAlgoParams->passenger_photo_cnt/*photo cnt*/, 15/*intervel*/, 0);
	}
	
	if (warn_status->cover)
    {
    	if (!(gLastCoverNodriver&0x01))
    	{
			if (al_warning_proc(LB_WARNING_DMS_CAMERA_COVER, LB_WARNING_DMS_DRIVER_LEAVE, 0, 0))
			{
				gLastCoverNodriver |= 0x01;
			}
    	}
		gDmsNN.lastNodriverCoverCnt = 0;
	}
	
	if (warn_status->driver_leave)
    {
    	if (!(gLastCoverNodriver&0x02))
    	{
			if (al_warning_proc(LB_WARNING_DMS_DRIVER_LEAVE, LB_WARNING_DMS_CAMERA_COVER, 0, 0))
			{
				gLastCoverNodriver |= 0x02;
			}
    	}
		gFacerecgDriverState = 0;
		gDmsNN.lastNodriverCoverCnt = 0;
	}
	
	return 0;
}

static int nn_dms_result_cb(int event, void *data)
{
    nn_dms_out_t *out = NULL;
	nn_dms_cfg_t *cfg= NULL;
	uint32_t index;
	nn_dms_face_t *face = NULL;
	nn_dms_state_t *state = NULL;
	
	if (event==NN_DMS_AUTO_CALIBRATE_DONE)
	{
		cfg= (nn_dms_cfg_t*)data;
		//dms_save_cali_parm(cfg->warn_cfg.calib_yaw, cfg->warn_cfg.calib_pitch);
		gDmsAlgoParams->calib_pitch = cfg->warn_cfg.calib_pitch;
		gDmsAlgoParams->calib_yaw = cfg->warn_cfg.calib_yaw;
		//osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_CALIBRATE_SUCCESS, NULL, 0);
		
		PNT_LOGI("calib_yaw:%f,calib_pitch: %f calib_position %d %d %d %d\n", cfg->warn_cfg.calib_yaw, cfg->warn_cfg.calib_pitch, cfg->warn_cfg.calib_position[0],
		cfg->warn_cfg.calib_position[1], cfg->warn_cfg.calib_position[2], cfg->warn_cfg.calib_position[3]);
		return 0;
	}
		
	out = (nn_dms_out_t*)data;
	pthread_mutex_lock(&gDmsNN.p_det_mutex);
	
	/* faces copy */
    if (gDmsNN.p_det_info->faces.p)
	{
        free(gDmsNN.p_det_info->faces.p);
        gDmsNN.p_det_info->faces.p = NULL;
        gDmsNN.p_det_info->faces.size = 0;
    }
    gDmsNN.p_det_info->faces.p = malloc(out->faces.size * sizeof(nn_dms_face_t));
    if (gDmsNN.p_det_info->faces.p) 
	{
        memcpy(gDmsNN.p_det_info->faces.p, out->faces.p, out->faces.size * sizeof(nn_dms_face_t));
        gDmsNN.p_det_info->faces.size = out->faces.size;
		gDmsNN.p_det_info->faces.driver_idx = out->faces.driver_idx;
    }
	/* persons copy */
	if (gDmsNN.p_det_info->persons.p) 
	{
        free(gDmsNN.p_det_info->persons.p);
        gDmsNN.p_det_info->persons.p = NULL;
        gDmsNN.p_det_info->persons.size = 0;
    }
    gDmsNN.p_det_info->persons.p = malloc(out->persons.size * sizeof(nn_dms_person_t));
    if (gDmsNN.p_det_info->persons.p)
	{
        memcpy(gDmsNN.p_det_info->persons.p, out->persons.p, out->persons.size * sizeof(nn_dms_person_t));
        gDmsNN.p_det_info->persons.size = out->persons.size;
		gDmsNN.p_det_info->persons.driver_idx = out->persons.driver_idx;
    }
	/* heads copy */
	if (gDmsNN.p_det_info->heads.p)
	{
        free(gDmsNN.p_det_info->heads.p);
        gDmsNN.p_det_info->heads.p = NULL;
        gDmsNN.p_det_info->heads.size = 0;
    }
    gDmsNN.p_det_info->heads.p = malloc(out->heads.size * sizeof(nn_dms_head_t));
    if (gDmsNN.p_det_info->heads.p) 
	{
        memcpy(gDmsNN.p_det_info->heads.p, out->heads.p, out->heads.size * sizeof(nn_dms_head_t));
        gDmsNN.p_det_info->heads.size = out->heads.size;
		gDmsNN.p_det_info->heads.driver_idx = out->heads.driver_idx;
    }
    gDmsNN.p_det_info->cover_state = out->cover_state;
	gDmsNN.p_det_info->driver_leave = out->driver_leave;
	gDmsNN.p_det_info->no_face = out->no_face;
	gDmsNN.p_det_info->warn_status = out->warn_status;
	/* obj copy */
    for (int i = 0; i < sizeof(gDmsNN.p_det_info->obj_boxes)/sizeof(gDmsNN.p_det_info->obj_boxes[0]); i++)
	{
        if (gDmsNN.p_det_info->obj_boxes[i].p) 
		{
            free(gDmsNN.p_det_info->obj_boxes[i].p);
            gDmsNN.p_det_info->obj_boxes[i].p = NULL;
            gDmsNN.p_det_info->obj_boxes[i].size = 0;
        }
        if (out->obj_boxes[i].size == 0)
            continue;
		
		PNT_LOGD("out->obj_boxes[%d].size:%d\n", i, out->obj_boxes[i].size);
		
        gDmsNN.p_det_info->obj_boxes[i].p = malloc(sizeof(nn_dmsobj_box_t) * out->obj_boxes[i].size);
        if (gDmsNN.p_det_info->obj_boxes[i].p) 
		{
            memcpy(gDmsNN.p_det_info->obj_boxes[i].p, out->obj_boxes[i].p, sizeof(nn_dmsobj_box_t) * out->obj_boxes[i].size);
            gDmsNN.p_det_info->obj_boxes[i].size = out->obj_boxes[i].size;
        }
    }
	
	pthread_mutex_unlock(&gDmsNN.p_det_mutex);
	if (gDmsNN.action == DMS_CALIB)
	{  
			//dms_cali_fun(out);
			
			if (m_dms_cali.cali_flag != DMS_CALI_END)
			{
			}
			else if (m_dms_cali.cali_flag == DMS_CALI_END)
			{
				gDmsNN.action = DMS_RECOGNITION;
			}
	}
	else
	{
		if (out->faces.size > 0)
		{
			if (out->faces.driver_idx < 0 || out->faces.driver_idx >= out->faces.size)
			{
				//PNT_LOGD("out->faces.driver_idx %d\n", out->faces.driver_idx);
				face = &out->faces.p[0];
			}
			else
			{
				face = &out->faces.p[out->faces.driver_idx];
			}
			state = &face->face_attr.dms_state;
			
			/*  注意: 要传要检测的人脸坐标和关键点信息*/
			gDmsNN.face.roi.x0 = face->box[0];
			gDmsNN.face.roi.y0 = face->box[1];
			gDmsNN.face.roi.x1 = face->box[2];
			gDmsNN.face.roi.y1 = face->box[3];
			if (gDmsNN.livingdet_flag == LIVING_DET_SINGLE_RGB)
			{
				int ret;
				ezax_face_kpts_t *face_kpts = gDmsNN.face.roi.in_ex_inform;
				/* face_kpts 是人脸的关键点信息，先要识别到人脸，再做活体检测 */
				for (int i=0; i < 10; i++)
				{
					face_kpts->kpts[i] = (float)face->pts[i];
				}
 				ret = nna_livingdet_process(gDmsNN.livingdet_hdl, NULL, &gDmsNN.face, gDmsNN.livingdet_out);
				if (!ret)
				{
					gDmsNN.score  = (ezax_livingdet_rt_t *)gDmsNN.livingdet_out->out_ex_inform;
					//PNT_LOGD("rgb live_score: %d\n", gDmsNN.score->live_score);
				}
			}
			else if(gDmsNN.livingdet_flag == LIVING_DET_SINGLE_NIR)
			{
				int ret;
				ezax_face_kpts_t *face_kpts = gDmsNN.face.roi.in_ex_inform;
				/* face_kpts 是人脸的关键点信息，先要识别到人脸，再做活体检测 */
				for (int i=0; i < 10; i++)
				{
					face_kpts->kpts[i] = (float)face->pts[i];
				}
				ret = nna_livingdet_process(gDmsNN.livingdet_hdl, &gDmsNN.face, NULL, gDmsNN.livingdet_out);
				if (!ret)
				{
					gDmsNN.score  = (ezax_livingdet_rt_t *)gDmsNN.livingdet_out->out_ex_inform;
					PNT_LOGD("nir live_score: %d\n", gDmsNN.score->live_score);
				}
			}

			PNT_LOGD("action: drink:%d,  call:%d, smoke:%d, yawn:%d, eyeclosed:%d\n", state->drink, state->call, state->smoke, state->yawn, state->eyeclosed);
	        PNT_LOGD("action: lookaround:%d, lookup:%d, lookdown:%d, red_resist_glasses:%d, cover_state:%d, no_face_mask:%d, headpose:%f %f %f\n",
	                state->lookaround, state->lookup, state->lookdown, face->face_attr.red_resist_glasses, out->cover_state, face->face_attr.no_face_mask,
	                face->face_attr.headpose[0], face->face_attr.headpose[1], face->face_attr.headpose[2]);
	    }
		else 
		{
			PNT_LOGD("cover_state:%d no_face: %d\n", out->cover_state, out->no_face);
		}
		dms_warning(out);
	}
    sem_post(&gDmsNN.frame_del_sem);

    return 0;

}

static EI_VOID *get_axframe_proc(EI_VOID *p)
{
    int ret;
    VIDEO_FRAME_INFO_S axFrame = {0};
    dms_nn_t *axframe_info;
    void *nn_hdl = NULL;
	char dmslib_ver[64]={0};

    axframe_info = (dms_nn_t *)p;
    PNT_LOGE("get_axframe_proc bThreadStart = %d\n", axframe_info->bThreadStart);

#ifdef AL_PRO_FRAMERATE_STATISTIC
	int64_t time1 = 0LL, time2 = 0LL, time_GetChnFrame1 = 0LL, time_GetChnFrame2 = 0LL,
	time_AlProFrame1 = 0LL, time_AlProFrame2 = 0LL;
	int64_t frame_cnt = 0;
#endif

    PNT_LOGE("********start_nn_dms******\n");
    memset(&axframe_info->m_nn_dms_cfg, 0, sizeof(axframe_info->m_nn_dms_cfg));
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
	axframe_info->m_nn_dms_cfg.no_belt_en = 1; //支持安全带检测使能
	axframe_info->m_nn_dms_cfg.headface_det_en = 1; //支持人脸和人头检测，如果置1，选择headfacedet.bin模型，否则使用dmsfacedet.bin相关模型
	axframe_info->m_nn_dms_cfg.headface_det_resolution = 1; //1是大分辨率检测，占用资源多，检测效果好，0是小分辨率检测，占用资源小，效果没有1好
	axframe_info->m_nn_dms_cfg.auto_calib_en = 1; //DMS 自动校准使能位
	if (axframe_info->action == DMS_CALIB)
	{
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_yaw = 0.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_pitch = 0.0;
//		osal_mq_send(warning_info.mq_id, LB_WARNING_DMS_CALIBRATE_START, NULL, 0);
	}
	else
	{
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_yaw = gDmsAlgoParams->calib_yaw;
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_pitch = gDmsAlgoParams->calib_pitch;
		/* if calib_position not set, driver detcect from whole region,other , driver detect for calib_position region */
		#if 1
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[0] = gDmsAlgoParams->driver_rect_xs;
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[1] = gDmsAlgoParams->driver_rect_ys;
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[2] = gDmsAlgoParams->driver_rect_xe;
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[3] = gDmsAlgoParams->driver_rect_ye;
		#endif
	}
	PNT_LOGI("calib_yaw:%f,calib_pitch: %f calib_position %d %d %d %d\n", 
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_yaw, 
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_pitch, 
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[0],
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[1], 
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[2], 
		axframe_info->m_nn_dms_cfg.warn_cfg.calib_position[3]);
	if (1 >= gDmsAlgoParams->dist_sensitivity)
	{
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.yaw_thresh[0] = -40.0; //驾驶员分神状态，相对于校准点的水平偏离门限角度，左负右正
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.yaw_thresh[1] = 40.0; //驾驶员分神状态，相对于校准点的水平偏离门限角度，左负右正
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.warn_interval = 5.0; //报警时间间隔
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.count = 1; // 触发一次报警状态的act 状态个数
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.time = 5.0;
	
		// 3.0s  报警状态统计总时间
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.act.ratio = 0.8; // 0.7  百分比，单个act.time中检测到70%  act动作即触发一个act状态
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.act.time = 3.0; // 3.0s，单次act统计时间 

		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.pitch_thresh[0] = -20.0; //驾驶员分神状态，相对于校准点的垂直偏离门限角度，上正下负
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.pitch_thresh[1] = 30.0; //驾驶员分神状态，相对于校准点的垂直偏离门限角度，上正下负
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.warn_interval = 5.0; //报警时间间隔
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.count = 1;// 触发一次报警状态的act 状态个数
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.time = 3.0;
		// 3.0s   报警状态统计总时间
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.act.ratio = 0.8;// 0.7	百分比，单个act.time中检测到70%  act动作即触发一个act状态
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.act.time = 3.0;// 3.0s，单次act统计时间 
	}
	else if (3 == gDmsAlgoParams->dist_sensitivity)
	{
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.yaw_thresh[0] = -40.0; //驾驶员分神状态，相对于校准点的水平偏离门限角度，左负右正
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.yaw_thresh[1] = 40.0; //驾驶员分神状态，相对于校准点的水平偏离门限角度，左负右正
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.warn_interval = 5.0; //报警时间间隔
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.count = 1; // 触发一次报警状态的act 状态个数
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.time = 1.5;
	
		// 3.0s  报警状态统计总时间
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.act.ratio = 0.6; // 0.7  百分比，单个act.time中检测到70%  act动作即触发一个act状态
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_yaw.act.time = 3.0; // 3.0s，单次act统计时间 

		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.pitch_thresh[0] = -20.0; //驾驶员分神状态，相对于校准点的垂直偏离门限角度，上正下负
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.pitch_thresh[1] = 30.0; //驾驶员分神状态，相对于校准点的垂直偏离门限角度，上正下负
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.warn_interval = 5.0; //报警时间间隔
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.count = 1;// 触发一次报警状态的act 状态个数
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.time = 3.0;
		// 3.0s   报警状态统计总时间
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.act.ratio = 0.6;// 0.7	百分比，单个act.time中检测到70%  act动作即触发一个act状态
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.act.time = 3.0;// 3.0s，单次act统计时间 
	}
	else
	{
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
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.act.ratio = 0.7;// 0.7	百分比，单个act.time中检测到70%  act动作即触发一个act状态
		axframe_info->m_nn_dms_cfg.warn_cfg.distracted_cfg.distracted_pitch.act.time = 3.0;// 3.0s，单次act统计时间 
	}

	axframe_info->m_nn_dms_cfg.warn_cfg.drink_cfg.warn_interval = 5.0;//驾驶员喝水报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.drink_cfg.count = 1;
	axframe_info->m_nn_dms_cfg.warn_cfg.drink_cfg.time = 2.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.drink_cfg.act.ratio = 0.7;
	axframe_info->m_nn_dms_cfg.warn_cfg.drink_cfg.act.time = 2.0;
	axframe_info->m_nn_dms_cfg.warn_cfg.drink_det_thresh = 0.7; //det level, low:0.6;mid:0.7;high:0.8

	axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.warn_interval = 5.0;//驾驶员打电话报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.count = 1;
	if (1 >= gDmsAlgoParams->call_sensitivity)
	{
		axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.time = 4.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.act.ratio = 0.7;
		axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.act.time = 3.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.call_det_thresh = 0.8; //det level, low:0.6;mid:0.7;high:0.8
	}
	else if (3 == gDmsAlgoParams->call_sensitivity)
	{
		axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.time = 2.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.act.ratio = 0.7;
		axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.act.time = 3.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.call_det_thresh = 0.6; //det level, low:0.6;mid:0.7;high:0.8
	}
	else
	{
		axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.time = 3.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.act.ratio = 0.7;
		axframe_info->m_nn_dms_cfg.warn_cfg.call_cfg.act.time = 3.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.call_det_thresh = 0.7; //det level, low:0.6;mid:0.7;high:0.8
	}

	axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.warn_interval = 5.0; //驾驶员抽烟报警时间间隔
	axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.count = 1;
	if (1 >= gDmsAlgoParams->smok_sensitivity)
	{
		axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.time = 2.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.act.ratio = 0.7;
		axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.act.time = 2.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.smoke_det_thresh = 0.8; //det level, low:0.6;mid:0.7;high:0.8
	}
	else if (3 == gDmsAlgoParams->smok_sensitivity)
	{
		axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.time = 2.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.act.ratio = 0.7;
		axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.act.time = 1.2;
		axframe_info->m_nn_dms_cfg.warn_cfg.smoke_det_thresh = 0.6; //det level, low:0.6;mid:0.7;high:0.8
	}
	else
	{
		axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.time = 2.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.act.ratio = 0.7;
		axframe_info->m_nn_dms_cfg.warn_cfg.smoke_cfg.act.time = 1.5;
		axframe_info->m_nn_dms_cfg.warn_cfg.smoke_det_thresh = 0.7; //det level, low:0.6;mid:0.7;high:0.8
	}

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
	/* 安全带检测*/
	axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.warn_interval = 10.0;//未检测到安全带报警间隔时间
	axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.count = 1;
	if (1 >= gDmsAlgoParams->belt_sensitivity)
	{
		axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.time = 5.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.act.ratio = 0.7;
		axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.act.time = 5.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.belt_det_thresh = 0.65;//det level, low:0.25;mid:0.35;high:0.65
	}
	else if (3 == gDmsAlgoParams->belt_sensitivity)
	{
		axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.time = 5.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.act.ratio = 0.7;
		axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.act.time = 5.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.belt_det_thresh = 0.25;//det level, low:0.25;mid:0.35;high:0.65
	}
	else
	{
		axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.time = 5.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.act.ratio = 0.7;
		axframe_info->m_nn_dms_cfg.warn_cfg.no_belt_cfg.act.time = 5.0;
		axframe_info->m_nn_dms_cfg.warn_cfg.belt_det_thresh = 0.45;//det level, low:0.25;mid:0.35;high:0.65
	}

	if (1 >= gDmsAlgoParams->fati_sensitivity)
	{
		/* level 2 fragile */
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.warn_interval = 10.0;//驾驶员打哈欠报警时间间隔
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.count = 1; // time 内检测到act 状态>= 3次，触发报警
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.time = 3;
	// 300s ,报警状态统计总时间
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.act.ratio = 0.7;// 0.7  百分比，单个act.time中检测到70%	act动作即触发一个act状态
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
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].count = 7;// time 内检测到act 状态>=6次，触发报警
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].time = 1 * 60;
	// 60s ,报警状态统计总时间
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].act.ratio = 0.7;// 0.5 百分比，单个act.time中检测到50%  act动作即触发一个act状态
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[1].act.time = 1.0;// 1.0s，单次act统计时间
		/* level 8 fragile */
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].warn_interval = 10.0;//驾驶员闭眼报警时间间隔
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].count = 1;// time 内检测到act 状态>=1次，触发报警
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].time = 2.5;
	// 2s ,报警状态统计总时间
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].act.ratio = 0.7;// 0.7  百分比，单个act.time中检测到70%  act动作即触发一个act状态
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[2].act.time = 2.0;// 2.0s，单次act统计时间 
		/* level 10 fragile */
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].warn_interval = 10.0;//驾驶员闭眼报警时间间隔
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].count = 1;// time 内检测到act 状态>=1次，触发报警
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].time = 4.5;
	// 4s ,报警状态统计总时间
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].act.ratio = 0.7;// 0.7  百分比，单个act.time中检测到70%  act动作即触发一个act状态
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].act.time = 4.5;// 4.0s，单次act统计时间
	}
	else if (3 == gDmsAlgoParams->fati_sensitivity)
	{
		/* level 2 fragile */
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.warn_interval = 10.0;//驾驶员打哈欠报警时间间隔
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.count = 1; // time 内检测到act 状态>= 3次，触发报警
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.time = 2;
	// 300s ,报警状态统计总时间
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.act.ratio = 0.7;// 0.7  百分比，单个act.time中检测到70%	act动作即触发一个act状态
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
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].time = 3.0;
	// 4s ,报警状态统计总时间
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].act.ratio = 0.7;// 0.7  百分比，单个act.time中检测到70%  act动作即触发一个act状态
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_eye[3].act.time = 3.0;// 4.0s，单次act统计时间
	}
	else
	{
		/* level 2 fragile */
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.warn_interval = 10.0;//驾驶员打哈欠报警时间间隔
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.count = 1; // time 内检测到act 状态>= 3次，触发报警
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.time = 3;
	// 300s ,报警状态统计总时间
		axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.fatigue_yawn.act.ratio = 0.7;// 0.7  百分比，单个act.time中检测到70%	act动作即触发一个act状态
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
	}
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.eyeclosed_headpose_range[0] = -45; //闭眼报警的人脸头部姿态检测范围，左负右正，上正下负，数组索引顺序: 左右下上
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.eyeclosed_headpose_range[1] = 45;
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.eyeclosed_headpose_range[2] = -15;
	axframe_info->m_nn_dms_cfg.warn_cfg.fatigue_cfg.eyeclosed_headpose_range[3] = 45;
	axframe_info->m_nn_dms_cfg.dump_cfg = 1;//dump 配置参数

	axframe_info->m_nn_dms_cfg.interest_box[0] = 0;//算法检测区域左上角X坐标
	axframe_info->m_nn_dms_cfg.interest_box[1] = 0;//算法检测区域左上角y坐标
	axframe_info->m_nn_dms_cfg.interest_box[2] = axframe_info->u32Width - 1;//算法检测区域右下角x坐标
	axframe_info->m_nn_dms_cfg.interest_box[3] = axframe_info->u32Height - 1;//算法检测区域右下角y坐标
	PNT_LOGD("interest_box x1:%d,y1:%d,x2:%d,y2:%d \n", axframe_info->m_nn_dms_cfg.interest_box[0], axframe_info->m_nn_dms_cfg.interest_box[1],
		axframe_info->m_nn_dms_cfg.interest_box[2], axframe_info->m_nn_dms_cfg.interest_box[3]);

    /* open libdms.so*/
    nn_hdl = nn_dms_open(&axframe_info->m_nn_dms_cfg);
    if (nn_hdl == NULL)
	{
        PNT_LOGE("nn_dms_open() failed!");
        return NULL;
    }
	axframe_info->nn_hdl = nn_hdl;
	
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
		PNT_LOGE("axframe_info->livingdet_hdl %p %s %d\n", axframe_info->livingdet_hdl, __FILE__, __LINE__);
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
		PNT_LOGE("axframe_info->livingdet_hdl %p %s %d\n", axframe_info->livingdet_hdl, __FILE__, __LINE__);
	}
	
	if (axframe_info->gray_flag == 1)
	{
		EI_MI_MBASE_MemAlloc(&axframe_info->gray_uv_phy_addr, &axframe_info->gray_uv_vir_addr, "gray_scale_image", NULL, axframe_info->u32Width*axframe_info->u32Height/2);
		memset(axframe_info->gray_uv_vir_addr, 0x80, axframe_info->u32Width*axframe_info->u32Height/2);
	}
#ifdef AL_PRO_FRAMERATE_STATISTIC
	time1 = osal_get_msec();
#endif

	int frame_count = 0;

	sleep(5);

    while (EI_TRUE == axframe_info->bThreadStart) 
	{
        memset(&axFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
		
#ifdef AL_PRO_FRAMERATE_STATISTIC
		time_GetChnFrame1 = osal_get_msec();
#endif

		if (gDmsAlgoParams->otherarea_cover & 0x80)
		{
			if (gDmsAlgoParams->otherarea_cover & 0x01)
			{
				dms_cover_init(gDmsAlgoParams->active_rect_xs, gDmsAlgoParams->active_rect_ys, gDmsAlgoParams->active_rect_xe, gDmsAlgoParams->active_rect_ye);
			}
			else
			{
				dms_cover_deinit();
			}
			gDmsAlgoParams->otherarea_cover &= (~0x80);
		}

		if (access("/mnt/card/DriverFaces", F_OK) == 0)
		{
			facerecg_register_by_images("/mnt/card/DriverFaces");
		}

		if (!gACCState || gCPUTemprature >= 95)
		{
			usleep(1000*1000);
			continue;
		}

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
		
#ifdef SAVE_AX_YUV_SP
		if (frame_count > 100 && frame_count < 150)
		{
			if (NULL == gDmsNN.ax_fout)
			{
				char srcfilename[128];
				sprintf(srcfilename, "/mnt/card/dms_raw_ch%d-%d.yuv", axframe_info->chn, frame_count);
				axframe_info->ax_fout = fopen(srcfilename, "wb+");
				if (axframe_info->ax_fout == NULL)
				{
					PNT_LOGE("file open error1\n");
				}
			}
		}
		frame_count ++;
#endif

		facerecg_nn_start(&axFrame);

		nn_dms_start(&axFrame, nn_hdl);

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

		EI_MI_VISS_ReleaseChnFrame(axframe_info->dev, axframe_info->chn, &axFrame);
    }

	EI_MI_VISS_DisableChn(axframe_info->dev, axframe_info->chn);
	
	if (axframe_info->livingdet_flag == LIVING_DET_SINGLE_RGB || axframe_info->livingdet_flag == LIVING_DET_SINGLE_NIR)
	{
		if (axframe_info->livingdet_hdl)
		{
			nna_livingdet_close(axframe_info->livingdet_hdl);
		}
	}
		
	PNT_LOGE("axframe_info->dev %d %d\n", axframe_info->dev, axframe_info->chn);
	PNT_LOGE("nn_dms_close 0\n");
	
    nn_dms_close(nn_hdl);
	
	PNT_LOGE("nn_dms_close 1\n");

    return NULL;
}

static EI_VOID *rgn_draw_proc(EI_VOID *p)
{
    dms_nn_t *axframe_info = &gDmsNN;

	unsigned char lastState = gAlComParam.mDmsRender;

	rgn_start();

    while (EI_TRUE == axframe_info->bThreadStart)
	{
		if (lastState != gAlComParam.mDmsRender)
		{
			lastState = gAlComParam.mDmsRender;
			
			if (gAlComParam.mDmsRender)
			{
				if(EI_MI_RGN_AddToChn(gDmsNN.Handle[0], &gDmsNN.stRgnChn, &gDmsNN.stRgnChnAttr[0]))
				{
					PNT_LOGE("EI_MI_RGN_AddToChn \n");
				}

				if(EI_MI_RGN_AddToChn(gDmsNN.Handle[1], &gDmsNN.stRgnChn, &gDmsNN.stRgnChnAttr[1]))
				{
					PNT_LOGE("EI_MI_RGN_AddToChn \n");
				}
			}
			else
			{
				EI_MI_RGN_DelFromChn(gDmsNN.Handle[0], &gDmsNN.stRgnChn);
				EI_MI_RGN_DelFromChn(gDmsNN.Handle[1], &gDmsNN.stRgnChn);
			}
		}
		else
		{
			if (gAlComParam.mDmsRender)
			{
				pthread_mutex_lock(&axframe_info->p_det_mutex);

				dms_draw_location_rgn(axframe_info->p_det_info);

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


static int start_get_axframe(dms_nn_t *axframe_info)
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
    stExtChnAttr.stFrameRate.s32DstFrameRate = 12;
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

RGN_HANDLE dms_cover_area_init(RGN_HANDLE rgnHandle, int lt_x, int lt_y, int rb_x, int rb_y)
{	
    dms_nn_t *handle = &gDmsNN;
	
    MDP_CHN_S stRgnChn = { 0 };
    stRgnChn.enModId = EI_ID_VISS;
    stRgnChn.s32DevId = handle->dev;
    stRgnChn.s32ChnId = handle->phyChn;
	
    RGN_ATTR_S stRegion = { 0 };
    RGN_CHN_ATTR_S stRgnChnAttr = { 0 };
    EI_S32 s32Ret, x = 0, y = 0, width = 0, height = 0;
	
	stRegion.enType = MOSAICEX_RGN;
	stRgnChnAttr.bShow = EI_TRUE;
	stRgnChnAttr.enType = MOSAICEX_RGN;
	stRgnChnAttr.unChnAttr.stMosaicExChn.enBlkSize = MOSAIC_BLK_SIZE_20;
	stRgnChnAttr.unChnAttr.stMosaicExChn.u32Layer = 0;
	
	if (rb_y <= 42)
	{
		return rgnHandle;
	}

	int cnt_row = (rb_x - lt_x)/600 + (((rb_x - lt_x)%600)?1:0);
	int cnt_col = (rb_y - lt_y)/360 + (((rb_y - lt_y)%360)?1:0);
	
	if (cnt_row <= 0 || cnt_col <= 0)
	{
		return rgnHandle;
	}
	
	width = (rb_x - lt_x)/cnt_row+7;
	height = (rb_y - lt_y)/cnt_col;

	for (int i=0; i<cnt_row*cnt_col; i++)
	{
		x = lt_x + width*(i%cnt_row);
		y = lt_y + height*(i/cnt_row);
		width = width;
		height = height;

		stRgnChnAttr.unChnAttr.stMosaicExChn.stRect.s32X = x;
		stRgnChnAttr.unChnAttr.stMosaicExChn.stRect.s32Y = y<42?42:y;
		stRgnChnAttr.unChnAttr.stMosaicExChn.stRect.u32Width = width;
		stRgnChnAttr.unChnAttr.stMosaicExChn.stRect.u32Height = y<42?(height-42+y):height;
		
	    s32Ret = EI_MI_RGN_Create(rgnHandle, &stRegion);
	    if (s32Ret)
	    {
	    	PNT_LOGE("create mosaic failed %08x", s32Ret);
	    }
		
	    s32Ret = EI_MI_RGN_AddToChn(rgnHandle, &stRgnChn, &stRgnChnAttr);
	    if (s32Ret)
	    {
	    	PNT_LOGE("add mosaic failed %08x\n", s32Ret);
	    }

		rgnHandle ++;
	}

	return rgnHandle;
}

void dms_cover_init(int lt_x, int lt_y, int rb_x, int rb_y)
{
    dms_nn_t *handle = &gDmsNN;
	
	RGN_HANDLE rgnHandle = DRIVERMOSAIC_HANDLE;
	
	if (rb_x - lt_x < 100)		rb_x = lt_x + 100;
	if (rb_y - lt_y < 100)		rb_y = lt_y + 100;

	dms_cover_deinit();

	rgnHandle = dms_cover_area_init(rgnHandle, 0, 0, handle->u32Width, lt_y);
	
	rgnHandle = dms_cover_area_init(rgnHandle, 0, lt_y, lt_x, rb_y);
	
	rgnHandle = dms_cover_area_init(rgnHandle, rb_x, lt_y, handle->u32Width, rb_y);
	
	rgnHandle = dms_cover_area_init(rgnHandle, 0, rb_y, handle->u32Width, handle->u32Height);

	handle->cover_area_handle = (rgnHandle-DRIVERMOSAIC_HANDLE);
}

void dms_cover_deinit(void)
{
    dms_nn_t *handle = &gDmsNN;
	
    MDP_CHN_S stRgnChn = { 0 };
    stRgnChn.enModId = EI_ID_VISS;
    stRgnChn.s32DevId = handle->dev;
    stRgnChn.s32ChnId = handle->phyChn;
	
	for (int i=0; i<handle->cover_area_handle; i++)
	{
		EI_MI_RGN_DelFromChn(i+DRIVERMOSAIC_HANDLE, &stRgnChn);
		EI_MI_RGN_Destroy(i+DRIVERMOSAIC_HANDLE);
	}

	handle->cover_area_handle = 0;
}

int dms_nn_init(int vissDev, int vissChn, int chn, int width, int height)
{
    dms_nn_t *axframe_info = &gDmsNN;

	if (gDmsAlgoParams->belt_sensitivity==0 && gDmsAlgoParams->smok_sensitivity==0 && 
		gDmsAlgoParams->call_sensitivity==0 && gDmsAlgoParams->dist_sensitivity==0 &&
		gDmsAlgoParams->fati_sensitivity==0)
	{
		return 0;
	}

    pthread_mutex_init(&axframe_info->p_det_mutex, NULL);
	
    axframe_info->p_det_info = malloc(sizeof(nn_dms_out_t));
	memset(axframe_info->p_det_info, 0, sizeof(nn_dms_out_t));
	
    if (!axframe_info->p_det_info)
    {
		PNT_LOGE("axframe_info->p_det_info malloc failed!\n");
		return -1;
    }

    axframe_info->dev = vissDev;
    axframe_info->chn = chn;
    axframe_info->phyChn = vissChn;
    axframe_info->frameRate = 25;
    axframe_info->u32Width = width;
    axframe_info->u32Height = height;

	axframe_info->gray_flag = 0;

	dms_al_para_init();

    sem_init(&axframe_info->frame_del_sem, 0, 0);

    start_get_axframe(axframe_info);

	axframe_info->cover_area_handle = 0;
	if (gDmsAlgoParams->otherarea_cover & 0x01)
	{
		dms_cover_init(gDmsAlgoParams->active_rect_xs, gDmsAlgoParams->active_rect_ys, gDmsAlgoParams->active_rect_xe, gDmsAlgoParams->active_rect_ye);
	}

	passengerReportTime = currentTimeMillis()/1000;

    return 0;
}

void dms_nn_stop(void)
{
    dms_nn_t *axframe_info = &gDmsNN;

	if (axframe_info->bThreadStart)
	{
		axframe_info->bThreadStart = EI_FALSE;

		usleep(200*1000);
		if (axframe_info->gray_flag == 1)
		{
			EI_MI_MBASE_MemFree(axframe_info->gray_uv_phy_addr, axframe_info->gray_uv_vir_addr);
		}

		rgn_stop();
	}
}




