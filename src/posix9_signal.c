/*
 * posix9_signal.c - POSIX signal emulation for Mac OS 9
 *
 * Mac OS 9 has no signals. We emulate them by:
 * - Maintaining a pending signal bitmask
 * - Polling in posix9_signal_process() from event loop
 * - Using Time Manager for SIGALRM
 * - Mapping Cmd+. to SIGINT
 *
 * Limitations:
 * - No true async delivery (must poll)
 * - SIGKILL/SIGSTOP just set flags, can't force-kill
 * - No process groups (kill() only works on self)
 */

#include "posix9.h"
#include "posix9/signal.h"

#include <Events.h>
#include <Timer.h>
#include <string.h>

/* ============================================================
 * Signal State
 * ============================================================ */

/* Signal handlers */
static sighandler_t signal_handlers[NSIG];

/* Pending signals bitmask */
static volatile sigset_t pending_signals = 0;

/* Blocked signals bitmask */
static sigset_t blocked_signals = 0;

/* Signal actions (for sigaction) */
static struct sigaction signal_actions[NSIG];

/* Alarm timer */
static TMTask alarm_task;
static Boolean alarm_installed = false;

/* Initialization flag */
static Boolean signals_initialized = false;

/* ============================================================
 * Default Signal Actions
 * ============================================================ */

typedef enum {
    SIG_ACTION_TERM,        /* Terminate process */
    SIG_ACTION_IGNORE,      /* Ignore */
    SIG_ACTION_CORE,        /* Terminate with core dump (same as term on OS9) */
    SIG_ACTION_STOP,        /* Stop process */
    SIG_ACTION_CONT         /* Continue if stopped */
} default_action_t;

static default_action_t default_actions[NSIG] = {
    [0] = SIG_ACTION_IGNORE,
    [SIGHUP] = SIG_ACTION_TERM,
    [SIGINT] = SIG_ACTION_TERM,
    [SIGQUIT] = SIG_ACTION_CORE,
    [SIGILL] = SIG_ACTION_CORE,
    [SIGTRAP] = SIG_ACTION_CORE,
    [SIGABRT] = SIG_ACTION_CORE,
    [SIGBUS] = SIG_ACTION_CORE,
    [SIGFPE] = SIG_ACTION_CORE,
    [SIGKILL] = SIG_ACTION_TERM,
    [SIGUSR1] = SIG_ACTION_TERM,
    [SIGSEGV] = SIG_ACTION_CORE,
    [SIGUSR2] = SIG_ACTION_TERM,
    [SIGPIPE] = SIG_ACTION_TERM,
    [SIGALRM] = SIG_ACTION_TERM,
    [SIGTERM] = SIG_ACTION_TERM,
    [SIGSTKFLT] = SIG_ACTION_TERM,
    [SIGCHLD] = SIG_ACTION_IGNORE,
    [SIGCONT] = SIG_ACTION_CONT,
    [SIGSTOP] = SIG_ACTION_STOP,
    [SIGTSTP] = SIG_ACTION_STOP,
    [SIGTTIN] = SIG_ACTION_STOP,
    [SIGTTOU] = SIG_ACTION_STOP,
    [SIGURG] = SIG_ACTION_IGNORE,
    [SIGXCPU] = SIG_ACTION_CORE,
    [SIGXFSZ] = SIG_ACTION_CORE,
    [SIGVTALRM] = SIG_ACTION_TERM,
    [SIGPROF] = SIG_ACTION_TERM,
    [SIGWINCH] = SIG_ACTION_IGNORE,
    [SIGIO] = SIG_ACTION_TERM,
    [SIGPWR] = SIG_ACTION_TERM,
    [SIGSYS] = SIG_ACTION_CORE
};

/* ============================================================
 * Initialization
 * ============================================================ */

static void init_signals(void)
{
    int i;

    if (signals_initialized) return;

    for (i = 0; i < NSIG; i++) {
        signal_handlers[i] = SIG_DFL;
        memset(&signal_actions[i], 0, sizeof(struct sigaction));
        signal_actions[i].sa_handler = SIG_DFL;
    }

    pending_signals = 0;
    blocked_signals = 0;
    signals_initialized = true;
}

/* ============================================================
 * Signal Set Operations
 * ============================================================ */

int sigemptyset(sigset_t *set)
{
    if (!set) {
        errno = EINVAL;
        return -1;
    }
    *set = 0;
    return 0;
}

