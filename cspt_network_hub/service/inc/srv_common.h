/*
 * srv_common.h
 *
 *  Created on: Aug 16, 2014
 *      Author: Yiming Xu
 *
   Data type definition for all modules/applications in this project.
 */

#ifndef SRV_COMMON_H_
#define SRV_COMMON_H_

typedef unsigned long long int service_handle;

#define SRV_INVALID 0xffffffff

typedef unsigned long long int uint64;
typedef long long int  int64;
typedef unsigned int   uint32;
typedef int            int32;
typedef unsigned short uint16;
typedef short          int16;
typedef unsigned char  uint8;
typedef char           int8;

#define SRV_OK      0
#define SRV_ERR    -1

#endif /* SRV_COMMON_H_ */
