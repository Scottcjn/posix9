/*
 * posix9_socket.c - POSIX socket implementation for Mac OS 9
 *
 * Maps POSIX socket API to Open Transport:
 *   socket()   -> OTOpenEndpointInContext
 *   connect()  -> OTConnect
 *   bind()     -> OTBind
 *   listen()   -> OTListen
 *   accept()   -> OTAccept
 *   send()     -> OTSnd
 *   recv()     -> OTRcv
 *   close()    -> OTCloseProvider
 *   select()   -> OTLook + polling
 *
 * Open Transport is inherently async; we wrap it for blocking semantics.
 */

#include "posix9.h"
#include "posix9/socket.h"

#include <OpenTransport.h>
#include <OpenTptInternet.h>
#include <string.h>

/* ============================================================
 * Socket Table
 * ============================================================ */

typedef struct {
    EndpointRef     ep;             /* Open Transport endpoint */
    int             domain;         /* AF_INET, etc. */
    int             type;           /* SOCK_STREAM, etc. */
    int             protocol;       /* IPPROTO_TCP, etc. */
    Boolean         inUse;          /* Slot in use */
    Boolean         bound;          /* Has been bound */
    Boolean         listening;      /* In listen mode */
    Boolean         connected;      /* Connection established */
    Boolean         nonblocking;    /* Non-blocking mode */
    TEndpointInfo   info;           /* Endpoint info */
    InetAddress     localAddr;      /* Local address */
    InetAddress     peerAddr;       /* Remote address */
    OTResult        asyncError;     /* Last async error */
    Boolean         readable;       /* Data available */
    Boolean         writable;       /* Can write */
    Boolean         hasOOB;         /* OOB data available */
} posix9_socket_entry;

#define MAX_SOCKETS 128
#define SOCKET_FD_BASE 1000         /* Socket FDs start at 1000 */

static posix9_socket_entry socket_table[MAX_SOCKETS];
static Boolean socket_table_initialized = false;
static Boolean ot_initialized = false;

/* DNS result storage */
static struct hostent   dns_result;
static char             dns_name[256];
static char *           dns_aliases[1] = { NULL };
static char *           dns_addrs[2] = { NULL, NULL };
static struct in_addr   dns_addr;

/* ============================================================
 * Open Transport Initialization
 * ============================================================ */

static OSStatus init_open_transport(void)
{
    OSStatus err;

    if (ot_initialized) return noErr;

    err = InitOpenTransportInContext(kInitOTForApplicationMask, NULL);
    if (err == noErr) {
        ot_initialized = true;
    }

    return err;
}

static void cleanup_open_transport(void)
{
    if (ot_initialized) {
        CloseOpenTransportInContext(NULL);
        ot_initialized = false;
    }
}

/* ============================================================
 * Socket Table Management
 * ============================================================ */

static void init_socket_table(void)
{
    int i;

    if (socket_table_initialized) return;

    for (i = 0; i < MAX_SOCKETS; i++) {
        socket_table[i].inUse = false;
        socket_table[i].ep = kOTInvalidEndpointRef;
    }

    socket_table_initialized = true;
}

static int alloc_socket(void)
{
    int i;

    init_socket_table();

    for (i = 0; i < MAX_SOCKETS; i++) {
        if (!socket_table[i].inUse) {
            memset(&socket_table[i], 0, sizeof(posix9_socket_entry));
            socket_table[i].inUse = true;
            socket_table[i].ep = kOTInvalidEndpointRef;
            return SOCKET_FD_BASE + i;
        }
    }

    errno = EMFILE;
    return -1;
}

static void free_socket(int fd)
{
    int idx = fd - SOCKET_FD_BASE;
    if (idx >= 0 && idx < MAX_SOCKETS) {
        socket_table[idx].inUse = false;
        socket_table[idx].ep = kOTInvalidEndpointRef;
    }
}

static posix9_socket_entry *get_socket(int fd)
{
    int idx = fd - SOCKET_FD_BASE;

    if (idx < 0 || idx >= MAX_SOCKETS) {
        errno = EBADF;
        return NULL;
    }

    if (!socket_table[idx].inUse) {
        errno = EBADF;
        return NULL;
    }

    return &socket_table[idx];
}

