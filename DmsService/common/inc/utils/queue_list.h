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
 * ���д�������ʼ��
 * param[in] plist ����ָ��
 * param[in] max_count ������󳤶�
*/
int32_t queue_list_create(queue_list_t* plist, int32_t max_count);

int32_t queue_list_destroy(queue_list_t* plist);

/**
 * ��ȡ����Ԫ������
 * param[in] plist ����ָ��
 * return int32_t ������
*/
int32_t queue_list_get_count(queue_list_t* plist);

/**
 * Ԫ������
 * param[in] plist ����ָ��
 * param[in] content ��Ҫ���������ָ��
 * return int32_t 0-�ɹ� !=0-ʧ��
*/
int32_t queue_list_push(queue_list_t* plist, void* content);

/**
 * Ԫ�س���(�����ȳ�)����ɾ�������д�Ԫ��
 * param[in] plist ����ָ��
 * return void* NULL-�� !=NULL-Ԫ������ָ��
*/
void* queue_list_popup(queue_list_t* plist);

/**
 * Ԫ�س���(������)����ɾ�������д�Ԫ��
 * param[in] plist ����ָ��
 * return void* NULL-�� !=NULL-Ԫ������ָ��
*/
void* queue_list_popup_last(queue_list_t* plist);

void queue_list_clear(queue_list_t* plist);

void queue_list_set_free(queue_list_t* plist, void(*f)(void*));

#ifdef __cplusplus
}
#endif

#endif

