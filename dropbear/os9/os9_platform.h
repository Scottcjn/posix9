/*
 * os9_platform.h - Mac OS 9 platform support for Dropbear
 *
 * Provides OS 9 specific implementations for Dropbear requirements.
 */

#ifndef OS9_PLATFORM_H
#define OS9_PLATFORM_H

#include <posix9.h>

/* ============================================================
 * Missing Types and Constants
 * ============================================================ */

/* socklen_t if not defined */
#ifndef _SOCKLEN_T
#define _SOCKLEN_T
typedef unsigned int socklen_t;
#endif

/* size_t max */
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

/* PATH_MAX */
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

/* Null device */
#define _PATH_DEVNULL "/dev/null"

/* ============================================================
 * User/Password Structures
 * ============================================================ */

struct passwd {
    char    *pw_name;       /* Username */
    char    *pw_passwd;     /* Password (or "x") */
    uid_t   pw_uid;         /* User ID */
    gid_t   pw_gid;         /* Group ID */
    char    *pw_gecos;      /* Real name */
    char    *pw_dir;        /* Home directory */
    char    *pw_shell;      /* Shell */
};

struct group {
    char    *gr_name;       /* Group name */
    char    *gr_passwd;     /* Group password */
    gid_t   gr_gid;         /* Group ID */
    char    **gr_mem;       /* Group members */
};

/* User functions */
struct passwd *getpwnam(const char *name);
struct passwd *getpwuid(uid_t uid);
struct group *getgrnam(const char *name);
struct group *getgrgid(gid_t gid);
void endpwent(void);

/* ============================================================
 * Entropy Source
 * ============================================================ */

/*
 * Get random bytes for cryptographic use.
 * Uses posix9's arc4random_buf which sources from Mac OS entropy.
 */
int os9_get_random_bytes(void *buf, size_t len);

/* ============================================================
 * Shell Execution
 * ============================================================ */

/*
 * Execute a shell command.
 * On OS 9, this launches MPW Shell or ToolServer.
 */
int os9_exec_shell(const char *shell, const char *command);

/*
 * Get the default shell path.
 */
const char *os9_get_default_shell(void);

/* ============================================================
 * Password Verification
 * ============================================================ */

/*
 * Verify a user's password.
 * Returns 1 if valid, 0 if invalid.
 *
 * On OS 9, we use a simple password file or always accept.
 */
int os9_verify_password(const char *username, const char *password);

/* ============================================================
 * Host Key Storage
 * ============================================================ */

/*
 * Get the path to host key files.
 * Returns Mac-style path to Preferences folder.
 */
const char *os9_get_hostkey_path(const char *keytype);

/*
 * Get the path to authorized_keys file.
 */
const char *os9_get_authorized_keys_path(const char *username);

/* ============================================================
 * PTY Emulation
 * ============================================================ */

/*
 * Since OS 9 has no PTYs, we emulate with a simple pipe-like
 * structure for stdin/stdout.
 */
typedef struct {
    int master_read;    /* Master reads from this */
    int master_write;   /* Master writes to this */
    int slave_read;     /* Slave reads from this */
    int slave_write;    /* Slave writes to this */
} os9_pty_t;

int os9_pty_open(os9_pty_t *pty);
int os9_pty_close(os9_pty_t *pty);
int os9_pty_get_name(os9_pty_t *pty, char *buf, size_t len);

/* ============================================================
 * Signal Handling
 * ============================================================ */

/* Poll for signals - call periodically */
#define os9_check_signals() posix9_signal_process()

/* ============================================================
 * Process Management Stubs
 * ============================================================ */

/* These all return appropriate values for single-process OS */
pid_t waitpid(pid_t pid, int *status, int options);
pid_t wait(int *status);

/* Wait options */
#define WNOHANG 1
#define WUNTRACED 2

/* Status macros */
#define WIFEXITED(s)    (((s) & 0x7F) == 0)
#define WEXITSTATUS(s)  (((s) >> 8) & 0xFF)
#define WIFSIGNALED(s)  (((s) & 0x7F) != 0 && ((s) & 0x7F) != 0x7F)
#define WTERMSIG(s)     ((s) & 0x7F)
#define WIFSTOPPED(s)   (((s) & 0xFF) == 0x7F)
#define WSTOPSIG(s)     (((s) >> 8) & 0xFF)

/* ============================================================
 * Syslog Replacement
 * ============================================================ */

/* Log to console or file instead of syslog */
#define LOG_EMERG   0
#define LOG_ALERT   1
#define LOG_CRIT    2
#define LOG_ERR     3
#define LOG_WARNING 4
#define LOG_NOTICE  5
#define LOG_INFO    6
#define LOG_DEBUG   7

#define LOG_DAEMON  (3 << 3)
#define LOG_AUTH    (4 << 3)
#define LOG_LOCAL0  (16 << 3)
#define LOG_PID     0x01
#define LOG_NDELAY  0x08

void openlog(const char *ident, int option, int facility);
void syslog(int priority, const char *format, ...);
void closelog(void);

/* ============================================================
 * Network Utilities
 * ============================================================ */

/* Get peer credentials (always returns root on OS 9) */
int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

/* fcntl for non-blocking (mapped to OT calls) */
#define F_GETFL     3
#define F_SETFL     4
#define O_NONBLOCK  0x0004

int fcntl(int fd, int cmd, ...);

/* ============================================================
 * Terminal Control (Stubs)
 * ============================================================ */

struct termios {
    unsigned long c_iflag;
    unsigned long c_oflag;
    unsigned long c_cflag;
    unsigned long c_lflag;
    unsigned char c_cc[32];
    speed_t c_ispeed;
    speed_t c_ospeed;
};

typedef unsigned long speed_t;
typedef unsigned long tcflag_t;

/* c_cc indices */
#define VINTR    0
#define VQUIT    1
#define VERASE   2
#define VKILL    3
#define VEOF     4
#define VTIME    5
#define VMIN     6

/* c_iflag */
#define IGNBRK  0x0001
#define BRKINT  0x0002
#define IGNPAR  0x0004
#define INPCK   0x0010
#define ISTRIP  0x0020
#define ICRNL   0x0100
#define IXON    0x0400

/* c_oflag */
#define OPOST   0x0001
#define ONLCR   0x0004

/* c_cflag */
#define CSIZE   0x0030
#define CS8     0x0030
#define CSTOPB  0x0040
#define CREAD   0x0080
#define PARENB  0x0100
#define CLOCAL  0x0800

/* c_lflag */
#define ISIG    0x0001
#define ICANON  0x0002
#define ECHO    0x0008
#define ECHOE   0x0010
#define ECHOK   0x0020
#define ECHONL  0x0040
#define NOFLSH  0x0080
#define IEXTEN  0x8000

/* termios actions */
#define TCSANOW     0
#define TCSADRAIN   1
#define TCSAFLUSH   2

/* Baud rates */
#define B0      0
#define B9600   9600
#define B19200  19200
#define B38400  38400
#define B57600  57600
#define B115200 115200

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

/* ============================================================
 * Resource Limits (Stubs)
 * ============================================================ */

struct rlimit {
    unsigned long rlim_cur;
    unsigned long rlim_max;
};

#define RLIMIT_NOFILE   7
#define RLIM_INFINITY   ((unsigned long)-1)

int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);

/* ============================================================
 * Initialization
 * ============================================================ */

/*
 * Initialize OS 9 platform layer.
 * Call before using any other os9_* functions.
 */
int os9_platform_init(void);

/*
 * Cleanup OS 9 platform layer.
 */
void os9_platform_cleanup(void);

#endif /* OS9_PLATFORM_H */
