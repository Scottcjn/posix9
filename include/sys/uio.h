/*
 * sys/uio.h - I/O vector operations for Mac OS 9
 */
#ifndef _SYS_UIO_H
#define _SYS_UIO_H

#include <sys/types.h>

/* I/O vector structure */
struct iovec {
    void    *iov_base;  /* base address */
    size_t  iov_len;    /* length */
};

/* Maximum number of I/O vectors */
#define IOV_MAX 1024
#define UIO_MAXIOV IOV_MAX

/* Functions - stubs on OS 9 */
ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

#endif /* _SYS_UIO_H */
