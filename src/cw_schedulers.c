/************************************************
 * @addtogroup CITS
 * @{
 * @file  : cw_schedulers.c
 * @brief : 
 * @author: wangshaobo
 * @email : wangshaobo@nebula-link.com
 * @date  : 2019-05-10 10:43:51
************************************************/

//-----------------------------------------------
// Copyright (c) CITS
//-----------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <thread.h>
#include <cw_scheduler.h>

cw_schedulers_t *cw_schedulers = NULL;

cw_schedulers_t *cw_schedulers_init(void)
{
	if(cw_schedulers)
		return cw_schedulers;
	
	cw_schedulers = (cw_schedulers_t *)calloc(1, sizeof(cw_schedulers_t));
	cw_schedulers->cw_scheduler_len = sysconf(_SC_NPROCESSORS_CONF) / 2;
	cw_schedulers->cw_scheduler = (cw_scheduler_t *)calloc(cw_schedulers->cw_scheduler_len, sizeof(cw_scheduler_t));
	for(int i = 0; i < cw_schedulers->cw_scheduler_len; i++)
		cw_scheduler_init(cw_schedulers->cw_scheduler + i);
	
	return cw_schedulers;
}

int cw_schedulers_exit(void)
{
	if(!cw_schedulers)
		return -1;
	
	for(int i = 0; i < cw_schedulers->cw_scheduler_len; i++)
		cw_scheduler_exit(cw_schedulers->cw_scheduler + i);
	free(cw_schedulers->cw_scheduler);
	
	return 1;
}

int cw_schedulers_run(void)
{
	if(!cw_schedulers)
		return -1;
	
	for(int i = 0; i < cw_schedulers->cw_scheduler_len; i++)
		create_thread2(&(cw_schedulers->cw_scheduler + i)->pthread, cw_scheduler_run, cw_schedulers->cw_scheduler + i);
	
	return 1;
}

int cw_schedulers_stop(void)
{
	if(!cw_schedulers)
		return -1;
	
	for(int i = 0; i < cw_schedulers->cw_scheduler_len; i++)
		cancel_thread((cw_schedulers->cw_scheduler + i)->pthread);
	
	return 1;
}