int sigfillset(sigset_t *set)
{
    if (!set) {
        errno = EINVAL;
        return -1;
    }
    *set = ~0UL;
    return 0;
}

int sigaddset(sigset_t *set, int signum)
{
    if (!set || signum < 1 || signum >= NSIG) {
        errno = EINVAL;
        return -1;
    }
    *set |= (1UL << signum);
    return 0;
}

int sigdelset(sigset_t *set, int signum)
{
    if (!set || signum < 1 || signum >= NSIG) {
        errno = EINVAL;
        return -1;
    }
    *set &= ~(1UL << signum);
    return 0;
}

int sigismember(const sigset_t *set, int signum)
{
    if (!set || signum < 1 || signum >= NSIG) {
        errno = EINVAL;
        return -1;
    }
    return (*set & (1UL << signum)) ? 1 : 0;
}

/* ============================================================
 * Signal Handling
 * ============================================================ */

sighandler_t signal(int signum, sighandler_t handler)
{
    sighandler_t old;

    init_signals();

    if (signum < 1 || signum >= NSIG) {
        errno = EINVAL;
        return SIG_ERR;
    }

    /* SIGKILL and SIGSTOP cannot be caught */
    if (signum == SIGKILL || signum == SIGSTOP) {
        errno = EINVAL;
        return SIG_ERR;
    }

    old = signal_handlers[signum];
    signal_handlers[signum] = handler;
    signal_actions[signum].sa_handler = handler;

    return old;
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
    init_signals();

    if (signum < 1 || signum >= NSIG) {
        errno = EINVAL;
        return -1;
    }

    if (signum == SIGKILL || signum == SIGSTOP) {
        errno = EINVAL;
        return -1;
    }

    if (oldact) {
        *oldact = signal_actions[signum];
    }

    if (act) {
        signal_actions[signum] = *act;
        signal_handlers[signum] = act->sa_handler;
    }

    return 0;
}

/* ============================================================
 * Signal Delivery
 * ============================================================ */

static void deliver_signal(int signum)
{
    sighandler_t handler;
    sigset_t old_blocked;

    handler = signal_handlers[signum];

    if (handler == SIG_IGN) {
        /* Ignored */
        return;
    }

    if (handler == SIG_DFL) {
        /* Default action */
        switch (default_actions[signum]) {
            case SIG_ACTION_TERM:
            case SIG_ACTION_CORE:
                /* Terminate - on OS 9, we just call ExitToShell */
                ExitToShell();
                break;

            case SIG_ACTION_STOP:
                /* Can't really stop on OS 9 */
                break;

            case SIG_ACTION_CONT:
            case SIG_ACTION_IGNORE:
                /* Nothing to do */
                break;
        }
        return;
    }

    /* User handler */
    /* Block signals during handler per sa_mask */
    old_blocked = blocked_signals;
    blocked_signals |= signal_actions[signum].sa_mask;

    if (!(signal_actions[signum].sa_flags & SA_NODEFER)) {
        blocked_signals |= (1UL << signum);
    }

    /* Call handler */
    handler(signum);

    /* Restore blocked signals */
    blocked_signals = old_blocked;

    /* Reset handler if SA_RESETHAND */
    if (signal_actions[signum].sa_flags & SA_RESETHAND) {
        signal_handlers[signum] = SIG_DFL;
        signal_actions[signum].sa_handler = SIG_DFL;
    }
}

int posix9_signal_process(void)
{
    int i;
    int delivered = 0;
    sigset_t to_deliver;

    init_signals();

    /* Check for Cmd+. (interrupt) */
    {
        EventRecord event;
        if (EventAvail(keyDownMask, &event)) {
            if ((event.modifiers & cmdKey) && (event.message & charCodeMask) == '.') {
                GetNextEvent(keyDownMask, &event);  /* Consume it */
                pending_signals |= (1UL << SIGINT);
            }
        }
    }

    /* Calculate which signals to deliver */
    to_deliver = pending_signals & ~blocked_signals;

    if (to_deliver == 0) {
        return 0;
    }

    /* Deliver each pending, unblocked signal */
    for (i = 1; i < NSIG; i++) {
        if (to_deliver & (1UL << i)) {
            pending_signals &= ~(1UL << i);
            deliver_signal(i);
            delivered++;
        }
    }

    return delivered;
}

int posix9_signal_pending(int signum)
{
    if (signum < 1 || signum >= NSIG) {
        return 0;
    }
    return (pending_signals & (1UL << signum)) ? 1 : 0;
}

