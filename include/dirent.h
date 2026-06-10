/*
 * dirent.h - Directory operations for Mac OS 9
 * This replaces Retro68's broken dirent.h
 */
#ifndef _DIRENT_H
#define _DIRENT_H

#include <sys/types.h>

/* Directory entry structure */
struct dirent {
    ino_t   d_ino;          /* inode number */
    char    d_name[256];    /* filename */
};

/* Opaque directory stream type */
typedef struct posix9_dir DIR;

/* Directory functions - implemented in posix9_dir.c */
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
void rewinddir(DIR *dirp);

/* Not commonly used, may be stubs */
int dirfd(DIR *dirp);
void seekdir(DIR *dirp, long loc);
long telldir(DIR *dirp);

#endif /* _DIRENT_H */
