/*
 * framequeue.cpp
 *
 *  Created on:
 *      Author: huangyikao
 */

#include "ref_queue.h"

#define LOG_TAG "app_frame_queue"
#include <log/log.h>

#include "live_video_priv.h"

using namespace std;

static void deleter(frame_dsc_s *dsc)
{
	if (dsc){
		/*TRACE_ERROR("dsc:%p priv:%p cb:%p is deleted  \n",
            dsc,
            dsc->release_arg,
            dsc->release_cb);*/
		if (0 != dsc->release_cb){
			dsc->release_cb(dsc->release_arg);
		}
        delete dsc;
	}

}



ref_queue::ref_queue(int depth) {
	_depth = depth;
	_is_pause_get = 0;
}

void ref_queue::pause_get(void) {
	lock_guard<mutex> lock(_mutex);
	_is_pause_get = 1;
}

void ref_queue::resume_get(void) {
	lock_guard<mutex> lock(_mutex);
	_is_pause_get = 0;
}

int ref_queue::put(const frame_dsc_s& dsc) {
	lock_guard<mutex> lock(_mutex);
	if ( _queue.size() >= _depth){
		/*TRACE_ERROR("_is_freeze:%d cur_sz:%u dsc:%p priv:%p full\n",
					_is_pause_get,
					_queue.size(),
					&dsc,
					dsc.release_arg
					);*/
		return SYS_ERROR;
	}
	frame_dsc_s *ptr = new frame_dsc_s;
	*ptr = dsc;
	//TRACE_DEBUG("dsc:%p priv:%p\n", ptr, ptr->release_arg);
	ref_node_t node(ptr, deleter);
	_queue.push(node);
    if (dsc.retain_cb){
        dsc.retain_cb(dsc.retain_arg);
    }

	return SYS_OK;
}


int ref_queue::get(frame_dsc_s& dsc) {
	lock_guard<mutex> lock(_mutex);
	if (_is_pause_get == 1 || _queue.empty()){
		/*TRACE_ERROR("_is_freeze:%d cur_sz:%u %s %s\n",
					_is_pause_get,
					_queue.size(),
					_is_pause_get?"stop_get":"",
					_queue.empty()?"empty":"");*/
		return SYS_ERROR;
	}
	ref_node_t node = _queue.front();
	_queue.pop();
	dsc = *node;
	return SYS_OK;

}

bool ref_queue::empty(void) {
	return _queue.empty();
}

int ref_queue::size(void) {
	return _queue.size();
}

int ref_queue::peek(frame_dsc_s& dsc) {
	lock_guard<mutex> lock(_mutex);
	if (_queue.empty()){
		/*TRACE_ERROR("_is_freeze:%d cur_sz:%llu %s\n",
					_is_pause_get,
					_queue.size(),
					_queue.empty()?"empty":"");*/
		return SYS_ERROR;
	}
	ref_node_t node = _queue.front();
	dsc = *node;
	return SYS_OK;
}
int ref_queue::flush(void) {
	lock_guard<mutex> lock(_mutex);
	if (_is_pause_get == 1){
		/*TRACE_ERROR("_is_freeze:%d cur_sz:%llu \n",
					_is_pause_get,
					_queue.size());*/
		return SYS_ERROR;
	}
	while (!_queue.empty()){
		_queue.pop();
	}
	return SYS_OK;
}


