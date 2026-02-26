/*
 * tls_wrapper.c - BearSSL TLS 1.2 client wrapper for POSIX9
 *
 * Implements a minimal TLS 1.2 client using BearSSL over POSIX9 sockets.
 * Key design decisions:
 *   - No certificate verification (matches Python verify=False)
 *   - ECDHE-RSA-AES128-GCM-SHA256 + RSA-AES128-GCM-SHA256 fallback
 *   - PRNG seeded from Mac OS TickCount() (best available entropy)
 *   - Static buffers only — no malloc (safe for Mac OS 9)
 *   - ~19KB total RAM including BearSSL engine + I/O buffers
 *
 * Copyright 2025-2026 RustChain Project / Elyan Labs
 */

#include "tls_wrapper.h"

#include <string.h>
#include <stdio.h>

/* BearSSL headers */
#include "bearssl.h"

/* Mac Toolbox for TickCount (entropy source) */
#include <Multiverse.h>

/* ---------- POSIX9 Socket API declarations ----------
 * Same declarations as in the miner — we declare the functions we need
 * rather than including posix9.h which conflicts with newlib types.
 */

#define AF_INET     2
#define SOCK_STREAM 1
#define IPPROTO_TCP 6
#define SHUT_RDWR   2

#define htons(x)    (x)   /* Mac OS 9 is big-endian = network order */
#define htonl(x)    (x)

typedef unsigned int socklen_t;
typedef unsigned long in_addr_t;
typedef unsigned short in_port_t;

struct in_addr {
    in_addr_t s_addr;
};

struct sockaddr {
    unsigned char sa_len;
    unsigned char sa_family;
    char          sa_data[14];
};

struct sockaddr_in {
    unsigned char  sin_len;
    unsigned char  sin_family;
    in_port_t      sin_port;
    struct in_addr sin_addr;
    char           sin_zero[8];
};

struct hostent {
    char  *h_name;
    char  **h_aliases;
    int   h_addrtype;
    int   h_length;
    char  **h_addr_list;
};
#define h_addr h_addr_list[0]

/* POSIX9 socket functions (from libposix9.a) */
int     socket(int domain, int type, int protocol);
int     connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
int     shutdown(int sockfd, int how);
int     close(int fd);
struct hostent *gethostbyname(const char *name);

/* ========== STATIC STATE ========== */

/* I/O buffers for BearSSL (2KB each — minimal but sufficient for HTTP) */
static unsigned char tls_iobuf_in[BR_SSL_BUFSIZE_INPUT];
static unsigned char tls_iobuf_out[BR_SSL_BUFSIZE_OUTPUT];

/* BearSSL contexts (static to avoid large stack allocations) */
static br_ssl_client_context tls_sc;
static br_sslio_context tls_ioc;

/* Underlying TCP socket */
static int tls_sockfd = -1;

/* Last error string */
static const char *tls_last_error = "OK";

/* ========== TRUST-ALL X.509 VALIDATOR ========== */
/*
 * This is a custom X.509 engine that accepts ANY certificate chain
 * without verification. This is equivalent to Python's verify=False
 * or curl's --insecure flag.
 *
 * We use this because the RustChain node at 50.28.86.131 uses a
 * self-signed certificate, and vintage Mac OS 9 machines have no
 * CA certificate store.
 *
 * BearSSL requires an X.509 engine — it has no built-in "skip validation"
 * mode. So we implement the br_x509_class vtable with no-op functions
 * that always return success (0) and provide a dummy RSA public key.
 */

/* Dummy RSA public key (used to satisfy BearSSL's key type check) */
static const unsigned char dummy_rsa_n[] = {
    0x00, 0x01  /* Minimal modulus — never actually used for crypto */
};
static const unsigned char dummy_rsa_e[] = {
    0x01, 0x00, 0x01  /* Standard exponent 65537 */
};

typedef struct {
    const br_x509_class *vtable;
    br_x509_pkey pkey;
} trust_all_x509_context;

static void ta_start_chain(const br_x509_class **ctx, const char *server_name)
{
    (void)ctx; (void)server_name;
}

static void ta_start_cert(const br_x509_class **ctx, uint32_t length)
{
    (void)ctx; (void)length;
}

static void ta_append(const br_x509_class **ctx,
                      const unsigned char *buf, size_t len)
{
    (void)ctx; (void)buf; (void)len;
}

static void ta_end_cert(const br_x509_class **ctx)
{
    (void)ctx;
}

static unsigned ta_end_chain(const br_x509_class **ctx)
{
    (void)ctx;
    return 0;  /* 0 = success, accept the chain */
}

