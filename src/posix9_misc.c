/*
 * posix9_misc.c - Miscellaneous POSIX functions for Mac OS 9
 *
 * Environment, time, memory, and other utilities.
 */

#include "posix9.h"
#include "posix9/unistd.h"

#include <Files.h>
#include <Gestalt.h>
#include <LowMem.h>
#include <DateTimeUtils.h>
#include <Timer.h>
#include <Processes.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================
 * Environment Variables
 * ============================================================ */

#define MAX_ENV_VARS 128
#define MAX_ENV_SIZE 4096

static char *env_vars[MAX_ENV_VARS + 1];  /* NULL terminated */
static char env_storage[MAX_ENV_SIZE];
static size_t env_used = 0;
static Boolean env_initialized = false;

/* Default environment */
static const char *default_env[] = {
    "HOME=/",
    "PATH=/bin:/usr/bin",
    "SHELL=/bin/sh",
    "USER=root",
    "TERM=vt100",
    "LANG=en_US",
    NULL
};

static void init_environment(void)
{
    int i;
    const char *p;
    size_t len;

    if (env_initialized) return;

    memset(env_vars, 0, sizeof(env_vars));
    env_used = 0;

    /* Copy default environment */
    for (i = 0; default_env[i] != NULL && i < MAX_ENV_VARS; i++) {
        p = default_env[i];
        len = strlen(p) + 1;

        if (env_used + len < MAX_ENV_SIZE) {
            strcpy(&env_storage[env_used], p);
            env_vars[i] = &env_storage[env_used];
            env_used += len;
        }
    }

    env_initialized = true;
}

char *getenv(const char *name)
{
    int i;
    size_t namelen;

    init_environment();

    if (!name) return NULL;
    namelen = strlen(name);

    for (i = 0; env_vars[i] != NULL; i++) {
        if (strncmp(env_vars[i], name, namelen) == 0 &&
            env_vars[i][namelen] == '=') {
            return &env_vars[i][namelen + 1];
        }
    }

    return NULL;
}

int setenv(const char *name, const char *value, int overwrite)
{
    int i;
    size_t namelen, vallen, totallen;
    char *newvar;

    init_environment();

    if (!name || name[0] == '\0' || strchr(name, '=') != NULL) {
        errno = EINVAL;
        return -1;
    }

    namelen = strlen(name);
    vallen = value ? strlen(value) : 0;
    totallen = namelen + 1 + vallen + 1;  /* name=value\0 */

    /* Check if already exists */
    for (i = 0; env_vars[i] != NULL; i++) {
        if (strncmp(env_vars[i], name, namelen) == 0 &&
            env_vars[i][namelen] == '=') {
            if (!overwrite) return 0;

            /* Replace - need space in storage */
            if (env_used + totallen >= MAX_ENV_SIZE) {
                errno = ENOMEM;
                return -1;
            }

            newvar = &env_storage[env_used];
            sprintf(newvar, "%s=%s", name, value ? value : "");
            env_vars[i] = newvar;
            env_used += totallen;
            return 0;
        }
    }

    /* Add new variable */
    if (i >= MAX_ENV_VARS - 1) {
        errno = ENOMEM;
        return -1;
    }

    if (env_used + totallen >= MAX_ENV_SIZE) {
        errno = ENOMEM;
        return -1;
    }

    newvar = &env_storage[env_used];
    sprintf(newvar, "%s=%s", name, value ? value : "");
    env_vars[i] = newvar;
    env_vars[i + 1] = NULL;
    env_used += totallen;

    return 0;
}

int unsetenv(const char *name)
{
    int i, j;
    size_t namelen;

    init_environment();

    if (!name || name[0] == '\0' || strchr(name, '=') != NULL) {
        errno = EINVAL;
        return -1;
    }

    namelen = strlen(name);

    for (i = 0; env_vars[i] != NULL; i++) {
        if (strncmp(env_vars[i], name, namelen) == 0 &&
            env_vars[i][namelen] == '=') {
            /* Shift remaining vars down */
            for (j = i; env_vars[j] != NULL; j++) {
                env_vars[j] = env_vars[j + 1];
            }
            return 0;
        }
    }

    return 0;  /* Not found is OK */
}

