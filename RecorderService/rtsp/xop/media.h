// PHZ
// 2018-5-16

#ifndef XOP_MEDIA_H
#define XOP_MEDIA_H

#include <memory>

namespace xop
{

/* RTSP服务支持的媒体类型 */
enum MediaType
{
	//PCMU = 0,	 
	PCMA = 8,
	H264 = 96,
	AAC  = 37,
	H265 = 265,   
	NONE
};	

enum FrameType
{
	VIDEO_FRAME_I = 0x01,	  
	VIDEO_FRAME_P = 0x02,
	VIDEO_FRAME_B = 0x03,    
	AUDIO_FRAME   = 0x11,   
};

struct AVFrame
{	
	AVFrame()
	{
		this->size = size;
		type = 0;
		timestamp = 0;
	}

	uint8_t* buffer = NULL; /* 帧数据 */
	uint32_t size = 0;				     /* 帧大小 */
	uint8_t  type = 0;				     /* 帧类型 */	
	uint32_t timestamp = 0;		  	     /* 时间戳 */
};

static const int MAX_MEDIA_CHANNEL = 2;

enum MediaChannelId
{
	channel_0,
	channel_1
};

typedef uint32_t MediaSessionId;

}

#endif

