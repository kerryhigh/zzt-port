#ifndef ZZT_RUNTIME_COMPAT_TIME_H
#define ZZT_RUNTIME_COMPAT_TIME_H

/*
 * Minimal declarations for compiling production zoo.c into the freestanding
 * WASM compatibility harness. The title-lifecycle replay injects deterministic
 * clocks through its harness hooks and never calls these host-time routines.
 */
typedef long time_t;

struct timespec {
	time_t tv_sec;
	long tv_nsec;
};

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
};

#define CLOCK_REALTIME 0

int clock_gettime(int clock_id, struct timespec *result);
struct tm *localtime_r(const time_t *time_value, struct tm *result);
time_t time(time_t *result);

#endif
