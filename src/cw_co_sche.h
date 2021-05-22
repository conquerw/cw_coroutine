/************************************************
 * @addtogroup CITS
 * @{
 * @file  : cw_co_sche.h
 * @brief : 
 * @author: wangshaobo
 * @email : wangshaobo@nebula-link.com
 * @date  : 2019-05-10 10:43:51
************************************************/

//-----------------------------------------------
// Copyright (c) CITS
//-----------------------------------------------

#ifndef _CW_CO_SCHE_H_
#define _CW_CO_SCHE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <ucontext.h>
#include <slist.h>
#include <map.h>
#include <queue.h>

#define SET_BIT(x)	 (1 << (x))
#define RESET_BIT(x) ~(1 << (x))

#define CW_COROUTINE_STACK_LEN (8 * 1024)
#define EPOLL_EVENT_LEN (128 * 1024)

typedef enum
{
	CW_COROUTINE_STATUS_WAIT_READ,
	CW_COROUTINE_STATUS_WAIT_WRITE,
	CW_COROUTINE_STATUS_NEW,
	CW_COROUTINE_STATUS_READY,
	CW_COROUTINE_STATUS_EXITED,
	CW_COROUTINE_STATUS_BUSY,
	CW_COROUTINE_STATUS_SLEEPING,
	CW_COROUTINE_STATUS_EXPIRED,
	CW_COROUTINE_STATUS_FDEOF,
	CW_COROUTINE_STATUS_DETACH,
	CW_COROUTINE_STATUS_CANCELLED,
	CW_COROUTINE_STATUS_PENDING_RUNCOMPUTE,
	CW_COROUTINE_STATUS_RUNCOMPUTE,
	CW_COROUTINE_STATUS_WAIT_IO_READ,
	CW_COROUTINE_STATUS_WAIT_IO_WRITE,
	CW_COROUTINE_STATUS_WAIT_MULTI
} cw_coroutine_status_e;

typedef void (*cw_coroutine_cb)(void *);

typedef struct
{
	ucontext_t context;
	char *stack;
	cw_coroutine_cb func;
	void *para;
	cw_coroutine_status_e status;
	
	unsigned long long sleep_micro_second;
	
	struct cw_scheduler *cw_scheduler;
	char enable_hook_syscall;
	
	map_node sleep_node;
	map_node wait_node;
	struct slist_node ready_node;
} cw_coroutine_t;

typedef struct cw_scheduler
{
	ucontext_t context;
	char *stack;
	
	cw_coroutine_t *cw_coroutine;
	
	int epoll_fd;
	struct epoll_event *epoll_event;
	int epoll_event_len;
	int new_event_len;
	
	int default_timeout;
	
	pthread_t pthread;
	
	map_root sleep_root;
	map_root wait_root;
	queue_t ready_queue;
} cw_scheduler_t;

typedef struct
{
	cw_scheduler_t *cw_scheduler;
	int cw_scheduler_len;
} cw_schedulers_t;

#ifdef __cplusplus
}
#endif

#endif