/* ============================================================
 * Signal Sending
 * ============================================================ */

int raise(int sig)
{
    init_signals();

    if (sig < 1 || sig >= NSIG) {
        errno = EINVAL;
        return -1;
    }

    pending_signals |= (1UL << sig);
    return 0;
}

int kill(pid_t pid, int sig)
{
    init_signals();

    /* On OS 9, we can only signal ourselves */
    /* pid 0 = process group, pid -1 = all, pid 1 = us */
    if (pid != 0 && pid != 1 && pid != -1 && pid != getpid()) {
        errno = ESRCH;
        return -1;
    }

    if (sig < 0 || sig >= NSIG) {
        errno = EINVAL;
        return -1;
    }

    /* sig 0 is just a check, don't actually send */
    if (sig == 0) {
        return 0;
    }

    pending_signals |= (1UL << sig);
    return 0;
}

/* ============================================================
 * Signal Blocking
 * ============================================================ */

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    init_signals();

    if (oldset) {
        *oldset = blocked_signals;
    }

    if (set) {
        switch (how) {
            case SIG_BLOCK:
                blocked_signals |= *set;
                break;
            case SIG_UNBLOCK:
                blocked_signals &= ~(*set);
                break;
            case SIG_SETMASK:
                blocked_signals = *set;
                break;
            default:
                errno = EINVAL;
                return -1;
        }
    }

    /* Can't block SIGKILL or SIGSTOP */
    blocked_signals &= ~((1UL << SIGKILL) | (1UL << SIGSTOP));

    return 0;
}

int sigpending(sigset_t *set)
{
    init_signals();

    if (!set) {
        errno = EINVAL;
        return -1;
    }

    *set = pending_signals & blocked_signals;
    return 0;
}

int sigsuspend(const sigset_t *mask)
{
    sigset_t old_blocked;

    init_signals();

    old_blocked = blocked_signals;
    blocked_signals = *mask;

    /* Wait for a signal */
    while ((pending_signals & ~blocked_signals) == 0) {
        /* Give time to system and check for events */
        SystemTask();
        posix9_signal_process();
        YieldToAnyThread();
    }

    blocked_signals = old_blocked;

    /* Deliver pending signals */
    posix9_signal_process();

    errno = EINTR;
    return -1;
}

/* ============================================================
 * Alarm Timer
 * ============================================================ */

/* Timer callback - runs at interrupt time */
static pascal void alarm_callback(TMTaskPtr task)
{
    (void)task;
    pending_signals |= (1UL << SIGALRM);
    alarm_installed = false;
}

unsigned int alarm(unsigned int seconds)
{
    unsigned int remaining = 0;

    init_signals();

    /* Cancel existing alarm */
    if (alarm_installed) {
        RmvTime((QElemPtr)&alarm_task);
        /* Calculate remaining time (rough approximation) */
        remaining = (-alarm_task.tmCount) / 1000;
        alarm_installed = false;
    }

    if (seconds == 0) {
        /* Just cancel, don't set new alarm */
        return remaining;
    }

    /* Set up new alarm */
    memset(&alarm_task, 0, sizeof(alarm_task));
    alarm_task.tmAddr = NewTimerUPP(alarm_callback);
    alarm_task.tmWakeUp = 0;
    alarm_task.tmReserved = 0;

    InsXTime((QElemPtr)&alarm_task);
    PrimeTime((QElemPtr)&alarm_task, seconds * -1000);  /* Negative = milliseconds */

    alarm_installed = true;

    return remaining;
}

/* ============================================================
 * Pause
 * ============================================================ */

int pause(void)
{
    init_signals();

    /* Wait for any signal */
    while ((pending_signals & ~blocked_signals) == 0) {
        SystemTask();
        posix9_signal_process();
        YieldToAnyThread();
    }

    /* Deliver pending signals */
    posix9_signal_process();

    errno = EINTR;
    return -1;
}

/* ============================================================
 * Process stubs (from posix9.h requirements)
 * ============================================================ */

pid_t getpid(void)
{
    return 1;  /* We're always PID 1 on OS 9 */
}

pid_t getppid(void)
{
    return 0;  /* No parent */
}

uid_t getuid(void)
{
    return 0;  /* Root equivalent */
}

uid_t geteuid(void)
{
    return 0;
}

gid_t getgid(void)
{
    return 0;
}

gid_t getegid(void)
{
    return 0;
}

/* fork() - not supported */
pid_t fork(void)
{
    errno = ENOSYS;
    return -1;
}
