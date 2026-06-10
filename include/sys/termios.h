/*
 * sys/termios.h - Terminal I/O stub for Mac OS 9
 * Mostly stubs - we redirect termios definitions from os9_platform.h
 */
#ifndef _SYS_TERMIOS_H
#define _SYS_TERMIOS_H

/* Speed type */
typedef unsigned long speed_t;
typedef unsigned long tcflag_t;

/* Terminal control structure */
struct termios {
    tcflag_t c_iflag;   /* input modes */
    tcflag_t c_oflag;   /* output modes */
    tcflag_t c_cflag;   /* control modes */
    tcflag_t c_lflag;   /* local modes */
    unsigned char c_cc[32];  /* control characters */
    speed_t c_ispeed;   /* input speed */
    speed_t c_ospeed;   /* output speed */
};

/* c_cc indices */
#define VINTR    0
#define VQUIT    1
#define VERASE   2
#define VKILL    3
#define VEOF     4
#define VTIME    5
#define VMIN     6
#define VEOL     7
#define VEOL2    8
#define VSTART   9
#define VSTOP    10
#define VSUSP    11
#define VREPRINT 12
#define VDISCARD 13
#define VWERASE  14
#define VLNEXT   15

/* c_iflag bits */
#define IGNBRK  0x0001
#define BRKINT  0x0002
#define IGNPAR  0x0004
#define PARMRK  0x0008
#define INPCK   0x0010
#define ISTRIP  0x0020
#define INLCR   0x0040
#define IGNCR   0x0080
#define ICRNL   0x0100
#define IUCLC   0x0200
#define IXON    0x0400
#define IXANY   0x0800
#define IXOFF   0x1000
#define IMAXBEL 0x2000

/* c_oflag bits */
#define OPOST   0x0001
#define ONLCR   0x0004

/* c_cflag bits */
#define CSIZE   0x0030
#define CS5     0x0000
#define CS6     0x0010
#define CS7     0x0020
#define CS8     0x0030
#define CSTOPB  0x0040
#define CREAD   0x0080
#define PARENB  0x0100
#define PARODD  0x0200
#define HUPCL   0x0400
#define CLOCAL  0x0800
#define CRTSCTS 0x1000

/* c_lflag bits */
#define ISIG    0x0001
#define ICANON  0x0002
#define ECHO    0x0008
#define ECHOE   0x0010
#define ECHOK   0x0020
#define ECHONL  0x0040
#define NOFLSH  0x0080
#define TOSTOP  0x0100
#define IEXTEN  0x8000

/* tcsetattr actions */
#define TCSANOW     0
#define TCSADRAIN   1
#define TCSAFLUSH   2

/* tcflow actions */
#define TCOOFF  0
#define TCOON   1
#define TCIOFF  2
#define TCION   3

/* tcflush queue selectors */
#define TCIFLUSH    0
#define TCOFLUSH    1
#define TCIOFLUSH   2

/* Baud rates */
#define B0      0
#define B50     50
#define B75     75
#define B110    110
#define B134    134
#define B150    150
#define B200    200
#define B300    300
#define B600    600
#define B1200   1200
#define B1800   1800
#define B2400   2400
#define B4800   4800
#define B9600   9600
#define B19200  19200
#define B38400  38400
#define B57600  57600
#define B115200 115200
#define B230400 230400

/* Functions - stubs in os9_platform.c */
int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int actions, const struct termios *termios_p);
int tcsendbreak(int fd, int duration);
int tcdrain(int fd);
int tcflush(int fd, int queue_selector);
int tcflow(int fd, int action);
speed_t cfgetispeed(const struct termios *termios_p);
speed_t cfgetospeed(const struct termios *termios_p);
int cfsetispeed(struct termios *termios_p, speed_t speed);
int cfsetospeed(struct termios *termios_p, speed_t speed);

#endif /* _SYS_TERMIOS_H */
