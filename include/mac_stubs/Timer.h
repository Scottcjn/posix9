/*
 * Timer.h - Timer Manager compatibility for Retro68
 * Only defines what Multiverse.h doesn't provide
 */
#ifndef __TIMER_COMPAT__
#define __TIMER_COMPAT__

/* Multiverse.h already provides most Timer Manager functions.
 * This header only adds missing definitions. */

/* TMTaskPtr typedef if not defined */
#ifndef TMTaskPtr
typedef void* TMTaskPtr;
#endif

/* InsXTime - extended time insert (alias to InsTime if not available) */
#ifndef InsXTime
#define InsXTime(tmTaskPtr) InsTime((QElemPtr)(tmTaskPtr))
#endif

/* UnsignedWide for Microseconds if not defined */
/* Note: MacCompat.h may already define this */
#if !defined(UnsignedWide) && !defined(__MACCOMPAT__)
typedef struct UnsignedWide {
    unsigned long hi;
    unsigned long lo;
} UnsignedWide;
#endif

#endif /* __TIMER_COMPAT__ */
