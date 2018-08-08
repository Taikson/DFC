#define _POSIX_C_SOURCE 199309L
#include "deltatime.h"

#include <time.h>
struct timespec detaltime_start, detaltime_stop;

extern void deltatime_init(){
    clock_gettime(CLOCK_MONOTONIC_RAW, &detaltime_start);
}
//printf("\r\n%lld",deltatime_get_nanoseconds());
extern uint64_t deltatime_get_nanoseconds(){
    clock_gettime(CLOCK_MONOTONIC_RAW, &detaltime_stop); 
    uint64_t diff=(uint64_t)((detaltime_stop.tv_sec - detaltime_start.tv_sec) * 1e6 + (detaltime_stop.tv_nsec - detaltime_start.tv_nsec) / 1e3);       
    detaltime_start.tv_nsec=detaltime_stop.tv_nsec;
    detaltime_start.tv_sec=detaltime_stop.tv_sec;
    return diff;
}