/* Check if fd is a socket */
Boolean posix9_is_socket(int fd)
{
    int idx = fd - SOCKET_FD_BASE;
    return (idx >= 0 && idx < MAX_SOCKETS && socket_table[idx].inUse);
}

/* ============================================================
 * Open Transport Notifier (for async events)
 * ============================================================ */

static pascal void socket_notifier(void *context, OTEventCode event,
                                   OTResult result, void *cookie)
{
    posix9_socket_entry *sock = (posix9_socket_entry *)context;

    (void)cookie;

    if (!sock) return;

    switch (event) {
        case T_DATA:
            sock->readable = true;
            break;

        case T_GODATA:
            sock->writable = true;
            break;

        case T_EXDATA:
            sock->hasOOB = true;
            break;

        case T_CONNECT:
            sock->connected = true;
            sock->asyncError = result;
            break;

        case T_DISCONNECT:
        case T_ORDREL:
            sock->connected = false;
            break;

        case T_LISTEN:
            /* Incoming connection available */
            sock->readable = true;
            break;

        case T_PASSCON:
            /* Connection passed to new endpoint */
            break;

        default:
            break;
    }
}

/* ============================================================
 * OT Error to POSIX errno mapping
 * ============================================================ */

static int ot_error_to_errno(OTResult err)
{
    switch (err) {
        case kOTNoError:            return 0;
        case kOTBadAddressErr:      return EADDRNOTAVAIL;
        case kOTBadOptionErr:       return EINVAL;
        case kOTAccessErr:          return EACCES;
        case kOTBadReferenceErr:    return EBADF;
        case kOTNoAddressErr:       return EDESTADDRREQ;
        case kOTOutStateErr:        return EINVAL;
        case kOTBadSequenceErr:     return EINVAL;
        case kOTSysErrorErr:        return EIO;
        case kOTLookErr:            return EAGAIN;
        case kOTBadDataErr:         return EMSGSIZE;
        case kOTBufferOverflowErr:  return ENOBUFS;
        case kOTFlowErr:            return EAGAIN;
        case kOTNoDataErr:          return EAGAIN;
        case kOTNoDisconnectErr:    return ENOTCONN;
        case kOTNoUDErr:            return 0;
        case kOTBadFlagErr:         return EINVAL;
        case kOTNoRelErr:           return ENOTCONN;
        case kOTNotSupportedErr:    return EOPNOTSUPP;
        case kOTStateChangeErr:     return EINVAL;
        case kOTNoStructureTypeErr: return EINVAL;
        case kOTBadNameErr:         return EINVAL;
        case kOTBadQLenErr:         return EINVAL;
        case kOTAddressBusyErr:     return EADDRINUSE;
        case kOTIndOutErr:          return EINVAL;
        case kOTProviderMismatchErr: return EAFNOSUPPORT;
        case kOTResQLenErr:         return EINVAL;
        case kOTResAddressErr:      return EADDRNOTAVAIL;
        case kOTQFullErr:           return ENOBUFS;
        case kOTProtocolErr:        return EPROTONOSUPPORT;
        case kOTBadSyncErr:         return EINVAL;
        case kOTCanceledErr:        return ECANCELED;
        case kEPERMErr:             return EPERM;
        case kENOENTErr:            return ENOENT;
        case kEINTRErr:             return EINTR;
        case kEIOErr:               return EIO;
        case kENXIOErr:             return ENXIO;
        case kEBADFErr:             return EBADF;
        case kEAGAINErr:            return EAGAIN;
        case kENOMEMErr:            return ENOMEM;
        case kEACCESErr:            return EACCES;
        case kEFAULTErr:            return EFAULT;
        case kEBUSYErr:             return EBUSY;
        case kEEXISTErr:            return EEXIST;
        case kENODEVErr:            return ENODEV;
        case kEINVALErr:            return EINVAL;
        case kENOTTYErr:            return ENOTTY;
        case kEPIPEErr:             return EPIPE;
        case kERANGEErr:            return ERANGE;
        case kEWOULDBLOCKErr:       return EWOULDBLOCK;
        case kEDEADLKErr:           return EDEADLK;
        case kENOTSOCKErr:          return ENOTSOCK;
        case kEDESTADDRREQErr:      return EDESTADDRREQ;
        case kEMSGSIZEErr:          return EMSGSIZE;
        case kEPROTOTYPEErr:        return EPROTOTYPE;
        case kENOPROTOOPTErr:       return ENOPROTOOPT;
        case kEPROTONOSUPPORTErr:   return EPROTONOSUPPORT;
        case kESOCKTNOSUPPORTErr:   return ESOCKTNOSUPPORT;
        case kEOPNOTSUPPErr:        return EOPNOTSUPP;
        case kEADDRINUSEErr:        return EADDRINUSE;
        case kEADDRNOTAVAILErr:     return EADDRNOTAVAIL;
        case kENETDOWNErr:          return ENETDOWN;
        case kENETUNREACHErr:       return ENETUNREACH;
        case kENETRESETErr:         return ENETRESET;
        case kECONNABORTEDErr:      return ECONNABORTED;
        case kECONNRESETErr:        return ECONNRESET;
        case kENOBUFSErr:           return ENOBUFS;
        case kEISCONNErr:           return EISCONN;
        case kENOTCONNErr:          return ENOTCONN;
        case kESHUTDOWNErr:         return ESHUTDOWN;
        case kETIMEDOUTErr:         return ETIMEDOUT;
        case kECONNREFUSEDErr:      return ECONNREFUSED;
        case kEHOSTDOWNErr:         return EHOSTDOWN;
        case kEHOSTUNREACHErr:      return EHOSTUNREACH;
        case kEALREADYErr:          return EALREADY;
        case kEINPROGRESSErr:       return EINPROGRESS;
        default:                    return EIO;
    }
}

