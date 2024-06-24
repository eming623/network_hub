/*
 * lnx_socket.c
 *
 *  Created on: Aug 17, 2014
 *      Author: Yiming Xu
 *
   A service API wrapper for Linux socket programming functions.
   Support TCP/UDP protocols in AF_INET family.
 */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "srv_socket.h"

int32 srv_skt_init()
{
	return SRV_OK;
}

service_handle srv_skt_socket(int32 socktype)
{
	int32 sock = -1;

	switch (socktype)
	{
	case SRV_DGRAM:
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		break;
	case SRV_STREAM:
		sock = socket(AF_INET, SOCK_STREAM, 0);
		break;
	default:
		break;
	}

	if (sock == -1)
		return SRV_INVALID;

	return (service_handle) sock;
}

int32 srv_skt_close(service_handle socket_handle)
{
	if (socket_handle == SRV_INVALID)
	{
		return SRV_ERR;
	}
	if (-1 == close(socket_handle))
	{
		return SRV_ERR;
	}
	return SRV_OK;
}

int32 srv_skt_opt(service_handle socket_handle, int32 level, int32 option_name,
		const void *option_value, uint32 option_len)
{
	if (socket_handle == SRV_INVALID)
	{
		return SRV_ERR;
	}
	if (-1
			== setsockopt((int32) socket_handle, level, option_name,
					option_value, (socklen_t) option_len))
	{
		return SRV_ERR;
	}
	return SRV_OK;
}

int32 srv_skt_bind(service_handle socket_handle,
		const srv_socket_addr_s *addr_ptr)
{
	struct sockaddr_in sin;

	if (socket_handle == SRV_INVALID || addr_ptr == NULL)
		return SRV_ERR;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(addr_ptr->ip);
	sin.sin_port = htons(addr_ptr->port);

	if (bind((int32) socket_handle, (struct sockaddr*) &sin, sizeof(sin)) == -1)
		return SRV_ERR;

	return SRV_OK;
}

int32 srv_skt_listen(service_handle socket_handle, int32 backlog)
{
	if (socket_handle == SRV_INVALID)
		return SRV_ERR;

	if (listen((int32) socket_handle, backlog) == -1)
		return SRV_ERR;

	return SRV_OK;
}

service_handle srv_skt_accept(service_handle socket_handle,
		srv_socket_addr_s *addr_ptr)
{
	int32 client_socket;
	struct sockaddr_in sin;
	socklen_t sinlen = sizeof(sin);

	if (socket_handle == SRV_INVALID || addr_ptr == NULL)
		return SRV_INVALID;

	client_socket = accept((int32) socket_handle, (struct sockaddr*) &sin,
			&sinlen);
	if (client_socket == -1)
		return SRV_INVALID;

	strcpy(addr_ptr->ip, inet_ntoa(sin.sin_addr));
	addr_ptr->port = ntohs(sin.sin_port);

	return (service_handle) client_socket;
}

int32 srv_skt_connect(service_handle socket_handle,
		const srv_socket_addr_s *addr_ptr)
{
	struct sockaddr_in sin;

	if (socket_handle == SRV_INVALID || addr_ptr == NULL)
		return SRV_ERR;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(addr_ptr->ip);
	sin.sin_port = htons(addr_ptr->port);

	if (connect((int32) socket_handle, (struct sockaddr*) &sin, sizeof(sin))
			== -1)
		return SRV_ERR;

	return SRV_OK;
}

int32 srv_skt_send(service_handle socket_handle, const char *buf_ptr, int32 len)
{
	if (socket_handle == SRV_INVALID || buf_ptr == NULL)
		return SRV_ERR;

	return send((int32) socket_handle, buf_ptr, len, 0);
}

int32 srv_skt_recv(service_handle socket_handle, char *buf, int32 len)
{
	if (socket_handle == SRV_INVALID || buf == NULL)
		return SRV_ERR;

	return recv((int32) socket_handle, buf, len, 0);
}

int32 srv_skt_sendto(service_handle socket_handle, const char *buf_ptr,
		int32 len, const srv_socket_addr_s *addr_ptr)
{
	struct sockaddr_in sin;

	if (socket_handle == SRV_INVALID || buf_ptr == NULL || addr_ptr == NULL)
		return SRV_ERR;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(addr_ptr->ip);
	sin.sin_port = htons(addr_ptr->port);

	return sendto((int32) socket_handle, buf_ptr, len, 0,
			(struct sockaddr*) &sin, sizeof(sin));
}

int32 srv_skt_recvfrom(service_handle socket_handle, char *buf_ptr, int32 len,
		srv_socket_addr_s *addr_ptr)
{
	struct sockaddr_in sin;
	socklen_t sinlen = sizeof(sin);
	int32 rcv_size;

	if (socket_handle == SRV_INVALID || buf_ptr == NULL)
		return SRV_ERR;

	rcv_size = recvfrom((int32) socket_handle, buf_ptr, len, 0,
			(struct sockaddr*) &sin, &sinlen);
	if (rcv_size < 0)
		return SRV_ERR;

	if (addr_ptr != NULL)
	{
		strcpy(addr_ptr->ip, inet_ntoa(sin.sin_addr));
		addr_ptr->port = ntohs(sin.sin_port);
	}

	return rcv_size;
}
//#endif

