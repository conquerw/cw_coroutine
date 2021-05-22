/************************************************
 * @addtogroup CITS
 * @{
 * @file  : cw_coroutine.h
 * @brief : 
 * @author: wangshaobo
 * @email : wangshaobo@nebula-link.com
 * @date  : 2019-05-10 10:43:51
************************************************/

//-----------------------------------------------
// Copyright (c) CITS
//-----------------------------------------------

#ifndef _CW_COROUTINE_H_
#define _CW_COROUTINE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <cw_co_sche.h>

int cw_coroutine_create(cw_scheduler_t *cw_scheduler, cw_coroutine_t *cw_coroutine, cw_coroutine_cb func, void *para);

void cw_coroutine_yield(cw_coroutine_t *cw_coroutine);

void cw_coroutine_resume(cw_coroutine_t *cw_coroutine);

void cw_coroutine_usleep(cw_coroutine_t *cw_coroutine, int micro_second);

void cw_coroutine_sleep(cw_coroutine_t *cw_coroutine, int second);

void cw_enable_hook_syscall(void);

#ifdef __cplusplus
}
#endif

#endif
