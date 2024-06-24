/*
 * srv_bitmap.h
 *
 *  Created on: Aug 17, 2014
 *      Author: Yiming Xu
 *
   Provides bitmap operation APIs to application layer.
 */

#ifndef SRV_BITMAP_H_
#define SRV_BITMAP_H_

#include "srv_common.h"

service_handle srv_bmp_create(int32 bit_num);
void srv_bmp_destroy(service_handle bitmap_handle);

uint32 srv_bmp_get_bit(service_handle bitmap_handle, int32 bit);
int32 srv_bmp_set_bit(service_handle bitmap_handle, int32 bit, uint32 val);

#endif /* SRV_BITMAP_H_ */
