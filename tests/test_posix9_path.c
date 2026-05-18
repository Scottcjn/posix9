#include "posix9.h"
#include "MacCompat.h"

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

int main(void)
{
    test_posix_paths_to_mac_paths();
    test_mac_paths_to_posix_paths();
    test_empty_inputs();

    return failures == 0 ? 0 : 1;
}
