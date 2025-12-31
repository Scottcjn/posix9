/*
 * MacCompat.h - Missing Mac OS definitions for Retro68
 * Adds constants not present in Multiverse.h
 */
#ifndef __MACCOMPAT__
#define __MACCOMPAT__

/* File attribute flags - not in Multiverse.h */
#ifndef ioDirMask
#define ioDirMask           0x10    /* bit 4: directory flag */
#endif

/* AFP error codes - not in Multiverse.h */
#ifndef afpAccessDenied
#define afpAccessDenied     -5000   /* AFP access denied error */
#endif
#ifndef afpObjectTypeErr
#define afpObjectTypeErr    -5025
#endif

/* HGetVol - get default volume/directory */
#ifndef HGetVol
pascal OSErr HGetVol(StringPtr volName, short *vRefNum, long *dirID);
#endif

/* DirCreate - create a directory */
#ifndef DirCreate
pascal OSErr DirCreate(short vRefNum, long parentDirID, ConstStr255Param name, long *createdDirID);
#endif

/* PBGetCatInfoSync - Multiverse.h provides this for both 68K and PPC
 * On 68K: inline trap
 * On PPC: external function imported from InterfaceLib
 * Don't define it here as it conflicts with Multiverse.h */

/* Time Manager extended functions */
#ifndef InsXTime
#define InsXTime(tmTaskPtr) InsTime(tmTaskPtr)
#endif

/* UnsignedWide for Microseconds */
#ifndef UnsignedWide
typedef struct UnsignedWide {
    unsigned long hi;
    unsigned long lo;
} UnsignedWide;
#endif

/* File System constants */
#ifndef fsRtDirID
#define fsRtDirID           2       /* Root directory ID */
#endif
#ifndef fsRtParID
#define fsRtParID           1       /* Root's parent ID */
#endif

/* HSetVol - set default volume/directory */
#ifndef HSetVol
pascal OSErr HSetVol(ConstStr255Param volName, short vRefNum, long dirID);
#endif

/* TimerUPP type for Time Manager */
#ifndef TimerUPP
typedef void* TimerUPP;
#endif

/* NewTimerUPP - create Timer UPP (may be identity on PPC) */
#ifndef NewTimerUPP
#define NewTimerUPP(proc) ((TimerUPP)(proc))
#endif

/* DisposeTimerUPP - dispose Timer UPP */
#ifndef DisposeTimerUPP
#define DisposeTimerUPP(upp) /* no-op */
#endif

/* OSStatus - may not be defined */
#ifndef OSStatus
typedef SInt32 OSStatus;
#endif

#endif /* __MACCOMPAT__ */
