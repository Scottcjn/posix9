/*
 * posix9_test.c - Simple test for POSIX9 library on Mac OS 9
 *
 * Tests basic file, directory, and path operations.
 */

#include "posix9.h"
#include <Multiverse.h>
#include <string.h>

/* Simple console output - writes to a log file */
static short logRefNum = 0;

static void log_init(void)
{
    FSSpec spec;
    OSErr err;

    /* Create log file on desktop */
    err = FSMakeFSSpec(0, 0, "\pPOSIX9 Test Log", &spec);
    if (err == fnfErr) {
        FSpCreate(&spec, 'ttxt', 'TEXT', smSystemScript);
    }
    FSpOpenDF(&spec, fsWrPerm, &logRefNum);
    SetEOF(logRefNum, 0);  /* Truncate */
}

static void log_write(const char *msg)
{
    long count;

    if (logRefNum) {
        count = strlen(msg);
        FSWrite(logRefNum, &count, msg);
    }
}

static void log_close(void)
{
    if (logRefNum) {
        FSClose(logRefNum);
        logRefNum = 0;
    }
}

/* Test functions */
static int test_path_translation(void)
{
    char mac_path[256];
    char posix_path[256];

    log_write("=== Testing Path Translation ===\n");

    /* POSIX to Mac */
    posix9_path_to_mac("/Volumes/Macintosh HD/test.txt", mac_path, sizeof(mac_path));
    log_write("POSIX '/Volumes/Macintosh HD/test.txt' -> Mac '");
    log_write(mac_path);
    log_write("'\n");

    /* Relative path */
    posix9_path_to_mac("./foo/bar", mac_path, sizeof(mac_path));
    log_write("POSIX './foo/bar' -> Mac '");
    log_write(mac_path);
    log_write("'\n");

    /* Mac to POSIX */
    posix9_path_from_mac("Macintosh HD:Users:test", posix_path, sizeof(posix_path));
    log_write("Mac 'Macintosh HD:Users:test' -> POSIX '");
    log_write(posix_path);
    log_write("'\n");

    return 0;
}

static int test_file_operations(void)
{
    int fd;
    char buf[128];
    ssize_t n;
    const char *test_data = "Hello from POSIX9!\n";

    log_write("\n=== Testing File Operations ===\n");

    /* Create and write test file */
    fd = open("/test_posix9.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        log_write("ERROR: Could not create test file\n");
        return -1;
    }

    n = write(fd, test_data, strlen(test_data));
    log_write("Wrote ");
    /* Simple number to string */
    buf[0] = '0' + (n / 10);
    buf[1] = '0' + (n % 10);
    buf[2] = '\0';
    log_write(buf);
    log_write(" bytes\n");

    close(fd);

    /* Read back */
    fd = open("/test_posix9.txt", O_RDONLY);
    if (fd < 0) {
        log_write("ERROR: Could not open test file for reading\n");
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    n = read(fd, buf, sizeof(buf) - 1);
    log_write("Read back: '");
    log_write(buf);
    log_write("'\n");

    close(fd);

    /* Stat the file */
    {
        struct stat st;
        if (stat("/test_posix9.txt", &st) == 0) {
            log_write("File size: ");
            buf[0] = '0' + (st.st_size / 10);
            buf[1] = '0' + (st.st_size % 10);
            buf[2] = '\0';
            log_write(buf);
            log_write(" bytes\n");
        }
    }

    /* Clean up */
    unlink("/test_posix9.txt");

    return 0;
}

static int test_directory_operations(void)
{
    DIR *dir;
    struct dirent *ent;
    int count = 0;

    log_write("\n=== Testing Directory Operations ===\n");

    /* List root directory */
    dir = opendir("/");
    if (!dir) {
        log_write("ERROR: Could not open root directory\n");
        return -1;
    }

    log_write("Contents of root:\n");
    while ((ent = readdir(dir)) != NULL && count < 10) {
        log_write("  - ");
        log_write(ent->d_name);
        log_write("\n");
        count++;
    }

    if (count >= 10) {
        log_write("  (truncated...)\n");
    }

    closedir(dir);

    return 0;
}

static int test_cwd(void)
{
    char cwd[256];

    log_write("\n=== Testing Current Working Directory ===\n");

    if (getcwd(cwd, sizeof(cwd))) {
        log_write("Current directory: ");
        log_write(cwd);
        log_write("\n");
    } else {
        log_write("ERROR: Could not get current directory\n");
        return -1;
    }

    return 0;
}

/* Main entry point */
int main(void)
{
    int failed = 0;

    log_init();

    log_write("POSIX9 Library Test\n");
    log_write("==================\n\n");

    if (test_path_translation() != 0) failed++;
    if (test_cwd() != 0) failed++;
    if (test_file_operations() != 0) failed++;
    if (test_directory_operations() != 0) failed++;

    log_write("\n==================\n");
    if (failed == 0) {
        log_write("All tests passed!\n");
    } else {
        log_write("Some tests failed.\n");
    }

    log_close();

    return failed;
}
