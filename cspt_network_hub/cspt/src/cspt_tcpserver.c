/*
 * cspt_tcpserver.c
 *
 *  Created on: Sep 15, 2014
 *      Author: Yiming Xu
 */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "cspt_tcpserver.h"
#include "cspt_config.h"

const srv_socket_addr_s g_server_addr =
{ "127.0.0.1", 23456 };

tcp_socket_node_info_s tcp_socket_node_info =
{ SRV_INVALID, 0, NULL, NULL };

void* tcpserver_entry(void* arg)
{

    printf("%s-%u:Enter tcpserver_entry\r\n", __FILE__, __LINE__);
    service_handle server_socket;
    service_handle client_socket;
    uint32 sock_opt;
    srv_socket_addr_s stClientAddr;
    while (1)
    {
        server_socket = srv_skt_socket(SRV_STREAM);
        if (SRV_INVALID == server_socket)
        {
            continue;
        }
        if (SRV_ERR
                == srv_skt_opt(server_socket, SOL_SOCKET, SO_REUSEADDR,
                        (const void*) &sock_opt, sizeof(sock_opt)))
        {
            srv_skt_close(server_socket);
            continue;
        }
        if (SRV_ERR == srv_skt_bind(server_socket, &g_server_addr))
        {
            srv_skt_close(server_socket);
            continue;
        }
        if (SRV_ERR == srv_skt_listen(server_socket, MAX_LISTEN))
        {
            srv_skt_close(server_socket);
            printf("4");
            continue;
        }

        while (1)
        {

            printf("%s-%u:Waiting TCP client\r\n", __FILE__, __LINE__);
            client_socket = srv_skt_accept(server_socket, &stClientAddr);
            if (SRV_INVALID == client_socket)
            {
                continue;
            }

            printf("%s-%u:There is a TCP client connected\r\n", __FILE__,
                    __LINE__);

            tcp_socket_node_info.tcp_socket = client_socket;

            while (1)
            {
                //Receive data
                if (SRV_ERR == cspt_recvdata())
                {
                    break;
                }
            }
        }
    }

    return NULL;
}

