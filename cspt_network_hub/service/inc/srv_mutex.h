/*
 * srv_mutex.h
 *
 *  Created on: Aug 17, 2014
 *      Author: Yiming Xu
 *
   A service API wrapper for mutex operations in pthread library.
 */

#ifndef SRV_MUTEX_H_
#define SRV_MUTEX_H_

#include "srv_common.h"

service_handle srv_mutex_create();
int32 srv_mutex_destroy(service_handle mutex_handle);

int32 srv_mutex_lock(service_handle mutex_handle);
int32 srv_mutex_unlock(service_handle mutex_handle);

#endif /* SRV_MUTEX_H_ */
