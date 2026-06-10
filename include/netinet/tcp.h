/*
 * netinet/tcp.h - TCP protocol definitions for Mac OS 9
 */
#ifndef _NETINET_TCP_H
#define _NETINET_TCP_H

/* TCP socket options (for setsockopt at IPPROTO_TCP level) */
#define TCP_NODELAY     0x0001      /* Don't delay send to coalesce packets */
#define TCP_MAXSEG      0x0002      /* Set maximum segment size */
#define TCP_KEEPIDLE    0x0003      /* Seconds before starting keepalive probes */
#define TCP_KEEPINTVL   0x0004      /* Seconds between keepalive probes */
#define TCP_KEEPCNT     0x0005      /* Number of keepalive probes */

#endif /* _NETINET_TCP_H */
