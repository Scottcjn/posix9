/*
 * sys/socket.h - Socket definitions for Mac OS 9
 * Maps to Open Transport internally
 */
#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H

/* Tell fake-rfc2553.h we have these */
#define HAVE_STRUCT_SOCKADDR_STORAGE 1

#include <sys/types.h>

/* Include our iovec definition */
#include <sys/uio.h>

/* Socket types */
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3

/* Address families */
#define AF_UNSPEC       0
#define AF_UNIX         1
#define AF_INET         2
#define AF_INET6        30

#define PF_UNSPEC       AF_UNSPEC
#define PF_UNIX         AF_UNIX
#define PF_INET         AF_INET
#define PF_INET6        AF_INET6

/* Socket options level */
#define SOL_SOCKET      0xffff

/* Socket options */
#define SO_DEBUG        0x0001
#define SO_REUSEADDR    0x0004
#define SO_KEEPALIVE    0x0008
#define SO_DONTROUTE    0x0010
#define SO_BROADCAST    0x0020
#define SO_LINGER       0x0080
#define SO_SNDBUF       0x1001
#define SO_RCVBUF       0x1002
#define SO_SNDTIMEO     0x1005
#define SO_RCVTIMEO     0x1006
#define SO_ERROR        0x1007
#define SO_TYPE         0x1008

/* Message flags */
#define MSG_OOB         0x1
#define MSG_PEEK        0x2
#define MSG_DONTROUTE   0x4
#define MSG_DONTWAIT    0x40
#define MSG_NOSIGNAL    0x4000

/* Shutdown flags */
#define SHUT_RD         0
#define SHUT_WR         1
#define SHUT_RDWR       2

/* sockaddr generic */
struct sockaddr {
    unsigned char   sa_len;
    unsigned char   sa_family;
    char            sa_data[14];
};

/* sockaddr_storage */
struct sockaddr_storage {
    unsigned char   ss_len;
    unsigned char   ss_family;
    char            __ss_pad1[6];
    long            __ss_align;
    char            __ss_pad2[112];
};

/* linger */
struct linger {
    int l_onoff;
    int l_linger;
};

/* Socket size type */
typedef unsigned int socklen_t;

/* msghdr for sendmsg/recvmsg */
struct msghdr {
    void            *msg_name;
    socklen_t       msg_namelen;
    struct iovec    *msg_iov;
    int             msg_iovlen;
    void            *msg_control;
    socklen_t       msg_controllen;
    int             msg_flags;
};

/* Socket functions - implemented in posix9_socket.c */
int socket(int domain, int type, int protocol);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);
int shutdown(int sockfd, int how);
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

#endif /* _SYS_SOCKET_H */
