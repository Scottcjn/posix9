/*
 * sys/ioctl.h - Stub for Mac OS 9 (no ioctl support)
 */
#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

/* ioctl is not supported on Mac OS 9 */
#define TIOCGWINSZ 0x5413
#define TIOCSWINSZ 0x5414
#define TIOCSCTTY  0x540E

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

/* Stub ioctl - always fails */
static inline int ioctl(int fd, unsigned long request, ...) {
    (void)fd;
    (void)request;
    return -1;
}

#endif /* _SYS_IOCTL_H */
