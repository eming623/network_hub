/*
 * cspt_config.c
 *
 *  Created on: Sep 15, 2014
 *      Author: Yiming Xu
 */
#include "stdio.h"
#include "cspt_config.h"


static srv_buffer_pool_s g_buffer_pool_arr[] =
{
	{32,     1000},
    {64,     1000},
    {128,    1000},
    {256,    1000},
    {512,    1000},
    {1024,   500},
    {2048,   500},
    {4096,   500},
    {8192,   500},
    {16384,  300},
    {32768,  300},
    {65536,  300},
    {131072, 300},
    {262144, 50},
    {524288, 20},
    {1048576,20}
};

static srv_task_s g_task_table_arr[] =
{
	{TID_KERNEL,    "KERNEL",    kernel_entry,    NULL, SRV_TASK_NORMAL_PRIO, SRV_MSQ_ENABLE},
	{TID_TCPSERVER, "TCPSERVER", tcpserver_entry, NULL, SRV_TASK_NORMAL_PRIO, SRV_MSQ_ENABLE},
	{TID_UDP,       "UDP",       udp_entry,       NULL, SRV_TASK_NORMAL_PRIO, SRV_MSQ_ENABLE},
	{TID_TCPSENDER, "TCPSENDER", tcpsender_entry,       NULL, SRV_TASK_NORMAL_PRIO, SRV_MSQ_ENABLE},
	{TID_L3RECVER,	"L3EMSGRECVER",tcpsender_entry,NULL, SRV_TASK_NORMAL_PRIO, SRV_MSQ_ENABLE}
};

static srv_trace_module_s trace_module_arr[] =
{
	{MID_KERNEL, "KERNEL", 1},
	{MID_TCPSERVER,"TCPSERVER",1},
	{MID_UDP,"UDP",1}
};

static srv_trace_level_s trace_level_arr[] =
{
	{LID_INFO, "INFO", 1},
	{LID_WARNING, "WARNING", 1},
	{LID_ERROR, "ERROR", 1}
};

int32 cspt_init()
{
	int32 ret;
	ret = srv_buf_init(g_buffer_pool_arr, sizeof(g_buffer_pool_arr)/sizeof(srv_buffer_pool_s));
	if(SRV_OK != ret)
	{
		printf("Buffer init failed.\r\n");
		return SRV_ERR;
	}

	ret = srv_skt_init();
	if(SRV_OK != ret)
	{
		printf("Socket init failed.\r\n");
		return SRV_ERR;
	}

	ret = srv_trace_init(trace_module_arr, sizeof(trace_module_arr) / sizeof(srv_trace_module_s),
						   trace_level_arr, sizeof(trace_level_arr) / sizeof(srv_trace_level_s));
	if(SRV_OK != ret)
	{
		printf("Trace init failed.\r\n");
		return SRV_ERR;
	}

	ret = srv_task_init(TID_MAX);
	if(SRV_OK != ret)
	{
		printf("Task init failed.\r\n");
		return SRV_ERR;
	}

	ret = srv_task_register_table(g_task_table_arr, sizeof(g_task_table_arr)/sizeof(srv_task_s));
	if(SRV_OK != ret)
	{
		printf("Task register failed.\r\n");
		return SRV_ERR;
	}

	 ret = srv_task_start_table();
	 if(SRV_OK != ret)
	 {
		 printf("Task start failed.\r\n");
		 return SRV_ERR;
	 }

	return SRV_OK;
}
