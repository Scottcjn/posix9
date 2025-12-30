/*
 * posix9_thread.c - POSIX threads implementation for Mac OS 9
 *
 * Maps pthreads to Mac OS Thread Manager:
 *   pthread_create()  -> NewThread
 *   pthread_join()    -> waiting on thread state
 *   pthread_mutex_*   -> atomic operations + yield
 *   pthread_cond_*    -> polling with yield
 *
 * Note: Thread Manager provides cooperative threads.
 * For preemptive, we'd use Multiprocessing Services (MP).
 */

#include "posix9.h"
#include "posix9/pthread.h"

#include <Threads.h>
#include <Timer.h>
#include <string.h>

/* ============================================================
 * Thread Table
 * ============================================================ */

typedef struct {
    ThreadID        threadID;       /* Mac OS Thread ID */
    Boolean         inUse;
    Boolean         detached;
    Boolean         finished;
    void *          result;         /* Return value */
    void *          (*start)(void*);
    void *          arg;
} posix9_thread_entry;

#define MAX_THREADS 64
static posix9_thread_entry thread_table[MAX_THREADS];
static Boolean thread_table_initialized = false;
static pthread_t main_thread_id = 0;

/* Thread-local storage */
#define MAX_KEYS 64
static void *tls_data[MAX_THREADS][MAX_KEYS];
static void (*tls_destructors[MAX_KEYS])(void *);
static Boolean tls_key_used[MAX_KEYS];

/* ============================================================
 * Internal Helpers
 * ============================================================ */

static void init_thread_table(void)
{
    int i, j;

    if (thread_table_initialized) return;

    for (i = 0; i < MAX_THREADS; i++) {
        thread_table[i].inUse = false;
        thread_table[i].threadID = kNoThreadID;
        for (j = 0; j < MAX_KEYS; j++) {
            tls_data[i][j] = NULL;
        }
    }

    for (i = 0; i < MAX_KEYS; i++) {
        tls_key_used[i] = false;
        tls_destructors[i] = NULL;
    }

    /* Reserve slot 0 for main thread */
    thread_table[0].inUse = true;
    GetCurrentThread(&thread_table[0].threadID);
    main_thread_id = 1;  /* pthread_t is 1-based */

    thread_table_initialized = true;
}

static int alloc_thread(void)
{
    int i;

    init_thread_table();

    for (i = 1; i < MAX_THREADS; i++) {  /* Start at 1, 0 is main */
        if (!thread_table[i].inUse) {
            memset(&thread_table[i], 0, sizeof(posix9_thread_entry));
            thread_table[i].inUse = true;
            return i + 1;  /* pthread_t is 1-based */
        }
    }

    return 0;
}

static posix9_thread_entry *get_thread(pthread_t thread)
{
    int idx = thread - 1;

    if (idx < 0 || idx >= MAX_THREADS) return NULL;
    if (!thread_table[idx].inUse) return NULL;

    return &thread_table[idx];
}

static int get_thread_index(ThreadID tid)
{
    int i;

    for (i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].inUse && thread_table[i].threadID == tid) {
            return i;
        }
    }

    return -1;
}

/* ============================================================
 * Thread Entry Point Wrapper
 * ============================================================ */

static pascal void *thread_entry(void *param)
{
    posix9_thread_entry *entry = (posix9_thread_entry *)param;
    void *result;

    /* Call user's start routine */
    result = entry->start(entry->arg);

    /* Store result and mark finished */
    entry->result = result;
    entry->finished = true;

    /* Run TLS destructors */
    {
        int idx = get_thread_index(entry->threadID);
        int key;

        if (idx >= 0) {
            for (key = 0; key < MAX_KEYS; key++) {
                if (tls_key_used[key] && tls_data[idx][key] && tls_destructors[key]) {
                    tls_destructors[key](tls_data[idx][key]);
                }
            }
        }
    }

    return result;
}

/* ============================================================
 * POSIX Thread Functions
 * ============================================================ */

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg)
{
    int slot;
    posix9_thread_entry *entry;
    OSErr err;
    Size stackSize = 0;

    init_thread_table();

    slot = alloc_thread();
    if (slot == 0) {
        return EAGAIN;
    }

    entry = get_thread(slot);
    entry->start = start_routine;
    entry->arg = arg;
    entry->detached = (attr && attr->detachstate == PTHREAD_CREATE_DETACHED);

    if (attr && attr->stacksize > 0) {
        stackSize = attr->stacksize;
    }

    /* Create the thread */
    err = NewThread(
        kCooperativeThread,
        (ThreadEntryTPP)thread_entry,
        entry,
        stackSize,
        kCreateIfNeeded,
        NULL,
        &entry->threadID
    );

    if (err != noErr) {
        entry->inUse = false;
        return EAGAIN;
    }

    *thread = slot;
    return 0;
}

