/*
 * tls_wrapper.h - BearSSL TLS 1.2 client wrapper for POSIX9
 *
 * Provides a simple blocking TLS API over POSIX9 BSD sockets.
 * No certificate verification (like Python's verify=False).
 * Designed for Mac OS 9 with ~19KB total RAM budget.
 *
 * Copyright 2025-2026 RustChain Project / Elyan Labs
 */

#ifndef TLS_WRAPPER_H
#define TLS_WRAPPER_H

#include <stddef.h>

/*
 * tls_connect - Establish a TLS connection to a remote host.
 *
 * Creates a TCP socket, connects to host:port, performs a TLS 1.2
 * handshake with ECDHE-RSA-AES128-GCM-SHA256 (or RSA fallback).
 * Certificate validation is skipped (self-signed OK).
 *
 * Returns 0 on success, -1 on error.
 */
int tls_connect(const char *host, int port);

/*
 * tls_send - Send data over the TLS connection.
 *
 * Writes all 'len' bytes through BearSSL's encrypted channel.
 * Blocks until all data is sent or an error occurs.
 *
 * Returns number of bytes sent, or -1 on error.
 */
int tls_send(const char *buf, int len);

/*
 * tls_recv - Receive data from the TLS connection.
 *
 * Reads up to 'len' bytes of decrypted data.
 * May return fewer bytes than requested (partial read).
 * Returns 0 on clean connection close.
 *
 * Returns number of bytes received, 0 on EOF, -1 on error.
 */
int tls_recv(char *buf, int len);

/*
 * tls_close - Shut down TLS session and close socket.
 *
 * Sends TLS close_notify, then closes the underlying TCP socket.
 * Safe to call even if connection already closed.
 */
void tls_close(void);

/*
 * tls_error_string - Get human-readable error description.
 *
 * Returns pointer to static string describing the last TLS error,
 * or "OK" if no error.
 */
const char *tls_error_string(void);

#endif /* TLS_WRAPPER_H */
