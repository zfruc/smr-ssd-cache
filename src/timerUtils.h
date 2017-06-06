/*
    This Timer Utilities is applied to count the **micro-second** of a process block
*/
#ifndef _TIMER_UTILS_H_
#define _TIMER_UTILS_H_

#include <libio.h>
#include <sys/time.h>

typedef struct timeval timeval;

typedef __suseconds_t microsecond_t;

extern void TimerStart(timeval* tv_start);

extern void TimerStop(timeval* tv_stop);

extern microsecond_t GetTimerInterval(timeval* tv_start, timeval* tv_stop);

extern double Mirco2Sec(microsecond_t msecond);

#endif // _TIMER_UTILS_H_
