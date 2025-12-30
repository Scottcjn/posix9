/*
 * posix9/time.h - Time functions for Mac OS 9
 */

#ifndef POSIX9_TIME_H
#define POSIX9_TIME_H

#include "types.h"

/* Time types */
typedef long time_t;
typedef long clock_t;

/* struct tm */
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

/* struct timeval (also in socket.h) */
#ifndef _STRUCT_TIMEVAL
#define _STRUCT_TIMEVAL
struct timeval {
    long tv_sec;    /* Seconds */
    long tv_usec;   /* Microseconds */
};
#endif

/* struct timespec (also in pthread.h) */
#ifndef _STRUCT_TIMESPEC
#define _STRUCT_TIMESPEC
struct timespec {
    time_t tv_sec;  /* Seconds */
    long tv_nsec;   /* Nanoseconds */
};
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
