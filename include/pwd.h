/*
 * pwd.h - Password database operations for Mac OS 9
 * OS 9 is single-user, so this is largely stubbed
 */
#ifndef _PWD_H
#define _PWD_H

#include <sys/types.h>

struct passwd {
    char    *pw_name;       /* username */
    char    *pw_passwd;     /* encrypted password */
    uid_t   pw_uid;         /* user ID */
    gid_t   pw_gid;         /* group ID */
    char    *pw_gecos;      /* real name */
    char    *pw_dir;        /* home directory */
    char    *pw_shell;      /* shell program */
};

/* Functions - returns single "root" user on OS 9 */
struct passwd *getpwnam(const char *name);
struct passwd *getpwuid(uid_t uid);
struct passwd *getpwent(void);
void setpwent(void);
void endpwent(void);

#endif /* _PWD_H */
