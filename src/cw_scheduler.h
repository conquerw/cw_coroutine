/************************************************
 * @addtogroup CITS
 * @{
 * @file  : cw_scheduler.h
 * @brief : 
 * @author: wangshaobo
 * @email : wangshaobo@nebula-link.com
 * @date  : 2019-05-10 10:43:51
************************************************/

//-----------------------------------------------
// Copyright (c) CITS
//-----------------------------------------------

#ifndef _CW_SCHEDULER_H_
#define _CW_SCHEDULER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <cw_co_sche.h>

int cw_scheduler_init(cw_scheduler_t *cw_scheduler);

int cw_scheduler_exit(cw_scheduler_t *cw_scheduler);

void *cw_scheduler_run(void *para);

int cw_scheduler_stop(cw_scheduler_t *cw_scheduler);

#ifdef __cplusplus
}
#endif

#endif
