/*
 * cspt_tcpsender.c
 *
 *  Created on: Oct 29, 2014
 *      Author: Yiming Xu
 */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "cspt_tcpsender.h"
#include "cspt_tcpserver.h"
#include "cspt_config.h"

void * tcpsender_entry(void* arg)
{
    printf("%s-%u:Enter tcpsender_entry\r\n", __FILE__, __LINE__);
    int32 message_id;
    void *message_ptr;
    int32 message_size;
    int32 sent_size;
    tcp_client_message_header_s *message_header_ptr;
    while (1)
    {
        message_size = srv_msg_rcv(TID_TCPSENDER, &message_id, &message_ptr);
        message_header_ptr = (tcp_client_message_header_s *) message_ptr;
        message_header_ptr->message_length = htonl(
                message_header_ptr->message_length);
        message_header_ptr->message_type = htonl(
                message_header_ptr->message_type);
        sent_size = srv_skt_send(tcp_socket_node_info.tcp_socket, message_ptr,
                message_size);
        printf("--=== sended size = %d ===--\n", sent_size);
    }
}
