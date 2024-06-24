/*
 * srv_buffer.h
 *
 *  Created on: Aug 17, 2014
 *      Author: Yiming Xu
 */

#ifndef SRV_BUFFER_H_
#define SRV_BUFFER_H_

#include "srv_common.h"

typedef struct SRV_BUF_POOL_
{
    int32 blocksize; // size of block
    int32 blocknr;   // number of blocks
}srv_buffer_pool_s;

int32 srv_buf_init(const srv_buffer_pool_s* buffer_pool_pt, int32 pool_size);

void* srv_buf_alloc(int32 size);
void srv_buf_free(void* buf);

#endif /* SRV_BUFFER_H_ */
