/*
 * cspt_adapter.c
 *
 *  Created on: Oct 27, 2014
 *      Author: Yiming Xu
 */

#include <stddef.h>
#include "cspt_adapter.h"
#include "srv_task.h"

void* cspt_alloc(uint32 message_length)
{
	void *message_ptr;
	cspt_header_s *cspt_message_header_ptr;

	message_ptr = srv_msg_alloc(message_length + CSPT_HEADER_LEN);
	if(NULL == message_ptr)
	{
		return NULL;
	}

	cspt_message_header_ptr = (cspt_header_s *)message_ptr;
	cspt_message_header_ptr->type = CSPT_MSG_TYPE_INTERNAL;
	cspt_message_header_ptr->length = message_length;
	cspt_message_header_ptr->message_ptr = (int8 *)message_ptr + CSPT_HEADER_LEN;

	return message_ptr;
}

void cspt_free(void* message_ptr)
{
	if(NULL == message_ptr)
	{
		return;
	}
	srv_msg_free(message_ptr);
}

int32 cspt_snd(int32 source_task_id, int32 dest_task_id, int32 priority, void *message_ptr)
{
	if(NULL == message_ptr)
	{
		return SRV_ERR;
	}

	if(SRV_OK == srv_msg_snd(source_task_id,dest_task_id,priority,1,message_ptr))
	{
		return SRV_OK;
	}
	else
	{
		return SRV_ERR;
	}
}

int32 cspt_rcv(int32 task_id,void** message_ptr_ptr)
{
	int32 message_id;
	if(SRV_ERR == srv_msg_rcv(task_id,&message_id,message_ptr_ptr))//lMsgId no use
	{
		return SRV_ERR;
	}
	else
	{
		return SRV_OK;
	}
}

