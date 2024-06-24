/*
 * cspt_udp.c
 *
 *  Created on: Oct 27, 2014
 *      Author: Yiming Xu
 */
#include <stdio.h>
#include <stddef.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include "cspt_udp.h"
#include "cspt_kernel.h"
#include "cspt_adapter.h"
#include "cspt_config.h"

extern int32 g_epoll_fd;

void* udp_entry(void* arg)
{
    printf("%s-%u:Enter udp_entry\r\n", __FILE__, __LINE__);

    udp_recv_msg();

    return NULL;
}

void udp_recv_msg()
{
    int8* pBuf = NULL;
    struct epoll_event events[MAX_UDP_RECV_NUM];
    struct sockaddr_in peerAddr;
    socklen_t peerAddrLen = sizeof(peerAddr);
    int32 num, i, size;

    while (1)
    {
        num = epoll_wait(g_epoll_fd, events, MAX_UDP_RECV_NUM, -1);
        for (i = 0; i < num; i++)
        {
            if (events[i].events & EPOLLIN)
            {
                pBuf = cspt_alloc(MAX_UDP_LEN);
                if (NULL == pBuf)
                {
                    printf("%s-%u:cspt_alloc failed\r\n", __FILE__, __LINE__);
                    continue;
                }

                size = recvfrom(events[i].data.fd, pBuf + CSPT_HEADER_LEN,
                        MAX_UDP_LEN, 0, (struct sockaddr*) &peerAddr,
                        &peerAddrLen);
                if (size > 0)
                {
                    ((cspt_header_s *) pBuf)->length = size;
                    ((cspt_header_s *) pBuf)->type = CSPT_MSG_TYPE_UDP;
                    ((cspt_header_s *) pBuf)->header.udp_header.ip_addr = ntohl(
                            peerAddr.sin_addr.s_addr);
                    ((cspt_header_s *) pBuf)->header.udp_header.port = ntohs(
                            peerAddr.sin_port);
                    ((cspt_header_s *) pBuf)->header.udp_header.socket =
                            events[i].data.fd;
                    if (SRV_OK
                            != cspt_snd(TID_UDP, TID_KERNEL,
                                    SRV_MSG_NORMAL_PRIO, pBuf))
                    {
                        printf("%s-%u:cspt_snd failed\r\n", __FILE__, __LINE__);
                        cspt_free(pBuf);
                    }
                }
                else
                {
                    cspt_free(pBuf);
                }
            }
        }
    }

}