int pthread_join(pthread_t thread, void **retval)
{
    posix9_thread_entry *entry;

    entry = get_thread(thread);
    if (!entry) return ESRCH;

    if (entry->detached) return EINVAL;

    /* Wait for thread to finish */
    while (!entry->finished) {
        YieldToAnyThread();
    }

    if (retval) {
        *retval = entry->result;
    }

    /* Clean up */
    entry->inUse = false;

    return 0;
}

int pthread_detach(pthread_t thread)
{
    posix9_thread_entry *entry;

    entry = get_thread(thread);
    if (!entry) return ESRCH;

    entry->detached = true;

    /* If already finished, clean up */
    if (entry->finished) {
        entry->inUse = false;
    }

    return 0;
}

void pthread_exit(void *retval)
{
    ThreadID currentThread;
    int idx;
    posix9_thread_entry *entry;

    GetCurrentThread(&currentThread);
    idx = get_thread_index(currentThread);

    if (idx >= 0) {
        entry = &thread_table[idx];
        entry->result = retval;
        entry->finished = true;

        /* Run TLS destructors */
        {
            int key;
            for (key = 0; key < MAX_KEYS; key++) {
                if (tls_key_used[key] && tls_data[idx][key] && tls_destructors[key]) {
                    tls_destructors[key](tls_data[idx][key]);
                }
            }
        }
    }

    /* Dispose of thread (doesn't return) */
    DisposeThread(currentThread, retval, false);
}

pthread_t pthread_self(void)
{
    ThreadID currentThread;
    int idx;

    init_thread_table();

    GetCurrentThread(&currentThread);
    idx = get_thread_index(currentThread);

    if (idx >= 0) {
        return idx + 1;
    }

    /* Unknown thread - shouldn't happen */
    return 0;
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
    return (t1 == t2);
}

int pthread_yield(void)
{
    YieldToAnyThread();
    return 0;
}

int pthread_cancel(pthread_t thread)
{
    posix9_thread_entry *entry;

    entry = get_thread(thread);
    if (!entry) return ESRCH;

    /* Thread Manager doesn't support cancel directly */
    /* We just mark it as finished */
    entry->finished = true;

    return 0;
}

/* ============================================================
 * Thread Attributes
 * ============================================================ */

int pthread_attr_init(pthread_attr_t *attr)
{
    attr->stacksize = 0;  /* Use default */
    attr->detachstate = PTHREAD_CREATE_JOINABLE;
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
    (void)attr;
    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
    attr->detachstate = detachstate;
    return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate)
{
    *detachstate = attr->detachstate;
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    attr->stacksize = stacksize;
    return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
    *stacksize = attr->stacksize;
    return 0;
}

/* ============================================================
 * Mutex Functions
 * ============================================================ */

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    (void)attr;
    mutex->locked = 0;
    mutex->owner = 0;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    (void)mutex;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    pthread_t self = pthread_self();

    /* Spin with yield until we get the lock */
    while (1) {
        /* Try to atomically set locked from 0 to 1 */
        /* On 68K/PPC we can use test-and-set or compare-and-swap */
        /* For simplicity, use a simple check since cooperative threading */
        if (mutex->locked == 0) {
            mutex->locked = 1;
            mutex->owner = self;
            return 0;
        }

        /* Yield to other threads */
        YieldToAnyThread();
    }
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    pthread_t self = pthread_self();

    if (mutex->locked == 0) {
        mutex->locked = 1;
        mutex->owner = self;
        return 0;
    }

    return EBUSY;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    mutex->owner = 0;
    mutex->locked = 0;
    return 0;
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    attr->type = PTHREAD_MUTEX_DEFAULT;
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    (void)attr;
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
    attr->type = type;
    return 0;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
    *type = attr->type;
    return 0;
}

/* ============================================================
 * Condition Variables
 * ============================================================ */

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    (void)attr;
    cond->waiting = 0;
    cond->signaled = 0;
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
    (void)cond;
    return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    cond->waiting++;

    /* Release mutex */
    pthread_mutex_unlock(mutex);

    /* Wait for signal */
    while (cond->signaled == 0) {
        YieldToAnyThread();
    }

    cond->waiting--;
    if (cond->waiting == 0) {
        cond->signaled = 0;
    }

    /* Reacquire mutex */
    pthread_mutex_lock(mutex);

    return 0;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                           const struct timespec *abstime)
{
    unsigned long endTicks;
    unsigned long now;

    /* Convert abstime to ticks (roughly) */
    endTicks = TickCount() + (abstime->tv_sec * 60);

    cond->waiting++;

    /* Release mutex */
    pthread_mutex_unlock(mutex);

    /* Wait for signal or timeout */
    while (cond->signaled == 0) {
        now = TickCount();
        if (now >= endTicks) {
            cond->waiting--;
            pthread_mutex_lock(mutex);
            return ETIMEDOUT;
        }
        YieldToAnyThread();
    }

    cond->waiting--;
    if (cond->waiting == 0) {
        cond->signaled = 0;
    }

    /* Reacquire mutex */
    pthread_mutex_lock(mutex);

    return 0;
}

