#include <sys/prctl.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "sample_comm.h"
#include "libuvcg.h"
#include "uvc_stream.h"
#include "rec_stream.h"
#include "osd.h"

#define CAR_CHANNEL_NAME "亿智-粤B666666"
#define CAR_RUN_TIME "2022-02-16 14:33:38"
#define MAX_STRING_LEN 256

pthread_t g_rgn_pid;
int g_rgn_thread_flag = 0;

uvc_context_t g_ctx = {0};
uvc_config_t g_cfg = {
    .sesnor = GC2053_MIPI_1920_1080_25FPS_RAW10, /* 仅支持了数字摄像头 */
    .viss_dev = 2, /* 0: DVP, 1: MIPI0, 2: MIPI2 */
    .uvc_dev[0] = {
        .enable = EI_TRUE,
        .name = "uvc.0",
        .stream[0] = { //主码流
            .enable = EI_TRUE,
            .name = "s0",
            .ippu_dev = 0,
            .ippu_chn = 0,
            .rotation = ROTATION_0,
            .venc_chn = 0,
        },
        .stream[1] = { //主码流
            .enable = EI_FALSE,
            .name = "s1",
            .ippu_dev = 0,
            .ippu_chn = 1,
            .rotation = ROTATION_0,
            .venc_chn = 1,
        },
    },
    .recorder = {
        .ippu_dev = 0,
        .ippu_chn = 2,
        .rotation = ROTATION_0,
        .framerate = 25,
        .width = 1920,
        .height = 1080,
        .bitrate = 8000000,
        .video_format = PT_H265,
        .venc_chn = 2,
    },
};

