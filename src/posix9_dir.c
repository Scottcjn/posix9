/*
 * posix9_dir.c - POSIX directory operations for Mac OS 9
 *
 * Maps POSIX directory operations to Mac OS File Manager:
 *   opendir()  -> Get FSSpec, store iteration state
 *   readdir()  -> PBGetCatInfoSync with index
 *   closedir() -> Free state
 *   mkdir()    -> DirCreate
 *   rmdir()    -> FSpDelete
 */

#include "posix9.h"

/* Mac OS headers - Retro68 provides everything via Multiverse.h */
#include <Multiverse.h>
#include "MacCompat.h"      /* Missing definitions for Retro68 */
#include <string.h>

/* ============================================================
 * Directory Stream Structure
 * ============================================================ */

struct posix9_dir {
    short       vRefNum;        /* Volume reference */
    long        dirID;          /* Directory ID */
    short       index;          /* Current iteration index (1-based) */
    Boolean     inUse;          /* Is this stream in use? */
    char        path[POSIX9_PATH_MAX];  /* Directory path (for debugging) */
    struct dirent entry;        /* Current directory entry */
};

/* Pool of directory streams */
#define MAX_DIR_STREAMS 32
static struct posix9_dir dir_pool[MAX_DIR_STREAMS];
static Boolean dir_pool_initialized = false;

/* ============================================================
 * Internal Helpers
 * ============================================================ */

static void init_dir_pool(void)
{
    int i;

    if (dir_pool_initialized) return;

    for (i = 0; i < MAX_DIR_STREAMS; i++) {
        dir_pool[i].inUse = false;
    }

    dir_pool_initialized = true;
}

static struct posix9_dir *alloc_dir(void)
{
    int i;

    init_dir_pool();

    for (i = 0; i < MAX_DIR_STREAMS; i++) {
        if (!dir_pool[i].inUse) {
            dir_pool[i].inUse = true;
            dir_pool[i].index = 1;  /* Mac indexes start at 1 */
            return &dir_pool[i];
        }
    }

    errno = EMFILE;
    return NULL;
}

static void free_dir(struct posix9_dir *dir)
{
    if (dir) {
        dir->inUse = false;
    }
}

/* Convert Pascal string to C string */
static void pstr_to_cstr(const Str255 pstr, char *cstr, size_t maxlen)
{
    size_t len = pstr[0];
    if (len >= maxlen) len = maxlen - 1;
    memcpy(cstr, &pstr[1], len);
    cstr[len] = '\0';
}

/* Convert C string to Pascal string */
static void cstr_to_pstr(const char *cstr, Str255 pstr)
{
    size_t len = strlen(cstr);
    if (len > 255) len = 255;
    pstr[0] = (unsigned char)len;
    memcpy(&pstr[1], cstr, len);
}

/* ============================================================
 * POSIX Directory Operations
 * ============================================================ */

DIR *opendir(const char *name)
{
    struct posix9_dir *dir;
    FSSpec spec;
    CInfoPBRec catInfo;
    OSErr err;

    /* Convert path to FSSpec */
    err = posix9_path_to_fsspec(name, &spec);
    if (err != noErr && err != fnfErr) {
        errno = posix9_macos_to_errno(err);
        return NULL;
    }

    /* Get catalog info to verify it's a directory and get dirID */
    memset(&catInfo, 0, sizeof(catInfo));
    catInfo.hFileInfo.ioVRefNum = spec.vRefNum;
    catInfo.hFileInfo.ioDirID = spec.parID;
    catInfo.hFileInfo.ioNamePtr = spec.name;
    catInfo.hFileInfo.ioFDirIndex = 0;

    err = PBGetCatInfoSync(&catInfo);
    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return NULL;
    }

    /* Verify it's a directory */
    if (!(catInfo.hFileInfo.ioFlAttrib & ioDirMask)) {
        errno = ENOTDIR;
        return NULL;
    }

    /* Allocate directory stream */
    dir = alloc_dir();
    if (!dir) return NULL;

    /* Store directory info */
    dir->vRefNum = spec.vRefNum;
    dir->dirID = catInfo.dirInfo.ioDrDirID;
    dir->index = 1;
    strncpy(dir->path, name, sizeof(dir->path) - 1);
    dir->path[sizeof(dir->path) - 1] = '\0';

    return (DIR *)dir;
}

struct dirent *readdir(DIR *dirp)
{
    struct posix9_dir *dir = (struct posix9_dir *)dirp;
    CInfoPBRec catInfo;
    Str255 name;
    OSErr err;

    if (!dir || !dir->inUse) {
        errno = EBADF;
        return NULL;
    }

