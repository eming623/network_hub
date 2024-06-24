/*
 * lnx_mutex.c
 *
 *  Created on: Aug 17, 2014
 *      Author: Yiming Xu
 */

#include <pthread.h>
#include <stdlib.h>
#include "srv_mutex.h"

service_handle srv_mutex_create()
{
    pthread_mutex_t* mutex;

    mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));

    if (mutex == NULL)
    	return SRV_INVALID;

    if (pthread_mutex_init(mutex,NULL) != 0)
    {
    	free(mutex);
        return SRV_INVALID;
    }
    return (service_handle)mutex;
}

int32 srv_mutex_destroy(service_handle mutex_handle)
{
    pthread_mutex_t* mutex = (pthread_mutex_t*)mutex_handle;

    if (mutex_handle == SRV_INVALID)
        return SRV_ERR;

    if (pthread_mutex_destroy(mutex) != 0)
        return SRV_ERR;

	free(mutex);

    return SRV_OK;
}

int32 srv_mutex_lock(service_handle mutex_handle)
{
    pthread_mutex_t* mutex = (pthread_mutex_t*)mutex_handle;

    if (mutex_handle == SRV_INVALID)
        return SRV_ERR;

    if (pthread_mutex_lock(mutex) != 0)
        return SRV_ERR;

    return SRV_OK;
}

int32 srv_mutex_unlock(service_handle mutex_handle)
{
    pthread_mutex_t* mutex = (pthread_mutex_t*)mutex_handle;

    if (mutex_handle == SRV_INVALID)
        return SRV_ERR;

    if (pthread_mutex_unlock(mutex) != 0)
        return SRV_ERR;

    return SRV_OK;
}

