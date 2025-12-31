/*
 * posix9_file.c - POSIX file I/O implementation for Mac OS 9
 *
 * Maps POSIX file operations to Mac OS File Manager calls:
 *   open()  -> FSpOpenDF / FSpCreate + FSpOpenDF
 *   read()  -> FSRead
 *   write() -> FSWrite
 *   close() -> FSClose
 *   lseek() -> SetFPos
 *   stat()  -> FSpGetFInfo + FSpGetCatInfo
 */

#include "posix9.h"

/* Mac OS headers - Retro68 provides everything via Multiverse.h */
#include <Multiverse.h>
#include "MacCompat.h"      /* Missing definitions for Retro68 */
#include <string.h>
#include <stdarg.h>

/* ============================================================
 * File Descriptor Table
 * ============================================================ */

/* Internal file descriptor structure */
typedef struct {
    short       refNum;         /* Mac OS file reference number */
    short       vRefNum;        /* Volume reference number */
    long        dirID;          /* Directory ID */
    int         flags;          /* Open flags (O_RDONLY, etc.) */
    Boolean     inUse;          /* Is this slot in use? */
    Boolean     isStdio;        /* Is this stdin/stdout/stderr? */
    char        name[64];       /* Filename (Pascal string converted) */
} posix9_fd_entry;

/* Global file descriptor table */
static posix9_fd_entry  fd_table[POSIX9_OPEN_MAX];
static Boolean          fd_table_initialized = false;

/* Global errno */
int posix9_errno = 0;

/* ============================================================
 * Mac OS Error to POSIX Errno Mapping
 * ============================================================ */

int posix9_macos_to_errno(OSErr err)
{
    switch (err) {
        case noErr:             return 0;
        case fnfErr:            return ENOENT;      /* File not found */
        case nsvErr:            return ENOENT;      /* No such volume */
        case dirNFErr:          return ENOENT;      /* Directory not found */
        case bdNamErr:          return EINVAL;      /* Bad filename */
        case ioErr:             return EIO;         /* I/O error */
        case tmfoErr:           return EMFILE;      /* Too many files open */
        case opWrErr:           return EACCES;      /* File already open for write */
        case permErr:           return EACCES;      /* Permission denied */
        case afpAccessDenied:   return EACCES;      /* AFP access denied */
        case wrPermErr:         return EROFS;       /* Write permission denied */
        case fLckdErr:          return EACCES;      /* File locked */
        case vLckdErr:          return EROFS;       /* Volume locked */
        case wPrErr:            return EROFS;       /* Hardware write protect */
        case dskFulErr:         return ENOSPC;      /* Disk full */
        case rfNumErr:          return EBADF;       /* Bad file ref number */
        case fnOpnErr:          return EBADF;       /* File not open */
        case eofErr:            return 0;           /* EOF is not an error */
        case posErr:            return EINVAL;      /* Position error */
        case dupFNErr:          return EEXIST;      /* Duplicate filename */
        case fBsyErr:           return EBUSY;       /* File busy */
        case dirFulErr:         return ENOSPC;      /* Directory full */
        case memFullErr:        return ENOMEM;      /* Memory full */
        case paramErr:          return EINVAL;      /* Parameter error */
        default:                return EIO;         /* Generic I/O error */
    }
}

/* ============================================================
 * Internal Helpers
 * ============================================================ */

/* Initialize file descriptor table */
static void init_fd_table(void)
{
    int i;

    if (fd_table_initialized) return;

    for (i = 0; i < POSIX9_OPEN_MAX; i++) {
        fd_table[i].inUse = false;
        fd_table[i].refNum = 0;
        fd_table[i].isStdio = false;
    }

    /* Reserve stdin/stdout/stderr */
    /* These map to console I/O - for now just mark as in use */
    fd_table[STDIN_FILENO].inUse = true;
    fd_table[STDIN_FILENO].isStdio = true;
    fd_table[STDOUT_FILENO].inUse = true;
    fd_table[STDOUT_FILENO].isStdio = true;
    fd_table[STDERR_FILENO].inUse = true;
    fd_table[STDERR_FILENO].isStdio = true;

    fd_table_initialized = true;
}

