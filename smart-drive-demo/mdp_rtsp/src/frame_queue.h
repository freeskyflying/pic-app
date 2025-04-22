#ifndef __FRAME_QUEUE_H__
#define __FRAME_QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>
#include "systype.h"

typedef enum {
	QU_MAIN_RTSP = 0,
	QU_MAIN_RECORD,		//1
	QU_SUN_RTSP,		//2
	QU_SUN_RECORD,		//3
	QU_THIRD_RTSP,		//4
	QU_THIRD_RECORD,		//5
	QU_WEB_WS,		//6

	QU_AUDIO_0_RTSP,

	MAX_QU
} frame_queues;

#define		FRAME_QUEUE_SIZE				32
#define		FRAME_QUEUE_NUM_MAX				MAX_QU

typedef void (* frame_release_cb_t)(void *);
typedef void (* frame_retain_cb_t)(void *);
typedef enum
{
    FRAME_TYPE_I_FRAME = 0,
    FRAME_TYPE_P_FRAME,
    FRAME_TYPE_B_FRAME,
}frame_type_e;

typedef	struct
{
	byte			byType;
	frame_type_e	eSubType;
	dword 			dwTargetFrameRate;
	dword 			dwFrameCounter;
	dword 			dwChnId;
	dword			dwLowerTimeStamp;
	dword			dwUpperTimeStamp;
	dword			dwDataLen;
    /*use when copy to user*/
    dword			dwUserBufferSize;
	dword			dwWidth;
	dword			dwHeight;
	frame_release_cb_t 	release_cb;
	void    *		release_arg;
	frame_retain_cb_t   retain_cb;
	void    *		retain_arg;
	byte	*		pbyData;
	byte 			byData[0];

}frame_dsc_s;


typedef	struct
{

}FRAME_QUEUE_INFO_S;


typedef enum
{
    FRAME_QUEUE_WAIT = 0,
    FRAME_QUEUE_NO_WAIT,
}frame_queue_wait_e;

sdword frame_queue_init_default();

sdword frame_queue_init(int pool_size, int *queue_depth);


sdword frame_queue_get_node_frame(dword dwFrameQueueNum, frame_dsc_s * pstFrameDsc, frame_queue_wait_e eFrameQueueWait);


sdword frame_queue_push_node(dword dwFrameQueueNum, frame_dsc_s * pstFrameDsc, dword dwTimeOut);

sbool frame_queue_is_empty(dword dwFrameQueueNum);

sdword frame_queue_flush(dword dwFrameQueueNum);

sdword frame_queue_counter(dword dwFrameQueueNum);

int live_video_set_data_to_buf(dword dwFrameQueueNum, frame_dsc_s *FrameDsc);


#ifdef __cplusplus
}
#endif

#endif