/* ============================================================
 * POSIX Socket Functions
 * ============================================================ */

int socket(int domain, int type, int protocol)
{
    int fd;
    posix9_socket_entry *sock;
    OSStatus err;
    OTConfigurationRef config;
    const char *configStr;

    /* Initialize OT if needed */
    err = init_open_transport();
    if (err != noErr) {
        errno = EIO;
        return -1;
    }

    /* Only support AF_INET for now */
    if (domain != AF_INET) {
        errno = EAFNOSUPPORT;
        return -1;
    }

    /* Determine configuration string */
    if (type == SOCK_STREAM) {
        configStr = kTCPName;  /* "tcp" */
        if (protocol == 0) protocol = IPPROTO_TCP;
    } else if (type == SOCK_DGRAM) {
        configStr = kUDPName;  /* "udp" */
        if (protocol == 0) protocol = IPPROTO_UDP;
    } else {
        errno = ESOCKTNOSUPPORT;
        return -1;
    }

    /* Allocate socket slot */
    fd = alloc_socket();
    if (fd < 0) return -1;

    sock = get_socket(fd);

    /* Create OT configuration */
    config = OTCreateConfiguration(configStr);
    if (config == kOTInvalidConfigurationRef) {
        free_socket(fd);
        errno = EPROTONOSUPPORT;
        return -1;
    }

    /* Open endpoint */
    sock->ep = OTOpenEndpointInContext(config, 0, &sock->info, &err, NULL);

    if (err != noErr || sock->ep == kOTInvalidEndpointRef) {
        free_socket(fd);
        errno = ot_error_to_errno(err);
        return -1;
    }

    /* Install notifier for async events */
    err = OTInstallNotifier(sock->ep, NewOTNotifyUPP(socket_notifier), sock);
    if (err != noErr) {
        OTCloseProvider(sock->ep);
        free_socket(fd);
        errno = EIO;
        return -1;
    }

    /* Set to synchronous mode by default */
    OTSetSynchronous(sock->ep);
    OTSetBlocking(sock->ep);

    /* Store socket info */
    sock->domain = domain;
    sock->type = type;
    sock->protocol = protocol;
    sock->writable = true;

    return fd;
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    posix9_socket_entry *sock;
    const struct sockaddr_in *sin;
    TBind req, ret;
    InetAddress reqAddr, retAddr;
    OSStatus err;

    (void)addrlen;

    sock = get_socket(sockfd);
    if (!sock) return -1;

    sin = (const struct sockaddr_in *)addr;

    /* Set up bind request */
    OTInitInetAddress(&reqAddr, ntohs(sin->sin_port), ntohl(sin->sin_addr.s_addr));

    req.addr.buf = (UInt8 *)&reqAddr;
    req.addr.len = sizeof(reqAddr);
    req.qlen = 0;

    ret.addr.buf = (UInt8 *)&retAddr;
    ret.addr.maxlen = sizeof(retAddr);

    /* Perform bind */
    err = OTBind(sock->ep, &req, &ret);
    if (err != noErr) {
        errno = ot_error_to_errno(err);
        return -1;
    }

    sock->bound = true;
    sock->localAddr = retAddr;

    return 0;
}

