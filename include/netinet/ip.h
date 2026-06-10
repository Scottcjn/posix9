/*
 * netinet/ip.h - IP header definitions for Mac OS 9
 */
#ifndef _NETINET_IP_H
#define _NETINET_IP_H

#include <stdint.h>
#include <netinet/in.h>

/* IP TOS values */
#define IPTOS_LOWDELAY      0x10
#define IPTOS_THROUGHPUT    0x08
#define IPTOS_RELIABILITY   0x04
#define IPTOS_MINCOST       0x02

/* IP header structure */
struct ip {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    unsigned int    ip_hl:4;
    unsigned int    ip_v:4;
#else
    unsigned int    ip_v:4;
    unsigned int    ip_hl:4;
#endif
    uint8_t         ip_tos;
    uint16_t        ip_len;
    uint16_t        ip_id;
    uint16_t        ip_off;
    uint8_t         ip_ttl;
    uint8_t         ip_p;
    uint16_t        ip_sum;
    struct in_addr  ip_src;
    struct in_addr  ip_dst;
};

/* IP fragment offset bits */
#define IP_RF       0x8000  /* reserved */
#define IP_DF       0x4000  /* don't fragment */
#define IP_MF       0x2000  /* more fragments */
#define IP_OFFMASK  0x1fff  /* mask for fragment offset */

/* IP socket options */
#define IP_OPTIONS      1
#define IP_HDRINCL      2
#define IP_TOS          3
#define IP_TTL          4
#define IP_RECVOPTS     5
#define IP_RECVRETOPTS  6
#define IP_RECVDSTADDR  7
#define IP_RETOPTS      8

#endif /* _NETINET_IP_H */
