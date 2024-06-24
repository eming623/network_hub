/*
 * cspt_udp.h
 *
 *  Created on: Oct 27, 2014
 *      Author: Yiming Xu
 */

#ifndef CSPT_UDP_H_
#define CSPT_UDP_H_

#include "srv_common.h"

#define MAX_UDP_LEN     1500

void* udp_entry(void* arg);

void udp_recv_msg();

#endif /* CSPT_UDP_H_ */
