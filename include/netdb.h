/*
 * netdb.h - Network database operations for Mac OS 9
 */
#ifndef _NETDB_H
#define _NETDB_H

#include <sys/socket.h>
#include <netinet/in.h>

/* Host entry */
struct hostent {
    char    *h_name;        /* official name of host */
    char    **h_aliases;    /* alias list */
    int     h_addrtype;     /* host address type */
    int     h_length;       /* length of address */
    char    **h_addr_list;  /* list of addresses */
};
#define h_addr h_addr_list[0]

/* Service entry */
struct servent {
    char    *s_name;        /* official service name */
    char    **s_aliases;    /* alias list */
    int     s_port;         /* port number */
    char    *s_proto;       /* protocol to use */
};

/* Protocol entry */
struct protoent {
    char    *p_name;        /* official protocol name */
    char    **p_aliases;    /* alias list */
    int     p_proto;        /* protocol number */
};

/* Address info (for getaddrinfo) */
struct addrinfo {
    int     ai_flags;
    int     ai_family;
    int     ai_socktype;
    int     ai_protocol;
    size_t  ai_addrlen;
    struct sockaddr *ai_addr;
    char    *ai_canonname;
    struct addrinfo *ai_next;
};

/* addrinfo flags */
#define AI_PASSIVE      0x0001
#define AI_CANONNAME    0x0002
#define AI_NUMERICHOST  0x0004
#define AI_NUMERICSERV  0x0008
#define AI_V4MAPPED     0x0010
#define AI_ALL          0x0020
#define AI_ADDRCONFIG   0x0040

/* getnameinfo flags */
#define NI_NUMERICHOST  0x0001
#define NI_NUMERICSERV  0x0002
#define NI_NOFQDN       0x0004
#define NI_NAMEREQD     0x0008
#define NI_DGRAM        0x0010

/* Error codes */
#define EAI_AGAIN       -3
#define EAI_BADFLAGS    -1
#define EAI_FAIL        -4
#define EAI_FAMILY      -6
#define EAI_MEMORY      -10
#define EAI_NONAME      -2
#define EAI_SERVICE     -8
#define EAI_SOCKTYPE    -7
#define EAI_SYSTEM      -11
#define EAI_OVERFLOW    -12

/* Size constants */
#define NI_MAXHOST  1025
#define NI_MAXSERV  32

/* Functions - may be stubs or partial implementations */
struct hostent *gethostbyname(const char *name);
struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type);
struct servent *getservbyname(const char *name, const char *proto);
struct servent *getservbyport(int port, const char *proto);
struct protoent *getprotobyname(const char *name);
struct protoent *getprotobynumber(int proto);

int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res);
void freeaddrinfo(struct addrinfo *res);
const char *gai_strerror(int errcode);
int getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
                char *host, socklen_t hostlen,
                char *serv, socklen_t servlen, int flags);

#endif /* _NETDB_H */
