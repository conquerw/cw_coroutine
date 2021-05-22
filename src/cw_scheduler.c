/************************************************
 * @addtogroup CITS
 * @{
 * @file  : cw_scheduler.c
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
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <time_tool.h>
#include <cw_co_sche.h>
#include <cw_coroutine.h>
#include <cw_scheduler.h>

static int cw_scheduler_find_min_timeout(cw_scheduler_t *cw_scheduler)
{
	cw_coroutine_t *cw_coroutine = map_first_entry(&cw_scheduler->sleep_root, cw_coroutine_t, sleep_node);
	if(!cw_coroutine)
		return cw_scheduler->default_timeout;
	
	uint64_t time = get_micro_second();
	if(cw_coroutine->sleep_micro_second > time)
		return cw_coroutine->sleep_micro_second - time;
	
	return 0;
}

static cw_coroutine_t *cw_scheduler_expire(cw_scheduler_t *cw_scheduler)
{
	cw_coroutine_t *cw_coroutine = map_first_entry(&cw_scheduler->sleep_root, cw_coroutine_t, sleep_node);
	if(!cw_coroutine)
		return NULL;
	
	uint64_t time = get_micro_second();
	if(cw_coroutine->sleep_micro_second <= time)
	{
		cw_coroutine->status &= RESET_BIT(CW_COROUTINE_STATUS_SLEEPING);
		map_remove(&cw_coroutine->cw_scheduler->sleep_root, cw_coroutine->sleep_node.key);
		return cw_coroutine;
	}
	
	return NULL;
}

static int cw_scheduler_epoll(cw_scheduler_t *cw_scheduler)
{
	cw_scheduler->new_event_len = 0;
	int timeout = cw_scheduler_find_min_timeout(cw_scheduler);
	if(!timeout || queue_len(&cw_scheduler->ready_queue) > 0)
		return 0;
	
	int nready = 0;
	while(1)
	{
		memset(cw_scheduler->epoll_event, 0x00, cw_scheduler->epoll_event_len * sizeof(struct epoll_event));
		nready = epoll_wait(cw_scheduler->epoll_fd, cw_scheduler->epoll_event, cw_scheduler->epoll_event_len, timeout / 1000);
		if(nready < 0)
		{
			if(errno == EINTR)
				continue;
			else
				return -1;
		}
		break;
	}
	
	cw_scheduler->new_event_len = nready;
	
	return 0;
}

static cw_coroutine_t *cw_scheduler_find_wait(cw_scheduler_t *cw_scheduler, int fd)
{
	if(!cw_scheduler)
		return NULL;
	
	cw_coroutine_t *cw_coroutine = map_get_entry(fd, &cw_scheduler->wait_root, cw_coroutine_t, wait_node);
	if(!cw_coroutine)
		return NULL;
	
	epoll_ctl(cw_scheduler->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	
	cw_coroutine->status &= RESET_BIT(CW_COROUTINE_STATUS_WAIT_MULTI);
	map_remove(&cw_scheduler->wait_root, cw_coroutine->wait_node.key);
	
	return cw_coroutine;
}

int cw_scheduler_init(cw_scheduler_t *cw_scheduler)
{
	if(!cw_scheduler)
		return -1;
	
	getcontext(&cw_scheduler->context);
	
	cw_scheduler->stack = (char *)calloc(CW_COROUTINE_STACK_LEN, sizeof(char));
	
	cw_scheduler->context.uc_stack.ss_sp = cw_scheduler->stack;
	cw_scheduler->context.uc_stack.ss_size = CW_COROUTINE_STACK_LEN;
	cw_scheduler->context.uc_stack.ss_flags = 0;
	cw_scheduler->context.uc_link = NULL;
	
	cw_scheduler->cw_coroutine = NULL;
	
	cw_scheduler->epoll_fd = epoll_create(1);
	cw_scheduler->epoll_event = (struct epoll_event *)calloc(EPOLL_EVENT_LEN, sizeof(struct epoll_event));
	cw_scheduler->epoll_event_len = EPOLL_EVENT_LEN;
	cw_scheduler->new_event_len = 0;
	
	cw_scheduler->default_timeout = 3000000;
	
	cw_scheduler->sleep_root = RB_ROOT;
	cw_scheduler->wait_root = RB_ROOT;
	queue_init(&cw_scheduler->ready_queue, 1024, 0);
	
	return 1;
}

int cw_scheduler_exit(cw_scheduler_t *cw_scheduler)
{
	if(!cw_scheduler)
		return -1;
	
	cw_scheduler->cw_coroutine->enable_hook_syscall = 0;
	queue_exit(&cw_scheduler->ready_queue);
	close(cw_scheduler->epoll_fd);
	if(cw_scheduler->stack)
		free(cw_scheduler->stack);
	
	return 1;
}

void *cw_scheduler_run(void *para)
{
	if(!para)
		return NULL;
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	cw_scheduler_t *cw_scheduler = (cw_scheduler_t *)para;
	cw_coroutine_t *cw_coroutine = NULL;
	
	while(1)
	{
		while((cw_coroutine = cw_scheduler_expire(cw_scheduler)) != NULL)
		{
			cw_coroutine->status |= SET_BIT(CW_COROUTINE_STATUS_READY);
			enqueue(&cw_scheduler->ready_queue, &cw_coroutine->ready_node);
		}
		
		cw_scheduler_epoll(cw_scheduler);
		for(int i = 0; i < cw_scheduler->new_event_len; i++)
		{
			struct epoll_event *ev = cw_scheduler->epoll_event + i;
			
			int fd = ev->data.fd;
			int is_eof = ev->events & EPOLLHUP;
			if(is_eof)
				errno = ECONNRESET;
			
			cw_coroutine = cw_scheduler_find_wait(cw_scheduler, fd);
			if(cw_coroutine)
			{
				cw_coroutine->status |= SET_BIT(CW_COROUTINE_STATUS_READY);
				enqueue(&cw_scheduler->ready_queue, &cw_coroutine->ready_node);
			}
		}
		
		while(queue_len(&cw_scheduler->ready_queue))
		{
			struct slist_node *node = dequeue(&cw_scheduler->ready_queue);
			if(node)
			{
				cw_coroutine = slist_entry(node, cw_coroutine_t, ready_node);
				if(cw_coroutine)
				{
					cw_scheduler->cw_coroutine = cw_coroutine;
					cw_coroutine_resume(cw_coroutine);
				}
			}
		}
	}
	
	return NULL;
}

int cw_scheduler_stop(cw_scheduler_t *cw_scheduler)
{
	if(!cw_scheduler)
		return -1;
	
	return 1;
}
