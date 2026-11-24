#ifndef ZZT_WINDOWS_COMPAT_H
#define ZZT_WINDOWS_COMPAT_H

#include <string.h>
#include <time.h>

/* POSIX spellings used by the shared libzoo core. */
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define strdup _strdup

static inline int zzt_clock_gettime(int clock_id, struct timespec *value) {
	(void) clock_id;
	return timespec_get(value, TIME_UTC) == TIME_UTC ? 0 : -1;
}

static inline struct tm *zzt_localtime_r(const time_t *value, struct tm *result) {
	return localtime_s(result, value) == 0 ? result : NULL;
}

#define CLOCK_REALTIME 0
#define clock_gettime zzt_clock_gettime
#define localtime_r zzt_localtime_r

#endif
