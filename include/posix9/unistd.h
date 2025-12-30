/*
 * posix9/unistd.h - Miscellaneous POSIX functions for Mac OS 9
 */

#ifndef POSIX9_UNISTD_H
#define POSIX9_UNISTD_H

#include "types.h"

/* Standard file descriptors */
#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

/* access() modes */
#define F_OK    0       /* Test existence */
#define X_OK    1       /* Test execute permission */
#define W_OK    2       /* Test write permission */
#define R_OK    4       /* Test read permission */

/* lseek() whence - also defined in posix9.h */
#ifndef SEEK_SET
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2
#endif

/* File operations (defined in posix9_file.c) */
int     close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
off_t   lseek(int fd, off_t offset, int whence);
int     dup(int oldfd);
int     dup2(int oldfd, int newfd);
int     fsync(int fd);
int     ftruncate(int fd, off_t length);

/* File system */
int     access(const char *path, int mode);
int     unlink(const char *path);
int     rmdir(const char *path);
int     chdir(const char *path);
char *  getcwd(char *buf, size_t size);
int     link(const char *oldpath, const char *newpath);
int     symlink(const char *target, const char *linkpath);
ssize_t readlink(const char *path, char *buf, size_t bufsiz);
int     chown(const char *path, uid_t owner, gid_t group);
int     fchown(int fd, uid_t owner, gid_t group);
int     chmod(const char *path, mode_t mode);
int     fchmod(int fd, mode_t mode);

/* Process */
pid_t   getpid(void);
pid_t   getppid(void);
uid_t   getuid(void);
uid_t   geteuid(void);
gid_t   getgid(void);
gid_t   getegid(void);
int     setuid(uid_t uid);
int     seteuid(uid_t uid);
int     setgid(gid_t gid);
int     setegid(gid_t gid);
int     setreuid(uid_t ruid, uid_t euid);
int     setregid(gid_t rgid, gid_t egid);
pid_t   fork(void);
pid_t   vfork(void);
int     execv(const char *path, char *const argv[]);
int     execve(const char *path, char *const argv[], char *const envp[]);
int     execvp(const char *file, char *const argv[]);
void    _exit(int status);

/* Misc */
int     gethostname(char *name, size_t len);
int     sethostname(const char *name, size_t len);
unsigned int sleep(unsigned int seconds);
int     usleep(useconds_t usec);
int     isatty(int fd);
char *  ttyname(int fd);
int     pipe(int pipefd[2]);
long    sysconf(int name);
char *  getlogin(void);
int     getlogin_r(char *buf, size_t bufsize);

/* sysconf names */
#define _SC_ARG_MAX         0
#define _SC_CHILD_MAX       1
#define _SC_CLK_TCK         2
#define _SC_NGROUPS_MAX     3
#define _SC_OPEN_MAX        4
#define _SC_STREAM_MAX      5
#define _SC_TZNAME_MAX      6
#define _SC_PAGESIZE        7
#define _SC_PAGE_SIZE       _SC_PAGESIZE

/* useconds_t */
typedef unsigned long useconds_t;

#endif /* POSIX9_UNISTD_H */
