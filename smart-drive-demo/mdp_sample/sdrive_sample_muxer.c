/*
 *------------------------------------------------------------------------------
 * @File      :    sample_venc.c
 * @Date      :    2021-3-16
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
#include "muxer_common.h"
#include "sample_comm.h"
#include "sdrive_sample_muxer.h"

#define FHD_FILE_CACHE_SIZE     (512 * 1024)
#define FHD_FILE_CACHE_NUM      (4)
#define user_RB16(x) ((((const uint8_t *)(x))[0] << 8) | ((const uint8_t *)(x))[1])

typedef struct _muxer_hdl {
    int mode;
    void *muxer;
    muxer_param_t muxer_parm;
    char *filename;
    char *tmpbuf;
    int tmpbuf_len;
    EI_U64   start_pts[2];
    int acodec_id;
    int file_cnt;
	VC_CHN VeChn;
} sample_muxer_hdl_t;

static int _get_filename(void *hdl, char *filename)
{
    sample_muxer_hdl_t *muxer_hdl = hdl;
	struct tm *p_tm; /* time variable */
	time_t now;
	now = time(NULL);
	p_tm = localtime(&now);
	if (muxer_hdl->mode == AV_MUXER_TYPE_MP4) {
		sprintf(filename,
			"/data/stream_chn%02d-%02d%02d%02d%02d%02d%02d.mp4", muxer_hdl->VeChn,
			p_tm->tm_year + 1900, p_tm->tm_mon + 1, p_tm->tm_mday,
			p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
	} else if (muxer_hdl->mode == AV_MUXER_TYPE_TS) {
		sprintf(filename,
			"/data/stream_chn%02d-%02d%02d%02d%02d%02d%02d.ts", muxer_hdl->VeChn,
			p_tm->tm_year + 1900, p_tm->tm_mon + 1, p_tm->tm_mday,
			p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
	}
    muxer_hdl->file_cnt++;
    return 0;
}

static int _file_closed(void *hdl, char *filename)
{
    return 0;
}

void sample_muxer_deinit(void *hdl)
{
	sample_muxer_hdl_t *muxer_hdl = (sample_muxer_hdl_t *)hdl;
	if (!hdl)
		return ;
    if (muxer_hdl->muxer) {
        muxer_context_stop(muxer_hdl->muxer);
        usleep(1500*1000);
        muxer_context_destroy(muxer_hdl->muxer);
        muxer_hdl->muxer = NULL;
    }

    if (muxer_hdl) {
        if (muxer_hdl->filename) {
            free(muxer_hdl->filename);
            muxer_hdl->filename = NULL;
        }
        if (muxer_hdl->tmpbuf) {
            free(muxer_hdl->tmpbuf);
            muxer_hdl->tmpbuf = NULL;
            muxer_hdl->tmpbuf_len = 0;
        }
        free(muxer_hdl);
    }
}
void *sample_muxer_init(int mode, char *filename, int type, VC_CHN VeChn)
{
    sample_muxer_hdl_t *muxer_hdl;
    static int first_flag;

    muxer_hdl = malloc(sizeof(sample_muxer_hdl_t));
    if (muxer_hdl == NULL) {
        EI_PRINT("malloc failed!\n");
        return NULL;
    }
    memset(muxer_hdl, 0, sizeof(sample_muxer_hdl_t));
    muxer_hdl->muxer = muxer_context_create();
    printf("%p,init muxer:%p\n",muxer_hdl,muxer_hdl->muxer);
    if (muxer_hdl->muxer == NULL)
        return NULL;

    muxer_hdl->muxer_parm.nb_streams = 2;
    muxer_hdl->mode = mode;
    muxer_hdl->start_pts[VIDEO_INDEX] = -1;
    muxer_hdl->start_pts[AUDIO_INDEX] = -1;
	muxer_hdl->VeChn = VeChn;

    muxer_hdl->muxer_parm.mode = mode;
    muxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].codec_type = AV_MEDIA_TYPE_VIDEO;
    muxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].codec_id = type;
    muxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].sample_rate = 20 * VIDEOTIMEFRAME;
    muxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].width = 1920;
    muxer_hdl->muxer_parm.stream_parm[VIDEO_INDEX].height = 1080;

    muxer_hdl->muxer_parm.stream_parm[AUDIO_INDEX].codec_type = AV_MEDIA_TYPE_AUDIO;
    muxer_hdl->muxer_parm.stream_parm[AUDIO_INDEX].codec_id = AV_CODEC_TYPE_AAC;
    muxer_hdl->muxer_parm.stream_parm[AUDIO_INDEX].sample_rate = 16000;
	muxer_hdl->muxer_parm.stream_parm[AUDIO_INDEX].channels = 1;
    muxer_hdl->acodec_id = AV_CODEC_TYPE_AAC;

    muxer_hdl->muxer_parm.fix_duration_param.app_data = muxer_hdl;
    muxer_hdl->muxer_parm.fix_duration_param.cb_get_next_file = NULL;/*_get_filename;*/
    muxer_hdl->muxer_parm.fix_duration_param.cb_file_closed = _file_closed;
    muxer_hdl->muxer_parm.fix_duration_param.cb_get_user_data = NULL;
    muxer_hdl->muxer_parm.fix_duration_param.file_duration = 100000;

    muxer_context_ctrl(muxer_hdl->muxer, MUXER_CONTEXT_SET_FILE_NAME, filename);
    muxer_context_start(muxer_hdl->muxer, &muxer_hdl->muxer_parm);
    return muxer_hdl;
}