    /* Get next entry */
    memset(&catInfo, 0, sizeof(catInfo));
    catInfo.hFileInfo.ioVRefNum = dir->vRefNum;
    catInfo.hFileInfo.ioDirID = dir->dirID;
    catInfo.hFileInfo.ioNamePtr = name;
    catInfo.hFileInfo.ioFDirIndex = dir->index;

    err = PBGetCatInfoSync(&catInfo);

    if (err == fnfErr) {
        /* No more entries */
        return NULL;
    }

    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return NULL;
    }

    /* Fill in dirent */
    pstr_to_cstr(name, dir->entry.d_name, sizeof(dir->entry.d_name));

    /* Use dirID or file ID as inode */
    if (catInfo.hFileInfo.ioFlAttrib & ioDirMask) {
        dir->entry.d_ino = catInfo.dirInfo.ioDrDirID;
    } else {
        dir->entry.d_ino = catInfo.hFileInfo.ioDirID;
    }

    /* Advance index for next call */
    dir->index++;

    return &dir->entry;
}

int closedir(DIR *dirp)
{
    struct posix9_dir *dir = (struct posix9_dir *)dirp;

    if (!dir || !dir->inUse) {
        errno = EBADF;
        return -1;
    }

    free_dir(dir);
    return 0;
}

void rewinddir(DIR *dirp)
{
    struct posix9_dir *dir = (struct posix9_dir *)dirp;

    if (dir && dir->inUse) {
        dir->index = 1;
    }
}

int mkdir(const char *path, mode_t mode)
{
    char mac_path[POSIX9_PATH_MAX];
    char parent_path[POSIX9_PATH_MAX];
    char *last_sep;
    Str255 dirname;
    FSSpec parentSpec;
    CInfoPBRec catInfo;
    long newDirID;
    OSErr err;

    (void)mode;  /* Mode is ignored on Mac OS 9 */

    /* Convert to Mac path */
    posix9_path_to_mac(path, mac_path, sizeof(mac_path));

    /* Find parent directory */
    strncpy(parent_path, mac_path, sizeof(parent_path) - 1);
    parent_path[sizeof(parent_path) - 1] = '\0';

    last_sep = strrchr(parent_path, ':');
    if (last_sep) {
        /* Extract directory name */
        cstr_to_pstr(last_sep + 1, dirname);
        *last_sep = '\0';  /* Truncate to parent */
    } else {
        /* No separator - create in current directory */
        cstr_to_pstr(mac_path, dirname);
        parent_path[0] = '\0';
    }

    /* Get parent FSSpec */
    if (parent_path[0]) {
        Str255 ppath;
        cstr_to_pstr(parent_path, ppath);
        err = FSMakeFSSpec(0, 0, ppath, &parentSpec);
        if (err != noErr && err != fnfErr) {
            errno = posix9_macos_to_errno(err);
            return -1;
        }

        /* Get parent's dirID */
        memset(&catInfo, 0, sizeof(catInfo));
        catInfo.hFileInfo.ioVRefNum = parentSpec.vRefNum;
        catInfo.hFileInfo.ioDirID = parentSpec.parID;
        catInfo.hFileInfo.ioNamePtr = parentSpec.name;
        catInfo.hFileInfo.ioFDirIndex = 0;

        err = PBGetCatInfoSync(&catInfo);
        if (err != noErr) {
            errno = posix9_macos_to_errno(err);
            return -1;
        }

        /* Create directory */
        err = DirCreate(parentSpec.vRefNum, catInfo.dirInfo.ioDrDirID,
                       dirname, &newDirID);
    } else {
        /* Create in current directory */
        short vRefNum;
        long dirID;
        HGetVol(NULL, &vRefNum, &dirID);
        err = DirCreate(vRefNum, dirID, dirname, &newDirID);
    }

    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    return 0;
}

int rmdir(const char *path)
{
    FSSpec spec;
    CInfoPBRec catInfo;
    OSErr err;

    /* Get FSSpec */
    err = posix9_path_to_fsspec(path, &spec);
    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    /* Verify it's a directory */
    memset(&catInfo, 0, sizeof(catInfo));
    catInfo.hFileInfo.ioVRefNum = spec.vRefNum;
    catInfo.hFileInfo.ioDirID = spec.parID;
    catInfo.hFileInfo.ioNamePtr = spec.name;
    catInfo.hFileInfo.ioFDirIndex = 0;

    err = PBGetCatInfoSync(&catInfo);
    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    if (!(catInfo.hFileInfo.ioFlAttrib & ioDirMask)) {
        errno = ENOTDIR;
        return -1;
    }

    /* Check if empty */
    if (catInfo.dirInfo.ioDrNmFls > 0) {
        errno = ENOTEMPTY;
        return -1;
    }

    /* Delete the directory */
    err = FSpDelete(&spec);
    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    return 0;
}
