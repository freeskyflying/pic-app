#ifndef _QUEUE_LIST_H_
#define _QUEUE_LIST_H_

#include "typedef.h"
#include "pthread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _queue_
{

	int32_t m_id;
	void* m_content;
	struct _queue_ *prev;
	struct _queue_ *next;

} queue_t;

typedef struct 
{

	int32_t m_init;
	int32_t m_max_count;
	queue_t *m_head;
	pthread_mutex_t m_mutex;
	void(*free)(void* content);

} queue_list_t;

/**
 * 队列创建及初始化
 * param[in] plist 队列指针
 * param[in] max_count 链表最大长度
*/
int32_t queue_list_create(queue_list_t* plist, int32_t max_count);

int32_t queue_list_destroy(queue_list_t* plist);

/**
 * 获取队列元素数量
 * param[in] plist 链表指针
 * return int32_t 链表长度
*/
int32_t queue_list_get_count(queue_list_t* plist);

/**
 * 元素入列
 * param[in] plist 队列指针
 * param[in] content 需要插入的内容指针
 * return int32_t 0-成功 !=0-失败
*/
int32_t queue_list_push(queue_list_t* plist, void* content);

/**
 * 元素出列(先入先出)，并删除队列中此元素
 * param[in] plist 队列指针
 * return void* NULL-空 !=NULL-元素内容指针
*/
void* queue_list_popup(queue_list_t* plist);

/**
 * 元素出列(先入后出)，并删除队列中此元素
 * param[in] plist 队列指针
 * return void* NULL-空 !=NULL-元素内容指针
*/
void* queue_list_popup_last(queue_list_t* plist);

void queue_list_clear(queue_list_t* plist);

void queue_list_set_free(queue_list_t* plist, void(*f)(void*));

#ifdef __cplusplus
}
#endif

#endif