int pthread_cond_signal(pthread_cond_t *cond)
{
    if (cond->waiting > 0) {
        cond->signaled = 1;
    }
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    if (cond->waiting > 0) {
        cond->signaled = cond->waiting;  /* Wake all */
    }
    return 0;
}

/* ============================================================
 * Read-Write Locks
 * ============================================================ */

int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr)
{
    (void)attr;
    pthread_mutex_init(&rwlock->mutex, NULL);
    rwlock->readers = 0;
    rwlock->writer = 0;
    return 0;
}

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
    pthread_mutex_destroy(&rwlock->mutex);
    return 0;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
    pthread_mutex_lock(&rwlock->mutex);

    while (rwlock->writer != 0) {
        pthread_mutex_unlock(&rwlock->mutex);
        YieldToAnyThread();
        pthread_mutex_lock(&rwlock->mutex);
    }

    rwlock->readers++;
    pthread_mutex_unlock(&rwlock->mutex);

    return 0;
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
    if (pthread_mutex_trylock(&rwlock->mutex) != 0) {
        return EBUSY;
    }

    if (rwlock->writer != 0) {
        pthread_mutex_unlock(&rwlock->mutex);
        return EBUSY;
    }

    rwlock->readers++;
    pthread_mutex_unlock(&rwlock->mutex);

    return 0;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
    pthread_t self = pthread_self();

    pthread_mutex_lock(&rwlock->mutex);

    while (rwlock->readers > 0 || rwlock->writer != 0) {
        pthread_mutex_unlock(&rwlock->mutex);
        YieldToAnyThread();
        pthread_mutex_lock(&rwlock->mutex);
    }

    rwlock->writer = self;
    pthread_mutex_unlock(&rwlock->mutex);

    return 0;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
    pthread_t self = pthread_self();

    if (pthread_mutex_trylock(&rwlock->mutex) != 0) {
        return EBUSY;
    }

    if (rwlock->readers > 0 || rwlock->writer != 0) {
        pthread_mutex_unlock(&rwlock->mutex);
        return EBUSY;
    }

    rwlock->writer = self;
    pthread_mutex_unlock(&rwlock->mutex);

    return 0;
}

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
    pthread_t self = pthread_self();

    pthread_mutex_lock(&rwlock->mutex);

    if (rwlock->writer == self) {
        rwlock->writer = 0;
    } else if (rwlock->readers > 0) {
        rwlock->readers--;
    }

    pthread_mutex_unlock(&rwlock->mutex);

    return 0;
}

/* ============================================================
 * Once Initialization
 * ============================================================ */

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
    if (once_control->done == 0) {
        init_routine();
        once_control->done = 1;
    }
    return 0;
}

/* ============================================================
 * Thread-Specific Data (TSD)
 * ============================================================ */

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    int i;

    for (i = 0; i < MAX_KEYS; i++) {
        if (!tls_key_used[i]) {
            tls_key_used[i] = true;
            tls_destructors[i] = destructor;
            *key = i;
            return 0;
        }
    }

    return EAGAIN;
}

int pthread_key_delete(pthread_key_t key)
{
    if (key >= MAX_KEYS) return EINVAL;

    tls_key_used[key] = false;
    tls_destructors[key] = NULL;

    return 0;
}

void *pthread_getspecific(pthread_key_t key)
{
    int idx;
    ThreadID tid;

    if (key >= MAX_KEYS) return NULL;

    GetCurrentThread(&tid);
    idx = get_thread_index(tid);

    if (idx < 0) return NULL;

    return tls_data[idx][key];
}

int pthread_setspecific(pthread_key_t key, const void *value)
{
    int idx;
    ThreadID tid;

    if (key >= MAX_KEYS) return EINVAL;

    GetCurrentThread(&tid);
    idx = get_thread_index(tid);

    if (idx < 0) return EINVAL;

    tls_data[idx][key] = (void *)value;
    return 0;
}

/* Additional errno definition for timeout */
#ifndef ETIMEDOUT
#define ETIMEDOUT 116
#endif
