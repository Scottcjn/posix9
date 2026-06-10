/*
 * grp.h - Group database operations for Mac OS 9
 * OS 9 is single-user, so this is largely stubbed
 */
#ifndef _GRP_H
#define _GRP_H

#include <sys/types.h>

struct group {
    char    *gr_name;       /* group name */
    char    *gr_passwd;     /* group password */
    gid_t   gr_gid;         /* group ID */
    char    **gr_mem;       /* group members */
};

/* Functions - returns single "wheel" group on OS 9 */
struct group *getgrnam(const char *name);
struct group *getgrgid(gid_t gid);
struct group *getgrent(void);
void setgrent(void);
void endgrent(void);

#endif /* _GRP_H */