/* Allocate a file descriptor slot */
static int alloc_fd(void)
{
    int i;

    init_fd_table();

    /* Start at 3 to skip stdin/stdout/stderr */
    for (i = 3; i < POSIX9_OPEN_MAX; i++) {
        if (!fd_table[i].inUse) {
            fd_table[i].inUse = true;
            return i;
        }
    }

    errno = EMFILE;
    return -1;
}

/* Free a file descriptor slot */
static void free_fd(int fd)
{
    if (fd >= 0 && fd < POSIX9_OPEN_MAX && !fd_table[fd].isStdio) {
        fd_table[fd].inUse = false;
        fd_table[fd].refNum = 0;
    }
}

/* Validate file descriptor */
static posix9_fd_entry *get_fd_entry(int fd)
{
    if (fd < 0 || fd >= POSIX9_OPEN_MAX) {
        errno = EBADF;
        return NULL;
    }

    if (!fd_table[fd].inUse) {
        errno = EBADF;
        return NULL;
    }

    return &fd_table[fd];
}

/* Convert C string to Pascal string */
static void c_to_pstr(const char *cstr, Str255 pstr)
{
    size_t len = strlen(cstr);
    if (len > 255) len = 255;
    pstr[0] = (unsigned char)len;
    memcpy(&pstr[1], cstr, len);
}

/* Convert Pascal string to C string */
static void p_to_cstr(const Str255 pstr, char *cstr, size_t maxlen)
{
    size_t len = pstr[0];
    if (len >= maxlen) len = maxlen - 1;
    memcpy(cstr, &pstr[1], len);
    cstr[len] = '\0';
}

/* ============================================================
 * Path Translation (basic implementation)
 * Full implementation in posix9_path.c
 * ============================================================ */

/* Forward declaration */
OSErr posix9_path_to_fsspec(const char *path, FSSpec *spec);

/* Basic path to FSSpec - handles common cases */
static OSErr path_to_fsspec_basic(const char *path, FSSpec *spec)
{
    Str255 ppath;
    OSErr err;
    char mac_path[POSIX9_PATH_MAX];
    const char *p;
    char *m;
    short vRefNum;
    long dirID;

    /* Handle absolute POSIX paths */
    if (path[0] == '/') {
        /* Skip leading / */
        p = path + 1;

        /* Check for /Volumes/ prefix */
        if (strncmp(p, "Volumes/", 8) == 0) {
            p += 8;
        }

        /* Convert / to : */
        m = mac_path;
        while (*p && m < mac_path + sizeof(mac_path) - 1) {
            if (*p == '/') {
                *m++ = ':';
            } else {
                *m++ = *p;
            }
            p++;
        }
        *m = '\0';
    }
    /* Handle relative paths (already use : in Mac OS) */
    else if (path[0] == ':' || strchr(path, ':') != NULL) {
        /* Already a Mac path */
        strncpy(mac_path, path, sizeof(mac_path) - 1);
        mac_path[sizeof(mac_path) - 1] = '\0';
    }
    /* Handle relative POSIX paths */
    else {
        /* Prepend : for relative path, convert / to : */
        m = mac_path;
        *m++ = ':';
        p = path;
        while (*p && m < mac_path + sizeof(mac_path) - 1) {
            if (*p == '/') {
                *m++ = ':';
            } else {
                *m++ = *p;
            }
            p++;
        }
        *m = '\0';
    }

    /* Convert to Pascal string and make FSSpec */
    c_to_pstr(mac_path, ppath);

    err = FSMakeFSSpec(0, 0, ppath, spec);

    return err;
}

/* ============================================================
 * POSIX File Operations
 * ============================================================ */