int putenv(char *string)
{
    char *eq;
    char name[256];
    size_t namelen;

    if (!string) {
        errno = EINVAL;
        return -1;
    }

    eq = strchr(string, '=');
    if (!eq) {
        errno = EINVAL;
        return -1;
    }

    namelen = eq - string;
    if (namelen >= sizeof(name)) {
        errno = EINVAL;
        return -1;
    }

    memcpy(name, string, namelen);
    name[namelen] = '\0';

    return setenv(name, eq + 1, 1);
}

/* Provide environ for programs that need it */
char **environ = NULL;

/* ============================================================
 * Time Functions
 * ============================================================ */

/* Mac OS epoch: Jan 1, 1904
   Unix epoch: Jan 1, 1970
   Difference: 2082844800 seconds */
#define MAC_TO_UNIX_OFFSET 2082844800UL

time_t time(time_t *tloc)
{
    unsigned long secs;
    time_t result;

    GetDateTime(&secs);
    result = (time_t)(secs - MAC_TO_UNIX_OFFSET);

    if (tloc) {
        *tloc = result;
    }

    return result;
}

int gettimeofday(struct timeval *tv, void *tz)
{
    UnsignedWide microTickCount;
    unsigned long secs;

    (void)tz;  /* Timezone not supported */

    if (!tv) {
        errno = EINVAL;
        return -1;
    }

    GetDateTime(&secs);
    tv->tv_sec = secs - MAC_TO_UNIX_OFFSET;

    /* Get microseconds from Microseconds() trap */
    Microseconds(&microTickCount);
    tv->tv_usec = microTickCount.lo % 1000000;

    return 0;
}

struct tm *localtime(const time_t *timep)
{
    static struct tm result;
    DateTimeRec dt;
    unsigned long macTime;

    if (!timep) return NULL;

    macTime = *timep + MAC_TO_UNIX_OFFSET;
    SecondsToDate(macTime, &dt);

    result.tm_sec = dt.second;
    result.tm_min = dt.minute;
    result.tm_hour = dt.hour;
    result.tm_mday = dt.day;
    result.tm_mon = dt.month - 1;       /* tm_mon is 0-11 */
    result.tm_year = dt.year - 1900;    /* tm_year is years since 1900 */
    result.tm_wday = dt.dayOfWeek - 1;  /* tm_wday is 0-6, Sun=0 */
    result.tm_yday = 0;                 /* TODO: calculate */
    result.tm_isdst = 0;                /* No DST info on classic Mac */

    return &result;
}

struct tm *gmtime(const time_t *timep)
{
    /* Mac OS 9 doesn't have good timezone support */
    /* Just return localtime for now */
    return localtime(timep);
}

time_t mktime(struct tm *tm)
{
    DateTimeRec dt;
    unsigned long secs;

    if (!tm) return -1;

    dt.second = tm->tm_sec;
    dt.minute = tm->tm_min;
    dt.hour = tm->tm_hour;
    dt.day = tm->tm_mday;
    dt.month = tm->tm_mon + 1;
    dt.year = tm->tm_year + 1900;
    dt.dayOfWeek = tm->tm_wday + 1;

    DateToSeconds(&dt, &secs);

    return (time_t)(secs - MAC_TO_UNIX_OFFSET);
}

char *ctime(const time_t *timep)
{
    static char buf[26];
    struct tm *tm;
    static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    tm = localtime(timep);
    if (!tm) return NULL;

    sprintf(buf, "%.3s %.3s %2d %02d:%02d:%02d %d\n",
            days[tm->tm_wday], months[tm->tm_mon],
            tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
            tm->tm_year + 1900);

    return buf;
}

