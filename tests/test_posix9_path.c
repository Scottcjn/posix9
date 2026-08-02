#include "posix9.h"
#include "MacCompat.h"
#include <string.h>

int posix9_errno;
static int failures;

static size_t test_strlen(const char *s)
{
    size_t len = 0;

    while (s[len] != '\0') {
        len++;
    }

    return len;
}

static void test_memcpy(unsigned char *dst, const char *src, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++) {
        dst[i] = (unsigned char)src[i];
    }
}

static int test_streq(const char *expected, const char *actual)
{
    size_t i = 0;

    while (expected[i] != '\0' && actual[i] != '\0') {
        if (expected[i] != actual[i]) {
            return 0;
        }
        i++;
    }

    return expected[i] == actual[i];
}

int posix9_macos_to_errno(OSErr err)
{
    return (int)-err;
}

pascal OSErr HGetVol(StringPtr volName, short *vRefNum, long *dirID)
{
    const char *name = "Macintosh HD";
    size_t len = test_strlen(name);

    volName[0] = (unsigned char)len;
    test_memcpy(&volName[1], name, len);
    *vRefNum = 1;
    *dirID = fsRtDirID;
    return noErr;
}

pascal OSErr HSetVol(ConstStr255Param volName, short vRefNum, long dirID)
{
    (void)volName;
    (void)vRefNum;
    (void)dirID;
    return noErr;
}

pascal OSErr FSMakeFSSpec(short vRefNum, long dirID, ConstStr255Param fileName, FSSpec *spec)
{
    (void)fileName;
    spec->vRefNum = vRefNum;
    spec->parID = dirID;
    return noErr;
}

pascal OSErr PBGetCatInfoSync(CInfoPBRec *pb)
{
    pb->hFileInfo.ioFlAttrib = ioDirMask;
    pb->dirInfo.ioDrDirID = pb->hFileInfo.ioDirID;
    return noErr;
}

static void assert_str_eq(const char *label, const char *expected, const char *actual)
{
    (void)label;

    if (!test_streq(expected, actual)) {
        failures++;
    }
}

static void test_posix_paths_to_mac_paths(void)
{
    char buf[POSIX9_PATH_MAX];

    assert_str_eq(
        "absolute volume path",
        "Macintosh HD:Users:scott:file.txt",
        posix9_path_to_mac("/Volumes/Macintosh HD/Users/scott/file.txt", buf, sizeof(buf)));

    assert_str_eq(
        "relative current directory path",
        ":folder:file.txt",
        posix9_path_to_mac("./folder/file.txt", buf, sizeof(buf)));

    assert_str_eq(
        "relative parent directory path",
        "::folder:file.txt",
        posix9_path_to_mac("../folder/file.txt", buf, sizeof(buf)));

    assert_str_eq(
        "collapses repeated POSIX separators",
        ":folder:child::peer",
        posix9_path_to_mac("folder//child/../peer", buf, sizeof(buf)));
}

static void test_mac_paths_to_posix_paths(void)
{
    char buf[POSIX9_PATH_MAX];

    assert_str_eq(
        "absolute Mac path",
        "/Volumes/Macintosh HD/Users/scott/file.txt",
        posix9_path_from_mac("Macintosh HD:Users:scott:file.txt", buf, sizeof(buf)));

    assert_str_eq(
        "relative Mac path",
        "./folder/file.txt",
        posix9_path_from_mac(":folder:file.txt", buf, sizeof(buf)));

    assert_str_eq(
        "relative parent Mac path",
        "./../folder/file.txt",
        posix9_path_from_mac("::folder:file.txt", buf, sizeof(buf)));

    assert_str_eq(
        "static buffer fallback",
        "./static/path",
        posix9_path_from_mac(":static:path", NULL, 0));
}

static void test_empty_inputs(void)
{
    char buf[POSIX9_PATH_MAX];

    assert_str_eq("null POSIX path", "", posix9_path_to_mac(NULL, buf, sizeof(buf)));
    assert_str_eq("empty Mac path", "", posix9_path_from_mac("", buf, sizeof(buf)));
}

/*
 * Regression for a real buffer overflow: posix9_path_to_mac() and
 * posix9_path_from_mac() bounds-check the "convert remaining path"
 * loop against dst_size, but several earlier writes (the default-volume
 * copy, the leading ':' run for parent directories, and a second write
 * hiding inside the ".." case of the already bounds-checked loop) did
 * not check dst_size at all. Any caller passing a small dst_size -
 * exactly what the dst_size parameter exists for - got memory
 * corruption instead of truncation. Verified with AddressSanitizer on
 * malloc()'d buffers (heap-buffer-overflow in posix9_path_to_mac at the
 * strcpy(d, default_volume) site, and in posix9_path_from_mac at the
 * unbounded leading "::::" loop); this test reproduces the same three
 * call sites on the stack with canary bytes so it runs without ASAN too.
 */
static void assert_no_overflow(const char *label, int corrupted)
{
    (void)label;
    if (corrupted) {
        failures++;
    }
}

static int canary_touched(const unsigned char *canary, size_t len, unsigned char fill)
{
    size_t i;

    for (i = 0; i < len; i++) {
        if (canary[i] != fill) {
            return 1;
        }
    }
    return 0;
}

static void test_small_dst_size_does_not_overflow(void)
{
    /* Bug 1: posix9_path_to_mac() strcpy()'d the default volume name
     * ("Macintosh HD", 12 bytes) straight into dst with no length check
     * at all when the path is absolute and has no /Volumes/ prefix. */
    struct { char buf[8]; unsigned char canary[8]; } small;

    memset(&small, 0xAA, sizeof(small));
    assert_str_eq("truncated default-volume path",
        "Macinto", posix9_path_to_mac("/foo", small.buf, sizeof(small.buf)));
    assert_no_overflow("default-volume copy overflow",
        canary_touched(small.canary, sizeof(small.canary), 0xAA));

    /* Bug 2: posix9_path_from_mac() wrote "/.." per leading ':' in an
     * unconditional loop with no bounds check whatsoever, so the number
     * of bytes written was controlled entirely by the input, not dst_size. */
    {
        struct { char buf[4]; unsigned char canary[16]; } tiny;

        memset(&tiny, 0xBB, sizeof(tiny));
        assert_str_eq("truncated leading-colon-run path",
            "./f", posix9_path_from_mac("::::::::::::foo", tiny.buf, sizeof(tiny.buf)));
        assert_no_overflow("leading colon-run overflow",
            canary_touched(tiny.canary, sizeof(tiny.canary), 0xBB));
    }

    /* Bug 3: even inside the bounds-checked "convert remaining path"
     * loop of posix9_path_to_mac(), the mid-path ".." case wrote a
     * second ':' with no check, one byte past what the loop condition
     * had just confirmed was available. */
    {
        struct { char buf[6]; unsigned char canary[8]; } mid;

        memset(&mid, 0xCC, sizeof(mid));
        assert_str_eq("truncated mid-path .. handling",
            ":abc", posix9_path_to_mac("abc/../z", mid.buf, sizeof(mid.buf)));
        assert_no_overflow("mid-path .. overflow",
            canary_touched(mid.canary, sizeof(mid.canary), 0xCC));
    }
}

int main(void)
{
    test_posix_paths_to_mac_paths();
    test_mac_paths_to_posix_paths();
    test_empty_inputs();
    test_small_dst_size_does_not_overflow();

    return failures == 0 ? 0 : 1;
}
