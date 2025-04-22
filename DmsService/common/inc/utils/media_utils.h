#ifndef _MEDIA_UTILS_H_
#define _MEDIA_UTILS_H_

#include "typedef.h"

#define MAX_VIDEO_NUM               1
#define MAX_AUDIO_NUM               1


#define AUDIO_CACHE_SIZE (128*1024)
#define VIDEO_CACHE_SIZE (4*1024*1024)
#define INDEX_CACHE_SIZE (16*1024)

#define CH1_VIDEO_STREAM_SIZE       (24*1024*1024)
#define CH2_VIDEO_STREAM_SIZE       (18*1024*1024)

#define CH1_VIDEO_WIDTH				1280
#define CH1_VIDEO_HEIGHT			720
#define CH2_VIDEO_WIDTH				1280
#define CH2_VIDEO_HEIGHT			720

#define VIDEO_FRAME_INDEX_SIZE      (128*1024)

#define PER_FILE_RECORD_TIME        1 //min

typedef enum RecordState{
    REC_STOP,       //停止录像
    REC_START,      //准备开始新文件
    REC_RECORDING,  //正在录制
    REC_CAMERAERR,  //未接摄像头
}RecordState;

/**
 * 视频编码方式,默认H264/H265两种编码方式.
 */
typedef enum EncodeFormat{
    H264,
    H265
} EncodeFormat;

/**
 * 录像编码的尺寸.
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
    CHANNEL_MASTER,//主码流
    CHANNEL_CHILD,//子码流
} CHANNEL_TYPE;

/**
 * 帧类型
 */
typedef enum {
    FRAME_I = 1,//I帧
    FRAME_P = 2,//P帧
    FRAME_B = 3,//B帧
} FRAME_TYPE;

/**
 * 镜头插入状态
 */
typedef enum {
    UN_PLUGIN = 0x00,//未插入
    PLUGIN_720P = 0x01,//已插入720P摄像头
    PLUGIN_1080P = 0x02,//已插入1080P摄像头
    UN_HIT = 0x00,   //未遮挡
    HIT = 0x10,      //已遮挡
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
    uint8_t mediaType;//媒体类型 1：音频 2:视频
    uint8_t  frameType;//帧类型
    uint8_t  frameRate;//帧率
    uint8_t  iFrameInterval;//帧间隔
    uint64_t warningFlag;//报警标识
    uint64_t OFFSET;//偏移
    uint32_t frameSize;//帧长度  
    uint64_t gpsTimeStamp;//GPS时间戳 

}__attribute__((packed))H264MediaIndex_t;

typedef struct
{

    uint64_t timeStamp;
    uint8_t mediaType;//媒体类型 1：音频 2:视频
    uint64_t OFFSET;
    uint32_t frameSize;
    uint64_t gpsTimeStamp;//GPS时间戳 

}__attribute__((packed)) AudioMediaIndex_t;

#endif
