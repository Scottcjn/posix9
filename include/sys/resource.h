/*
 * sys/resource.h - Resource limits for Mac OS 9
 * Mostly stubs
 */
#ifndef _SYS_RESOURCE_H
#define _SYS_RESOURCE_H

#include <sys/types.h>

/* Resource limit type */
typedef unsigned long rlim_t;

/* Resource limit structure */
struct rlimit {
    rlim_t  rlim_cur;   /* soft limit */
    rlim_t  rlim_max;   /* hard limit */
};

/* Resource types */
#define RLIMIT_CPU      0   /* CPU time */
#define RLIMIT_FSIZE    1   /* file size */
#define RLIMIT_DATA     2   /* data segment size */
#define RLIMIT_STACK    3   /* stack size */
#define RLIMIT_CORE     4   /* core file size */
#define RLIMIT_RSS      5   /* resident set size */
#define RLIMIT_MEMLOCK  6   /* locked memory */
#define RLIMIT_NPROC    7   /* number of processes */
#define RLIMIT_NOFILE   8   /* number of open files */

/* Special values */
#define RLIM_INFINITY   (~(rlim_t)0)
#define RLIM_SAVED_MAX  RLIM_INFINITY
#define RLIM_SAVED_CUR  RLIM_INFINITY

/* Resource usage structure */
struct rusage {
    struct timeval ru_utime;    /* user time used */
    struct timeval ru_stime;    /* system time used */
    long    ru_maxrss;          /* max resident set size */
    long    ru_ixrss;           /* shared memory size */
    long    ru_idrss;           /* unshared data size */
    long    ru_isrss;           /* unshared stack size */
    long    ru_minflt;          /* page reclaims */
    long    ru_majflt;          /* page faults */
    long    ru_nswap;           /* swaps */
    long    ru_inblock;         /* block input operations */
    long    ru_oublock;         /* block output operations */
    long    ru_msgsnd;          /* messages sent */
    long    ru_msgrcv;          /* messages received */
    long    ru_nsignals;        /* signals received */
    long    ru_nvcsw;           /* voluntary context switches */
    long    ru_nivcsw;          /* involuntary context switches */
};

/* rusage who */
#define RUSAGE_SELF     0
#define RUSAGE_CHILDREN (-1)

/* Functions - stubs */
int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);
int getrusage(int who, struct rusage *usage);

#endif /* _SYS_RESOURCE_H */
