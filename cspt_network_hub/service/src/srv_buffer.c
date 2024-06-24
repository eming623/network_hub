/*
 * srv_buffer.c
 *
 *  Created on: Aug 17, 2014
 *      Author: Yiming Xu
 *
   A implementation of manage management for upper layer application.
   This module allocate a memory pool in initialization in one time. All memory
   allocate/free by this API is using memory from the memory pool.
 */

#include <stdlib.h>
#include "srv_buffer.h"
#include "srv_mutex.h"
#include "srv_list.h"
#include "srv_bitmap.h"

typedef struct
{
	srv_list_node_s link;
	int32 pool_id;
} buffer_head_s;

typedef struct
{
	srv_list_node_s root;
	int32 block_size;
	int32 block_num;
	uint8 *buffer_ptr;
	service_handle mutex;
	service_handle bitmap_id;
} buffer_pool_s;

static buffer_pool_s *g_buffer_pool_ptr;
static int32 g_buffer_pool_size;

int32 srv_buf_init(const srv_buffer_pool_s *buffer_pool_ptr, int32 pool_size)
{
	int32 index1;
	int32 index2;
	int32 realsize;
	uint8 *tmp;
	buffer_head_s *head;

	if (buffer_pool_ptr == NULL || pool_size <= 0)
		return SRV_ERR;

	g_buffer_pool_ptr = (buffer_pool_s*) malloc(
			sizeof(buffer_pool_s) * pool_size);
	if (g_buffer_pool_ptr == NULL)
		return SRV_ERR;

	for (index1 = 0; index1 < pool_size; index1++)
	{
		// attach a head for each block
		realsize = buffer_pool_ptr[index1].blocksize + sizeof(buffer_head_s);

		g_buffer_pool_ptr[index1].buffer_ptr = (uint8*) malloc(
				buffer_pool_ptr[index1].blocknr * realsize);
		if (g_buffer_pool_ptr[index1].buffer_ptr == NULL)
			return SRV_ERR;

		// link all blocks
		SRV_LIST_INIT_ROOT(&(g_buffer_pool_ptr[index1].root));
		for (index2 = 0; index2 < buffer_pool_ptr[index1].blocknr; index2++)
		{
			tmp = g_buffer_pool_ptr[index1].buffer_ptr + index2 * realsize;

			head = (buffer_head_s*) tmp;
			head->pool_id = index1;

			SRV_LIST_INIT_NODE(&(head->link), tmp);
			SRV_LIST_INSERT_BEFORE(&(g_buffer_pool_ptr[index1].root),
					&(head->link));
		}

		g_buffer_pool_ptr[index1].block_size =
				buffer_pool_ptr[index1].blocksize;
		g_buffer_pool_ptr[index1].block_num = buffer_pool_ptr[index1].blocknr;

		//Init the bitmap
		g_buffer_pool_ptr[index1].bitmap_id = srv_bmp_create(
				g_buffer_pool_ptr[index1].block_num);
		if (g_buffer_pool_ptr[index1].bitmap_id == SRV_INVALID)
		{
			//need to free the buffer by malloc
			return SRV_ERR;
		}

		// create mutex
		g_buffer_pool_ptr[index1].mutex = srv_mutex_create();
		if (g_buffer_pool_ptr[index1].mutex == SRV_INVALID)
			return SRV_ERR;
	}

	g_buffer_pool_size = pool_size;

	return SRV_OK;
}

void* srv_buf_alloc(int32 size)
{
	int32 index;
	srv_list_node_s *link;
	uint8 *buf;

	// TODO: use binary search to improve efficiency
	for (index = 0; index < g_buffer_pool_size; index++)
	{
		srv_mutex_lock(g_buffer_pool_ptr[index].mutex);
		if (size
				<= g_buffer_pool_ptr[index].block_size
				&& !SRV_LIST_IS_EMPTY(&(g_buffer_pool_ptr[index].root)))
		{
			link = SRV_LIST_GET_NEXT(&(g_buffer_pool_ptr[index].root));
			buf = SRV_LIST_GET_ENTRY(link);

			SRV_LIST_REMOVE(link);

			srv_bmp_set_bit(g_buffer_pool_ptr[index].bitmap_id,
					(buf - g_buffer_pool_ptr[index].buffer_ptr)
							/ (sizeof(buffer_head_s)
									+ g_buffer_pool_ptr[index].block_size), 1);

			srv_mutex_unlock(g_buffer_pool_ptr[index].mutex);

			return (void*) (buf + sizeof(buffer_head_s));
		}

		srv_mutex_unlock(g_buffer_pool_ptr[index].mutex);
	}

	return NULL;
}

void srv_buf_free(void *buf)
{
	buffer_head_s *buffer_head_ptr;
	int32 poolid;

	if (buf == NULL)
		return;

	buffer_head_ptr = (buffer_head_s*) (buf - sizeof(buffer_head_s));
	poolid = buffer_head_ptr->pool_id;

	if (poolid >= g_buffer_pool_size)
		return;

	if (0
			== srv_bmp_get_bit(g_buffer_pool_ptr[poolid].bitmap_id,
					((uint8*) buffer_head_ptr
							- g_buffer_pool_ptr[poolid].buffer_ptr)
							/ (sizeof(buffer_head_s)
									+ g_buffer_pool_ptr[poolid].block_size)))
	{
		printf("Buf has been freed.\r\n");
		return;
	}

	srv_mutex_lock(g_buffer_pool_ptr[poolid].mutex);
	SRV_LIST_INSERT_BEFORE(&(g_buffer_pool_ptr[poolid].root),
			&(buffer_head_ptr->link));

	srv_bmp_set_bit(g_buffer_pool_ptr[poolid].bitmap_id,
			((uint8*) buffer_head_ptr - g_buffer_pool_ptr[poolid].buffer_ptr)
					/ (sizeof(buffer_head_s)
							+ g_buffer_pool_ptr[poolid].block_size), 0);
	srv_mutex_unlock(g_buffer_pool_ptr[poolid].mutex);
}

