#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "queue_list.h"
#include "pnt_log.h"

#define QUEUE_LIST_LOG		PNT_LOGE

int32_t queue_list_create(queue_list_t* plist, int32_t max_count)
{
	if (plist->m_init)
	{
		QUEUE_LIST_LOG("%08x list is inited\n", (int32_t)plist);
		return 0;
	}

	plist->m_max_count = max_count?max_count:1;
	plist->m_head = NULL;
	plist->free = NULL;

	pthread_mutex_init(&plist->m_mutex, NULL);

	plist->m_init = 1;
	QUEUE_LIST_LOG("%08x list init success\n", (int32_t)plist);

	return 0;
}

int32_t queue_list_destroy(queue_list_t* plist)
{
	if (plist->m_head != NULL)
	{
		QUEUE_LIST_LOG("try to destroy a not empty list.");
	}
	
	pthread_mutex_destroy(&plist->m_mutex);

	memset(plist, 0, sizeof(queue_list_t));

	return 0;
}

int32_t queue_list_get_count(queue_list_t* plist)
{
	int32_t count = 0;

	if (!plist->m_init)
	{
		QUEUE_LIST_LOG("%08x list hadn't inited", (int32_t)plist);
		return 0;
	}

	pthread_mutex_lock(&plist->m_mutex);

	queue_t* head = plist->m_head;

	while(head != NULL)
	{
		head = head->next;
		count ++;
	}

	pthread_mutex_unlock(&plist->m_mutex);

	return count;
}

int32_t queue_list_push(queue_list_t* plist, void* content)
{
	int32_t ret = 0, count = 0;

	if (!plist->m_init)
	{
		QUEUE_LIST_LOG("%08x list hadn't inited", (int32_t)plist);
		return -1;
	}

	pthread_mutex_lock(&plist->m_mutex);

	queue_t* head = plist->m_head;
	queue_t* tail = NULL;

	while(NULL != head)
	{
		count ++;
		if (head->next == NULL)
		{
			tail = head;
			break;
		}
		head = head->next;
	}

	if (count >= plist->m_max_count) // 达到上限，移除第一个节点（先入列的）
	{
		queue_t* newhead = NULL;
		
		head = plist->m_head;
		newhead = head->next;
		newhead->prev = head->prev;
		plist->m_head = newhead; /* 更新next为head */
		
		if (head->m_content)
		{
			if (NULL == plist->free)
				free(head->m_content);
			else
				plist->free(head->m_content);
			head->m_content = NULL;
		}
		free(head);
		head = NULL;
		count -= 1;
	}

	queue_t * pnode = (queue_t*)malloc(sizeof(queue_t));
	if (NULL == pnode)
	{
		ret = -1;
		goto exit;
	}

	pnode->m_id = count;
	pnode->m_content = content;
	pnode->next = NULL;
	pnode->prev = NULL;

	if (0 == count)
	{
		plist->m_head = pnode;
		plist->m_head->prev = pnode;
	}
	else
	{
		plist->m_head->prev = pnode;
		pnode->prev = tail;
		tail->next = pnode;
	}

exit:

	pthread_mutex_unlock(&plist->m_mutex);

	return ret;
}

void* queue_list_popup(queue_list_t* plist)
{
	void* ret = NULL;

	if (!plist->m_init)
	{
		QUEUE_LIST_LOG("%08x list hadn't inited", (int32_t)plist);
		return ret;
	}

	pthread_mutex_lock(&plist->m_mutex);

	queue_t* head = plist->m_head;

	if (NULL == head)
	{
		goto exit;
	}

	queue_t* newhead = NULL;
	newhead = (queue_t*)head->next;

	ret = head->m_content;
	if (NULL != newhead)
		newhead->prev = head->prev;
	plist->m_head = newhead; /* 更新next为head */

	free(head);
	head = NULL;

exit:

	pthread_mutex_unlock(&plist->m_mutex);

	return ret;
}

void* queue_list_popup_last(queue_list_t* plist)
{
	void* ret = NULL;

	if (!plist->m_init)
	{
		QUEUE_LIST_LOG("%08x list hadn't inited", (int32_t)plist);
		return ret;
	}

	pthread_mutex_lock(&plist->m_mutex);

	queue_t* head = plist->m_head;
	queue_t* tail = NULL;

	/* 找到最后一个节点 */
	while (NULL != head)
	{
		if (head->next == NULL)
		{
			tail = head;
			break;
		}
		else
		{
			if (head->next->next == NULL)
			{
				tail = head->next;
				break;
			}
			else
			{
				head = head->next;
			}
		}
	}

	if (tail == NULL)
	{
		goto exit;
	}

	ret = tail->m_content;

	if (tail == head)
	{
		plist->m_head = NULL;
	}
	else
	{
		plist->m_head->prev = head;
		head->next = NULL;
	}

	/* 删除最后一个节点 */
	free(tail);
	tail = NULL;

exit:

	pthread_mutex_unlock(&plist->m_mutex);

	return ret;
}

void queue_list_clear(queue_list_t* plist)
{
	if (!plist->m_init)
	{
		QUEUE_LIST_LOG("%08x list hadn't inited", (int32_t)plist);
		return;
	}

	pthread_mutex_lock(&plist->m_mutex);

	queue_t* head = plist->m_head;
	queue_t* temp = NULL;

	while(head != NULL)
	{
		temp = head;
		head = head->next;

		if (NULL != temp->m_content)
		{
			if (NULL == plist->free)
				free(temp->m_content);
			else
				plist->free(temp->m_content);
		}
		free(temp);
	}

	plist->m_head = NULL;

	pthread_mutex_unlock(&plist->m_mutex);
}

void queue_list_set_free(queue_list_t* plist, void(*f)(void*))
{
	if (!plist->m_init)
	{
		QUEUE_LIST_LOG("%08x list hadn't inited", (int32_t)plist);
		return;
	}

	plist->free = f;
}

