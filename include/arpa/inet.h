/*
 * arpa/inet.h - Internet address manipulation for Mac OS 9
 */
#ifndef _ARPA_INET_H
#define _ARPA_INET_H

#include <sys/types.h>
#include <netinet/in.h>

/* Convert network to presentation format */
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);

/* Convert presentation to network format */
int inet_pton(int af, const char *src, void *dst);

/* Old-style functions */
in_addr_t inet_addr(const char *cp);
char *inet_ntoa(struct in_addr in);
int inet_aton(const char *cp, struct in_addr *inp);

/* Network to host byte order */
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);

/* Host to network byte order */
uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);

#endif /* _ARPA_INET_H */
