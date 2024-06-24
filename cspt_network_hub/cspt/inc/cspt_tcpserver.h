/*
 * cspt_tcpserver.h
 *
 *  Created on: Sep 15, 2014
 *      Author: Yiming Xu
 */

#ifndef CSPT_TCPSERVER_H_
#define CSPT_TCPSERVER_H_

#include "srv_common.h"

#define MAX_LISTEN  5

typedef struct
{
	service_handle tcp_socket;
	uint32 receive_length;
	int8 *head_ptr;
	int8 *message_ptr;
}tcp_socket_node_info_s;

typedef struct
{
	uint32 message_type;
	uint32 message_length;
}tcp_client_message_header_s;

#define CLIENT_MSG_HDR_LEN sizeof(tcp_client_message_header_s)

extern tcp_socket_node_info_s tcp_socket_node_info;

void* tcpserver_entry(void* arg);

int32 cspt_recvdata();

#endif /* CSPT_TCPSERVER_H_ */
