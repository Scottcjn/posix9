/*
 * posix9_path.c - Path translation between POSIX and Mac OS 9
 *
 * POSIX paths:  /Volumes/Macintosh HD/Users/scott/file.txt
 * Mac paths:    Macintosh HD:Users:scott:file.txt
 *
 * Key differences:
 * - POSIX uses / as separator, Mac uses :
 * - POSIX uses / as root, Mac uses volume name
 * - POSIX relative paths start without /, Mac relative start with :
 */

#include "posix9.h"

#include <Files.h>
#include <Folders.h>
#include <string.h>

/* Default volume name if none specified */
static char default_volume[64] = "Macintosh HD";

/* Current working directory (Mac path) */
static char cwd_mac[POSIX9_PATH_MAX] = "";
static short cwd_vRefNum = 0;
static long cwd_dirID = 0;
static Boolean cwd_initialized = false;

/* ============================================================
 * Internal Helpers
 * ============================================================ */

/* Initialize current working directory */
static void init_cwd(void)
{
    OSErr err;
    FSSpec spec;
    CInfoPBRec catInfo;
    Str255 name;

    if (cwd_initialized) return;

    /* Get the application's directory as initial CWD */
    err = HGetVol(name, &cwd_vRefNum, &cwd_dirID);
    if (err == noErr) {
        /* Build path from volume root */
        /* For now, just use the volume name */
        p_to_cstr(name, cwd_mac, sizeof(cwd_mac));
    } else {
        /* Fallback to root of default volume */
        strcpy(cwd_mac, default_volume);
        cwd_vRefNum = 0;
        cwd_dirID = fsRtDirID;
    }

    cwd_initialized = true;
}

/* Convert C string to Pascal string (local helper) */
static void c2pstr(const char *cstr, Str255 pstr)
{
    size_t len = strlen(cstr);
    if (len > 255) len = 255;
    pstr[0] = (unsigned char)len;
    memcpy(&pstr[1], cstr, len);
}

/* Convert Pascal string to C string (local helper) */
static void p2cstr(const Str255 pstr, char *cstr, size_t maxlen)
{
    size_t len = pstr[0];
    if (len >= maxlen) len = maxlen - 1;
    memcpy(cstr, &pstr[1], len);
    cstr[len] = '\0';
}

/* ============================================================
 * Path Translation Functions
 * ============================================================ */

/*
 * Convert POSIX path to Mac OS 9 path
 *
 * Examples:
 *   /                           -> (volume root)
 *   /Volumes/Macintosh HD       -> Macintosh HD:
 *   /Volumes/Macintosh HD/foo   -> Macintosh HD:foo
 *   /Users/scott                -> Macintosh HD:Users:scott
 *   ./foo/bar                   -> :foo:bar
 *   ../foo                      -> ::foo
 *   foo/bar                     -> :foo:bar
 */
char *posix9_path_to_mac(const char *posix_path, char *dst, size_t dst_size)
{
    static char static_buf[POSIX9_PATH_MAX];
    char *out;
    size_t out_size;
    const char *p;
    char *d;

    /* Use static buffer if no destination provided */
    if (dst == NULL) {
        out = static_buf;
        out_size = sizeof(static_buf);
    } else {
        out = dst;
        out_size = dst_size;
    }

    if (posix_path == NULL || posix_path[0] == '\0') {
        out[0] = '\0';
        return out;
    }

    d = out;
    p = posix_path;

    /* Handle absolute paths */
    if (*p == '/') {
        p++;  /* Skip leading / */

        /* Check for /Volumes/ prefix */
        if (strncmp(p, "Volumes/", 8) == 0) {
            p += 8;
            /* Next component is volume name */
        } else {
            /* Assume default volume */
            init_cwd();
            strcpy(d, default_volume);
            d += strlen(d);
            *d++ = ':';
        }
    }
    /* Handle relative paths */
    else {
        *d++ = ':';  /* Mac relative paths start with : */

        /* Handle . and .. */
        if (*p == '.') {
            if (p[1] == '/' || p[1] == '\0') {
                p++;  /* Skip . */
                if (*p == '/') p++;
            } else if (p[1] == '.' && (p[2] == '/' || p[2] == '\0')) {
                *d++ = ':';  /* :: means parent directory */
                p += 2;
                if (*p == '/') p++;
            }
        }
    }

    /* Convert remaining path components */
    while (*p && d < out + out_size - 1) {
        if (*p == '/') {
            *d++ = ':';
            p++;
            /* Handle consecutive slashes */
            while (*p == '/') p++;
            /* Handle .. in middle of path */
            if (p[0] == '.' && p[1] == '.' && (p[2] == '/' || p[2] == '\0')) {
                *d++ = ':';
                p += 2;
                if (*p == '/') p++;
            }
        } else {
            *d++ = *p++;
        }
    }

    *d = '\0';

    /* Remove trailing : if present (unless it's volume root) */
    if (d > out + 1 && d[-1] == ':' && d[-2] != ':') {
        d[-1] = '\0';
    }

    return out;
}

