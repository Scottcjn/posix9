/*
 * os9_platform.c - Mac OS 9 platform implementation for Dropbear
 *
 * Implements OS 9 specific functionality required by Dropbear SSH.
 */

#include "os9_platform.h"

#include <Files.h>
#include <Folders.h>
#include <Processes.h>
#include <AppleEvents.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ============================================================
 * Static Data
 * ============================================================ */

/* Single user entry - OS 9 is single user */
static struct passwd os9_passwd = {
    "root",         /* pw_name */
    "x",            /* pw_passwd */
    0,              /* pw_uid */
    0,              /* pw_gid */
    "System Administrator",  /* pw_gecos */
    "/",            /* pw_dir */
    "/bin/sh"       /* pw_shell */
};

/* Single group entry */
static struct group os9_group = {
    "wheel",        /* gr_name */
    "",             /* gr_passwd */
    0,              /* gr_gid */
    NULL            /* gr_mem */
};

/* Config paths */
static char config_base[256] = "";
static Boolean config_initialized = false;

/* Logging */
static char log_ident[64] = "dropbear";
static int log_facility = LOG_DAEMON;

/* ============================================================
 * Initialization
 * ============================================================ */

static void init_config_path(void)
{
    FSSpec prefSpec;
    OSErr err;

    if (config_initialized) return;

    /* Find Preferences folder */
    err = FindFolder(kOnSystemDisk, kPreferencesFolderType,
                     kCreateFolder, &prefSpec.vRefNum, &prefSpec.parID);

    if (err == noErr) {
        /* Build path: "Macintosh HD:System Folder:Preferences:dropbear:" */
        sprintf(config_base, "::Preferences:dropbear:");
    } else {
        /* Fallback to current directory */
        strcpy(config_base, ":dropbear:");
    }

    /* Create dropbear config folder if needed */
    {
        char mac_path[256];
        Str255 ppath;
        long dirID;

        sprintf(mac_path, "%s", config_base);
        /* Convert to Pascal string */
        ppath[0] = strlen(mac_path);
        memcpy(&ppath[1], mac_path, ppath[0]);

        /* Try to create directory (ignore error if exists) */
        DirCreate(0, 0, ppath, &dirID);
    }

    config_initialized = true;
}

int os9_platform_init(void)
{
    /* Initialize POSIX9 layer */
    posix9_init();

    /* Initialize config paths */
    init_config_path();

    /* Set default environment variables */
    setenv("HOME", "/", 0);
    setenv("USER", "root", 0);
    setenv("SHELL", os9_get_default_shell(), 0);

    return 0;
}

void os9_platform_cleanup(void)
{
    posix9_cleanup();
}

/* ============================================================
 * User/Group Functions
 * ============================================================ */

struct passwd *getpwnam(const char *name)
{
    /* OS 9 is single-user - always return root */
    (void)name;
    return &os9_passwd;
}

struct passwd *getpwuid(uid_t uid)
{
    (void)uid;
    return &os9_passwd;
}

struct group *getgrnam(const char *name)
{
    (void)name;
    return &os9_group;
}

struct group *getgrgid(gid_t gid)
{
    (void)gid;
    return &os9_group;
}

void endpwent(void)
{
    /* Nothing to do */
}

/* ============================================================
 * Password Verification
 * ============================================================ */

/*
 * Simple password file format:
 * username:password
 *
 * Store in Preferences:dropbear:passwd
 */
int os9_verify_password(const char *username, const char *password)
{
    char path[256];
    char line[256];
    char *file_user, *file_pass;
    FILE *fp;

    init_config_path();

    sprintf(path, "%spasswd", config_base);

    /* Convert to POSIX path for fopen */
    {
        char posix_path[256];
        posix9_path_from_mac(path, posix_path, sizeof(posix_path));

        fp = fopen(posix_path, "r");
    }

    if (!fp) {
        /* No password file - accept any password for "root" */
        if (strcmp(username, "root") == 0) {
            return 1;
        }
        return 0;
    }

    while (fgets(line, sizeof(line), fp)) {
        /* Remove newline */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        /* Parse user:pass */
        file_user = line;
        file_pass = strchr(line, ':');

        if (!file_pass) continue;
        *file_pass++ = '\0';

        if (strcmp(file_user, username) == 0) {
            fclose(fp);
            /* Simple plaintext comparison (not secure, but simple) */
            return (strcmp(file_pass, password) == 0) ? 1 : 0;
        }
    }

    fclose(fp);
    return 0;
}

