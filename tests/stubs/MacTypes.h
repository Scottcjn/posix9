#ifndef POSIX9_TEST_MAC_TYPES_H
#define POSIX9_TEST_MAC_TYPES_H

#include <stddef.h>

#ifndef pascal
#define pascal
#endif

#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif

typedef unsigned char Boolean;
typedef signed long SInt32;
typedef unsigned long UInt32;
typedef short OSErr;
typedef unsigned char Str255[256];
typedef unsigned char *StringPtr;
typedef const unsigned char *ConstStr255Param;

typedef struct FSSpec {
    short vRefNum;
    long parID;
    Str255 name;
} FSSpec;

typedef struct CInfoPBRec {
    struct {
        short ioVRefNum;
        long ioDirID;
        StringPtr ioNamePtr;
        short ioFDirIndex;
        unsigned char ioFlAttrib;
    } hFileInfo;
    struct {
        long ioDrDirID;
    } dirInfo;
} CInfoPBRec;

#ifndef noErr
#define noErr 0
#endif
#ifndef fnfErr
#define fnfErr (-43)
#endif

#endif /* POSIX9_TEST_MAC_TYPES_H */
