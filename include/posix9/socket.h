/*
 * posix9/socket.h - POSIX socket definitions for Mac OS 9
 * Maps to Open Transport
 */

#ifndef POSIX9_SOCKET_H
#define POSIX9_SOCKET_H

#include "types.h"

/* Address families */
#define AF_UNSPEC       0
#define AF_UNIX         1       /* Not supported on OS 9 */
#define AF_INET         2       /* IPv4 */
#define AF_INET6        30      /* IPv6 - limited support */

#define PF_UNSPEC       AF_UNSPEC
#define PF_UNIX         AF_UNIX
#define PF_INET         AF_INET
#define PF_INET6        AF_INET6

/* Socket types */
#define SOCK_STREAM     1       /* TCP */
#define SOCK_DGRAM      2       /* UDP */
#define SOCK_RAW        3       /* Raw IP */

/* Protocol numbers */
#define IPPROTO_IP      0
#define IPPROTO_ICMP    1
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17

/* Socket options levels */
#define SOL_SOCKET      0xFFFF

/* Socket options */
#define SO_DEBUG        0x0001
#define SO_ACCEPTCONN   0x0002
#define SO_REUSEADDR    0x0004
#define SO_KEEPALIVE    0x0008
#define SO_DONTROUTE    0x0010
#define SO_BROADCAST    0x0020
#define SO_LINGER       0x0080
#define SO_OOBINLINE    0x0100
#define SO_SNDBUF       0x1001
#define SO_RCVBUF       0x1002
#define SO_SNDLOWAT     0x1003
#define SO_RCVLOWAT     0x1004
#define SO_SNDTIMEO     0x1005
#define SO_RCVTIMEO     0x1006
#define SO_ERROR        0x1007
#define SO_TYPE         0x1008

/* TCP options */
#define TCP_NODELAY     0x01

/* Shutdown modes */
#define SHUT_RD         0
#define SHUT_WR         1
#define SHUT_RDWR       2

/* Message flags */
#define MSG_OOB         0x01
#define MSG_PEEK        0x02
#define MSG_DONTROUTE   0x04
#define MSG_DONTWAIT    0x40
#define MSG_NOSIGNAL    0x4000

/* Special addresses */
#define INADDR_ANY          0x00000000UL
#define INADDR_BROADCAST    0xFFFFFFFFUL
#define INADDR_LOOPBACK     0x7F000001UL
#define INADDR_NONE         0xFFFFFFFFUL

/* socklen_t */
typedef unsigned int socklen_t;

/* in_addr_t and in_port_t */
typedef unsigned long   in_addr_t;
typedef unsigned short  in_port_t;

/* IPv4 address structure */
struct in_addr {
    in_addr_t s_addr;
};

/* Socket address structures */
struct sockaddr {
    unsigned char   sa_len;         /* Total length */
    unsigned char   sa_family;      /* Address family */
    char            sa_data[14];    /* Address data */
};

struct sockaddr_in {
    unsigned char   sin_len;        /* Length (16) */
    unsigned char   sin_family;     /* AF_INET */
    in_port_t       sin_port;       /* Port (network byte order) */
    struct in_addr  sin_addr;       /* IPv4 address */
    char            sin_zero[8];    /* Padding */
};

/* For storage of any address type */
struct sockaddr_storage {
    unsigned char   ss_len;
    unsigned char   ss_family;
    char            __ss_pad1[6];
    long long       __ss_align;
    char            __ss_pad2[112];
};

/* Host entry for DNS */
struct hostent {
    char    *h_name;            /* Official name */
    char    **h_aliases;        /* Alias list */
    int     h_addrtype;         /* Address type (AF_INET) */
    int     h_length;           /* Address length */
    char    **h_addr_list;      /* List of addresses */
};
#define h_addr h_addr_list[0]   /* First address */

/* Linger structure */
struct linger {
    int l_onoff;
    int l_linger;
};

/* timeval for select() */
struct timeval {
    long tv_sec;
    long tv_usec;
};

/* fd_set for select() */
#define FD_SETSIZE  256

typedef struct fd_set {
    unsigned long fds_bits[FD_SETSIZE / 32];
} fd_set;

#define FD_ZERO(set)        memset((set), 0, sizeof(fd_set))
#define FD_SET(fd, set)     ((set)->fds_bits[(fd) / 32] |= (1UL << ((fd) % 32)))
#define FD_CLR(fd, set)     ((set)->fds_bits[(fd) / 32] &= ~(1UL << ((fd) % 32)))
#define FD_ISSET(fd, set)   ((set)->fds_bits[(fd) / 32] & (1UL << ((fd) % 32)))

/* Byte order conversion (Mac OS 9 is big-endian, same as network order) */
#define htons(x)    (x)
#define htonl(x)    (x)
#define ntohs(x)    (x)
#define ntohl(x)    (x)

/* Socket functions */
int     socket(int domain, int type, int protocol);
int     bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int     listen(int sockfd, int backlog);
int     accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int     connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);

int     shutdown(int sockfd, int how);
int     getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
int     setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
int     getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int     getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

int     select(int nfds, fd_set *readfds, fd_set *writefds,
               fd_set *exceptfds, struct timeval *timeout);

/* DNS functions */
struct hostent *gethostbyname(const char *name);
struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type);

/* Address conversion */
in_addr_t       inet_addr(const char *cp);
char *          inet_ntoa(struct in_addr in);
int             inet_aton(const char *cp, struct in_addr *inp);
const char *    inet_ntop(int af, const void *src, char *dst, socklen_t size);
int             inet_pton(int af, const char *src, void *dst);

/* Hostname */
int     gethostname(char *name, size_t len);
int     sethostname(const char *name, size_t len);

#endif /* POSIX9_SOCKET_H */
