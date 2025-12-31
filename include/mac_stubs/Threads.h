/*
 * Threads.h - Stub for Mac OS Thread Manager
 * For cross-compilation only - actual implementation on Mac OS 9
 *
 * Only defines what Multiverse.h doesn't provide.
 */
#ifndef __THREADS__
#define __THREADS__

#include <MacTypes.h>

/* Thread ID */
typedef UInt32 ThreadID;

/* Special thread ID values */
#ifndef kNoThreadID
#define kNoThreadID         0
#endif
#ifndef kCurrentThreadID
#define kCurrentThreadID    1
#endif

/* Thread state */
typedef UInt16 ThreadState;
enum {
    kReadyThreadState       = 0,
    kStoppedThreadState     = 1,
    kRunningThreadState     = 2
};

/* Thread options */
typedef UInt32 ThreadOptions;
enum {
    kUsePremptiveThread     = (1 << 0),
    kCreateIfNeeded         = (1 << 1),
    kFPUNotNeeded           = (1 << 2),
    kExactMatchThread       = (1 << 3)
};

/* Thread style */
typedef UInt32 ThreadStyle;
enum {
    kCooperativeThread      = (1 << 0),
    kPreemptiveThread       = (1 << 1)
};

/* Thread entry proc */
typedef void* (*ThreadEntryProcPtr)(void* threadParam);
typedef ThreadEntryProcPtr ThreadEntryTPP;

/* Thread termination proc */
typedef void (*ThreadTerminationProcPtr)(ThreadID threadTerminated, void* terminationProcParam);

/* Thread scheduler proc */
typedef ThreadID (*ThreadSchedulerProcPtr)(void);

/* Thread switcher proc */
typedef void (*ThreadSwitchProcPtr)(ThreadID threadBeingSwitched, void* switchProcParam);

/* Thread Manager functions */
/* NewThread is the actual Mac OS API name */
OSErr NewThread(ThreadStyle threadStyle, ThreadEntryTPP threadEntry,
                void* threadParam, Size stackSize, ThreadOptions options,
                void** threadResult, ThreadID* threadMade);

/* CreateThread is an alias sometimes used */
#define CreateThread NewThread

OSErr DisposeThread(ThreadID threadToDump, void* threadResult, Boolean recycleThread);

OSErr GetCurrentThread(ThreadID* currentThreadID);

OSErr YieldToThread(ThreadID suggestedThread);

OSErr YieldToAnyThread(void);

OSErr SetThreadState(ThreadID threadToSet, ThreadState newState, ThreadID suggestedThread);

OSErr GetThreadState(ThreadID threadToGet, ThreadState* threadState);

OSErr SetThreadTerminator(ThreadID thread, ThreadTerminationProcPtr terminationProc,
                          void* terminationProcParam);

OSErr SetThreadSwitcher(ThreadID thread, ThreadSwitchProcPtr threadSwitcher,
                        void* switchProcParam, Boolean inOrOut);

OSErr GetFreeThreadCount(ThreadStyle threadStyle, SInt16* freeCount);

OSErr GetSpecificFreeThreadCount(ThreadStyle threadStyle, Size stackSize,
                                  SInt16* freeCount);

#endif /* __THREADS__ */
