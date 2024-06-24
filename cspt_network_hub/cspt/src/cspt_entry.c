/*
 * cspt_entry.c
 *
 *  Created on: Oct 26, 2014
 *      Author: Yiming Xu
 */
#include <stdio.h>
#include <unistd.h>
#include "cspt_config.h"

int main()
{
	int32 ret;
	printf("welcome to CSPT network hub!\r\n");
	ret = cspt_init();

	if(SRV_OK != ret)
	{
		printf("%s-%u:cspt_init failed.\r\n",__FILE__,__LINE__);
		return -1;
	}

	while(1)
	{
		sleep(TIMEOUT);
	}

	return 1;
}