int listen(int sockfd, int backlog)
{
    posix9_socket_entry *sock;
    TBind req, ret;
    OSStatus err;

    sock = get_socket(sockfd);
    if (!sock) return -1;

    if (sock->type != SOCK_STREAM) {
        errno = EOPNOTSUPP;
        return -1;
    }

    /* For TCP, we need to unbind and rebind with qlen > 0 */
    if (sock->bound) {
        /* Already bound, just set qlen */
        /* OT requires re-bind to change qlen */
        req.addr.buf = (UInt8 *)&sock->localAddr;
        req.addr.len = sizeof(sock->localAddr);
        req.qlen = backlog > 0 ? backlog : 5;

        ret.addr.buf = (UInt8 *)&sock->localAddr;
        ret.addr.maxlen = sizeof(sock->localAddr);

        OTUnbind(sock->ep);
        err = OTBind(sock->ep, &req, &ret);
        if (err != noErr) {
            errno = ot_error_to_errno(err);
            return -1;
        }
    }

    sock->listening = true;
    return 0;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    posix9_socket_entry *sock, *newsock;
    int newfd;
    TCall call;
    InetAddress clientAddr;
    OSStatus err;
    EndpointRef newep;
    struct sockaddr_in *sin;

    sock = get_socket(sockfd);
    if (!sock) return -1;

    if (!sock->listening) {
        errno = EINVAL;
        return -1;
    }

    /* Set up call structure to receive connection info */
    memset(&call, 0, sizeof(call));
    call.addr.buf = (UInt8 *)&clientAddr;
    call.addr.maxlen = sizeof(clientAddr);

    /* Wait for connection */
    err = OTListen(sock->ep, &call);
    if (err != noErr) {
        errno = ot_error_to_errno(err);
        return -1;
    }

    /* Allocate new socket for accepted connection */
    newfd = alloc_socket();
    if (newfd < 0) {
        OTSndDisconnect(sock->ep, &call);
        return -1;
    }

    newsock = get_socket(newfd);

    /* Open new endpoint for accepted connection */
    newsock->ep = OTOpenEndpointInContext(
        OTCreateConfiguration(kTCPName),
        0, &newsock->info, &err, NULL);

    if (err != noErr) {
        free_socket(newfd);
        OTSndDisconnect(sock->ep, &call);
        errno = ot_error_to_errno(err);
        return -1;
    }

    /* Accept the connection on new endpoint */
    err = OTAccept(sock->ep, newsock->ep, &call);
    if (err != noErr) {
        OTCloseProvider(newsock->ep);
        free_socket(newfd);
        errno = ot_error_to_errno(err);
        return -1;
    }

    /* Install notifier */
    OTInstallNotifier(newsock->ep, NewOTNotifyUPP(socket_notifier), newsock);
    OTSetSynchronous(newsock->ep);
    OTSetBlocking(newsock->ep);

    /* Copy socket properties */
    newsock->domain = sock->domain;
    newsock->type = sock->type;
    newsock->protocol = sock->protocol;
    newsock->connected = true;
    newsock->peerAddr = clientAddr;
    newsock->writable = true;

    /* Return client address */
    if (addr && addrlen) {
        sin = (struct sockaddr_in *)addr;
        sin->sin_len = sizeof(*sin);
        sin->sin_family = AF_INET;
        sin->sin_port = htons(clientAddr.fPort);
        sin->sin_addr.s_addr = htonl(clientAddr.fHost);
        memset(sin->sin_zero, 0, sizeof(sin->sin_zero));
        *addrlen = sizeof(*sin);
    }

    return newfd;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    posix9_socket_entry *sock;
    const struct sockaddr_in *sin;
    TCall sndCall;
    InetAddress destAddr;
    OSStatus err;

    (void)addrlen;

    sock = get_socket(sockfd);
    if (!sock) return -1;

    sin = (const struct sockaddr_in *)addr;

    /* Set up destination address */
    OTInitInetAddress(&destAddr, ntohs(sin->sin_port), ntohl(sin->sin_addr.s_addr));

    memset(&sndCall, 0, sizeof(sndCall));
    sndCall.addr.buf = (UInt8 *)&destAddr;
    sndCall.addr.len = sizeof(destAddr);

    /* Connect */
    err = OTConnect(sock->ep, &sndCall, NULL);
    if (err != noErr) {
        errno = ot_error_to_errno(err);
        return -1;
    }

    sock->connected = true;
    sock->peerAddr = destAddr;

    return 0;
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
    posix9_socket_entry *sock;
    OTResult result;
    OTFlags otFlags = 0;

    sock = get_socket(sockfd);
    if (!sock) return -1;

    if (!sock->connected && sock->type == SOCK_STREAM) {
        errno = ENOTCONN;
        return -1;
    }

    if (flags & MSG_OOB) otFlags |= T_EXPEDITED;

    result = OTSnd(sock->ep, (void *)buf, len, otFlags);

    if (result < 0) {
        errno = ot_error_to_errno(result);
        return -1;
    }

    return (ssize_t)result;
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
    posix9_socket_entry *sock;
    OTResult result;
    OTFlags otFlags = 0;

    sock = get_socket(sockfd);
    if (!sock) return -1;

    result = OTRcv(sock->ep, buf, len, &otFlags);

    if (result < 0) {
        if (result == kOTNoDataErr) {
            if (sock->nonblocking) {
                errno = EAGAIN;
                return -1;
            }
            return 0;  /* EOF */
        }
        errno = ot_error_to_errno(result);
        return -1;
    }

    sock->readable = false;  /* Will be set again by notifier */

    return (ssize_t)result;
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen)
{
    posix9_socket_entry *sock;
    const struct sockaddr_in *sin;
    TUnitData udata;
    InetAddress destAddr;
    OSStatus err;

    (void)flags;
    (void)addrlen;

    sock = get_socket(sockfd);
    if (!sock) return -1;

    if (sock->type != SOCK_DGRAM) {
        /* For STREAM, just use send */
        return send(sockfd, buf, len, flags);
    }

    sin = (const struct sockaddr_in *)dest_addr;
    OTInitInetAddress(&destAddr, ntohs(sin->sin_port), ntohl(sin->sin_addr.s_addr));

    memset(&udata, 0, sizeof(udata));
    udata.addr.buf = (UInt8 *)&destAddr;
    udata.addr.len = sizeof(destAddr);
    udata.udata.buf = (UInt8 *)buf;
    udata.udata.len = len;

    err = OTSndUData(sock->ep, &udata);
    if (err != noErr) {
        errno = ot_error_to_errno(err);
        return -1;
    }

    return (ssize_t)len;
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen)
{
    posix9_socket_entry *sock;
    TUnitData udata;
    InetAddress srcAddr;
    OTFlags otFlags = 0;
    OSStatus err;
    struct sockaddr_in *sin;

    (void)flags;

    sock = get_socket(sockfd);
    if (!sock) return -1;

    if (sock->type != SOCK_DGRAM) {
        /* For STREAM, just use recv */
        return recv(sockfd, buf, len, flags);
    }

    memset(&udata, 0, sizeof(udata));
    udata.addr.buf = (UInt8 *)&srcAddr;
    udata.addr.maxlen = sizeof(srcAddr);
    udata.udata.buf = (UInt8 *)buf;
    udata.udata.maxlen = len;

    err = OTRcvUData(sock->ep, &udata, &otFlags);
    if (err != noErr) {
        errno = ot_error_to_errno(err);
        return -1;
    }

    if (src_addr && addrlen) {
        sin = (struct sockaddr_in *)src_addr;
        sin->sin_len = sizeof(*sin);
        sin->sin_family = AF_INET;
        sin->sin_port = htons(srcAddr.fPort);
        sin->sin_addr.s_addr = htonl(srcAddr.fHost);
        *addrlen = sizeof(*sin);
    }

    return (ssize_t)udata.udata.len;
}