static EI_S32 __video_cb(video_cb_param_t *cb_param)
{
	EI_S32 ret = -1;
	EI_U32 dev = 0;
	EI_U32 stream = 0;

	if (!cb_param) {
		_DBG("cb_param is NULL\n");
		return -1;
	}

	for (EI_U32 i = 0; i < UVC_MAX_DEV_NUM; i++) {
		if (!g_cfg.uvc_dev[i].enable)
			continue;

		for (EI_U32 j = 0; j < UVC_MAX_STREAM_NUM; j++) {
			if (!g_cfg.uvc_dev[i].stream[j].enable)
				continue;

			if (cb_param->ippu_dev == g_cfg.uvc_dev[i].stream[j].ippu_dev
				&& cb_param->ippu_chn == g_cfg.uvc_dev[i].stream[j].ippu_chn) {
				dev = i;
				stream = j;
			}
		}
	}

	stream_param_t *stream_param = &g_ctx.uvc_dev[dev].stream[stream];
	libuvcg_video_buffer_t buffer = {0};
	EI_U32 buffer_size = 0;

	if (cb_param->format == COMP_VIDEO_FORMAT_NV12 || cb_param->format == COMP_VIDEO_FORMAT_NV21) {
		cb_param->raw_frame->stVFrame.u32PlaneSize[0] = stream_param->info.width * stream_param->info.height;
		cb_param->raw_frame->stVFrame.u32PlaneSize[1] = EI_ALIGN_DOWN(stream_param->info.width
								* stream_param->info.height / 2, 2);
	}

	if (cb_param->format < COMP_VIDEO_FORMAT_NV12) {
		for (EI_S32 i = 0; i < ENC_STREAM_MAX_NUM; i++) {
			if (!cb_param->enc_stream[i])
				continue;
            
			buffer_size += cb_param->enc_stream[i]->pstPack.u32Len[0];
			buffer_size += cb_param->enc_stream[i]->pstPack.u32Len[1];
		}
	} else {
		if (!cb_param->raw_frame) {
			_DBG("raw frame is NULL\n");
			return -1;
		}

		buffer_size += cb_param->raw_frame->stVFrame.u32PlaneSize[0];
		buffer_size += cb_param->raw_frame->stVFrame.u32PlaneSize[1];
	}

	ret = libuvcg_stream_request_vbuffer(stream_param->hdl, &buffer, buffer_size);
	if (ret != 0) {
		_DBG("libuvcg_stream_request_vbuffer[%u][%u] failed\n",
			stream_param->dev, stream_param->stream);
		return -1;
	}

	EI_VOID *buffer_mem = *buffer.mem;
	if (cb_param->format < COMP_VIDEO_FORMAT_NV12) {
		for (EI_S32 i = 0; i < ENC_STREAM_MAX_NUM; i++) {
			if (!cb_param->enc_stream[i])
				continue;

			memcpy(buffer_mem, cb_param->enc_stream[i]->pstPack.pu8Addr[0],
				cb_param->enc_stream[i]->pstPack.u32Len[0]);

			buffer_mem += cb_param->enc_stream[i]->pstPack.u32Len[0];

			if (cb_param->enc_stream[i]->pstPack.u32Len[1]) {
				memcpy(buffer_mem, cb_param->enc_stream[i]->pstPack.pu8Addr[1],
					cb_param->enc_stream[i]->pstPack.u32Len[1]);

				buffer_mem += cb_param->enc_stream[i]->pstPack.u32Len[1];
			}
		}
	} else {
		memcpy(buffer_mem, (EI_U8 *)cb_param->raw_frame->stVFrame.ulPlaneVirAddr[0],
			cb_param->raw_frame->stVFrame.u32PlaneSize[0]);

		buffer_mem += cb_param->raw_frame->stVFrame.u32PlaneSize[0];

		if (cb_param->raw_frame->stVFrame.u32PlaneSize[1]) {
			memcpy(buffer_mem, (EI_U8 *)cb_param->raw_frame->stVFrame.ulPlaneVirAddr[1],
				cb_param->raw_frame->stVFrame.u32PlaneSize[1]);

			buffer_mem += cb_param->raw_frame->stVFrame.u32PlaneSize[1];
		}

		if (cb_param->raw_frame->stVFrame.u32PlaneSize[2]) {
			memcpy(buffer_mem, (EI_U8 *)cb_param->raw_frame->stVFrame.ulPlaneVirAddr[2],
				cb_param->raw_frame->stVFrame.u32PlaneSize[2]);

			buffer_mem += cb_param->raw_frame->stVFrame.u32PlaneSize[2];
		}
	}

	ret = libuvcg_stream_queue_vbuffer(stream_param->hdl, &buffer);
	if (ret != 0) {
		_DBG("libuvcg_stream_queue_vbuffer[%u][%u] failed\n",
			stream_param->dev, stream_param->stream);
		return -1;
	}

	//video_cap(*buffer.mem, buffer_size);

	return 0;
}

static EI_VOID *__uvc_process_thread(EI_VOID *arg)
{
	prctl(PR_SET_NAME, "uvc");

	libuvcg_run();

	_DBG("exit\n");
	return NULL;
}

static EI_S32 __create_uvc_device(EI_S32 dev)
{
    if (!g_cfg.uvc_dev[dev].enable) {
        _DBG("uvc[%u] is disabled\n", dev);
        return -1;
    }

    device_param_t *dev_param = &g_ctx.uvc_dev[dev];
    if (dev_param->hdl) {
        _DBG("uvc[%u] is created\n", dev);
        return 0;
    }

    dev_param->hdl = libuvcg_uvc_create(g_cfg.uvc_dev[dev].name);
    if (!dev_param->hdl) {
        _DBG("libuvcg_uvc_create[%u] failed\n", dev);
        return -1;
    }

    return 0;
}


static EI_S32 __destroy_uvc_device(EI_S32 dev)
{
	_DBG("dev[%u] destroy success\n", dev);
    return 0;
}

