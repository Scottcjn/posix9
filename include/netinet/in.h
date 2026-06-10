/*
 * netinet/in.h - Internet address definitions for Mac OS 9
 */
#ifndef _NETINET_IN_H
#define _NETINET_IN_H

/* Tell fake-rfc2553.h we already have these */
#define HAVE_STRUCT_SOCKADDR_STORAGE 1
#define HAVE_STRUCT_IN6_ADDR 1
#define HAVE_STRUCT_SOCKADDR_IN6 1
#define HAVE_STRUCT_ADDRINFO 1

#include <sys/socket.h>
#include <stdint.h>

/* IP protocols */
#define IPPROTO_IP      0
#define IPPROTO_ICMP    1
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17

/* Ports */
#define IPPORT_RESERVED     1024

/* Special addresses */
#define INADDR_ANY          ((uint32_t)0x00000000)
#define INADDR_BROADCAST    ((uint32_t)0xffffffff)
#define INADDR_LOOPBACK     ((uint32_t)0x7f000001)
#define INADDR_NONE         ((uint32_t)0xffffffff)

/* IPv4 address */
struct in_addr {
    uint32_t s_addr;
};

/* IPv4 socket address */
struct sockaddr_in {
    unsigned char   sin_len;
    unsigned char   sin_family;
    uint16_t        sin_port;
    struct in_addr  sin_addr;
    char            sin_zero[8];
};

/* IPv6 address */
struct in6_addr {
    uint8_t s6_addr[16];
};

/* IPv6 socket address */
struct sockaddr_in6 {
    unsigned char   sin6_len;
    unsigned char   sin6_family;
    uint16_t        sin6_port;
    uint32_t        sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t        sin6_scope_id;
};

/* Byte order conversion */
uint16_t htons(uint16_t hostshort);
uint16_t ntohs(uint16_t netshort);
uint32_t htonl(uint32_t hostlong);
uint32_t ntohl(uint32_t netlong);

#endif /* _NETINET_IN_H */