int shutdown(int sockfd, int how)
{
    posix9_socket_entry *sock;
    OSStatus err = noErr;

    sock = get_socket(sockfd);
    if (!sock) return -1;

    if (how == SHUT_RD || how == SHUT_RDWR) {
        /* Can't really shut down read side in OT */
    }

    if (how == SHUT_WR || how == SHUT_RDWR) {
        err = OTSndOrderlyDisconnect(sock->ep);
    }

    if (err != noErr) {
        errno = ot_error_to_errno(err);
        return -1;
    }

    return 0;
}

int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    posix9_socket_entry *sock;
    struct sockaddr_in *sin;

    sock = get_socket(sockfd);
    if (!sock) return -1;

    sin = (struct sockaddr_in *)addr;
    sin->sin_len = sizeof(*sin);
    sin->sin_family = AF_INET;
    sin->sin_port = htons(sock->localAddr.fPort);
    sin->sin_addr.s_addr = htonl(sock->localAddr.fHost);
    memset(sin->sin_zero, 0, sizeof(sin->sin_zero));
    *addrlen = sizeof(*sin);

    return 0;
}

int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    posix9_socket_entry *sock;
    struct sockaddr_in *sin;

    sock = get_socket(sockfd);
    if (!sock) return -1;

    if (!sock->connected) {
        errno = ENOTCONN;
        return -1;
    }

    sin = (struct sockaddr_in *)addr;
    sin->sin_len = sizeof(*sin);
    sin->sin_family = AF_INET;
    sin->sin_port = htons(sock->peerAddr.fPort);
    sin->sin_addr.s_addr = htonl(sock->peerAddr.fHost);
    memset(sin->sin_zero, 0, sizeof(sin->sin_zero));
    *addrlen = sizeof(*sin);

    return 0;
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
    posix9_socket_entry *sock;

    sock = get_socket(sockfd);
    if (!sock) return -1;

    /* Basic implementation - return defaults */
    if (level == SOL_SOCKET) {
        switch (optname) {
            case SO_TYPE:
                *(int *)optval = sock->type;
                *optlen = sizeof(int);
                return 0;

            case SO_ERROR:
                *(int *)optval = sock->asyncError;
                sock->asyncError = 0;
                *optlen = sizeof(int);
                return 0;

            default:
                errno = ENOPROTOOPT;
                return -1;
        }
    }

    errno = ENOPROTOOPT;
    return -1;
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
    posix9_socket_entry *sock;

    (void)optval;
    (void)optlen;

    sock = get_socket(sockfd);
    if (!sock) return -1;

    /* Basic implementation - accept but mostly ignore */
    if (level == SOL_SOCKET) {
        switch (optname) {
            case SO_REUSEADDR:
            case SO_KEEPALIVE:
            case SO_BROADCAST:
                /* Accept these, OT handles automatically */
                return 0;

            default:
                errno = ENOPROTOOPT;
                return -1;
        }
    }

    if (level == IPPROTO_TCP && optname == TCP_NODELAY) {
        /* OT TCP has no Nagle by default, so this is a no-op */
        return 0;
    }

    errno = ENOPROTOOPT;
    return -1;
}

