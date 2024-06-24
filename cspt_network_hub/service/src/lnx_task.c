/*
 * lnx_task.c
 *
 *  Created on: Aug 17, 2014
 *      Author: Yiming Xu
 */

#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "srv_task.h"

static void* task_entry(void* arg);

service_handle srv_task_create(srv_task_s* task_ptr)
{
	pthread_t thdid;

    if (task_ptr == NULL)
        return SRV_INVALID;

    if (pthread_create(&thdid,NULL,task_entry,(void*)task_ptr) != 0)
        return SRV_INVALID;

    return (service_handle)thdid;
}

void srv_task_sleep(uint32 usec)
{
	struct timespec requestTime, remainTime;
	requestTime.tv_sec = usec / 1000;
	requestTime.tv_nsec = (usec % 1000) * 1000000;
	while (nanosleep(&requestTime,&remainTime) == -1)
	{
	    if (errno != EINTR)
	    	break;

	    requestTime = remainTime;
	}
}


static void* task_entry(void* arg)
{
    srv_task_s* pTask = (srv_task_s*)arg;

    if (arg == NULL)
        return NULL;

    pTask->fpEntry((void*)pTask);

    return NULL;
}