int open(const char *path, int flags, ...)
{
    FSSpec spec;
    OSErr err;
    short refNum;
    int fd;
    SInt8 permission;
    va_list ap;
    mode_t mode = 0644;  /* Default mode, ignored on Mac OS */

    init_fd_table();

    /* Get mode if O_CREAT */
    if (flags & O_CREAT) {
        va_start(ap, flags);
        mode = va_arg(ap, int);  /* mode_t promoted to int */
        va_end(ap);
    }

    /* Convert path to FSSpec */
    err = path_to_fsspec_basic(path, &spec);

    /* Handle O_CREAT - create file if it doesn't exist */
    if (err == fnfErr && (flags & O_CREAT)) {
        /* File doesn't exist, create it */
        err = FSpCreate(&spec, 'TEXT', 'TEXT', smSystemScript);
        if (err != noErr && err != dupFNErr) {
            errno = posix9_macos_to_errno(err);
            return -1;
        }
        /* Re-get FSSpec after creation */
        err = path_to_fsspec_basic(path, &spec);
    }

    /* O_EXCL - fail if file exists */
    if ((flags & O_CREAT) && (flags & O_EXCL) && err == noErr) {
        /* This means file already existed */
        errno = EEXIST;
        return -1;
    }

    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    /* Determine permission */
    if ((flags & O_RDWR) == O_RDWR) {
        permission = fsRdWrPerm;
    } else if (flags & O_WRONLY) {
        permission = fsWrPerm;
    } else {
        permission = fsRdPerm;
    }

    /* Open the data fork */
    err = FSpOpenDF(&spec, permission, &refNum);
    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    /* Handle O_TRUNC - truncate file */
    if (flags & O_TRUNC) {
        SetEOF(refNum, 0);
    }

    /* Handle O_APPEND - seek to end */
    if (flags & O_APPEND) {
        SetFPos(refNum, fsFromLEOF, 0);
    }

    /* Allocate file descriptor */
    fd = alloc_fd();
    if (fd < 0) {
        FSClose(refNum);
        return -1;
    }

    /* Store in table */
    fd_table[fd].refNum = refNum;
    fd_table[fd].vRefNum = spec.vRefNum;
    fd_table[fd].dirID = spec.parID;
    fd_table[fd].flags = flags;
    p_to_cstr(spec.name, fd_table[fd].name, sizeof(fd_table[fd].name));

    return fd;
}

int close(int fd)
{
    posix9_fd_entry *entry;
    OSErr err;

    entry = get_fd_entry(fd);
    if (!entry) return -1;

    /* Don't close stdio */
    if (entry->isStdio) {
        return 0;
    }

    err = FSClose(entry->refNum);
    free_fd(fd);

    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    return 0;
}

ssize_t read(int fd, void *buf, size_t count)
{
    posix9_fd_entry *entry;
    OSErr err;
    long bytes = count;

    entry = get_fd_entry(fd);
    if (!entry) return -1;

    /* Handle stdio - TODO: implement console input */
    if (entry->isStdio) {
        if (fd == STDIN_FILENO) {
            /* Console input not yet implemented */
            errno = ENOSYS;
            return -1;
        }
        errno = EBADF;
        return -1;
    }

    err = FSRead(entry->refNum, &bytes, buf);

    /* EOF is not an error for read() */
    if (err == eofErr) {
        return bytes;  /* Return bytes read before EOF */
    }

    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    return (ssize_t)bytes;
}

ssize_t write(int fd, const void *buf, size_t count)
{
    posix9_fd_entry *entry;
    OSErr err;
    long bytes = count;

    entry = get_fd_entry(fd);
    if (!entry) return -1;

    /* Handle stdio - TODO: implement console output */
    if (entry->isStdio) {
        if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
            /* For now, use DebugStr or just succeed silently */
            /* Real implementation would write to console window */
            return count;
        }
        errno = EBADF;
        return -1;
    }

    /* Handle O_APPEND */
    if (entry->flags & O_APPEND) {
        SetFPos(entry->refNum, fsFromLEOF, 0);
    }

    err = FSWrite(entry->refNum, &bytes, buf);
    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    return (ssize_t)bytes;
}

