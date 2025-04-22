#include <unistd.h>
#include <malloc.h>
#include <pthread.h>

#define LOG_TAG "live_test"

#include "live_server.h"
#include "frame_queue.h"
#include <log/log.h>

#include "ei_type.h"

static pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;

int get_counter = 0;
/*TODO replace it*/
int live_video_get_data_from_buf(int channel,
				unsigned char *buffer,
				unsigned int buffer_size,
				unsigned int *size){
	EI_S32 s32Ret = EI_SUCCESS;

	get_counter++;
	//if (get_counter % 5 == 4)
	//EI_PRINT("get_counter = %d \n", get_counter);

	frame_dsc_s FrameDsc;
	FrameDsc.pbyData = buffer;
	FrameDsc.dwUserBufferSize = buffer_size;
	
	pthread_mutex_lock(&buffer_lock);
	if (SYS_OK == frame_queue_get_node_frame(channel,
			&FrameDsc, FRAME_QUEUE_NO_WAIT)) {
		*size = FrameDsc.dwDataLen;
		s32Ret = EI_SUCCESS;
	}else{
		//printf("frame_queue_get_node_frame failed !\n");
		s32Ret = EI_FAILURE;
	}
	pthread_mutex_unlock(&buffer_lock);
	
    return s32Ret;
}

int live_video_set_data_to_buf(dword dwFrameQueueNum, frame_dsc_s *FrameDsc)
{
	int ret = EI_SUCCESS;

	pthread_mutex_lock(&buffer_lock);
	ret = frame_queue_push_node(dwFrameQueueNum, FrameDsc, 0);
	if (ret != EI_SUCCESS) {
		pthread_mutex_unlock(&buffer_lock);
		/* printf("%d-%d frame_queue_push_node error \n", __func__, __LINE__); */
		return EI_FAILURE;
	}
	pthread_mutex_unlock(&buffer_lock);
	
	return EI_SUCCESS;
}