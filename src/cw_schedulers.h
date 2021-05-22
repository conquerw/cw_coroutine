/************************************************
 * @addtogroup CITS
 * @{
 * @file  : cw_schedulers.h
 * @brief : 
 * @author: wangshaobo
 * @email : wangshaobo@nebula-link.com
 * @date  : 2019-05-10 10:43:51
************************************************/

//-----------------------------------------------
// Copyright (c) CITS
//-----------------------------------------------

#ifndef _CW_SCHEDULERS_H_
#define _CW_SCHEDULERS_H_

#ifdef __cplusplus
extern "C"
{
#endif

cw_schedulers_t *cw_schedulers_init(void);

int cw_schedulers_exit(void);

int cw_schedulers_run(void);

int cw_schedulers_stop(void);

#ifdef __cplusplus
}
#endif

#endif