/* ============================================================
 * select() implementation
 * ============================================================ */

int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout)
{
    int count = 0;
    int fd;
    posix9_socket_entry *sock;
    unsigned long endTime;
    unsigned long now;
    fd_set readResult, writeResult, exceptResult;

    FD_ZERO(&readResult);
    FD_ZERO(&writeResult);
    FD_ZERO(&exceptResult);

    /* Calculate end time */
    if (timeout) {
        endTime = TickCount() + (timeout->tv_sec * 60) +
                  (timeout->tv_usec * 60 / 1000000);
    } else {
        endTime = 0xFFFFFFFF;  /* Forever */
    }

    do {
        count = 0;

        for (fd = SOCKET_FD_BASE; fd < nfds && fd < SOCKET_FD_BASE + MAX_SOCKETS; fd++) {
            sock = get_socket(fd);
            if (!sock) continue;

            /* Check for events */
            OTLook(sock->ep);

            if (readfds && FD_ISSET(fd, readfds)) {
                if (sock->readable || sock->listening) {
                    FD_SET(fd, &readResult);
                    count++;
                }
            }

            if (writefds && FD_ISSET(fd, writefds)) {
                if (sock->writable && sock->connected) {
                    FD_SET(fd, &writeResult);
                    count++;
                }
            }

            if (exceptfds && FD_ISSET(fd, exceptfds)) {
                if (sock->hasOOB) {
                    FD_SET(fd, &exceptResult);
                    count++;
                }
            }
        }

        if (count > 0) break;

        /* Give time to other processes */
        SystemTask();

        now = TickCount();
    } while (now < endTime);

    /* Copy results back */
    if (readfds) *readfds = readResult;
    if (writefds) *writefds = writeResult;
    if (exceptfds) *exceptfds = exceptResult;

    return count;
}