off_t lseek(int fd, off_t offset, int whence)
{
    posix9_fd_entry *entry;
    OSErr err;
    short posMode;
    long pos;

    entry = get_fd_entry(fd);
    if (!entry) return -1;

    if (entry->isStdio) {
        errno = ESPIPE;  /* Can't seek on stdio */
        return -1;
    }

    /* Map whence to Mac OS position mode */
    switch (whence) {
        case SEEK_SET:  posMode = fsFromStart; break;
        case SEEK_CUR:  posMode = fsFromMark;  break;
        case SEEK_END:  posMode = fsFromLEOF;  break;
        default:
            errno = EINVAL;
            return -1;
    }

    err = SetFPos(entry->refNum, posMode, offset);
    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    /* Get current position */
    err = GetFPos(entry->refNum, &pos);
    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    return (off_t)pos;
}

int fstat(int fd, struct stat *buf)
{
    posix9_fd_entry *entry;
    OSErr err;
    long eof;
    FCBPBRec fcbPB;
    CInfoPBRec catInfo;
    Str255 name;

    entry = get_fd_entry(fd);
    if (!entry) return -1;

    /* Clear stat buffer */
    memset(buf, 0, sizeof(*buf));

    if (entry->isStdio) {
        /* Return character device for stdio */
        buf->st_mode = S_IFCHR | 0666;
        buf->st_nlink = 1;
        return 0;
    }

    /* Get file size */
    err = GetEOF(entry->refNum, &eof);
    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    /* Get FCB info for more details */
    fcbPB.ioRefNum = entry->refNum;
    fcbPB.ioFCBIndx = 0;
    fcbPB.ioNamePtr = name;
    err = PBGetFCBInfoSync(&fcbPB);

    /* Fill in stat structure */
    buf->st_dev = entry->vRefNum;
    buf->st_ino = entry->dirID;  /* Use dirID as inode */
    buf->st_mode = S_IFREG | 0644;  /* Regular file, rw-r--r-- */
    buf->st_nlink = 1;
    buf->st_uid = 0;
    buf->st_gid = 0;
    buf->st_size = eof;
    buf->st_blksize = 512;
    buf->st_blocks = (eof + 511) / 512;

    /* Get modification time from catalog */
    memset(&catInfo, 0, sizeof(catInfo));
    catInfo.hFileInfo.ioVRefNum = entry->vRefNum;
    catInfo.hFileInfo.ioDirID = entry->dirID;
    c_to_pstr(entry->name, name);
    catInfo.hFileInfo.ioNamePtr = name;
    catInfo.hFileInfo.ioFDirIndex = 0;

    err = PBGetCatInfoSync(&catInfo);
    if (err == noErr) {
        /* Mac OS time is seconds since Jan 1, 1904
           Unix time is seconds since Jan 1, 1970
           Difference is 2082844800 seconds */
        unsigned long macTime = catInfo.hFileInfo.ioFlMdDat;
        buf->st_mtime = macTime - 2082844800UL;
        buf->st_atime = buf->st_mtime;
        buf->st_ctime = buf->st_mtime;
    }

    return 0;
}

int stat(const char *path, struct stat *buf)
{
    FSSpec spec;
    CInfoPBRec catInfo;
    OSErr err;

    /* Clear stat buffer */
    memset(buf, 0, sizeof(*buf));

    /* Convert path to FSSpec */
    err = path_to_fsspec_basic(path, &spec);
    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    /* Get catalog info */
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

    /* Fill in stat structure */
    buf->st_dev = spec.vRefNum;
    buf->st_ino = spec.parID;  /* Use parID as inode */
    buf->st_nlink = 1;
    buf->st_uid = 0;
    buf->st_gid = 0;
    buf->st_blksize = 512;

    /* Check if directory */
    if (catInfo.hFileInfo.ioFlAttrib & ioDirMask) {
        buf->st_mode = S_IFDIR | 0755;
        buf->st_size = 0;
        buf->st_blocks = 0;
    } else {
        buf->st_mode = S_IFREG | 0644;
        buf->st_size = catInfo.hFileInfo.ioFlLgLen;  /* Logical length */
        buf->st_blocks = (buf->st_size + 511) / 512;
    }

    /* Convert Mac time to Unix time */
    {
        unsigned long macTime = catInfo.hFileInfo.ioFlMdDat;
        buf->st_mtime = macTime - 2082844800UL;
        buf->st_atime = buf->st_mtime;
        buf->st_ctime = buf->st_mtime;
    }

    return 0;
}

