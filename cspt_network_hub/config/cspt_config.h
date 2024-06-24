/*
 * cspt_config.h
 *
 *  Created on: Sep 15, 2014
 *      Author: Yiming Xu
 */

#ifndef CSPT_CONFIG_H_
#define CSPT_CONFIG_H_

#include "srv_common.h"
#include "srv_buffer.h"
#include "srv_task.h"
#include "srv_trace.h"
#include "srv_socket.h"
#include "cspt_kernel.h"
#include "cspt_tcpserver.h"
#include "cspt_tcpsender.h"
#include "cspt_udp.h"

#define TIMEOUT 1000000

typedef enum
{
	TID_KERNEL,
	TID_TCPSERVER,
	TID_UDP,
	TID_TCPSENDER,
	TID_L3RECVER,
	TID_MAX
}cspt_tid_e;

typedef enum
{
	MID_KERNEL,
	MID_TCPSERVER,
	MID_UDP,
	MID_TCPSENDER,
	MID_L3RECVER,
	MID_MAX
}cspt_trace_mid_e;

typedef enum
{
	LID_INFO,
	LID_WARNING,
	LID_ERROR,
	LID_MAX
}cspt_trace_lid_e;


int32 cspt_init();


#endif /* CSPT_CONFIG_H_ */
