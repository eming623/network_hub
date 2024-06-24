/*
 * srv_trace.c
 *
 *  Created on: Aug 17, 2014
 *      Author: Yiming Xu
 *
   The implementaiton of trace service to application layer
 */

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "srv_trace.h"
#include "srv_buffer.h"

#define MAX_TRACE_BUF 1024

typedef struct
{
	srv_trace_module_s* module_config_ptr;
	srv_trace_level_s*  level_config_ptr;
}trace_config_s;

static trace_config_s* g_trace_config_ptr = NULL;
static char g_trace_buffer[MAX_TRACE_BUF] = {0};

int32 srv_trace_init(const srv_trace_module_s* pstModule, int32 modulenr, const srv_trace_level_s* pstLevel, int32 levelnr)
{
	int32        nr,i;

	if (pstModule==NULL || pstLevel==NULL)
		return SRV_ERR;

	g_trace_config_ptr = (trace_config_s*)srv_buf_alloc(sizeof(trace_config_s));
	if (g_trace_config_ptr == NULL)
		return SRV_ERR;

	// register module information.

	nr = 0;
	for (i=0; i<modulenr; i++)
	{
		if (pstModule[i].mid > nr)
			nr = pstModule[i].mid;
	}
	nr++;

	g_trace_config_ptr->module_config_ptr = (srv_trace_module_s*)srv_buf_alloc(sizeof(srv_trace_module_s)*nr);
	if (g_trace_config_ptr->module_config_ptr == NULL)
		return SRV_ERR;

	for (i=0; i<modulenr; i++)
	{
		g_trace_config_ptr->module_config_ptr[pstModule[i].mid].mid    = pstModule[i].mid;
		g_trace_config_ptr->module_config_ptr[pstModule[i].mid].name   = pstModule[i].name;
		g_trace_config_ptr->module_config_ptr[pstModule[i].mid].enable = pstModule[i].enable;
	}

	// register level information.

	nr = 0;
	for (i=0; i<levelnr; i++)
	{
		if (pstLevel[i].lid > nr)
			nr = pstLevel[i].lid;
	}
	nr++;

	g_trace_config_ptr->level_config_ptr = (srv_trace_level_s*)srv_buf_alloc(sizeof(srv_trace_level_s)*nr);
	if (g_trace_config_ptr->level_config_ptr == NULL)
		return SRV_ERR;

	for (i=0; i<modulenr; i++)
	{
		g_trace_config_ptr->level_config_ptr[pstLevel[i].lid].lid    = pstLevel[i].lid;
		g_trace_config_ptr->level_config_ptr[pstLevel[i].lid].name   = pstLevel[i].name;
		g_trace_config_ptr->level_config_ptr[pstLevel[i].lid].enable = pstLevel[i].enable;
	}

	return SRV_OK;
}

void srv_trace_print(int32 mid, int32 level, char* filename, int32 linenr, char* format, ...)
{
    va_list    marker;
    int32        headsize;
    time_t     cursec;
    struct tm* pstCurTime;

    if (!g_trace_config_ptr->module_config_ptr[mid].enable || !g_trace_config_ptr->level_config_ptr[level].enable)
    	return;

    // get current time
    cursec = time(NULL);
    pstCurTime = localtime(&cursec);
    if (pstCurTime == NULL)
    	return;

    // print trace head
    headsize = sprintf(g_trace_buffer, "<MID: %s> -- <LEVEL: %s> -- <FILE: %s> -- <LINE: %d> -- <%d-%d-%d %d:%d:%d> --",
    		           g_trace_config_ptr->module_config_ptr[mid].name, g_trace_config_ptr->level_config_ptr[level].name, filename, linenr,
    		           pstCurTime->tm_year+1900, pstCurTime->tm_mon+1, pstCurTime->tm_mday,
    		           pstCurTime->tm_hour, pstCurTime->tm_min, pstCurTime->tm_sec);

    // print trace body
    va_start(marker, format);
    vsprintf(&g_trace_buffer[headsize], format, marker);
    va_end(marker);

    printf(g_trace_buffer);
}


