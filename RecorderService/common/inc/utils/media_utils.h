#ifndef _MEDIA_UTILS_H_
#define _MEDIA_UTILS_H_

#include "typedef.h"
#if (defined(INDIA_2CH) || defined(CAM2CH))
#define MAX_VIDEO_NUM               2
#else
#if (defined(ONLY_3rdCH) || defined(CAM1CH))
#define MAX_VIDEO_NUM               1
#else
#define MAX_VIDEO_NUM               3
#endif
#endif
#define MAX_AUDIO_NUM               1

#ifdef INDIA_2CH
#define CH1_VIDEO_STREAM_SIZE       (60*1024*1024)
#define CH2_VIDEO_STREAM_SIZE       (40*1024*1024)
#define CH3_VIDEO_STREAM_SIZE       (40*1024*1024)
#else
#ifdef VIETNAM_2CH
#define CH1_VIDEO_STREAM_SIZE       (60*1024*1024)
#define CH2_VIDEO_STREAM_SIZE       (40*1024*1024)
#define CH3_VIDEO_STREAM_SIZE       (40*1024*1024)
#else
#define CH1_VIDEO_STREAM_SIZE       (30*1024*1024)
#define CH2_VIDEO_STREAM_SIZE       (20*1024*1024)
#define CH3_VIDEO_STREAM_SIZE       (20*1024*1024)
#endif
#endif

#define CH1_VIDEO_WIDTH				1920
#define CH1_VIDEO_HEIGHT			1080
#define CH1SUB_VIDEO_WIDTH			640
#define CH1SUB_VIDEO_HEIGHT			360

#define CH2_VIDEO_WIDTH				1280
#define CH2_VIDEO_HEIGHT			720
#define CH2SUB_VIDEO_WIDTH			640
#define CH2SUB_VIDEO_HEIGHT			360

#define CH3_VIDEO_WIDTH				1280
#define CH3_VIDEO_HEIGHT			720
#define CH3SUB_VIDEO_WIDTH			640
#define CH3SUB_VIDEO_HEIGHT			360

#define VIDEO_FRAME_INDEX_SIZE      (128*1024)

#define PER_FILE_RECORD_TIME        1 //min

#ifdef ONLY_3rdCH
#define DMS_CAM_CHANNEL				0
#define ADAS_CAM_CHANNEL			-1
#else
#define DMS_CAM_CHANNEL				1
#define ADAS_CAM_CHANNEL			0
#endif

#ifdef H265
#define VIDEO_PT_H265
#endif

typedef enum RecordState{
    REC_STOP,       //停止录像
    REC_START,      //准备开始新文件
    REC_RECORDING,  //正在录制
    REC_CAMERAERR,  //未接摄像�?
}RecordState;

/**
 * 视频编码方式,默认H264/H265两种编码方式.
 */
typedef enum EncodeFormat{
    H264,
    H265
} EncodeFormat;

/**
 * 录像编码的尺�?.
 * 有CIF,D1,720P,1080P四种尺寸可以选择.
 */
typedef enum Resolution{
    RES_CIF, // 352*288PAL 352*240NTSC
    RES_D1, // 704*576PAL 720*480NTSC
    RES_WD1, // 720*576PAL 960*480NTSC
    RES_720p,
    RES_1080p
} Resolution;

/**
 * 编码模式.
 */
typedef enum EncodeType{
    CBR,
    VBR,
} EncodeType;

/**
 * 通道类型
 */
typedef enum {
    CHANNEL_MASTER,//主码�?
    CHANNEL_CHILD,//子码�?
} CHANNEL_TYPE;

/**
 * 帧类�?
 */
typedef enum {
    FRAME_I = 1,//I�?
    FRAME_P = 2,//P�?
    FRAME_B = 3,//B�?
} FRAME_TYPE;

/**
 * 镜头插入状�?
 */
typedef enum {
    UN_PLUGIN = 0x00,//未插�?
    PLUGIN_720P = 0x01,//已插�?720P摄像�?
    PLUGIN_1080P = 0x02,//已插�?1080P摄像�?
    UN_HIT = 0x00,   //未遮�?
    HIT = 0x10,      //已遮�?
} CAMERA_STATE;

/**
 * OSD字体颜色.
 */
typedef enum Color{
    RED,
    GREEN,
    BLUE,
} Color;

typedef struct
{
    
    uint32_t width;
    uint32_t hight;
    uint32_t bitrate;
    uint32_t framerate;
 //   PAYLOAD_TYPE_E enType;
    uint32_t video_duration;

} video_encode_info_t;

typedef struct
{
    
    uint32_t frameSize;
	uint32_t frameType;
    uint64_t timeStamp;
    char* frameBuffer;

} FrameInfo_t;

/**
 * 索引文件记录信息
 */
typedef struct
{

    uint64_t timeStamp;//时间
    uint8_t mediaType;//媒体类型 1：音�? 2:视频
    uint8_t  frameType;//帧类�?
    uint8_t  frameRate;//帧率
    uint8_t  iFrameInterval;//帧间�?
    uint64_t warningFlag;//报警标识
    uint64_t OFFSET;//偏移
    uint32_t frameSize;//帧长�?  
    uint64_t gpsTimeStamp;//GPS时间�? 

}__attribute__((packed))H264MediaIndex_t;

typedef struct
{

    uint64_t timeStamp;
    uint8_t mediaType;//媒体类型 1：音�? 2:视频
    uint64_t OFFSET;
    uint32_t frameSize;
    uint64_t gpsTimeStamp;//GPS时间�? 

}__attribute__((packed)) AudioMediaIndex_t;

#endif
