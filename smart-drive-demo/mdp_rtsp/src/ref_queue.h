/*
 * framequeue.h
 *
 *  Created on: 2021Äê5ÔÂ25ÈÕ
 *      Author: huangyikao
 */

#ifndef REF_QUEUE_H_
#define REF_QUEUE_H_
#include <memory>
#include <queue>
#include <mutex>

#include "frame_queue.h"

typedef std::shared_ptr<frame_dsc_s> ref_node_t;
class ref_queue {
public:
	ref_queue(int depth);


	void pause_get(void);
	void resume_get(void);
	bool empty(void);
	int size(void);
	int put(const frame_dsc_s &dsc);
	int get(frame_dsc_s &dsc);
	int peek(frame_dsc_s &dsc);
	int flush(void);
private:
	unsigned int _depth;
	std::queue<ref_node_t> _queue;
	int _is_pause_get;
	std::mutex _mutex;
};


#endif /* REF_QUEUE_H_ */
