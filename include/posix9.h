/*
 * posix9.h - Main header for POSIX9 compatibility layer
 *
 * Provides POSIX-compatible APIs on Mac OS 9 by mapping to Toolbox calls.
 * Include this header instead of standard POSIX headers.
 */

#ifndef POSIX9_H
#define POSIX9_H

#include "posix9/types.h"
#include "posix9/errno.h"
#include "posix9/socket.h"
#include "posix9/pthread.h"
#include "posix9/signal.h"
#include "posix9/time.h"
#include "posix9/unistd.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * File I/O Operations (posix9_file.c)
 * ============================================================ */

/* open() flags */
#define O_RDONLY    0x0000      /* read only */
#define O_WRONLY    0x0001      /* write only */
#define O_RDWR      0x0002      /* read/write */
#define O_APPEND    0x0008      /* append on write */
#define O_CREAT     0x0200      /* create if nonexistent */
#define O_TRUNC     0x0400      /* truncate to zero length */
#define O_EXCL      0x0800      /* error if already exists */
#define O_NONBLOCK  0x0004      /* non-blocking I/O */

/* lseek() whence values */
#define SEEK_SET    0           /* from beginning */
#define SEEK_CUR    1           /* from current position */
#define SEEK_END    2           /* from end */

/* File operations */
int     open(const char *path, int flags, ...);
int     close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
off_t   lseek(int fd, off_t offset, int whence);
int     fstat(int fd, struct stat *buf);
int     stat(const char *path, struct stat *buf);
int     lstat(const char *path, struct stat *buf);  /* no symlinks on OS9, same as stat */
int     unlink(const char *path);
int     rename(const char *oldpath, const char *newpath);
int     fsync(int fd);
int     ftruncate(int fd, off_t length);
int     dup(int oldfd);
int     dup2(int oldfd, int newfd);

/* ============================================================
 * Directory Operations (posix9_dir.c)
 * ============================================================ */

DIR *   opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int     closedir(DIR *dirp);
void    rewinddir(DIR *dirp);
int     mkdir(const char *path, mode_t mode);
int     rmdir(const char *path);
int     chdir(const char *path);
char *  getcwd(char *buf, size_t size);

/* ============================================================
 * Path Translation (posix9_path.c)
 * ============================================================ */

/*
 * Convert POSIX path to Mac OS 9 path
 * Input:  "/Volumes/Macintosh HD/Users/scott/file.txt"
 * Output: "Macintosh HD:Users:scott:file.txt"
 *
 * Returns pointer to static buffer or dst if provided
 */
char *  posix9_path_to_mac(const char *posix_path, char *dst, size_t dst_size);

/*
 * Convert Mac OS 9 path to POSIX path
 * Input:  "Macintosh HD:Users:scott:file.txt"
 * Output: "/Volumes/Macintosh HD/Users/scott/file.txt"
 */
char *  posix9_path_from_mac(const char *mac_path, char *dst, size_t dst_size);

/*
 * Get FSSpec from POSIX path
 */
OSErr   posix9_path_to_fsspec(const char *path, FSSpec *spec);

/* ============================================================
 * Process Stubs (posix9_process.c)
 * Note: Mac OS 9 has no real process model
 * ============================================================ */

pid_t   getpid(void);           /* Returns 1 (single process) */
pid_t   getppid(void);          /* Returns 0 */
uid_t   getuid(void);           /* Returns 0 */
uid_t   geteuid(void);          /* Returns 0 */
gid_t   getgid(void);           /* Returns 0 */
gid_t   getegid(void);          /* Returns 0 */

/* fork() - NOT SUPPORTED, returns -1 with ENOSYS */
pid_t   fork(void);

/* exec family - launches as separate app, does not replace */
int     execv(const char *path, char *const argv[]);
int     execve(const char *path, char *const argv[], char *const envp[]);
int     execvp(const char *file, char *const argv[]);

/* ============================================================
 * Standard I/O file descriptors
 * ============================================================ */

#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

/* ============================================================
 * Initialization
 * ============================================================ */

/*
 * Initialize POSIX9 layer - call once at startup
 * Sets up stdin/stdout/stderr mappings and internal tables
 */
int     posix9_init(void);

/*
 * Cleanup POSIX9 layer - call at shutdown
 */
void    posix9_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* POSIX9_H */