static EI_S32 __create_uvc_stream(EI_S32 dev, EI_S32 stream)
{
	if (!g_cfg.uvc_dev[dev].enable) {
		_DBG("uvc[%u] is disabled\n", dev);
		return -1;
	}

	if (!g_cfg.uvc_dev[dev].stream[stream].enable) {
		_DBG("uvc[%u] stream[%u] is disabled\n", dev, stream);
		return -1;
	}

	device_param_t *dev_param = &g_ctx.uvc_dev[dev];
	if (!dev_param->hdl) {
		_DBG("uvc[%u] is not create\n", dev);
		return -1;
	}

	stream_param_t *stream_param = &g_ctx.uvc_dev[dev].stream[stream];
	stream_param->dev = dev;
	stream_param->stream = stream;

	libuvcg_video_cb_t *stream_cb = &g_ctx.uvc_dev[dev].stream_cb[stream];
	stream_cb->ops = &g_ctx.uvc_ops;
	stream_cb->ctx = stream_param;

	stream_config_t *stream_cfg = &g_cfg.uvc_dev[dev].stream[stream];
	stream_param->hdl = libuvcg_stream_create(dev_param->hdl, stream_cfg->name, stream_cb);
	if (!stream_param->hdl) {
		_DBG("libuvcg_stream_create[%u][%u] failed\n", dev, stream);
		return -1;
	}
    return 0;
}

static EI_VOID __destroy_uvc_stream(EI_VOID *ctx)
{
	if (!ctx) {
		_DBG("ctx is NULL\n");
		return;
	}

	stream_param_t *param = (stream_param_t *)ctx;
	_DBG("dev[%u] stream[%u] destroy success\n", param->dev, param->stream);
}

static EI_S32 __start_uvc_stream(EI_VOID *ctx)
{
	stream_param_t *stream_param = NULL;

	if (!ctx) {
		_DBG("ctx is NULL\n");
		return -1;
	}
	stream_param = (stream_param_t *)ctx;

    return start_uvc_stream(stream_param);
}

static EI_VOID __stop_uvc_stream(EI_VOID *ctx)
{
	stream_param_t *stream_param = NULL;

	if (!ctx) {
		_DBG("ctx is NULL\n");
		return;
	}
	stream_param = (stream_param_t *)ctx;

    stop_uvc_stream(stream_param);
}

static EI_S32 __set_uvc_info(EI_VOID *ctx, const libuvcg_video_info_t *info)
{
	if (!ctx) {
		_DBG("ctx is NULL");
		return -1;
	}

	if (!info) {
		_DBG("info is NULL");
		return -1;
	}

	stream_param_t *param = (stream_param_t *)ctx;
	memcpy(&param->info, info, sizeof(libuvcg_video_info_t));

	return 0;
}

static EI_S32 __set_uvc_framerate(EI_VOID *ctx, EI_U32 framerate)
{
	if (!ctx) {
		_DBG("ctx is NULL");
		return -1;
	}

	stream_param_t *param = (stream_param_t *)ctx;
	param->framerate = framerate;

	return 0;
}

static EI_S32 uvc_start(void)
{
    EI_S32 ret = 0;
    EI_S32 dev = 0, stream = 0;
    
    ret = libuvcg_env_init();
	if (ret != 0) {
		_DBG("libuvcg_env_init failed\n");
		return -1;
	}
    _DBG("\n");

    g_ctx.video_cb = __video_cb;
	g_ctx.uvc_ops.set_info = __set_uvc_info;
	g_ctx.uvc_ops.set_fps = __set_uvc_framerate;
	g_ctx.uvc_ops.start = __start_uvc_stream;
	g_ctx.uvc_ops.stop = __stop_uvc_stream;
	g_ctx.uvc_ops.destroy = __destroy_uvc_stream;
    _DBG("\n");

	for (dev = 0; dev < UVC_MAX_DEV_NUM; dev++) {
		if (!g_cfg.uvc_dev[dev].enable) {
			continue;
		}

		ret = __create_uvc_device(dev);
		if (ret != 0) {
			_DBG("create_device failed\n");
			return -1;
		}

		for (stream = 0; stream < UVC_MAX_STREAM_NUM; stream++) {
			if (!g_cfg.uvc_dev[dev].stream[stream].enable) {
				continue;
			}

			ret = __create_uvc_stream(dev, stream);
			if (ret != 0) {
				_DBG("create_stream failed\n");
				return -1;
			}
		}
	}
	
    _DBG("\n");
	pthread_create(&g_ctx.uvc_pid, NULL, __uvc_process_thread, NULL);

	return 0;
}

