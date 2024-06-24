/*
 * srv_socket.h
 *
 *  Created on: Aug 17, 2014
 *      Author: Yiming Xu
 *
   A service API wrapper for Linux socket programming functions.
   Support TCP/UDP protocols in AF_INET family.
 */

#ifndef SRV_SOCKET_H_
#define SRV_SOCKET_H_

#include "srv_common.h"

#define SRV_DGRAM 0
#define SRV_STREAM 1

#define MAX_IP_ADDR 16

typedef struct
{
	char ip[MAX_IP_ADDR];
	int32 port;
} srv_socket_addr_s;

int32 srv_skt_init();

service_handle srv_skt_socket(int32 socktype);

int32 srv_skt_close(service_handle hSock);

int32 srv_skt_opt(service_handle hSock, int32 level, int32 option_name,
		const void *option_value, uint32 option_len);

int32 srv_skt_bind(service_handle hSock, const srv_socket_addr_s *pstAddr);

int32 srv_skt_listen(service_handle hSock, int32 backlog);

service_handle srv_skt_accept(service_handle hSock, srv_socket_addr_s *pstAddr);

int32 srv_skt_connect(service_handle hSock, const srv_socket_addr_s *pstAddr);

int32 srv_skt_send(service_handle hSock, const char *buf, int32 len);

int32 srv_skt_recv(service_handle hSock, char *buf, int32 len);

int32 srv_skt_sendto(service_handle hSock, const char *buf, int32 len,
		const srv_socket_addr_s *pstAddr);

int32 srv_skt_recvfrom(service_handle hSock, char *buf, int32 len,
		srv_socket_addr_s *pstAddr);

#endif /* SRV_SOCKET_H_ */
