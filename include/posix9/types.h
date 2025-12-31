/*
 * posix9/types.h - POSIX type definitions for Mac OS 9
 * Part of the POSIX9 compatibility layer
 *
 * Note: When compiling with Retro68/newlib, many types are already
 * defined in system headers. We use guards to avoid conflicts.
 */

#ifndef POSIX9_TYPES_H
#define POSIX9_TYPES_H

#include <MacTypes.h>

/* Try to include system types first if available */
#ifdef __NEWLIB__
#include <sys/types.h>
#endif

/* Standard POSIX types - only define if not already defined */
#ifndef _SSIZE_T_DECLARED
#ifndef ssize_t
typedef long            ssize_t;
#endif
#endif

/* size_t is always provided by the compiler */

#ifndef _OFF_T_DECLARED
#ifndef off_t
typedef long            off_t;
#endif
#endif

#ifndef _MODE_T_DECLARED
#ifndef mode_t
typedef unsigned long   mode_t;  /* Match newlib */
#endif
#endif

#ifndef _PID_T_DECLARED
#ifndef pid_t
typedef int             pid_t;   /* Match newlib */
#endif
#endif

#ifndef _INO_T_DECLARED
#ifndef ino_t
typedef unsigned short  ino_t;   /* Match newlib */
#endif
#endif

#ifndef dev_t
typedef short           dev_t;
#endif

#ifndef nlink_t
typedef unsigned short  nlink_t;
#endif

#ifndef _UID_T_DECLARED
#ifndef uid_t
typedef unsigned short  uid_t;
#endif
#endif

#ifndef _GID_T_DECLARED
#ifndef gid_t
typedef unsigned short  gid_t;
#endif
#endif

#ifndef _TIME_T_DECLARED
#ifndef time_t
typedef long            time_t;
#endif
#endif

#ifndef _USECONDS_T_DECLARED
#ifndef useconds_t
typedef unsigned long   useconds_t;     /* Microseconds for usleep() */
#endif
#endif

#ifndef blksize_t
typedef long            blksize_t;
#endif

#ifndef blkcnt_t
typedef long            blkcnt_t;
#endif

/* File descriptor type - maps to Mac OS refNum internally */
typedef short           posix9_fd_t;

/* Maximum path length */
#define POSIX9_PATH_MAX     1024
#define POSIX9_NAME_MAX     255

/* Maximum open files */
#define POSIX9_OPEN_MAX     256

/* File type flags for mode_t */
#define S_IFMT      0170000     /* file type mask */
#define S_IFREG     0100000     /* regular file */
#define S_IFDIR     0040000     /* directory */
#define S_IFCHR     0020000     /* character device */
#define S_IFBLK     0060000     /* block device */
#define S_IFIFO     0010000     /* FIFO */
#define S_IFLNK     0120000     /* symbolic link */
#define S_IFSOCK    0140000     /* socket */

/* File permission bits */
#define S_IRWXU     0000700     /* owner rwx */
#define S_IRUSR     0000400     /* owner r */
#define S_IWUSR     0000200     /* owner w */
#define S_IXUSR     0000100     /* owner x */
#define S_IRWXG     0000070     /* group rwx */
#define S_IRGRP     0000040     /* group r */
#define S_IWGRP     0000020     /* group w */
#define S_IXGRP     0000010     /* group x */
#define S_IRWXO     0000007     /* other rwx */
#define S_IROTH     0000004     /* other r */
#define S_IWOTH     0000002     /* other w */
#define S_IXOTH     0000001     /* other x */

/* File type test macros */
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

/* struct stat - file information */
struct stat {
    dev_t       st_dev;         /* device ID */
    ino_t       st_ino;         /* inode number (parID + name hash) */
    mode_t      st_mode;        /* file mode */
    nlink_t     st_nlink;       /* number of hard links (always 1) */
    uid_t       st_uid;         /* owner user ID (always 0) */
    gid_t       st_gid;         /* owner group ID (always 0) */
    dev_t       st_rdev;        /* device ID (if special file) */
    off_t       st_size;        /* file size in bytes */
    time_t      st_atime;       /* last access time */
    time_t      st_mtime;       /* last modification time */
    time_t      st_ctime;       /* last status change time */
    blksize_t   st_blksize;     /* optimal I/O block size */
    blkcnt_t    st_blocks;      /* number of 512-byte blocks */
};

/* struct dirent - directory entry */
struct dirent {
    ino_t       d_ino;          /* inode number */
    char        d_name[POSIX9_NAME_MAX + 1];  /* filename */
};

/* DIR - directory stream (opaque) */
typedef struct posix9_dir DIR;

#endif /* POSIX9_TYPES_H */