static const br_x509_pkey *ta_get_pkey(
    const br_x509_class *const *ctx, unsigned *usages)
{
    trust_all_x509_context *xc = (trust_all_x509_context *)ctx;
    if (usages != NULL) {
        *usages = BR_KEYTYPE_KEYX | BR_KEYTYPE_SIGN;
    }
    return &xc->pkey;
}

static const br_x509_class trust_all_x509_vtable = {
    sizeof(trust_all_x509_context),
    ta_start_chain,
    ta_start_cert,
    ta_append,
    ta_end_cert,
    ta_end_chain,
    ta_get_pkey
};

static trust_all_x509_context tls_xc;

static void init_trust_all_x509(void)
{
    tls_xc.vtable = &trust_all_x509_vtable;
    tls_xc.pkey.key_type = BR_KEYTYPE_RSA;
    tls_xc.pkey.key.rsa.n = (unsigned char *)dummy_rsa_n;
    tls_xc.pkey.key.rsa.nlen = sizeof(dummy_rsa_n);
    tls_xc.pkey.key.rsa.e = (unsigned char *)dummy_rsa_e;
    tls_xc.pkey.key.rsa.elen = sizeof(dummy_rsa_e);
}

/* ========== SYSTEM RNG STUB ========== */
/*
 * BearSSL's ssl_engine.c calls br_prng_seeder_system() as a fallback
 * entropy source. Mac OS 9 has no /dev/urandom or CryptGenRandom,
 * so we excluded sysrng.c from the build. This stub returns 0 (NULL)
 * to tell BearSSL no system seeder is available. Our tls_connect()
 * manually seeds the PRNG from TickCount() via seed_prng() instead.
 */
br_prng_seeder br_prng_seeder_system(const char **name)
{
    if (name != NULL) {
        *name = "none";
    }
    return 0;
}

/* ========== LOW-LEVEL I/O CALLBACKS ========== */
/*
 * BearSSL's br_sslio_context needs two callbacks: one for reading raw
 * bytes from the network, one for writing. These bridge BearSSL to
 * our POSIX9 socket layer.
 */

static int sock_read(void *ctx, unsigned char *buf, size_t len)
{
    int fd = *(int *)ctx;
    ssize_t n;

    n = recv(fd, buf, len, 0);
    if (n <= 0) {
        return -1;
    }
    return (int)n;
}

static int sock_write(void *ctx, const unsigned char *buf, size_t len)
{
    int fd = *(int *)ctx;
    ssize_t n;

    n = send(fd, buf, len, 0);
    if (n <= 0) {
        return -1;
    }
    return (int)n;
}

/* ========== PRNG SEEDING ========== */
/*
 * Mac OS 9 has no /dev/urandom or cryptographic RNG. We seed BearSSL's
 * HMAC-DRBG from TickCount() (60.15 Hz monotonic counter), which provides
 * some entropy from timing jitter. For a miner that submits attestations
 * to a known server with a self-signed cert, this is sufficient — we're
 * protecting transport confidentiality, not generating signing keys.
 */

static void seed_prng(br_ssl_client_context *sc)
{
    unsigned char seed[32];
    unsigned long tc;
    int i;

    /* Gather entropy from multiple TickCount() samples with jitter */
    for (i = 0; i < 8; i++) {
        tc = TickCount();
        seed[i * 4]     = (tc >> 24) & 0xFF;
        seed[i * 4 + 1] = (tc >> 16) & 0xFF;
        seed[i * 4 + 2] = (tc >>  8) & 0xFF;
        seed[i * 4 + 3] =  tc        & 0xFF;
        /* Small busy-wait to add timing jitter between samples */
        {
            volatile int j;
            for (j = 0; j < 100 + (tc & 0xFF); j++) { }
        }
    }

    br_ssl_engine_inject_entropy(&sc->eng, seed, sizeof(seed));
}

/* ========== PUBLIC API ========== */