static EI_S32 uvc_stop(void)
{
    EI_S32 ret = 0;
    EI_S32 dev = 0;
    
	if (g_ctx.uvc_pid) {
		libuvcg_exit();
		pthread_join(g_ctx.uvc_pid, NULL);
	}
	for (dev = 0; dev < UVC_MAX_DEV_NUM; dev++) {
		if (!g_cfg.uvc_dev[dev].enable) {
			continue;
		}

		ret = __destroy_uvc_device(dev);
		if (ret != 0) {
			_DBG("create_device failed\n");
			return -1;
		}
	}

	libuvcg_env_deinit();

    return 0;
}

static EI_S32 recorder_start(void)
{
    start_rec_stream();
    return 0;
}

static EI_S32 recorder_stop(void)
{
    stop_rec_stream();
    return 0;
}


static EI_S32 media_init(void)
{
    return start_video_source();
}

static EI_S32 media_deinit(void)
{
    return stop_video_source();
}

static int __get_time_string(char *buf, int size)
{
    time_t timep;
    time(&timep);
    struct tm *p = localtime(&timep);
    snprintf(buf, size-1, "%04d-%02d-%02d %02d:%02d:%02d", (1900+p->tm_year),
            (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    return 0;
}

static int __get_channel_name(char *buf, int size)
{
    snprintf(buf, size-1, CAR_CHANNEL_NAME);

    return 0;
}

static void __get_rgn_info(char str[][MAX_STRING_LEN])
{
    ISP_FRAME_INFO_S stIspFrame = {0};
    int ispDev = g_cfg.uvc_dev[0].stream[0].ippu_dev;

    __get_channel_name(str[RGN_TYPE_CHANNEL], MAX_STRING_LEN);
    __get_time_string(str[RGN_TYPE_TIME], MAX_STRING_LEN);
    
    EI_MI_ISP_GetFrameInfo(ispDev, &stIspFrame);
    snprintf(str[RGN_TYPE_3A_AwbMode], MAX_STRING_LEN - 1, "AwbMode: %d", stIspFrame.u32AwbMode);
    snprintf(str[RGN_TYPE_3A_ColorTempId], MAX_STRING_LEN - 1, "ColorTempId: %d", stIspFrame.u32ColorTempId);    
    snprintf(str[RGN_TYPE_3A_AeMode], MAX_STRING_LEN - 1, "AeMode: %d", stIspFrame.u32AeMode);
    snprintf(str[RGN_TYPE_3A_FlickerMode], MAX_STRING_LEN - 1, "FlickerMode: %d", stIspFrame.u32FlickerMode);
    snprintf(str[RGN_TYPE_3A_ISO], MAX_STRING_LEN - 1, "ISO: %d", stIspFrame.u32ISO);
    snprintf(str[RGN_TYPE_3A_ExposureTime], MAX_STRING_LEN - 1, "ExposureTime: %d", stIspFrame.u32ExposureTime);    
    snprintf(str[RGN_TYPE_3A_IspDgain], MAX_STRING_LEN - 1, "IspDgain: %d", stIspFrame.u32IspDgain);
    snprintf(str[RGN_TYPE_3A_Again], MAX_STRING_LEN - 1, "Again: %d", stIspFrame.u32Again);
    snprintf(str[RGN_TYPE_3A_Dgain], MAX_STRING_LEN - 1, "Dgain: %d", stIspFrame.u32Dgain);    
    snprintf(str[RGN_TYPE_3A_Ratio], MAX_STRING_LEN - 1, "Ratio: %d %d %d", stIspFrame.au32Ratio[0], stIspFrame.au32Ratio[1], stIspFrame.au32Ratio[2]);
    snprintf(str[RGN_TYPE_3A_BV], MAX_STRING_LEN - 1, "BV: %d", stIspFrame.s32BV);
    snprintf(str[RGN_TYPE_3A_FNumber], MAX_STRING_LEN - 1, "FNumber: %d", stIspFrame.u32FNumber);    
}

static void * _update_rgn_proc(void * arg)
{
    char str[RGN_TYPE_MAX][MAX_STRING_LEN];
    int ippu_uvc_chn = g_cfg.uvc_dev[0].stream[0].ippu_chn;
    int ippu_rec_chn = g_cfg.recorder.ippu_chn;
    g_rgn_thread_flag = 1;

    while (g_rgn_thread_flag) {
        __get_rgn_info(str);

        //update_rgn(RGN_TYPE_CHANNEL, ippu_uvc_chn, str[RGN_TYPE_CHANNEL]);
        update_rgn(RGN_TYPE_TIME, ippu_uvc_chn, str[RGN_TYPE_TIME]);

#if 1
        update_rgn(RGN_TYPE_3A_AwbMode, ippu_uvc_chn, str[RGN_TYPE_3A_AwbMode]);
        update_rgn(RGN_TYPE_3A_ColorTempId, ippu_uvc_chn, str[RGN_TYPE_3A_ColorTempId]);
        update_rgn(RGN_TYPE_3A_AeMode, ippu_uvc_chn, str[RGN_TYPE_3A_AeMode]);
        update_rgn(RGN_TYPE_3A_FlickerMode, ippu_uvc_chn, str[RGN_TYPE_3A_FlickerMode]);
        update_rgn(RGN_TYPE_3A_ISO, ippu_uvc_chn, str[RGN_TYPE_3A_ISO]);
        update_rgn(RGN_TYPE_3A_ExposureTime, ippu_uvc_chn, str[RGN_TYPE_3A_ExposureTime]);
        update_rgn(RGN_TYPE_3A_IspDgain, ippu_uvc_chn, str[RGN_TYPE_3A_IspDgain]);
        update_rgn(RGN_TYPE_3A_Again, ippu_uvc_chn, str[RGN_TYPE_3A_Again]);
        update_rgn(RGN_TYPE_3A_Dgain, ippu_uvc_chn, str[RGN_TYPE_3A_Dgain]);
        update_rgn(RGN_TYPE_3A_Ratio, ippu_uvc_chn, str[RGN_TYPE_3A_Ratio]);
        update_rgn(RGN_TYPE_3A_BV, ippu_uvc_chn, str[RGN_TYPE_3A_BV]);
        update_rgn(RGN_TYPE_3A_FNumber, ippu_uvc_chn, str[RGN_TYPE_3A_FNumber]);

        //update_rgn(RGN_TYPE_CHANNEL, ippu_rec_chn, str[RGN_TYPE_CHANNEL]);
        update_rgn(RGN_TYPE_TIME, ippu_rec_chn, str[RGN_TYPE_TIME]);
        update_rgn(RGN_TYPE_3A_AwbMode, ippu_rec_chn, str[RGN_TYPE_3A_AwbMode]);
        update_rgn(RGN_TYPE_3A_ColorTempId, ippu_rec_chn, str[RGN_TYPE_3A_ColorTempId]);
        update_rgn(RGN_TYPE_3A_AeMode, ippu_rec_chn, str[RGN_TYPE_3A_AeMode]);
        update_rgn(RGN_TYPE_3A_FlickerMode, ippu_rec_chn, str[RGN_TYPE_3A_FlickerMode]);
        update_rgn(RGN_TYPE_3A_ISO, ippu_rec_chn, str[RGN_TYPE_3A_ISO]);
        update_rgn(RGN_TYPE_3A_ExposureTime, ippu_rec_chn, str[RGN_TYPE_3A_ExposureTime]);
        update_rgn(RGN_TYPE_3A_IspDgain, ippu_rec_chn, str[RGN_TYPE_3A_IspDgain]);
        update_rgn(RGN_TYPE_3A_Again, ippu_rec_chn, str[RGN_TYPE_3A_Again]);
        update_rgn(RGN_TYPE_3A_Dgain, ippu_rec_chn, str[RGN_TYPE_3A_Dgain]);
        update_rgn(RGN_TYPE_3A_Ratio, ippu_rec_chn, str[RGN_TYPE_3A_Ratio]);
        update_rgn(RGN_TYPE_3A_BV, ippu_rec_chn, str[RGN_TYPE_3A_BV]);
        update_rgn(RGN_TYPE_3A_FNumber, ippu_rec_chn, str[RGN_TYPE_3A_FNumber]);

#endif
        sleep(1);
    }
    return NULL;
}

static int rgn_start(void)
{
    int devId = g_cfg.uvc_dev[0].stream[0].ippu_dev;
    int ippu_uvc_chn = g_cfg.uvc_dev[0].stream[0].ippu_chn;
    int ippu_rec_chn = g_cfg.recorder.ippu_chn;

    int bmp_width_3a = 25 * 16;
    int bmp_height_3a = 32;
    int bmp_3a_px = 16;
    int bmp_3a_py = 50;
    int bmp_3a_h_diff = 5;

    int chn_str_len = strlen(CAR_CHANNEL_NAME) ;
    int chn_bmp_width = chn_str_len * 16;
    _DBG("chn_str_len = %d, chn_bmp_width = %d", chn_str_len, chn_bmp_width);
    int chn_bmp_height = 32;
    int chn_px = 16;
    int chn_py = 16;

    int time_str_len = strlen(CAR_RUN_TIME);
    int time_bmp_width = time_str_len * 16;
    int time_bmp_height = 32;
    int time_px = 600;
    int time_py = 16;

    //create_rgn(RGN_TYPE_CHANNEL, devId, ippu_uvc_chn, chn_bmp_width, chn_bmp_height, chn_px, chn_py);
    create_rgn(RGN_TYPE_TIME, devId, ippu_uvc_chn, time_bmp_width, time_bmp_height, time_px, time_py);

#if 1
    create_rgn(RGN_TYPE_3A_AwbMode, devId, ippu_uvc_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py);
    create_rgn(RGN_TYPE_3A_AeMode, devId, ippu_uvc_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 1 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_FlickerMode, devId, ippu_uvc_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 2 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_ISO, devId, ippu_uvc_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 3 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_ExposureTime, devId, ippu_uvc_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 4 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_IspDgain, devId, ippu_uvc_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 5 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_Again, devId, ippu_uvc_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 6 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_Dgain, devId, ippu_uvc_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 7 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_Ratio, devId, ippu_uvc_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 8 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_BV, devId, ippu_uvc_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 9 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_FNumber, devId, ippu_uvc_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 10 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_ColorTempId, devId, ippu_uvc_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 11 * (bmp_height_3a + bmp_3a_h_diff));

    //create_rgn(RGN_TYPE_CHANNEL, devId, ippu_rec_chn, chn_bmp_width, chn_bmp_height, chn_px, chn_py);
    create_rgn(RGN_TYPE_TIME, devId, ippu_rec_chn, time_bmp_width, time_bmp_height, time_px, time_py);

    create_rgn(RGN_TYPE_3A_AwbMode, devId, ippu_rec_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py);
    create_rgn(RGN_TYPE_3A_AeMode, devId, ippu_rec_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 1 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_FlickerMode, devId, ippu_rec_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 2 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_ISO, devId, ippu_rec_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 3 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_ExposureTime, devId, ippu_rec_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 4 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_IspDgain, devId, ippu_rec_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 5 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_Again, devId, ippu_rec_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 6 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_Dgain, devId, ippu_rec_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 7 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_Ratio, devId, ippu_rec_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 8 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_BV, devId, ippu_rec_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 9 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_FNumber, devId, ippu_rec_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 10 * (bmp_height_3a + bmp_3a_h_diff));
    create_rgn(RGN_TYPE_3A_ColorTempId, devId, ippu_rec_chn, bmp_width_3a, bmp_height_3a, bmp_3a_px, bmp_3a_py + 11 * (bmp_height_3a + bmp_3a_h_diff));
#endif

    pthread_create(&g_rgn_pid, NULL, _update_rgn_proc, NULL);
    
    return 0;
}

static int rgn_stop(void)
{
    int devId = g_cfg.uvc_dev[0].stream[0].ippu_dev;
    int ippu_uvc_chn = g_cfg.uvc_dev[0].stream[0].ippu_chn;
    int ippu_rec_chn = g_cfg.recorder.ippu_chn;

    if (g_rgn_thread_flag == 1)  {
        g_rgn_thread_flag = 0;
        pthread_join(g_rgn_pid, NULL);
    }

    destory_rgn(RGN_TYPE_CHANNEL, devId, ippu_uvc_chn);
    destory_rgn(RGN_TYPE_TIME, devId, ippu_uvc_chn);

    destory_rgn(RGN_TYPE_3A_AwbMode, devId, ippu_uvc_chn);
    destory_rgn(RGN_TYPE_3A_AeMode, devId, ippu_uvc_chn);
    destory_rgn(RGN_TYPE_3A_FlickerMode, devId, ippu_uvc_chn);
    destory_rgn(RGN_TYPE_3A_ISO, devId, ippu_uvc_chn);
    destory_rgn(RGN_TYPE_3A_ExposureTime, devId, ippu_uvc_chn);
    destory_rgn(RGN_TYPE_3A_IspDgain, devId, ippu_uvc_chn);
    destory_rgn(RGN_TYPE_3A_Again, devId, ippu_uvc_chn);
    destory_rgn(RGN_TYPE_3A_Dgain, devId, ippu_uvc_chn);
    destory_rgn(RGN_TYPE_3A_BV, devId, ippu_uvc_chn);
    destory_rgn(RGN_TYPE_3A_FNumber, devId, ippu_uvc_chn);
    
    destory_rgn(RGN_TYPE_3A_AwbMode, devId, ippu_rec_chn);
    destory_rgn(RGN_TYPE_3A_AeMode, devId, ippu_rec_chn);
    destory_rgn(RGN_TYPE_3A_FlickerMode, devId, ippu_rec_chn);
    destory_rgn(RGN_TYPE_3A_ISO, devId, ippu_rec_chn);
    destory_rgn(RGN_TYPE_3A_ExposureTime, devId, ippu_rec_chn);
    destory_rgn(RGN_TYPE_3A_IspDgain, devId, ippu_rec_chn);
    destory_rgn(RGN_TYPE_3A_Again, devId, ippu_rec_chn);
    destory_rgn(RGN_TYPE_3A_Dgain, devId, ippu_rec_chn);
    destory_rgn(RGN_TYPE_3A_BV, devId, ippu_rec_chn);
    destory_rgn(RGN_TYPE_3A_FNumber, devId, ippu_rec_chn);

    return 0;
}


int main(int argc, char *argv[])
{
    media_init();
    uvc_start();
    recorder_start();
    rgn_start();

    while(1) sleep(1);
    rgn_stop();
    recorder_stop();
    return 0;
}