/* ============================================================
 * DNS Functions
 * ============================================================ */

struct hostent *gethostbyname(const char *name)
{
    InetHostInfo hostInfo;
    OSStatus err;

    err = OTInetStringToAddress(NULL, (char *)name, &hostInfo);
    if (err != noErr) {
        return NULL;
    }

    /* Fill in hostent structure */
    strncpy(dns_name, hostInfo.name, sizeof(dns_name) - 1);
    dns_result.h_name = dns_name;
    dns_result.h_aliases = dns_aliases;
    dns_result.h_addrtype = AF_INET;
    dns_result.h_length = sizeof(struct in_addr);
    dns_addr.s_addr = htonl(hostInfo.addrs[0]);
    dns_addrs[0] = (char *)&dns_addr;
    dns_result.h_addr_list = dns_addrs;

    return &dns_result;
}

struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type)
{
    InetHostInfo hostInfo;
    InetHost host;
    OSStatus err;

    (void)len;
    (void)type;

    host = ntohl(((struct in_addr *)addr)->s_addr);

    err = OTInetAddressToName(NULL, host, dns_name);
    if (err != noErr) {
        return NULL;
    }

    dns_result.h_name = dns_name;
    dns_result.h_aliases = dns_aliases;
    dns_result.h_addrtype = AF_INET;
    dns_result.h_length = sizeof(struct in_addr);
    memcpy(&dns_addr, addr, sizeof(dns_addr));
    dns_addrs[0] = (char *)&dns_addr;
    dns_result.h_addr_list = dns_addrs;

    return &dns_result;
}

/* ============================================================
 * Address Conversion
 * ============================================================ */

in_addr_t inet_addr(const char *cp)
{
    InetHost host;
    OSStatus err;

    err = OTInetStringToHost(cp, &host);
    if (err != noErr) {
        return INADDR_NONE;
    }

    return htonl(host);
}

char *inet_ntoa(struct in_addr in)
{
    static char buf[16];
    InetHost host = ntohl(in.s_addr);

    OTInetHostToString(host, buf);
    return buf;
}

int inet_aton(const char *cp, struct in_addr *inp)
{
    InetHost host;
    OSStatus err;

    err = OTInetStringToHost(cp, &host);
    if (err != noErr) {
        return 0;
    }

    inp->s_addr = htonl(host);
    return 1;
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    if (af != AF_INET || size < 16) {
        errno = EAFNOSUPPORT;
        return NULL;
    }

    OTInetHostToString(ntohl(((struct in_addr *)src)->s_addr), dst);
    return dst;
}

int inet_pton(int af, const char *src, void *dst)
{
    InetHost host;
    OSStatus err;

    if (af != AF_INET) {
        errno = EAFNOSUPPORT;
        return -1;
    }

    err = OTInetStringToHost(src, &host);
    if (err != noErr) {
        return 0;
    }

    ((struct in_addr *)dst)->s_addr = htonl(host);
    return 1;
}

/* ============================================================
 * Hostname
 * ============================================================ */

int gethostname(char *name, size_t len)
{
    /* Get from OT or return default */
    strncpy(name, "macintosh", len - 1);
    name[len - 1] = '\0';
    return 0;
}

int sethostname(const char *name, size_t len)
{
    (void)name;
    (void)len;
    errno = EPERM;
    return -1;
}

/* ============================================================
 * Close socket (called from posix9_file.c close())
 * ============================================================ */

int posix9_close_socket(int fd)
{
    posix9_socket_entry *sock;

    sock = get_socket(fd);
    if (!sock) return -1;

    if (sock->ep != kOTInvalidEndpointRef) {
        /* Send disconnect if connected */
        if (sock->connected) {
            OTSndDisconnect(sock->ep, NULL);
        }
        OTCloseProvider(sock->ep);
    }

    free_socket(fd);
    return 0;
}
