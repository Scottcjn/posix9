/*
 * posix9/signal.h - POSIX signal definitions for Mac OS 9
 *
 * Mac OS 9 has no signals. We emulate them via:
 * - Deferred Task Manager for async delivery
 * - Polling in main event loop
 * - Notification Manager for user alerts
 */

#ifndef POSIX9_SIGNAL_H
#define POSIX9_SIGNAL_H

#include "types.h"

/* Signal numbers */
#define SIGHUP      1       /* Hangup */
#define SIGINT      2       /* Interrupt (Ctrl+C) */
#define SIGQUIT     3       /* Quit */
#define SIGILL      4       /* Illegal instruction */
#define SIGTRAP     5       /* Trace trap */
#define SIGABRT     6       /* Abort */
#define SIGBUS      7       /* Bus error */
#define SIGFPE      8       /* Floating point exception */
#define SIGKILL     9       /* Kill (cannot be caught) */
#define SIGUSR1     10      /* User signal 1 */
#define SIGSEGV     11      /* Segmentation fault */
#define SIGUSR2     12      /* User signal 2 */
#define SIGPIPE     13      /* Broken pipe */
#define SIGALRM     14      /* Alarm clock */
#define SIGTERM     15      /* Termination */
#define SIGSTKFLT   16      /* Stack fault */
#define SIGCHLD     17      /* Child status changed */
#define SIGCONT     18      /* Continue */
#define SIGSTOP     19      /* Stop (cannot be caught) */
#define SIGTSTP     20      /* Terminal stop */
#define SIGTTIN     21      /* Background read from tty */
#define SIGTTOU     22      /* Background write to tty */
#define SIGURG      23      /* Urgent condition on socket */
#define SIGXCPU     24      /* CPU time limit exceeded */
#define SIGXFSZ     25      /* File size limit exceeded */
#define SIGVTALRM   26      /* Virtual timer expired */
#define SIGPROF     27      /* Profiling timer expired */
#define SIGWINCH    28      /* Window size changed */
#define SIGIO       29      /* I/O possible */
#define SIGPWR      30      /* Power failure */
#define SIGSYS      31      /* Bad system call */

#define NSIG        32      /* Number of signals */

/* Signal handler types */
typedef void (*sig_t)(int);
typedef void (*sighandler_t)(int);

/* Special signal handlers */
#define SIG_DFL     ((sighandler_t)0)   /* Default action */
#define SIG_IGN     ((sighandler_t)1)   /* Ignore signal */
#define SIG_ERR     ((sighandler_t)-1)  /* Error return */

/* Signal action flags */
#define SA_NOCLDSTOP    0x0001  /* Don't generate SIGCHLD on stop */
#define SA_NOCLDWAIT    0x0002  /* Don't create zombies */
#define SA_SIGINFO      0x0004  /* Invoke with 3 args */
#define SA_ONSTACK      0x0008  /* Use alternate stack */
#define SA_RESTART      0x0010  /* Restart syscall on signal */
#define SA_NODEFER      0x0040  /* Don't block signal during handler */
#define SA_RESETHAND    0x0080  /* Reset handler after catch */

/* Signal set type */
typedef unsigned long sigset_t;

/* sigaction structure */
struct sigaction {
    sighandler_t    sa_handler;     /* Signal handler */
    sigset_t        sa_mask;        /* Signals to block during handler */
    int             sa_flags;       /* Flags */
};

/* siginfo_t (simplified) */
typedef struct {
    int     si_signo;       /* Signal number */
    int     si_errno;       /* Error number */
    int     si_code;        /* Signal code */
    pid_t   si_pid;         /* Sending process ID */
    uid_t   si_uid;         /* Sending user ID */
    void *  si_addr;        /* Faulting address */
    int     si_status;      /* Exit value or signal */
} siginfo_t;

/* Signal set manipulation */
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signum);
int sigdelset(sigset_t *set, int signum);
int sigismember(const sigset_t *set, int signum);

/* Signal handling */
sighandler_t signal(int signum, sighandler_t handler);
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);

/* Signal sending */
int raise(int sig);
int kill(pid_t pid, int sig);

/* Signal blocking */
#define SIG_BLOCK       0
#define SIG_UNBLOCK     1
#define SIG_SETMASK     2

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int sigpending(sigset_t *set);
int sigsuspend(const sigset_t *mask);

/* Alarm */
unsigned int alarm(unsigned int seconds);

/* Pause */
int pause(void);

/* ============================================================
 * POSIX9 Signal Processing
 * ============================================================ */

/*
 * Process pending signals - call this from your event loop
 * Returns number of signals delivered
 */
int posix9_signal_process(void);

/*
 * Check if a signal is pending without delivering
 */
int posix9_signal_pending(int signum);

#endif /* POSIX9_SIGNAL_H */
