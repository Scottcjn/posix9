/*
 * localoptions.h - Dropbear configuration for Mac OS 9
 *
 * This file configures Dropbear for the POSIX9 compatibility layer.
 * Copy to dropbear source root before building.
 */

#ifndef DROPBEAR_LOCALOPTIONS_H
#define DROPBEAR_LOCALOPTIONS_H

/* ============================================================
 * Platform Identification
 * ============================================================ */

#define DROPBEAR_MACOS9 1
#define POSIX9_COMPAT 1

/* ============================================================
 * Server Configuration
 * ============================================================ */

/* Enable SSH server */
#define DROPBEAR_SVR_PASSWORD_AUTH 1
#define DROPBEAR_SVR_PUBKEY_AUTH 1

/* Default port */
#define DROPBEAR_DEFPORT "22"

/* Max auth attempts */
#define MAX_AUTH_TRIES 5

/* Idle timeout (0 = no timeout) */
#define DEFAULT_IDLE_TIMEOUT 0

/* ============================================================
 * Disable Client Features
 * (We only build the server)
 * ============================================================ */

#define DROPBEAR_CLI_PASSWORD_AUTH 0
#define DROPBEAR_CLI_PUBKEY_AUTH 0
#define DROPBEAR_CLI_AGENTFWD 0
#define DROPBEAR_CLI_LOCALTCPFWD 0
#define DROPBEAR_CLI_REMOTETCPFWD 0
#define DROPBEAR_CLI_NETCAT 0

/* ============================================================
 * Disable Features Requiring fork()
 * ============================================================ */

/* No forking on OS 9 */
#define DROPBEAR_USE_VFORK 0

/* Disable port forwarding (requires fork) */
#define DROPBEAR_SVR_LOCALTCPFWD 0
#define DROPBEAR_SVR_REMOTETCPFWD 0

/* Disable X11 forwarding (no X11 on OS 9) */
#define DROPBEAR_X11FWD 0

/* Disable agent forwarding (no Unix sockets) */
#define DROPBEAR_SVR_AGENTFWD 0
#define DROPBEAR_CLI_AGENTFWD 0

/* ============================================================
 * Crypto Configuration
 * ============================================================ */

/* Key exchange algorithms */
#define DROPBEAR_CURVE25519 1
#define DROPBEAR_ECDH 1
#define DROPBEAR_DH_GROUP14_SHA256 1
#define DROPBEAR_DH_GROUP14_SHA1 1
#define DROPBEAR_DH_GROUP16 0       /* Too slow on G3/G4 */
#define DROPBEAR_KEXGUESS2 0

/* Ciphers - prefer faster ones for G3/G4 */
#define DROPBEAR_AES128 1
#define DROPBEAR_AES256 1
#define DROPBEAR_3DES 1             /* Slower but widely supported */
#define DROPBEAR_CHACHA20POLY1305 1 /* Fast on PPC with good compiler */
#define DROPBEAR_ENABLE_GCM_MODE 1

/* MACs */
#define DROPBEAR_SHA2_256_HMAC 1
#define DROPBEAR_SHA1_HMAC 1
#define DROPBEAR_SHA1_96_HMAC 1

/* Host key types */
#define DROPBEAR_RSA 1
#define DROPBEAR_DSS 1
#define DROPBEAR_ECDSA 1
#define DROPBEAR_ED25519 1

/* Key sizes */
#define DROPBEAR_DEFAULT_RSA_SIZE 2048
#define DROPBEAR_MIN_RSA_KEYLEN 1024    /* Accept older keys */

/* ============================================================
 * Compression
 * ============================================================ */

/* Disable zlib compression (optional, saves code size) */
#define DROPBEAR_ZLIB 0

/* ============================================================
 * Buffer Sizes
 * ============================================================ */

/* Reduce memory usage for OS 9 */
#define RECV_MAX_PAYLOAD_LEN 32768
#define TRANS_MAX_PAYLOAD_LEN 32768
#define MAX_PACKET_LEN 35000

/* Maximum channels (usually just 1 for shell) */
#define DROPBEAR_MAX_CHANNELS 8

/* ============================================================
 * File Paths (Mac OS 9 style)
 * ============================================================ */

/* These use Mac path separators via POSIX9 translation */
#define DROPBEAR_PATH_SSH_PROGRAM "/bin/ssh"
#define DROPBEAR_SFTPSERVER ""          /* Disabled */
#define DROPBEAR_PIDFILE ""             /* No PID file */

/* ============================================================
 * OS 9 Specific Overrides
 * ============================================================ */

/* No syslog on OS 9 */
#define DROPBEAR_SYSLOG 0

/* No /dev/urandom - use posix9 arc4random */
#define DROPBEAR_USE_DEVRANDOM 0

/* No privilege separation (single user OS) */
#define DROPBEAR_SVR_MULTIUSER 0

/* No password shadow file */
#define HAVE_SHADOW_H 0

/* No utmp/wtmp */
#define HAVE_UTMP_H 0
#define HAVE_UTMPX_H 0
#define HAVE_LASTLOG_H 0

/* No setpriority/getpriority */
#define HAVE_SETPRIORITY 0

/* No setvbuf */
#define HAVE_SETVBUF 0

/* ============================================================
 * PTY Configuration
 * ============================================================ */

/* No real PTYs on OS 9 - we fake it */
#define DROPBEAR_PTY 0
#define USE_DEV_PTMX 0
#define HAVE_OPENPTY 0

/* ============================================================
 * Misc
 * ============================================================ */

/* No motd */
#define MOTD_FILENAME ""

/* Banner file */
#define DROPBEAR_BANNER ""

/* Debug - disable for production */
#define DEBUG_TRACE 0

/* Use our own entropy source */
#define DROPBEAR_PRNGD_SOCKET ""

#endif /* DROPBEAR_LOCALOPTIONS_H */
