/*
 * posix9/time.h - Time functions for Mac OS 9
 *
 * Note: When compiling with Retro68/newlib, time types may already
 * be defined. We use guards to avoid conflicts.
 */

#ifndef POSIX9_TIME_H
#define POSIX9_TIME_H

#include "types.h"

/* Time types - use system types if available */
#ifndef _TIME_T_DECLARED
#ifndef time_t
typedef long time_t;
#endif
#endif

#ifndef _CLOCK_T_DECLARED
#ifndef clock_t
typedef long clock_t;
#endif
#endif

/* struct tm - only define if not in system headers */
#ifndef _TM_DEFINED
#ifndef __struct_tm_defined
struct tm {
    int tm_sec;     /* Seconds (0-60) */
    int tm_min;     /* Minutes (0-59) */
    int tm_hour;    /* Hours (0-23) */
    int tm_mday;    /* Day of month (1-31) */
    int tm_mon;     /* Month (0-11) */
    int tm_year;    /* Years since 1900 */
    int tm_wday;    /* Day of week (0-6, Sunday = 0) */
    int tm_yday;    /* Day of year (0-365) */
    int tm_isdst;   /* Daylight saving time flag */
};
#define __struct_tm_defined 1
#endif
#endif

/* struct timeval - check multiple guards */
#ifndef _STRUCT_TIMEVAL
#ifndef _TIMEVAL_DEFINED
struct timeval {
    long tv_sec;    /* Seconds */
    long tv_usec;   /* Microseconds */
};
#define _STRUCT_TIMEVAL 1
#define _TIMEVAL_DEFINED 1
#endif
#endif

/* struct timespec - check multiple guards */
#ifndef _STRUCT_TIMESPEC
#ifndef _TIMESPEC_DEFINED
struct timespec {
    time_t tv_sec;  /* Seconds */
    long tv_nsec;   /* Nanoseconds */
};
#define _STRUCT_TIMESPEC 1
#define _TIMESPEC_DEFINED 1
#endif
#endif

/* struct timezone */
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

/* Clocks per second */
#define CLOCKS_PER_SEC  60

/* Time functions */
time_t      time(time_t *tloc);
struct tm * localtime(const time_t *timep);
struct tm * gmtime(const time_t *timep);
time_t      mktime(struct tm *tm);
char *      ctime(const time_t *timep);
char *      asctime(const struct tm *tm);
size_t      strftime(char *s, size_t max, const char *format, const struct tm *tm);
clock_t     clock(void);
double      difftime(time_t time1, time_t time0);

/* gettimeofday */
int gettimeofday(struct timeval *tv, void *tz);

/* nanosleep */
int nanosleep(const struct timespec *req, struct timespec *rem);

#endif /* POSIX9_TIME_H */