size_t strftime(char *s, size_t max, const char *format, const struct tm *tm)
{
    /* Simplified strftime - just handle common formats */
    static const char *days[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
                                 "Thursday", "Friday", "Saturday"};
    static const char *months[] = {"January", "February", "March", "April",
                                   "May", "June", "July", "August",
                                   "September", "October", "November", "December"};
    char *out = s;
    char *end = s + max - 1;
    const char *p = format;
    char temp[32];

    while (*p && out < end) {
        if (*p != '%') {
            *out++ = *p++;
            continue;
        }

        p++;  /* Skip % */

        switch (*p) {
            case 'Y':
                sprintf(temp, "%04d", tm->tm_year + 1900);
                break;
            case 'y':
                sprintf(temp, "%02d", tm->tm_year % 100);
                break;
            case 'm':
                sprintf(temp, "%02d", tm->tm_mon + 1);
                break;
            case 'd':
                sprintf(temp, "%02d", tm->tm_mday);
                break;
            case 'H':
                sprintf(temp, "%02d", tm->tm_hour);
                break;
            case 'M':
                sprintf(temp, "%02d", tm->tm_min);
                break;
            case 'S':
                sprintf(temp, "%02d", tm->tm_sec);
                break;
            case 'A':
                strcpy(temp, days[tm->tm_wday]);
                break;
            case 'a':
                strncpy(temp, days[tm->tm_wday], 3);
                temp[3] = '\0';
                break;
            case 'B':
                strcpy(temp, months[tm->tm_mon]);
                break;
            case 'b':
                strncpy(temp, months[tm->tm_mon], 3);
                temp[3] = '\0';
                break;
            case '%':
                temp[0] = '%';
                temp[1] = '\0';
                break;
            default:
                temp[0] = '%';
                temp[1] = *p;
                temp[2] = '\0';
                break;
        }

        {
            char *t = temp;
            while (*t && out < end) {
                *out++ = *t++;
            }
        }

        p++;
    }

    *out = '\0';
    return out - s;
}

/* ============================================================
 * Sleep Functions
 * ============================================================ */

unsigned int sleep(unsigned int seconds)
{
    unsigned long endTicks;
    unsigned long now;

    endTicks = TickCount() + (seconds * 60);

    do {
        SystemTask();
        posix9_signal_process();
        YieldToAnyThread();
        now = TickCount();
    } while (now < endTicks && !posix9_signal_pending(0));

    if (now < endTicks) {
        return (endTicks - now) / 60;
    }

    return 0;
}

int usleep(useconds_t usec)
{
    unsigned long endTicks;
    unsigned long now;
    unsigned long ticks;

    /* Convert microseconds to ticks (1 tick = 1/60 second) */
    ticks = (usec * 60) / 1000000;
    if (ticks == 0 && usec > 0) ticks = 1;

    endTicks = TickCount() + ticks;

    do {
        SystemTask();
        now = TickCount();
    } while (now < endTicks);

    return 0;
}

/* ============================================================
 * TTY Functions
 * ============================================================ */

int isatty(int fd)
{
    /* stdin/stdout/stderr are always TTYs on Mac OS 9 */
    if (fd >= 0 && fd <= 2) {
        return 1;
    }
    return 0;
}

char *ttyname(int fd)
{
    static char tty[] = "/dev/console";

    if (!isatty(fd)) {
        errno = ENOTTY;
        return NULL;
    }

    return tty;
}

/* ============================================================
 * User/Login Functions
 * ============================================================ */

char *getlogin(void)
{
    static char name[] = "root";
    return name;
}

int getlogin_r(char *buf, size_t bufsize)
{
    if (bufsize < 5) {
        return ERANGE;
    }
    strcpy(buf, "root");
    return 0;
}

/* UID/GID stubs - always root on Mac OS 9 */
int setuid(uid_t uid)
{
    (void)uid;
    return 0;
}

int seteuid(uid_t uid)
{
    (void)uid;
    return 0;
}

int setgid(gid_t gid)
{
    (void)gid;
    return 0;
}

int setegid(gid_t gid)
{
    (void)gid;
    return 0;
}

int setreuid(uid_t ruid, uid_t euid)
{
    (void)ruid;
    (void)euid;
    return 0;
}

int setregid(gid_t rgid, gid_t egid)
{
    (void)rgid;
    (void)egid;
    return 0;
}

/* ============================================================
 * File System Stubs
 * ============================================================ */

int access(const char *path, int mode)
{
    struct stat st;

    if (stat(path, &st) != 0) {
        return -1;
    }

    /* Mac OS 9 doesn't have real permissions */
    /* Just check if file exists for all modes */
    (void)mode;

    return 0;
}

int link(const char *oldpath, const char *newpath)
{
    /* Mac OS 9 doesn't support hard links */
    (void)oldpath;
    (void)newpath;
    errno = ENOSYS;
    return -1;
}

