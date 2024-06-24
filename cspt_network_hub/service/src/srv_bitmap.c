/*
 * srv_bitmap.c
 *
 *  Created on: Aug 17, 2014
 *      Author: Yiming Xu
 *
   Bitmap service implementation for upper layer applications.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "srv_bitmap.h"

#define SRV_BMP_BITS 32

service_handle srv_bmp_create(int32 bit_num)
{
	uint32 *bitmap_ptr;
	int32 size;

	((bit_num % SRV_BMP_BITS) == 0) ?
			(size = bit_num / SRV_BMP_BITS) :
			(size = bit_num / SRV_BMP_BITS + 1);
    // Memory management module has dependency to bitmap service. This function
	// need to be called before memory management module, so it uses raw malloc.
	bitmap_ptr = (uint32*) malloc(size * sizeof(uint32));
	if (bitmap_ptr == NULL)
		return SRV_INVALID;

	memset(bitmap_ptr, 0, size * sizeof(uint32));

	return (service_handle) bitmap_ptr;
}

void srv_bmp_destroy(service_handle bitmap_handle)
{
	uint32 *bitmap_ptr = (uint32*) bitmap_handle;

	if (bitmap_handle == SRV_INVALID)
		return;

	free(bitmap_ptr);
}

uint32 srv_bmp_get_bit(service_handle bitmap_handle, int32 bit)
{
	uint32 *bitmap_ptr = (uint32*) bitmap_handle;
	uint32 val;
	int32 ind, subind;

	if (bitmap_handle == SRV_INVALID)
		return SRV_ERR;

	ind = bit / SRV_BMP_BITS;
	subind = bit % SRV_BMP_BITS;

	val = (bitmap_ptr[ind] >> subind) & 0x1;

	return val;
}

int32 srv_bmp_set_bit(service_handle bitmap_handle, int32 bit, uint32 val)
{
	uint32 *bitmap_ptr = (uint32*) bitmap_handle;
	int32 ind, subind;

	if (bitmap_handle == SRV_INVALID)
		return SRV_ERR;

	ind = bit / SRV_BMP_BITS;
	subind = bit % SRV_BMP_BITS;

	if (val)
		bitmap_ptr[ind] |= 1 << subind;
	else
		bitmap_ptr[ind] &= ~(1 << subind);

	return SRV_OK;
}
