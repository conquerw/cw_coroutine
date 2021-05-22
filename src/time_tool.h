/************************************************
 * @addtogroup CITS
 * @{
 * @file  : time_tool.h
 * @brief : 
 * @author: wangshaobo
 * @email : wangshaobo@nebula-link.com
 * @date  : 2019-05-10 10:43:51
************************************************/

//-----------------------------------------------
// Copyright (c) CITS
//-----------------------------------------------

#ifndef _TIME_TOOL_H_
#define _TIME_TOOL_H_

#include <time.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C"
{
#endif

static inline void print_nanosecond(void)
{
	struct timespec current_time;
	
	clock_gettime(CLOCK_REALTIME, &current_time);
	
	printf("current time: %ld s, %ld ns\n", current_time.tv_sec, current_time.tv_nsec);
}

static inline unsigned long long get_micro_second(void)
{
    struct timeval current_time;
	
    gettimeofday(&current_time, NULL);
	
	return current_time.tv_sec * 1000000 + current_time.tv_usec;
}

static inline double get_millisecond(void)
{
    struct timeval current_time;
	
    gettimeofday(&current_time, NULL);
	
	double time = (double)current_time.tv_sec * 1000 + (double)current_time.tv_usec / 1000;
	
	return time;
}

static inline double get_second(void)
{
    struct timeval current_time;
	
    gettimeofday(&current_time, NULL);
	
	double time = current_time.tv_sec + (double)current_time.tv_usec / 1000000;
	
	return time;
}

static inline int time_to_dsecond(double time)
{
	time_t second = time;
	struct tm tm;
	
	localtime_r(&second, &tm);
	
	int dsecond = (time - second + tm.tm_sec) * 1000;
	
	return dsecond;
}

static inline int time_to_minuteoftheyear(double time)
{
	time_t second = time;
	struct tm tm;
	
	localtime_r(&second, &tm);
	
	int minuteoftheyear = (tm.tm_yday * 24 + tm.tm_hour) * 60 + tm.tm_min;
	
	return minuteoftheyear;
}

static inline double dsecond_to_time(int dsecond)
{
	time_t second = 0;
	struct tm tm;
	
	time(&second);
	localtime_r(&second, &tm);
	
	tm.tm_sec = dsecond / 1000;
	
	second = mktime(&tm);
	
	double time = second + (double)(dsecond % 1000) / 1000;
	
	return time;
}

#ifdef __cplusplus
}
#endif

#endif
