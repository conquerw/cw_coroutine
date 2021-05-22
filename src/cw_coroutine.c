/************************************************
 * @addtogroup CITS
 * @{
 * @file  : cw_coroutine.c
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
#include <string.h>
#include <stdbool.h>
#include <poll.h>
#include <sys/epoll.h>
#include <time_tool.h>
#include <cw_coroutine.h>

int cw_coroutine_create(cw_scheduler_t *cw_scheduler, cw_coroutine_t *cw_coroutine, cw_coroutine_cb func, void *para)
{
	if(!cw_scheduler || !cw_coroutine)
		return -1;
	
	getcontext(&cw_coroutine->context);
	
	cw_coroutine->stack = (char *)calloc(CW_COROUTINE_STACK_LEN, sizeof(char));
	cw_coroutine->func = func;
	cw_coroutine->para = para;
	
	cw_coroutine->context.uc_stack.ss_sp = cw_coroutine->stack;
	cw_coroutine->context.uc_stack.ss_size = CW_COROUTINE_STACK_LEN;
	cw_coroutine->context.uc_stack.ss_flags = 0;
	cw_coroutine->context.uc_link = &(cw_scheduler->context);
	
	makecontext(&(cw_coroutine->context), (void (*)(void))cw_coroutine->func, 1, cw_coroutine->para);
	
	cw_coroutine->status |= SET_BIT(CW_COROUTINE_STATUS_READY);
	cw_coroutine->sleep_micro_second = 0;
	cw_coroutine->cw_scheduler = cw_scheduler;
	
	enqueue(&cw_scheduler->ready_queue, &cw_coroutine->ready_node);
	
	return 1;
}

static void cw_coroutine_yield_i(cw_coroutine_t *cw_coroutine)
{
	if(!cw_coroutine)
		return;
	
	swapcontext(&cw_coroutine->context, &cw_coroutine->cw_scheduler->context);
}

void cw_coroutine_yield(cw_coroutine_t *cw_coroutine)
{
	if(!cw_coroutine)
		return;
	
	enqueue(&cw_coroutine->cw_scheduler->ready_queue, &cw_coroutine->ready_node);
	cw_coroutine_yield_i(cw_coroutine);
}

void cw_coroutine_resume(cw_coroutine_t *cw_coroutine)
{
	if(!cw_coroutine)
		return;
	
	swapcontext(&cw_coroutine->cw_scheduler->context, &cw_coroutine->context);
}

void cw_coroutine_usleep(cw_coroutine_t *cw_coroutine, int micro_second)
{
	if(!cw_coroutine)
		return;
	
	cw_coroutine_t *cw_coroutine2 = map_get_entry(cw_coroutine->sleep_micro_second, &cw_coroutine->cw_scheduler->sleep_root, cw_coroutine_t, sleep_node);
	if(cw_coroutine2)
		map_remove(&cw_coroutine->cw_scheduler->sleep_root, cw_coroutine->sleep_node.key);
	
	cw_coroutine->sleep_micro_second = get_micro_second() + micro_second;
	
	while(1)
	{
		cw_coroutine2 = map_get_entry(cw_coroutine->sleep_micro_second, &cw_coroutine->cw_scheduler->sleep_root, cw_coroutine_t, sleep_node);
		if(cw_coroutine2)
			cw_coroutine->sleep_micro_second++;
		else
		{
			cw_coroutine->status |= SET_BIT(CW_COROUTINE_STATUS_SLEEPING);
			cw_coroutine->sleep_node.key = cw_coroutine->sleep_micro_second;
			map_add(&cw_coroutine->cw_scheduler->sleep_root, &cw_coroutine->sleep_node);
			break;
		}
	}
	
	cw_coroutine_yield_i(cw_coroutine);
}

void cw_coroutine_sleep(cw_coroutine_t *cw_coroutine, int second)
{
	if(!cw_coroutine)
		return;
	
	cw_coroutine_t *cw_coroutine2 = map_get_entry(cw_coroutine->sleep_micro_second, &cw_coroutine->cw_scheduler->sleep_root, cw_coroutine_t, sleep_node);
	if(cw_coroutine2)
		map_remove(&cw_coroutine->cw_scheduler->sleep_root, cw_coroutine->sleep_node.key);
	
	cw_coroutine->sleep_micro_second = get_micro_second() + second * 1000000;
	
	while(1)
	{
		cw_coroutine2 = map_get_entry(cw_coroutine->sleep_micro_second, &cw_coroutine->cw_scheduler->sleep_root, cw_coroutine_t, sleep_node);
		if(cw_coroutine2)
			cw_coroutine->sleep_micro_second++;
		else
		{
			cw_coroutine->status |= SET_BIT(CW_COROUTINE_STATUS_SLEEPING);
			cw_coroutine->sleep_node.key = cw_coroutine->sleep_micro_second;
			map_add(&cw_coroutine->cw_scheduler->sleep_root, &cw_coroutine->sleep_node);
			break;
		}
	}
	
	cw_coroutine_yield_i(cw_coroutine);
}

static uint32_t pollevent2epoll(short events)
{
	uint32_t e = 0;
	if(events & POLLIN)
		e |= EPOLLIN;
	if(events & POLLOUT)
		e |= EPOLLOUT;
	if(events & POLLHUP)
		e |= EPOLLHUP;
	if(events & POLLERR)
		e |= EPOLLERR;
	if(events & POLLRDNORM)
		e |= EPOLLRDNORM;
	if(events & POLLWRNORM)
		e |= EPOLLWRNORM;
	return e;
}

extern cw_schedulers_t *cw_schedulers;

int cw_poll_inner(struct pollfd fds[], nfds_t nfds, int timeout)
{
	if(!cw_schedulers)
		return -1;
	
	pthread_t pthread = pthread_self();
	
	int i = 0;
	for(; i < cw_schedulers->cw_scheduler_len; i++)
	{
		if((cw_schedulers->cw_scheduler + i)->pthread == pthread)
			break;
	}
	
	cw_scheduler_t *cw_scheduler = cw_schedulers->cw_scheduler + i;
	
	cw_coroutine_t *cw_coroutine = cw_scheduler->cw_coroutine;
	if(!cw_coroutine)
		return -1;
	
	for(nfds_t i = 0; i < nfds; i++)
	{
		struct epoll_event ev;
		memset(&ev, 0x00, sizeof(ev));
		ev.events = pollevent2epoll(fds[i].events);
		ev.data.fd = fds[i].fd;
		epoll_ctl(cw_scheduler->epoll_fd, EPOLL_CTL_ADD, fds[i].fd, &ev);
		
		cw_coroutine->status |= SET_BIT(CW_COROUTINE_STATUS_WAIT_MULTI);
		cw_coroutine->wait_node.key = fds[i].fd;
		map_add(&cw_coroutine->cw_scheduler->wait_root, &cw_coroutine->wait_node);
	}
	
	cw_coroutine_yield_i(cw_coroutine);
	
	return 1;
}

void cw_enable_hook_syscall(void)
{
	if(!cw_schedulers)
		return;
	
	pthread_t pthread = pthread_self();
	
	int i = 0;
	for(; i < cw_schedulers->cw_scheduler_len; i++)
	{
		if((cw_schedulers->cw_scheduler + i)->pthread == pthread)
			break;
	}
	
	cw_scheduler_t *cw_scheduler = cw_schedulers->cw_scheduler + i;
	
	cw_coroutine_t *cw_coroutine = cw_scheduler->cw_coroutine;
	if(!cw_coroutine)
		return;
	
	cw_coroutine->enable_hook_syscall = 1;
}

int cw_is_enable_hook_syscall(void)
{
	if(!cw_schedulers)
		return 0;
	
	pthread_t pthread = pthread_self();
	
	int i = 0;
	for(; i < cw_schedulers->cw_scheduler_len; i++)
	{
		if((cw_schedulers->cw_scheduler + i)->pthread == pthread)
			break;
	}
	
	cw_scheduler_t *cw_scheduler = cw_schedulers->cw_scheduler + i;
	
	cw_coroutine_t *cw_coroutine = cw_scheduler->cw_coroutine;
	if(!cw_coroutine)
		return 0;
	
	if(cw_coroutine->enable_hook_syscall == 1)
		return 1;
	
	return 0;
}