/* ============================================================
 * Entropy Source
 * ============================================================ */

int os9_get_random_bytes(void *buf, size_t len)
{
    arc4random_buf(buf, len);
    return 0;
}

/* ============================================================
 * Shell Execution
 * ============================================================ */

const char *os9_get_default_shell(void)
{
    static char shell_path[256] = "";

    if (shell_path[0] == '\0') {
        /* Try to find MPW Shell */
        /* Default to a placeholder */
        strcpy(shell_path, "/Applications/MPW Shell");
    }

    return shell_path;
}

int os9_exec_shell(const char *shell, const char *command)
{
    FSSpec appSpec;
    LaunchParamBlockRec launchParams;
    OSErr err;
    char mac_path[256];

    /* Convert POSIX path to Mac path */
    posix9_path_to_mac(shell, mac_path, sizeof(mac_path));

    /* Get FSSpec for application */
    {
        Str255 ppath;
        ppath[0] = strlen(mac_path);
        memcpy(&ppath[1], mac_path, ppath[0]);
        err = FSMakeFSSpec(0, 0, ppath, &appSpec);
    }

    if (err != noErr) {
        return -1;
    }

    /* Set up launch parameters */
    memset(&launchParams, 0, sizeof(launchParams));
    launchParams.launchBlockID = extendedBlock;
    launchParams.launchEPBLength = extendedBlockLen;
    launchParams.launchFileFlags = 0;
    launchParams.launchControlFlags = launchContinue | launchNoFileFlags;
    launchParams.launchAppSpec = &appSpec;
    launchParams.launchAppParameters = NULL;

    /* TODO: Pass command as Apple Event or parameter */
    (void)command;

    err = LaunchApplication(&launchParams);

    return (err == noErr) ? 0 : -1;
}

/* ============================================================
 * Host Key Paths
 * ============================================================ */

const char *os9_get_hostkey_path(const char *keytype)
{
    static char path[256];

    init_config_path();

    sprintf(path, "%sdropbear_%s_host_key", config_base, keytype);

    return path;
}

const char *os9_get_authorized_keys_path(const char *username)
{
    static char path[256];

    (void)username;

    init_config_path();

    sprintf(path, "%sauthorized_keys", config_base);

    return path;
}

/* ============================================================
 * PTY Emulation
 * ============================================================ */

int os9_pty_open(os9_pty_t *pty)
{
    /* Create pipe-like structure using temp files */
    /* This is a simple implementation */

    memset(pty, 0, sizeof(*pty));

    /* For now, just use stdin/stdout */
    pty->master_read = STDIN_FILENO;
    pty->master_write = STDOUT_FILENO;
    pty->slave_read = STDIN_FILENO;
    pty->slave_write = STDOUT_FILENO;

    return 0;
}

int os9_pty_close(os9_pty_t *pty)
{
    (void)pty;
    return 0;
}

int os9_pty_get_name(os9_pty_t *pty, char *buf, size_t len)
{
    (void)pty;
    strncpy(buf, "/dev/console", len - 1);
    buf[len - 1] = '\0';
    return 0;
}

/* ============================================================
 * Process Management
 * ============================================================ */

pid_t waitpid(pid_t pid, int *status, int options)
{
    /* No child processes on OS 9 */
    (void)pid;
    (void)status;
    (void)options;
    errno = ECHILD;
    return -1;
}

pid_t wait(int *status)
{
    return waitpid(-1, status, 0);
}

/* ============================================================
 * Syslog Replacement
 * ============================================================ */

static FILE *log_file = NULL;

void openlog(const char *ident, int option, int facility)
{
    (void)option;

    if (ident) {
        strncpy(log_ident, ident, sizeof(log_ident) - 1);
    }
    log_facility = facility;

    /* Open log file in Preferences */
    if (!log_file) {
        char path[256];
        init_config_path();
        sprintf(path, "%sdropbear.log", config_base);

        /* Convert to POSIX path */
        {
            char posix_path[256];
            posix9_path_from_mac(path, posix_path, sizeof(posix_path));
            log_file = fopen(posix_path, "a");
        }
    }
}

