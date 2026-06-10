/*
 * sys/un.h - Unix domain socket stub for Mac OS 9
 * Mac OS 9 doesn't support Unix domain sockets
 */
#ifndef _SYS_UN_H
#define _SYS_UN_H

#include <sys/socket.h>

/* Unix domain socket address - not really supported on OS 9 */
struct sockaddr_un {
    unsigned char   sun_len;
    unsigned char   sun_family;
    char            sun_path[104];
};

#endif /* _SYS_UN_H */
