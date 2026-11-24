#ifndef ZZT_TEST_TIME_H
#define ZZT_TEST_TIME_H
#include <stdint.h>
typedef long time_t;
struct timespec { time_t tv_sec; long tv_nsec; };
struct tm { int tm_sec; int tm_min; int tm_hour; };
#define CLOCK_REALTIME 0
int clock_gettime(int clock_id, struct timespec *value);
struct tm *localtime_r(const time_t *value, struct tm *result);
time_t time(time_t *result);
#endif
