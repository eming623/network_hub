/*
 * srv_task.h
 *
 *  Created on: Aug 17, 2014
 *      Author: Yiming Xu
 */

#ifndef SRV_TASK_H_
#define SRV_TASK_H_

#include "srv_common.h"

#define SRV_TASK_HIGH_PRIO   0
#define SRV_TASK_NORMAL_PRIO 1
#define SRV_TASK_LOW_PRIO    2

#define SRV_MSQ_ENABLE   1
#define SRV_MSQ_DISABLE  0

#define SRV_MSG_HIGH_PRIO   0
#define SRV_MSG_NORMAL_PRIO 1
#define SRV_MSG_LOW_PRIO    2

typedef struct SRV_TASK_
{
    int32     tid;
    char*     name;
    void*     (*fpEntry)(void*);
    void*     arg;
    int32     prio;
    int32     queue_enable; // 1: create a message queue for this task, 0: not create
}srv_task_s;

int32 srv_task_init(int32 maxtablesize);

int32 srv_task_register_table(const srv_task_s *task_table_ptr, int32 table_size);
int32 srv_task_start_table();

int32 srv_task_register(const srv_task_s* task_ptr);
int32 srv_task_start(int32 tid);

void srv_task_sleep(uint32 usec);

void* srv_msg_alloc(int32 size);
void srv_msg_free(void* message_ptr);

int32 srv_msg_snd(int32 stid, int32 dtid, int32 prio, int32 msgid, void* message_ptr);
int32 srv_msg_rcv(int32 tid, int32* message_id_ptr, void** message_ptr_ptr);

int32 srv_msg_getsrctid(void *message_ptr);

int32 srv_get_msg_num(int32 tid);

#endif /* SRV_TASK_H_ */
