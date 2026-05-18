#ifndef POSIX9_TEST_MULTIVERSE_H
#define POSIX9_TEST_MULTIVERSE_H

#include "MacTypes.h"

pascal OSErr FSMakeFSSpec(short vRefNum, long dirID, ConstStr255Param fileName, FSSpec *spec);
pascal OSErr PBGetCatInfoSync(CInfoPBRec *pb);

#endif /* POSIX9_TEST_MULTIVERSE_H */