/*
 * Convert Mac OS 9 path to POSIX path
 *
 * Examples:
 *   Macintosh HD:           -> /Volumes/Macintosh HD
 *   Macintosh HD:foo        -> /Volumes/Macintosh HD/foo
 *   :foo:bar                -> ./foo/bar
 *   ::foo                   -> ../foo
 */
char *posix9_path_from_mac(const char *mac_path, char *dst, size_t dst_size)
{
    static char static_buf[POSIX9_PATH_MAX];
    char *out;
    size_t out_size;
    const char *p;
    char *d;

    /* Use static buffer if no destination provided */
    if (dst == NULL) {
        out = static_buf;
        out_size = sizeof(static_buf);
    } else {
        out = dst;
        out_size = dst_size;
    }

    if (mac_path == NULL || mac_path[0] == '\0') {
        out[0] = '\0';
        return out;
    }

    d = out;
    p = mac_path;

    /* Check for relative path (starts with :) */
    if (*p == ':') {
        p++;
        *d++ = '.';

        /* Handle :: (parent directory) */
        while (*p == ':') {
            *d++ = '/';
            *d++ = '.';
            *d++ = '.';
            p++;
        }

        if (*p) {
            *d++ = '/';
        }
    }
    /* Absolute path (starts with volume name) */
    else {
        strcpy(d, "/Volumes/");
        d += 9;
    }

    /* Convert remaining path */
    while (*p && d < out + out_size - 1) {
        if (*p == ':') {
            /* Handle :: (parent directory) */
            if (p[1] == ':') {
                *d++ = '/';
                *d++ = '.';
                *d++ = '.';
                p++;
            } else {
                *d++ = '/';
            }
            p++;
        } else {
            *d++ = *p++;
        }
    }

    *d = '\0';

    return out;
}

/*
 * Convert POSIX path to FSSpec
 */
OSErr posix9_path_to_fsspec(const char *path, FSSpec *spec)
{
    char mac_path[POSIX9_PATH_MAX];
    Str255 ppath;
    OSErr err;

    /* Convert to Mac path */
    posix9_path_to_mac(path, mac_path, sizeof(mac_path));

    /* Convert to Pascal string */
    c2pstr(mac_path, ppath);

    /* Make FSSpec */
    err = FSMakeFSSpec(0, 0, ppath, spec);

    return err;
}

/* ============================================================
 * Current Working Directory
 * ============================================================ */

char *getcwd(char *buf, size_t size)
{
    init_cwd();

    if (buf == NULL) {
        errno = EINVAL;
        return NULL;
    }

    if (strlen(cwd_mac) >= size) {
        errno = ERANGE;
        return NULL;
    }

    /* Return as POSIX path */
    posix9_path_from_mac(cwd_mac, buf, size);

    return buf;
}

int chdir(const char *path)
{
    FSSpec spec;
    CInfoPBRec catInfo;
    OSErr err;
    char mac_path[POSIX9_PATH_MAX];

    /* Convert to Mac path */
    posix9_path_to_mac(path, mac_path, sizeof(mac_path));

    /* Get FSSpec */
    err = posix9_path_to_fsspec(path, &spec);
    if (err != noErr && err != fnfErr) {
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

    /* Update CWD */
    strncpy(cwd_mac, mac_path, sizeof(cwd_mac) - 1);
    cwd_mac[sizeof(cwd_mac) - 1] = '\0';
    cwd_vRefNum = spec.vRefNum;
    cwd_dirID = catInfo.dirInfo.ioDrDirID;

    /* Set Mac OS working directory */
    HSetVol(NULL, cwd_vRefNum, cwd_dirID);

    return 0;
}

/* ============================================================
 * Volume Name Configuration
 * ============================================================ */

/*
 * Set the default volume name used for paths without /Volumes/ prefix
 */
void posix9_set_default_volume(const char *name)
{
    strncpy(default_volume, name, sizeof(default_volume) - 1);
    default_volume[sizeof(default_volume) - 1] = '\0';
}

/*
 * Get the current default volume name
 */
const char *posix9_get_default_volume(void)
{
    return default_volume;
}