#ifdef __cplusplus
extern "C" {
#endif
typedef struct ref_queue_pool{
	ref_queue **queues;
	unsigned int pool_size;
}ref_queue_pool_t;
static ref_queue_pool_t qp = {0};
sdword frame_queue_init_default()
{
	int pool[FRAME_QUEUE_NUM_MAX] = {0};
	for(int i = 0; i < FRAME_QUEUE_NUM_MAX; i++)
	{
		pool[i] = FRAME_QUEUE_SIZE;
	}

	return frame_queue_init(FRAME_QUEUE_NUM_MAX, pool);
}

sdword frame_queue_init(int pool_size, int *queue_depth)
{
	/*for (unsigned int i = 0; i < qp.pool_size; i++){
		if (qp.queues){
			ref_queue *q = qp.queues[i];
			if (q){
				delete q;
			}
		}
	}*/

	qp.pool_size = pool_size;
	if (pool_size > 0 && queue_depth){
		qp.queues = new ref_queue *[pool_size];
		for (unsigned int i = 0; i < qp.pool_size; i++){
			qp.queues[i] = new ref_queue(queue_depth[i]);
		}
		return SYS_OK;
	}else{
		return SYS_ERROR;
	}
}


sdword frame_queue_get_node_frame(dword dwFrameQueueNum, frame_dsc_s * pstFrameDsc, frame_queue_wait_e eFrameQueueWait)
{
	sdword Ret = SYS_OK;
    dword CopySize = 0;
    dword UserDataSize = 0;
	if (dwFrameQueueNum < qp.pool_size ){
		ref_queue *q = qp.queues[dwFrameQueueNum];
		frame_dsc_s dsc = {0};
		frame_dsc_s tmp = {0};
		if (SYS_OK == q->peek(dsc)){
			byte *dst = pstFrameDsc->pbyData;
            UserDataSize = pstFrameDsc->dwUserBufferSize;
			*pstFrameDsc = dsc;
            /*restore*/
            pstFrameDsc->dwUserBufferSize = UserDataSize;
			pstFrameDsc->pbyData = dst;
            if (pstFrameDsc->dwUserBufferSize ){
                CopySize = (pstFrameDsc->dwUserBufferSize > dsc.dwDataLen)?
                    dsc.dwDataLen:pstFrameDsc->dwUserBufferSize;
            }else{
                CopySize = dsc.dwDataLen;
            }
            if (pstFrameDsc->dwUserBufferSize < dsc.dwDataLen){
                printf("dwUserBufferSize:%d dwDataLen:%d\n",
                    pstFrameDsc->dwUserBufferSize,
                    dsc.dwDataLen);
            }
			if (dst && dsc.pbyData){
				memcpy(dst, dsc.pbyData, CopySize);
				Ret =  SYS_OK;
				free(dsc.pbyData);
			}else{
				Ret =  SYS_ERROR;
			}
			q->get(tmp);
		}else{
			Ret = SYS_ERROR;
		}
	}else{
		Ret = SYS_ERROR;
	}
	return Ret;
}


sdword frame_queue_push_node(dword dwFrameQueueNum, frame_dsc_s * pstFrameDsc, dword dwTimeOut)
{
	if (dwFrameQueueNum < qp.pool_size && 
			frame_queue_counter(dwFrameQueueNum) < FRAME_QUEUE_SIZE){
		#if 1
		byte *pbyData = (byte *) malloc(pstFrameDsc->dwDataLen);
		memcpy(pbyData, pstFrameDsc->pbyData, pstFrameDsc->dwDataLen);
		pstFrameDsc->pbyData = pbyData;
		#endif
		
		ref_queue *q = qp.queues[dwFrameQueueNum];
		frame_dsc_s &dsc = *pstFrameDsc;
		return q->put(dsc);
	}else{
		return SYS_ERROR;
	}
}

sbool frame_queue_is_empty(dword dwFrameQueueNum)
{
	if (dwFrameQueueNum < qp.pool_size ){
		ref_queue *q = qp.queues[dwFrameQueueNum];
		if (q->empty()){
			return SYS_TRUE;
		}else{
			return SYS_FAILS;
		}
	}else{
		return SYS_ERROR;
	}
}

sdword frame_queue_flush(dword dwFrameQueueNum)
{
	if (dwFrameQueueNum < qp.pool_size ){
		ref_queue *q = qp.queues[dwFrameQueueNum];
		return q->flush();
	}else{
		return SYS_ERROR;
	}
}

sdword frame_queue_counter(dword dwFrameQueueNum)
{
	if (dwFrameQueueNum < qp.pool_size ){
		ref_queue *q = qp.queues[dwFrameQueueNum];
		return q->size();
	}else{
		return SYS_ERROR;
	}
}

#ifdef __cplusplus
}
#endif
