/*
 * syslog.h - System logging stub for Mac OS 9
 * Redirects to file or console logging
 */
#ifndef _SYSLOG_H
#define _SYSLOG_H

/* Log priorities */
#define LOG_EMERG   0
#define LOG_ALERT   1
#define LOG_CRIT    2
#define LOG_ERR     3
#define LOG_WARNING 4
#define LOG_NOTICE  5
#define LOG_INFO    6
#define LOG_DEBUG   7

/* Log facilities */
#define LOG_KERN    (0 << 3)
#define LOG_USER    (1 << 3)
#define LOG_MAIL    (2 << 3)
#define LOG_DAEMON  (3 << 3)
#define LOG_AUTH    (4 << 3)
#define LOG_SYSLOG  (5 << 3)
#define LOG_LPR     (6 << 3)
#define LOG_NEWS    (7 << 3)
#define LOG_UUCP    (8 << 3)
#define LOG_CRON    (9 << 3)
#define LOG_AUTHPRIV (10 << 3)
#define LOG_LOCAL0  (16 << 3)
#define LOG_LOCAL1  (17 << 3)
#define LOG_LOCAL2  (18 << 3)
#define LOG_LOCAL3  (19 << 3)
#define LOG_LOCAL4  (20 << 3)
#define LOG_LOCAL5  (21 << 3)
#define LOG_LOCAL6  (22 << 3)
#define LOG_LOCAL7  (23 << 3)

/* Option flags */
#define LOG_PID     0x01
#define LOG_CONS    0x02
#define LOG_ODELAY  0x04
#define LOG_NDELAY  0x08
#define LOG_NOWAIT  0x10
#define LOG_PERROR  0x20

/* Extract priority/facility */
#define LOG_PRIMASK 0x07
#define LOG_PRI(p)  ((p) & LOG_PRIMASK)
#define LOG_FAC(p)  (((p) >> 3) & 0x1f)

/* Functions - implemented in os9_platform.c */
void openlog(const char *ident, int option, int facility);
void syslog(int priority, const char *format, ...);
void closelog(void);

#endif /* _SYSLOG_H */