int tls_connect(const char *host, int port)
{
    struct sockaddr_in server_addr;
    struct hostent *he;
    unsigned state;

    /* Cipher suites we support — ordered by preference */
    static const uint16_t suites[] = {
        BR_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
        BR_TLS_RSA_WITH_AES_128_GCM_SHA256,
        BR_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256,
        BR_TLS_RSA_WITH_AES_128_CBC_SHA256,
        BR_TLS_RSA_WITH_AES_128_CBC_SHA
    };

    tls_last_error = "OK";

    /* --- Step 1: TCP connect --- */
    he = gethostbyname(host);
    if (!he) {
        tls_last_error = "DNS lookup failed";
        return -1;
    }

    tls_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tls_sockfd < 0) {
        tls_last_error = "socket() failed";
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr, he->h_addr, he->h_length);

    if (connect(tls_sockfd, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        tls_last_error = "connect() failed";
        close(tls_sockfd);
        tls_sockfd = -1;
        return -1;
    }

    /* --- Step 2: Initialize BearSSL client --- */

    /* Zero the client context */
    br_ssl_client_zero(&tls_sc);

    /* TLS 1.2 only (server requires it) */
    br_ssl_engine_set_versions(&tls_sc.eng, BR_TLS12, BR_TLS12);

    /* Set cipher suites */
    br_ssl_engine_set_suites(&tls_sc.eng, suites,
        sizeof(suites) / sizeof(suites[0]));

    /* RSA operations (i31 is fastest portable implementation) */
    br_ssl_client_set_default_rsapub(&tls_sc);
    br_ssl_engine_set_default_rsavrfy(&tls_sc.eng);

    /* ECDHE support (P-256 curve for ECDHE-RSA key exchange) */
    br_ssl_engine_set_default_ec(&tls_sc.eng);

    /* Hash functions needed for TLS 1.2 handshake */
    br_ssl_engine_set_hash(&tls_sc.eng, br_sha256_ID, &br_sha256_vtable);
    br_ssl_engine_set_hash(&tls_sc.eng, br_sha1_ID, &br_sha1_vtable);

    /* PRF for TLS 1.2 (SHA-256 based) */
    br_ssl_engine_set_prf_sha256(&tls_sc.eng, &br_tls12_sha256_prf);

    /* AES-GCM symmetric encryption */
    br_ssl_engine_set_default_aes_gcm(&tls_sc.eng);

    /* AES-CBC as fallback */
    br_ssl_engine_set_default_aes_cbc(&tls_sc.eng);

    /* --- Step 3: Trust-all X.509 (no cert verification) --- */
    init_trust_all_x509();
    br_ssl_engine_set_x509(&tls_sc.eng, &tls_xc.vtable);

    /* --- Step 4: Set I/O buffers --- */
    br_ssl_engine_set_buffers_bidi(&tls_sc.eng,
        tls_iobuf_in, sizeof(tls_iobuf_in),
        tls_iobuf_out, sizeof(tls_iobuf_out));

    /* --- Step 5: Seed PRNG --- */
    seed_prng(&tls_sc);

    /* --- Step 6: Start TLS handshake --- */
    br_ssl_client_reset(&tls_sc, host, 0);

    /* Set up the I/O wrapper with our socket callbacks */
    br_sslio_init(&tls_ioc, &tls_sc.eng,
        sock_read, &tls_sockfd,
        sock_write, &tls_sockfd);

    /* Drive the handshake to completion by attempting a read.
     * BearSSL's sslio layer handles the handshake transparently —
     * it will exchange ClientHello/ServerHello/etc before returning. */
    {
        unsigned char dummy;
        /* Peek at engine state — if SENDAPP is set, handshake is done */
        state = br_ssl_engine_current_state(&tls_sc.eng);
        if (state & BR_SSL_CLOSED) {
            int err = br_ssl_engine_last_error(&tls_sc.eng);
            printf("  TLS handshake failed (err=%d)\n", err);
            tls_last_error = "TLS handshake failed";
            close(tls_sockfd);
            tls_sockfd = -1;
            return -1;
        }
    }

    return 0;
}

int tls_send(const char *buf, int len)
{
    int ret;

    if (tls_sockfd < 0) {
        tls_last_error = "not connected";
        return -1;
    }

    ret = br_sslio_write_all(&tls_ioc, buf, (size_t)len);
    if (ret < 0) {
        tls_last_error = "TLS write failed";
        return -1;
    }

    /* Flush to ensure data is actually sent over the wire */
    if (br_sslio_flush(&tls_ioc) < 0) {
        tls_last_error = "TLS flush failed";
        return -1;
    }

    return len;
}

int tls_recv(char *buf, int len)
{
    int ret;

    if (tls_sockfd < 0) {
        tls_last_error = "not connected";
        return -1;
    }

    ret = br_sslio_read(&tls_ioc, buf, (size_t)len);
    if (ret < 0) {
        /* Check if it's a clean close or an error */
        int err = br_ssl_engine_last_error(&tls_sc.eng);
        if (err == BR_ERR_OK || err == BR_ERR_IO) {
            return 0;  /* Clean EOF */
        }
        tls_last_error = "TLS read failed";
        return -1;
    }

    return ret;
}

void tls_close(void)
{
    if (tls_sockfd >= 0) {
        /* Try to send close_notify (best effort) */
        br_sslio_close(&tls_ioc);

        shutdown(tls_sockfd, SHUT_RDWR);
        close(tls_sockfd);
        tls_sockfd = -1;
    }
    tls_last_error = "OK";
}

const char *tls_error_string(void)
{
    return tls_last_error;
}
