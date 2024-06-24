/*
 * srv_trace.h
 *
 *  Created on: Aug 17, 2014
 *      Author: Yiming Xu
 *
   Provides trace service API to application layer
 */

#ifndef SRV_TRACE_H_
#define SRV_TRACE_H_

#include "srv_common.h"

typedef struct
{
	int32 mid;
	char *name;
	int32 enable;
} srv_trace_module_s;

typedef struct
{
	int32 lid;
	char *name;
	int32 enable;
} srv_trace_level_s;

int32 srv_trace_init(const srv_trace_module_s *pstModule, int32 modulenr,
		const srv_trace_level_s *pstLevel, int32 levelnr);

void srv_trace_print(int32 mid, int32 level, char *filename, int32 linenr,
		char *format, ...);

#define SRV_TRACE_PRINT(mid, level, format...) \
	    srv_trace_print(mid, level, __FILE__, __LINE__, format)

#endif /* SRV_TRACE_H_ */
