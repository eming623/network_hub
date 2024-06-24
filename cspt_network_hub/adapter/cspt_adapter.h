/*
 * cspt_adapter.h
 *
 *  Created on: Oct 27, 2011
 *      Author: Yiming Xu
 */

#ifndef CSPT_ADAPTER_H_
#define CSPT_ADAPTER_H_

#include "srv_common.h"

typedef enum
{
	CSPT_MSG_TYPE_INTERNAL,
	CSPT_MSG_TYPE_UDP
}cspt_message_type_e;

typedef struct
{
	uint32 ip_addr;
	uint16 port;
	uint16 reserved;
	int32  socket;
}udp_header_s;

typedef struct CSPT_MSG_HDR_
{
	uint16 type;
	uint16 length;
	union
	{
		udp_header_s udp_header;
	}header;
	void *message_ptr;
}cspt_header_s;

#define CSPT_HEADER_LEN  sizeof(cspt_header_s)

void* cspt_alloc(uint32 ulMsgLen);

void cspt_free(void* pMsg);

int32 cspt_snd(int32 lStid, int32 lDtid, int32 lMsgPrio, void *pMsg);

int32 cspt_rcv(int32 lTid,void** ppMsg);


#endif /* CSPT_ADAPTER_H_ */
