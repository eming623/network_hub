/*
 * lnx_msq.c
 *
 *  Created on: Aug 17, 2014
 *      Author: Yiming Xu
 *
   A message queue implementation for upper layer applications.
 */

#include <pthread.h>
#include <stdlib.h>
#include "srv_common.h"

typedef struct MSG_BLOCK_
{
	int32 id;
	int32 size;
	void *body;
} MSG_BLOCK;

typedef struct MSG_QUEUE_
{
	int32 size;
	int32 front;
	int32 rear;
	MSG_BLOCK *pMsg;
	pthread_mutex_t lock;
	pthread_cond_t cond;
} MSG_QUEUE;

service_handle srv_msq_create(int32 queuesize)
{
	MSG_QUEUE *pstQueue;

	pstQueue = (MSG_QUEUE*) malloc(sizeof(MSG_QUEUE));
	if (pstQueue == NULL)
		return SRV_INVALID;

	if (pthread_mutex_init(&(pstQueue->lock), NULL) != 0)
	{
		free(pstQueue);
		return SRV_INVALID;
	}

	if (pthread_cond_init(&(pstQueue->cond), NULL) != 0)
	{
		pthread_mutex_destroy(&(pstQueue->lock));
		free(pstQueue);
		return SRV_INVALID;
	}

	pstQueue->size = queuesize + 1;

	pstQueue->pMsg = (MSG_BLOCK*) malloc(sizeof(MSG_BLOCK) * pstQueue->size);
	if (pstQueue->pMsg == NULL)
	{
		pthread_mutex_destroy(&(pstQueue->lock));
		pthread_cond_destroy(&(pstQueue->cond));
		free(pstQueue);
		return SRV_INVALID;
	}

	pstQueue->front = pstQueue->rear = 0;

	return (service_handle) pstQueue;
}

int32 srv_msq_destroy(service_handle hMsq)
{
	MSG_QUEUE *pstQueue = (MSG_QUEUE*) hMsq;

	if (hMsq == SRV_INVALID)
		return SRV_ERR;

	pthread_mutex_destroy(&(pstQueue->lock));
	pthread_cond_destroy(&(pstQueue->cond));

	free(pstQueue->pMsg);
	free(pstQueue);

	return SRV_OK;
}

int32 srv_msq_snd(service_handle hMsq, int32 prio, int32 msgid, void *pMsg,
		int32 msgsize)
{
	MSG_QUEUE *pstQueue = (MSG_QUEUE*) hMsq;
	int32 index;

	if (hMsq == SRV_INVALID || pMsg == NULL)
		return SRV_ERR;

	pthread_mutex_lock(&(pstQueue->lock));

	index = (pstQueue->rear + 1) % pstQueue->size;

	// see if queue is full. if so, exit.
	if (index == pstQueue->front)
	{
		pthread_mutex_unlock(&(pstQueue->lock));
		return SRV_ERR;
	}

	pstQueue->pMsg[pstQueue->rear].id = msgid;
	pstQueue->pMsg[pstQueue->rear].body = pMsg;
	pstQueue->pMsg[pstQueue->rear].size = msgsize;

	pstQueue->rear = index;

	pthread_cond_signal(&(pstQueue->cond));

	pthread_mutex_unlock(&(pstQueue->lock));

	return SRV_OK;
}

int32 srv_msq_rcv(service_handle hMsq, int32 *pMsgid, void **ppMsg)
{
	MSG_QUEUE *pstQueue = (MSG_QUEUE*) hMsq;
	int32 msgsize;

	if (hMsq == SRV_INVALID || pMsgid == NULL || ppMsg == NULL)
		return SRV_ERR;

	pthread_mutex_lock(&(pstQueue->lock));

	// see if queue is empty. if so, exit.
	while (pstQueue->front == pstQueue->rear)
		pthread_cond_wait(&(pstQueue->cond), &(pstQueue->lock));

	pthread_mutex_unlock(&(pstQueue->lock));

	pthread_mutex_lock(&(pstQueue->lock));

	*ppMsg = pstQueue->pMsg[pstQueue->front].body;
	*pMsgid = pstQueue->pMsg[pstQueue->front].id;
	msgsize = pstQueue->pMsg[pstQueue->front].size;

	pstQueue->front = (pstQueue->front + 1) % pstQueue->size;

	pthread_mutex_unlock(&(pstQueue->lock));

	return msgsize;
}

//query message number in message queue
int32 srv_msq_num(service_handle queue_handle)
{
	int32 num;
	MSG_QUEUE *queue_ptr = (MSG_QUEUE*) queue_handle;
	if (queue_handle == SRV_INVALID)
	{
		return SRV_ERR;
	}
	pthread_mutex_lock(&(queue_ptr->lock));
	num = (queue_ptr->rear + queue_ptr->size + 1 - queue_ptr->front)
			% (queue_ptr->size + 1);
	pthread_mutex_unlock(&(queue_ptr->lock));
	return num;
}

