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
#ifndef __SDR_SAMPLE_MUXER_H__
#define __SDR_SAMPLE_MUXER_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

void sample_muxer_deinit(void *hdl);
void *sample_muxer_init(int mode, char *filename, int type, VC_CHN VeChn);
int sample_muxer_write_packet(void *hdl, VENC_STREAM_S *pstStream);
int sample_muxer_write_aenc_packet(void *hdl, AUDIO_STREAM_S *pstStream);
int oscl_cfile_alloc_cache(int fd, int num, int size);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SDR_SAMPLE_MUXER_H__ */