/* lstat - same as stat on Mac OS 9 (no symlinks) */
int lstat(const char *path, struct stat *buf)
{
    return stat(path, buf);
}

int unlink(const char *path)
{
    FSSpec spec;
    OSErr err;

    err = path_to_fsspec_basic(path, &spec);
    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    err = FSpDelete(&spec);
    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    return 0;
}

int rename(const char *oldpath, const char *newpath)
{
    FSSpec oldSpec, newSpec;
    OSErr err;
    Str255 newName;
    char *lastSlash;
    char newDir[POSIX9_PATH_MAX];
    char newFile[POSIX9_NAME_MAX];

    /* Get FSSpec for old path */
    err = path_to_fsspec_basic(oldpath, &oldSpec);
    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    /* Parse new path to get directory and filename */
    /* For now, only support same-directory rename */
    lastSlash = strrchr(newpath, '/');
    if (lastSlash == NULL) {
        lastSlash = strrchr(newpath, ':');
    }

    if (lastSlash) {
        strncpy(newFile, lastSlash + 1, sizeof(newFile) - 1);
    } else {
        strncpy(newFile, newpath, sizeof(newFile) - 1);
    }
    newFile[sizeof(newFile) - 1] = '\0';

    /* Rename the file */
    c_to_pstr(newFile, newName);
    err = FSpRename(&oldSpec, newName);
    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    return 0;
}

int fsync(int fd)
{
    posix9_fd_entry *entry;
    ParamBlockRec pb;
    OSErr err;

    entry = get_fd_entry(fd);
    if (!entry) return -1;

    if (entry->isStdio) {
        return 0;  /* No-op for stdio */
    }

    /* Flush the file */
    memset(&pb, 0, sizeof(pb));
    pb.ioParam.ioRefNum = entry->refNum;
    err = PBFlushFileSync(&pb);

    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    /* Also flush the volume */
    memset(&pb, 0, sizeof(pb));
    pb.ioParam.ioVRefNum = entry->vRefNum;
    PBFlushVolSync(&pb);

    return 0;
}

int ftruncate(int fd, off_t length)
{
    posix9_fd_entry *entry;
    OSErr err;

    entry = get_fd_entry(fd);
    if (!entry) return -1;

    if (entry->isStdio) {
        errno = EINVAL;
        return -1;
    }

    err = SetEOF(entry->refNum, length);
    if (err != noErr) {
        errno = posix9_macos_to_errno(err);
        return -1;
    }

    return 0;
}

int dup(int oldfd)
{
    posix9_fd_entry *entry;
    int newfd;

    entry = get_fd_entry(oldfd);
    if (!entry) return -1;

    newfd = alloc_fd();
    if (newfd < 0) return -1;

    /* Copy entry */
    memcpy(&fd_table[newfd], entry, sizeof(posix9_fd_entry));

    return newfd;
}

int dup2(int oldfd, int newfd)
{
    posix9_fd_entry *entry;

    entry = get_fd_entry(oldfd);
    if (!entry) return -1;

    if (newfd < 0 || newfd >= POSIX9_OPEN_MAX) {
        errno = EBADF;
        return -1;
    }

    /* Close newfd if open */
    if (fd_table[newfd].inUse && !fd_table[newfd].isStdio) {
        close(newfd);
    }

    /* Copy entry */
    memcpy(&fd_table[newfd], entry, sizeof(posix9_fd_entry));
    fd_table[newfd].inUse = true;

    return newfd;
}

/* ============================================================
 * Initialization
 * ============================================================ */

int posix9_init(void)
{
    init_fd_table();
    return 0;
}

void posix9_cleanup(void)
{
    int i;

    /* Close all open files */
    for (i = 3; i < POSIX9_OPEN_MAX; i++) {
        if (fd_table[i].inUse && !fd_table[i].isStdio) {
            close(i);
        }
    }

    fd_table_initialized = false;
}