int symlink(const char *target, const char *linkpath)
{
    /* Mac OS 9 has aliases, but they're not POSIX symlinks */
    (void)target;
    (void)linkpath;
    errno = ENOSYS;
    return -1;
}

ssize_t readlink(const char *path, char *buf, size_t bufsiz)
{
    /* No symlinks on Mac OS 9 */
    (void)path;
    (void)buf;
    (void)bufsiz;
    errno = EINVAL;
    return -1;
}

int chown(const char *path, uid_t owner, gid_t group)
{
    /* No ownership on Mac OS 9 */
    (void)path;
    (void)owner;
    (void)group;
    return 0;
}

int fchown(int fd, uid_t owner, gid_t group)
{
    (void)fd;
    (void)owner;
    (void)group;
    return 0;
}

int chmod(const char *path, mode_t mode)
{
    /* No permissions on Mac OS 9 */
    (void)path;
    (void)mode;
    return 0;
}

int fchmod(int fd, mode_t mode)
{
    (void)fd;
    (void)mode;
    return 0;
}

/* ============================================================
 * Pipe (stub - not really supported)
 * ============================================================ */

int pipe(int pipefd[2])
{
    /* Could implement with temp files, but for now... */
    (void)pipefd;
    errno = ENOSYS;
    return -1;
}

/* ============================================================
 * sysconf
 * ============================================================ */

long sysconf(int name)
{
    switch (name) {
        case _SC_ARG_MAX:
            return 65536;
        case _SC_CHILD_MAX:
            return 1;  /* No child processes */
        case _SC_CLK_TCK:
            return 60;  /* Ticks per second */
        case _SC_NGROUPS_MAX:
            return 0;
        case _SC_OPEN_MAX:
            return 256;
        case _SC_STREAM_MAX:
            return 256;
        case _SC_TZNAME_MAX:
            return 8;
        case _SC_PAGESIZE:
            return 4096;
        default:
            errno = EINVAL;
            return -1;
    }
}

/* ============================================================
 * Exit
 * ============================================================ */

void _exit(int status)
{
    (void)status;
    ExitToShell();
}

/* Also provide exit() */
void exit(int status)
{
    _exit(status);
}

/* ============================================================
 * vfork (same as fork - returns error)
 * ============================================================ */

pid_t vfork(void)
{
    errno = ENOSYS;
    return -1;
}

/* ============================================================
 * Random Numbers
 * ============================================================ */

static unsigned long random_seed = 1;

void srandom(unsigned int seed)
{
    random_seed = seed;
}

long random(void)
{
    /* Simple LCG */
    random_seed = random_seed * 1103515245 + 12345;
    return (random_seed >> 16) & 0x7FFFFFFF;
}

void srand(unsigned int seed)
{
    srandom(seed);
}

int rand(void)
{
    return (int)(random() % ((unsigned long)RAND_MAX + 1));
}

/* For arc4random, use system entropy */
unsigned int arc4random(void)
{
    UnsignedWide us;
    Microseconds(&us);
    random_seed ^= us.lo ^ (us.hi << 16);
    return (unsigned int)random();
}

void arc4random_buf(void *buf, size_t n)
{
    unsigned char *p = (unsigned char *)buf;
    size_t i;

    for (i = 0; i < n; i++) {
        p[i] = (unsigned char)(arc4random() & 0xFF);
    }
}

/* ============================================================
 * Memory Functions
 * ============================================================ */

void *mmap(void *addr, size_t length, int prot, int flags,
           int fd, off_t offset)
{
    /* No mmap on Mac OS 9 - allocate memory instead */
    void *ptr;

    (void)addr;
    (void)prot;
    (void)flags;
    (void)fd;
    (void)offset;

    ptr = NewPtr(length);
    if (!ptr) {
        errno = ENOMEM;
        return (void *)-1;
    }

    return ptr;
}

int munmap(void *addr, size_t length)
{
    (void)length;
    DisposePtr((Ptr)addr);
    return 0;
}

/* mmap constants */
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4
#define PROT_NONE   0x0

#define MAP_SHARED      0x01
#define MAP_PRIVATE     0x02
#define MAP_ANONYMOUS   0x20
#define MAP_ANON        MAP_ANONYMOUS
#define MAP_FAILED      ((void *)-1)