void syslog(int priority, const char *format, ...)
{
    va_list ap;
    char buf[1024];
    time_t now;
    struct tm *tm;
    const char *level;

    switch (priority) {
        case LOG_EMERG:   level = "EMERG"; break;
        case LOG_ALERT:   level = "ALERT"; break;
        case LOG_CRIT:    level = "CRIT"; break;
        case LOG_ERR:     level = "ERROR"; break;
        case LOG_WARNING: level = "WARN"; break;
        case LOG_NOTICE:  level = "NOTICE"; break;
        case LOG_INFO:    level = "INFO"; break;
        case LOG_DEBUG:   level = "DEBUG"; break;
        default:          level = "???"; break;
    }

    now = time(NULL);
    tm = localtime(&now);

    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    if (log_file) {
        fprintf(log_file, "%04d-%02d-%02d %02d:%02d:%02d %s[%s]: %s\n",
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                tm->tm_hour, tm->tm_min, tm->tm_sec,
                log_ident, level, buf);
        fflush(log_file);
    }

    /* Also print to stderr for debugging */
    fprintf(stderr, "%s[%s]: %s\n", log_ident, level, buf);
}

void closelog(void)
{
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

/* ============================================================
 * fcntl
 * ============================================================ */

int fcntl(int fd, int cmd, ...)
{
    va_list ap;
    int arg;

    va_start(ap, cmd);
    arg = va_arg(ap, int);
    va_end(ap);

    switch (cmd) {
        case F_GETFL:
            /* Return 0 (blocking mode) */
            return 0;

        case F_SETFL:
            /* Set non-blocking via OT */
            if (posix9_is_socket(fd)) {
                /* Socket - would call OTSetNonBlocking */
                /* For now, just accept it */
                return 0;
            }
            return 0;

        default:
            errno = EINVAL;
            return -1;
    }
}

/* ============================================================
 * Terminal Control (Stubs)
 * ============================================================ */

int tcgetattr(int fd, struct termios *termios_p)
{
    (void)fd;
    memset(termios_p, 0, sizeof(*termios_p));

    /* Default settings */
    termios_p->c_iflag = ICRNL | IXON;
    termios_p->c_oflag = OPOST | ONLCR;
    termios_p->c_cflag = CS8 | CREAD | CLOCAL;
    termios_p->c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK | IEXTEN;
    termios_p->c_ispeed = B9600;
    termios_p->c_ospeed = B9600;

    return 0;
}

int tcsetattr(int fd, int actions, const struct termios *termios_p)
{
    (void)fd;
    (void)actions;
    (void)termios_p;
    return 0;
}

int tcsendbreak(int fd, int duration)
{
    (void)fd;
    (void)duration;
    return 0;
}

int tcdrain(int fd)
{
    (void)fd;
    return 0;
}

int tcflush(int fd, int queue_selector)
{
    (void)fd;
    (void)queue_selector;
    return 0;
}

int tcflow(int fd, int action)
{
    (void)fd;
    (void)action;
    return 0;
}

speed_t cfgetispeed(const struct termios *termios_p)
{
    return termios_p->c_ispeed;
}

speed_t cfgetospeed(const struct termios *termios_p)
{
    return termios_p->c_ospeed;
}

int cfsetispeed(struct termios *termios_p, speed_t speed)
{
    termios_p->c_ispeed = speed;
    return 0;
}

int cfsetospeed(struct termios *termios_p, speed_t speed)
{
    termios_p->c_ospeed = speed;
    return 0;
}

/* ============================================================
 * Resource Limits
 * ============================================================ */

int getrlimit(int resource, struct rlimit *rlim)
{
    switch (resource) {
        case RLIMIT_NOFILE:
            rlim->rlim_cur = 256;
            rlim->rlim_max = 256;
            return 0;

        default:
            rlim->rlim_cur = RLIM_INFINITY;
            rlim->rlim_max = RLIM_INFINITY;
            return 0;
    }
}

int setrlimit(int resource, const struct rlimit *rlim)
{
    (void)resource;
    (void)rlim;
    return 0;
}