int sample_muxer_write_packet(void *hdl, VENC_STREAM_S *pstStream)
{
	sample_muxer_hdl_t *pmuxer_hdl = (sample_muxer_hdl_t *)hdl;
    VENC_PACK_S *pstPack;
    muxer_packet_t pin = {0};
    int ret = 0;

    if (pmuxer_hdl == NULL || pstStream == NULL) {
        EI_PRINT("err param!\n");
        EI_PRINT("%s %d  \n", __func__, __LINE__);
        return -1;
    }
    pstPack = &pstStream->pstPack;
    if (pstPack->pu8Addr[1]) {
        if (pstPack->u32Len[0] + pstPack->u32Len[1] > pmuxer_hdl->tmpbuf_len) {
            pmuxer_hdl->tmpbuf_len = pstPack->u32Len[0] + pstPack->u32Len[1];
            if (pmuxer_hdl->tmpbuf)
                free (pmuxer_hdl->tmpbuf);
            pmuxer_hdl->tmpbuf = malloc(pmuxer_hdl->tmpbuf_len);
            if (pmuxer_hdl->tmpbuf == NULL) {
                pmuxer_hdl->tmpbuf_len = 0;
                return -1;
            }
        }
        memcpy(pmuxer_hdl->tmpbuf, pstPack->pu8Addr[0], pstPack->u32Len[0]);
        memcpy(pmuxer_hdl->tmpbuf + pstPack->u32Len[0],
            pstPack->pu8Addr[1], pstPack->u32Len[1]);
        pin.data = (unsigned char *)pmuxer_hdl->tmpbuf;
        pin.size = pstPack->u32Len[0] + pstPack->u32Len[1];
    } else {
        pin.data = pstPack->pu8Addr[0];
        pin.size = pstPack->u32Len[0];
    }
    pin.stream_index = VIDEO_INDEX;

    if (pmuxer_hdl->start_pts[0] < 0)
        pmuxer_hdl->start_pts[0] = pstPack->u64PTS;
    pin.timestamp = pstPack->u64PTS / 1000;
    if (pstPack->enDataType == DATA_TYPE_I_FRAME) {
        pin.flags |= AV_PKT_FLAG_KEY;
    }
    if (pstPack->enDataType == DATA_TYPE_INITPACKET) {
        printf("AV_PKT_INIT_FRAME\n");
        pin.flags |= AV_PKT_INIT_FRAME;
    }
    if (pstPack->enDataType == DATA_TYPE_P_FRAME)
        pin.flags |= AV_PKT_P_FRAME;
    if (pstPack->enDataType == DATA_TYPE_B_FRAME)
        pin.flags |= AV_PKT_B_FRAME;
    if (pstPack->bFrameEnd)
        pin.flags |= AV_PKT_ENDING_FRAME;
    //printf("%p,save pin:%p\n",pmuxer_hdl,pmuxer_hdl->muxer);
    ret = muxer_context_write(pmuxer_hdl->muxer, &pin);
    //EI_PRINT("ret:%d,vstream,timestamp:%llu\n", ret, pin.timestamp);

    return 0;
}

int sample_muxer_write_aenc_packet(void *hdl, AUDIO_STREAM_S *pstStream)
{
    sample_muxer_hdl_t *pmuxer_hdl = (sample_muxer_hdl_t *)hdl;
    muxer_packet_t pin = { 0 };
    int index = 1;
    int ret = 0;

    if (pmuxer_hdl == NULL || pstStream == NULL) {
        EI_PRINT("err param!\n");
        EI_PRINT("%s %d  \n", __func__, __LINE__);
        return -1;
    }
    if (pmuxer_hdl->mode == AV_MUXER_TYPE_MP4 &&
        pmuxer_hdl->acodec_id == AV_CODEC_TYPE_AAC &&
        pstStream->u32Len > 2 && (user_RB16(pstStream->pStream) & 0xfff0) == 0xfff0) {
        pin.data = pstStream->pStream + 7;
        if (pstStream->u32Len - 7 > 0)
            pin.size = pstStream->u32Len - 7;
    } else {
        pin.data = pstStream->pStream;
        pin.size = pstStream->u32Len;
    }
    pin.stream_index = AUDIO_INDEX;
    if (pmuxer_hdl->start_pts[index] < 0)
        pmuxer_hdl->start_pts[index] = pstStream->u64TimeStamp;
    pin.timestamp = pstStream->u64TimeStamp / 1000;
    ret = muxer_context_write(pmuxer_hdl->muxer, &pin);
    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
