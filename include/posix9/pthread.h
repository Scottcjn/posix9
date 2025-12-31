/*
 * posix9/pthread.h - POSIX threads for Mac OS 9
 * Maps to Thread Manager and Multiprocessing Services
 */

#ifndef POSIX9_PTHREAD_H
#define POSIX9_PTHREAD_H

#include "types.h"

/* ============================================================
 * timespec for timed operations - must be defined before use
 * ============================================================ */

#ifndef _STRUCT_TIMESPEC
#define _STRUCT_TIMESPEC
struct timespec {
    time_t  tv_sec;
    long    tv_nsec;
};
#endif

/* Thread ID type */
typedef unsigned long pthread_t;

/* Thread attributes */
typedef struct {
    size_t      stacksize;
    int         detachstate;
} pthread_attr_t;

/* Mutex types */
typedef struct {
    volatile int    locked;
    pthread_t       owner;
} pthread_mutex_t;

typedef struct {
    int type;
} pthread_mutexattr_t;

/* Condition variable */
typedef struct {
    volatile int    waiting;
    volatile int    signaled;
} pthread_cond_t;

typedef struct {
    int dummy;
} pthread_condattr_t;

/* Read-write lock */
typedef struct {
    pthread_mutex_t mutex;
    int             readers;
    pthread_t       writer;
} pthread_rwlock_t;

typedef struct {
    int dummy;
} pthread_rwlockattr_t;

/* Once control */
typedef struct {
    volatile int done;
} pthread_once_t;

#define PTHREAD_ONCE_INIT { 0 }

/* Thread key for thread-local storage */
typedef unsigned int pthread_key_t;

/* Detach states */
#define PTHREAD_CREATE_JOINABLE     0
#define PTHREAD_CREATE_DETACHED     1

/* Mutex types */
#define PTHREAD_MUTEX_NORMAL        0
#define PTHREAD_MUTEX_ERRORCHECK    1
#define PTHREAD_MUTEX_RECURSIVE     2
#define PTHREAD_MUTEX_DEFAULT       PTHREAD_MUTEX_NORMAL

/* Initializers */
#define PTHREAD_MUTEX_INITIALIZER   { 0, 0 }
#define PTHREAD_COND_INITIALIZER    { 0, 0 }
#define PTHREAD_RWLOCK_INITIALIZER  { PTHREAD_MUTEX_INITIALIZER, 0, 0 }

/* ============================================================
 * Thread Functions
 * ============================================================ */

/* Create a new thread */
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);

/* Wait for thread to terminate */
int pthread_join(pthread_t thread, void **retval);

/* Detach a thread */
int pthread_detach(pthread_t thread);

/* Exit current thread */
void pthread_exit(void *retval);

/* Get current thread ID */
pthread_t pthread_self(void);

/* Compare thread IDs */
int pthread_equal(pthread_t t1, pthread_t t2);

/* Yield to other threads */
int pthread_yield(void);

/* Cancel a thread */
int pthread_cancel(pthread_t thread);

/* ============================================================
 * Thread Attributes
 * ============================================================ */

int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);

/* ============================================================
 * Mutex Functions
 * ============================================================ */

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type);

/* ============================================================
 * Condition Variables
 * ============================================================ */

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                           const struct timespec *abstime);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);

/* ============================================================
 * Read-Write Locks
 * ============================================================ */

int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);

/* ============================================================
 * Once Initialization
 * ============================================================ */

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

/* ============================================================
 * Thread-Specific Data (TSD)
 * ============================================================ */

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);

#endif /* POSIX9_PTHREAD_H */