int32 cspt_recvdata()
{
    fd_set fd_set_data;
    uint32 head_length;
    uint32 body_length;
    tcp_client_message_header_s *message_header_ptr;
    int8 *message_ptr;

    while (1)
    {
        printf("--=== Running ===-- \n");
        FD_ZERO(&fd_set_data);
        FD_SET((int32)tcp_socket_node_info.tcp_socket, &fd_set_data);

        if (1 != select(FD_SETSIZE, &fd_set_data, NULL, NULL, NULL))
        {
            printf("--=== exit ===-- \n");
            break;
        }
        printf("--=== ulRecvLen = %d ===-- \n",
                tcp_socket_node_info.receive_length);
        if (tcp_socket_node_info.receive_length < CLIENT_MSG_HDR_LEN)
        {
            if (tcp_socket_node_info.receive_length == 0)
            {
                tcp_socket_node_info.head_ptr = srv_buf_alloc(
                        sizeof(CLIENT_MSG_HDR_LEN));
                if (NULL == tcp_socket_node_info.head_ptr)
                {
                    printf("%s-%u:Buffer allocate failed\r\n", __FILE__,
                            __LINE__);
                    return SRV_ERR;
                }
            }

            head_length = srv_skt_recv(tcp_socket_node_info.tcp_socket,
                    tcp_socket_node_info.head_ptr
                            + tcp_socket_node_info.receive_length,
                    CLIENT_MSG_HDR_LEN - tcp_socket_node_info.receive_length);

            if (0 == head_length)
            {

                printf("%s-%u:Client socket closed\r\n", __FILE__, __LINE__);
                tcp_socket_node_info.receive_length = 0;
                srv_buf_free(tcp_socket_node_info.head_ptr);
                tcp_socket_node_info.head_ptr = NULL;
                return SRV_ERR;
            }

            if (-1 == head_length)
            {

                printf("%s-%u:Net problem\r\n", __FILE__, __LINE__);
                tcp_socket_node_info.receive_length = 0;
                srv_buf_free(tcp_socket_node_info.head_ptr);
                tcp_socket_node_info.head_ptr = NULL;
                return SRV_ERR;
            }

            tcp_socket_node_info.receive_length += head_length;
            if (tcp_socket_node_info.receive_length > CLIENT_MSG_HDR_LEN)
            {
                printf("%s-%u:Received head length exceed\r\n", __FILE__,
                        __LINE__);
                tcp_socket_node_info.receive_length = 0;
                srv_buf_free(tcp_socket_node_info.head_ptr);
                tcp_socket_node_info.head_ptr = NULL;
                return SRV_ERR;
            }

            if (tcp_socket_node_info.receive_length < CLIENT_MSG_HDR_LEN)
            {
                continue;
            }

            //Receive head OK
            if (1 != select(FD_SETSIZE, &fd_set_data, NULL, NULL, NULL))
            {
                break;
            }
        }

        if (tcp_socket_node_info.receive_length == CLIENT_MSG_HDR_LEN)
        {
            message_header_ptr =
                    (tcp_client_message_header_s *) tcp_socket_node_info.head_ptr;
            message_header_ptr->message_length = ntohl(
                    message_header_ptr->message_length);
            message_header_ptr->message_type = ntohl(
                    message_header_ptr->message_type);
            tcp_socket_node_info.message_ptr = srv_buf_alloc(
                    message_header_ptr->message_length + CLIENT_MSG_HDR_LEN);
            if (NULL == tcp_socket_node_info.message_ptr)
            {
                printf("%s-%u:Buffer allocate failed\r\n", __FILE__, __LINE__);
                tcp_socket_node_info.receive_length = 0;
                srv_buf_free(tcp_socket_node_info.head_ptr);
                tcp_socket_node_info.head_ptr = NULL;
                return SRV_ERR;
            }
            memcpy(tcp_socket_node_info.message_ptr,
                    tcp_socket_node_info.head_ptr, CLIENT_MSG_HDR_LEN);
        }
        body_length = srv_skt_recv(tcp_socket_node_info.tcp_socket,
                tcp_socket_node_info.message_ptr
                        + tcp_socket_node_info.receive_length,
                message_header_ptr->message_length + CLIENT_MSG_HDR_LEN
                        - tcp_socket_node_info.receive_length);
        if (0 == body_length)
        {
            printf("%s-%u:Client socket closed\r\n", __FILE__, __LINE__);
            tcp_socket_node_info.receive_length = 0;
            srv_buf_free(tcp_socket_node_info.head_ptr);
            srv_buf_free(tcp_socket_node_info.message_ptr);
            tcp_socket_node_info.head_ptr = NULL;
            tcp_socket_node_info.message_ptr = NULL;
            return SRV_ERR;
        }

        if (-1 == body_length)
        {
            printf("%s-%u:Net problem\r\n", __FILE__, __LINE__);
            tcp_socket_node_info.receive_length = 0;
            srv_buf_free(tcp_socket_node_info.head_ptr);
            srv_buf_free(tcp_socket_node_info.message_ptr);
            tcp_socket_node_info.head_ptr = NULL;
            tcp_socket_node_info.message_ptr = NULL;
            return SRV_ERR;
        }

        tcp_socket_node_info.receive_length += body_length;

        if (tcp_socket_node_info.receive_length
                > message_header_ptr->message_length + CLIENT_MSG_HDR_LEN)
        {
            printf("%s-%u:Received head length exceed\r\n", __FILE__, __LINE__);
            tcp_socket_node_info.receive_length = 0;
            srv_buf_free(tcp_socket_node_info.head_ptr);
            srv_buf_free(tcp_socket_node_info.message_ptr);
            tcp_socket_node_info.head_ptr = NULL;
            tcp_socket_node_info.message_ptr = NULL;
            return SRV_ERR;
        }

        if (tcp_socket_node_info.receive_length
                < message_header_ptr->message_length + CLIENT_MSG_HDR_LEN)
        {
            continue;
        }

        //Receive a complete message from client, then send the message to kernel
        message_ptr = srv_msg_alloc(
                message_header_ptr->message_length + CLIENT_MSG_HDR_LEN);

        memcpy(message_ptr, tcp_socket_node_info.message_ptr,
                message_header_ptr->message_length + CLIENT_MSG_HDR_LEN);
        while (NULL == message_ptr)
        {
            printf("%s-%u:Message allocate failed\r\n", __FILE__, __LINE__);
            message_ptr = srv_msg_alloc(
                    message_header_ptr->message_length + CLIENT_MSG_HDR_LEN);

        }
        if (SRV_OK
                != srv_msg_snd(TID_TCPSERVER, TID_KERNEL, SRV_MSG_NORMAL_PRIO,
                        1, message_ptr))
        {
            srv_msg_free(message_ptr);
        }
        tcp_socket_node_info.receive_length = 0;
        srv_buf_free(tcp_socket_node_info.head_ptr);
        srv_buf_free(tcp_socket_node_info.message_ptr);
        tcp_socket_node_info.head_ptr = NULL;
        tcp_socket_node_info.message_ptr = NULL;

    }

    return SRV_OK;
}

