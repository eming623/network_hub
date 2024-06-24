/*
 * srv_task.c
 *
 *  Created on: Aug 17, 2014
 *      Author: Yiming Xu
 */

#include <stdio.h>
#include "srv_task.h"
#include "srv_buffer.h"

#define MAX_MSQ_SIZE 500

typedef struct TASK_BLOCK_
{
	int32 tid;
	char *name;
	void* (*fpEntry)(void*);
	void *arg;
	int32 prio;
	int32 isMsqEnable;

	service_handle task_handle;
	service_handle queue_handle;
} task_block_s;

typedef struct MSG_HDR_
{
	int32 stid;
	int32 dtid;
	int32 bodysize;
	int32 reserve;
} message_header_s;

extern service_handle srv_task_create(srv_task_s *task_ptr);
extern service_handle srv_msq_create(int32 queuesize);
extern int32 srv_msq_destroy(service_handle queue_handle);
extern int32 srv_msq_snd(service_handle queue_handle, int32 prio, int32 msgid,
		void *message_ptr, int32 message_size);
extern int32 srv_msq_rcv(service_handle queue_handle, int32 *message_id_ptr,
		void **message_ptr_ptr);
extern int32 srv_msq_num(service_handle queue_handle);

static task_block_s *g_task_table_ptr = NULL;
static int32 g_max_task_table_size = 0;

int32 srv_task_init(int32 maxtablesize)
{
	int32 i;

	// see if the task table has already exist. if so, exit.
	if (g_task_table_ptr != NULL)
		return SRV_ERR;

	g_task_table_ptr = (task_block_s*) srv_buf_alloc(
			sizeof(task_block_s) * maxtablesize);
	if (g_task_table_ptr == NULL)
		return SRV_ERR;

	// initialize the task table
	for (i = 0; i < maxtablesize; i++)
	{
		g_task_table_ptr[i].tid = SRV_INVALID;
		g_task_table_ptr[i].task_handle = SRV_INVALID;
		g_task_table_ptr[i].queue_handle = SRV_INVALID;
	}

	g_max_task_table_size = maxtablesize;

	return SRV_OK;
}

int32 srv_task_register_table(const srv_task_s *task_table_ptr,
		int32 table_size)
{
	int32 i;

	if (task_table_ptr == NULL || table_size <= 0)
		return SRV_ERR;

	for (i = 0; i < table_size; i++)
	{
		if (srv_task_register(&task_table_ptr[i]) != SRV_OK)
			return SRV_ERR;
	}

	return SRV_OK;
}

int32 srv_task_start_table()
{
	int32 tid;

	if (g_task_table_ptr == NULL)
		return SRV_ERR;

	for (tid = 0; tid < g_max_task_table_size; tid++)
		srv_task_start(tid);

	return SRV_OK;
}

int32 srv_task_register(const srv_task_s *task_ptr)
{
	if (task_ptr == NULL
			|| task_ptr->tid
					>= g_max_task_table_size|| task_ptr->fpEntry == NULL)
		return SRV_ERR;

	// see if the thread index has been used. if so, exit.
	if (g_task_table_ptr[task_ptr->tid].tid != SRV_INVALID)
		return SRV_ERR;

	g_task_table_ptr[task_ptr->tid].tid = task_ptr->tid;
	g_task_table_ptr[task_ptr->tid].name = task_ptr->name;
	g_task_table_ptr[task_ptr->tid].fpEntry = task_ptr->fpEntry;
	g_task_table_ptr[task_ptr->tid].arg = task_ptr->arg;
	g_task_table_ptr[task_ptr->tid].prio = task_ptr->prio;
	g_task_table_ptr[task_ptr->tid].isMsqEnable = task_ptr->queue_enable;

	// see if user need a message queue. if so, create it.
	if (task_ptr->queue_enable)
	{
		g_task_table_ptr[task_ptr->tid].queue_handle = srv_msq_create(
				MAX_MSQ_SIZE);
		if (g_task_table_ptr[task_ptr->tid].queue_handle == SRV_INVALID)
		{
			g_task_table_ptr[task_ptr->tid].tid = SRV_INVALID;
			return SRV_ERR;
		}
	}

	return SRV_OK;
}

int32 srv_task_start(int32 tid)
{
	if (tid >= g_max_task_table_size || g_task_table_ptr[tid].tid == SRV_INVALID)
		return SRV_ERR;

	// see if the task has already started. if so, exit.
	if (g_task_table_ptr[tid].task_handle != SRV_INVALID)
		return SRV_ERR;

	g_task_table_ptr[tid].task_handle = srv_task_create(
			(srv_task_s*) &g_task_table_ptr[tid]);
	if (g_task_table_ptr[tid].task_handle == SRV_INVALID)
		return SRV_ERR;

	return SRV_OK;
}

void* srv_msg_alloc(int32 size)
{
	uint8 *pMsg;

	pMsg = (uint8*) srv_buf_alloc(sizeof(message_header_s) + size);
	if (pMsg == NULL)
		return NULL;

	((message_header_s*) pMsg)->bodysize = size;

	return (pMsg + sizeof(message_header_s));
}

void srv_msg_free(void *message_ptr)
{
	if (message_ptr == NULL)
		return;
	srv_buf_free(((uint8*) message_ptr - sizeof(message_header_s)));
}

int32 srv_msg_snd(int32 stid, int32 dtid, int32 prio, int32 msgid,
		void *message_ptr)
{
	message_header_s *pHdr;

	if (message_ptr == NULL || dtid >= g_max_task_table_size)
		return SRV_ERR;

	pHdr = (message_header_s*) (message_ptr - sizeof(message_header_s));

	pHdr->stid = stid;
	pHdr->dtid = dtid;

	return srv_msq_snd(g_task_table_ptr[dtid].queue_handle, prio, msgid, pHdr,
			(pHdr->bodysize + sizeof(message_header_s)));
}

int32 srv_msg_rcv(int32 tid, int32 *message_id_ptr, void **message_ptr_ptr)
{
	void *header_ptr;

	if (tid >= g_max_task_table_size
			|| message_id_ptr == NULL
			|| message_ptr_ptr == NULL)
		return SRV_ERR;

	if (srv_msq_rcv(g_task_table_ptr[tid].queue_handle, message_id_ptr,
			&header_ptr) <= 0)
		return SRV_ERR;

	*message_ptr_ptr = (uint8*) header_ptr + sizeof(message_header_s);

	return ((message_header_s*) header_ptr)->bodysize;
}

int32 srv_msg_getsrctid(void *message_ptr)
{
	if (NULL == message_ptr)
	{
		return SRV_INVALID;
	}
	return ((message_header_s*)
			((uint8*) message_ptr - sizeof(message_header_s)))->stid;
}

int32 srv_get_msg_num(int32 tid)
{
	if (tid >= g_max_task_table_size)
	{
		return SRV_ERR;
	}
	return srv_msq_num(g_task_table_ptr[tid].queue_handle);
}

