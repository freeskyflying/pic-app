#ifndef __UVC_I_H__
#define __UVC_I_H__


#define MDA_FMT		"[M][%s:%d] "

#define _DBG(fmt, ...) \
    do { \
        struct timespec t; \
        clock_gettime(CLOCK_MONOTONIC, &t); \
        fprintf(stderr, "[%7ld.%03ld]"MDA_FMT fmt, \
            t.tv_sec, t.tv_nsec/1000000L,  \
            __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define ENC_STREAM_MAX_NUM 1  
#define UVC_MAX_STREAM_NUM 2
#define UVC_MAX_SNS_TYPE_SWITCH_NUM 4
#define UVC_MAX_STREAM_CALLBACK_NUM 4
    
#define UVC_MAX_DEV_NUM 2

typedef enum comp_video_format {
	/* encode format */
	COMP_VIDEO_FORMAT_H264 = 0,
	COMP_VIDEO_FORMAT_H265,
	COMP_VIDEO_FORMAT_MJPEG,
	COMP_VIDEO_FORMAT_JPEG,
	/* raw fromat */
	COMP_VIDEO_FORMAT_NV12,
	COMP_VIDEO_FORMAT_NV21,
	COMP_VIDEO_FORMAT_YUYV,
} comp_video_format_e;

typedef struct stream_param {
	EI_VOID *hdl;
	EI_U32 dev;
	EI_U32 stream;
	libuvcg_video_info_t info;
	EI_U32 framerate;
	EI_BOOL running;
	comp_video_format_e format;
} stream_param_t;

typedef struct device_param {
	EI_VOID *hdl;
	stream_param_t stream[UVC_MAX_STREAM_NUM];
	libuvcg_video_cb_t stream_cb[UVC_MAX_STREAM_NUM];
} device_param_t;

typedef struct video_cb_param {
	IPPU_DEV ippu_dev;
	IPPU_CHN ippu_chn;
	VC_CHN vc_chn;
	comp_video_format_e format;  //0: venc  1:raw
	union {
		VENC_STREAM_S *enc_stream[ENC_STREAM_MAX_NUM];
		VIDEO_FRAME_INFO_S *raw_frame;
	};
} video_cb_param_t;

typedef struct stream_config {
	EI_BOOL enable;
	EI_CHAR name[8];
	IPPU_DEV ippu_dev;
	IPPU_CHN ippu_chn;
	ROTATION_E rotation;
	EI_S32 venc_chn;
} stream_config_t;

typedef struct sns_type_switch_param {
	libuvcg_video_format_e trigger_format;
	SIZE_S trigger_size;
	EI_U32 target_framerate;
	WDR_MODE_E target_wdr_mode;
	EI_U32 framerate_reduce;
} sns_type_switch_param_t;

typedef struct sns_type_switch_config {
	EI_BOOL enable;
	sns_type_switch_param_t param[UVC_MAX_SNS_TYPE_SWITCH_NUM];
} sns_type_switch_config_t;

typedef struct uvc_device_config {
	EI_BOOL enable;
	EI_CHAR name[8];
	stream_config_t stream[UVC_MAX_STREAM_NUM];
	EI_BOOL face_exposure;
	sns_type_switch_config_t sns_type_switch;
} uvc_device_config_t;

typedef struct rec_channel_config {
	// video
	IPPU_DEV ippu_dev;
	IPPU_CHN ippu_chn;
	ROTATION_E rotation;
	EI_S32 venc_chn;
	EI_S32 width;
	EI_S32 height;
	EI_U32 framerate;
	EI_U32 bitrate;	
	PAYLOAD_TYPE_E video_format;
	
	// audio
	EI_U32 sample_rate;
	PAYLOAD_TYPE_E audio_format;
	EI_S32 audio_channel;
} rec_channel_config_t;

typedef struct uvc_config {
    SNS_TYPE_E sesnor;
    VISS_DEV viss_dev;
	uvc_device_config_t uvc_dev[UVC_MAX_DEV_NUM];
	rec_channel_config_t recorder;
} uvc_config_t;

typedef EI_S32 (*video_callback)(video_cb_param_t *param);
typedef EI_S32 (*uvc_start_stream)(IPPU_DEV dev, IPPU_CHN chn);
typedef EI_S32 (*uvc_stop_stream)(IPPU_DEV dev, IPPU_CHN chn);

typedef struct uvc_context {
    EI_S32 camerNum;
    SAMPLE_VISS_CONFIG_S vissConfig;
    VBUF_CONFIG_S vbufConfig;
    CH_FORMAT_S stFormat[VISS_DEV_MAX_CHN_NUM];

	device_param_t uvc_dev[UVC_MAX_DEV_NUM];
	pthread_t uvc_pid;
	pthread_t video_pid[UVC_MAX_DEV_NUM][UVC_MAX_STREAM_NUM];
	pthread_t rec_pid;
	libuvcg_video_cb_ops_t uvc_ops;
	video_callback video_cb;
	uvc_start_stream start_stream_cb[UVC_MAX_STREAM_CALLBACK_NUM];
	uvc_stop_stream stop_stream_cb[UVC_MAX_STREAM_CALLBACK_NUM];
	EI_BOOL face_exposure;
} uvc_context_t;



#endif
