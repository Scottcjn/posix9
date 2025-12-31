/*
 * OpenTransport.h - Stub for Mac OS Open Transport
 * For cross-compilation only - actual implementation on Mac OS 9
 */
#ifndef __OPENTRANSPORT__
#define __OPENTRANSPORT__

#include <MacTypes.h>

/* OSStatus - needed for OT function return types */
#ifndef OSStatus
typedef SInt32 OSStatus;
#endif

/* Open Transport types */
typedef void* EndpointRef;
typedef void* ProviderRef;
typedef void* OTClientContextPtr;
typedef UInt32 OTFlags;
typedef UInt32 OTOpenFlags;
typedef SInt32 OTResult;
typedef UInt32 OTTimeout;
typedef SInt32 OTReason;
typedef UInt32 OTQLen;
typedef UInt32 OTByteCount;
typedef UInt32 OTEventCode;

/* OT Address */
typedef struct OTAddress {
    UInt16 fAddressType;
    UInt8  fAddress[1];
} OTAddress;

/* TBind structure */
typedef struct TBind {
    OTAddress *addr;
    OTQLen qlen;
} TBind;

/* TCall structure */
typedef struct TCall {
    OTAddress *addr;
    void *opt;
    void *udata;
    SInt32 sequence;
} TCall;

/* TDiscon structure */
typedef struct TDiscon {
    void *udata;
    OTReason reason;
    SInt32 sequence;
} TDiscon;

/* OT Configuration */
typedef void* OTConfiguration;

/* InetAddress */
typedef struct InetAddress {
    UInt8  fAddressType;
    UInt8  fUnused;
    UInt16 fPort;
    UInt32 fHost;
} InetAddress;

/* DNS result */
typedef struct InetHostInfo {
    char   name[256];
    UInt32 addrs[10];
} InetHostInfo;

/* OT notification types */
enum {
    T_LISTEN        = 0x0001,
    T_CONNECT       = 0x0002,
    T_DATA          = 0x0004,
    T_EXDATA        = 0x0008,
    T_DISCONNECT    = 0x0010,
    T_GODATA        = 0x0100,
    T_ORDREL        = 0x0080,
    T_PASSCON       = 0x0200,   /* Pass connection indicator */
    T_UDERR         = 0x0400    /* Unitdata error indication */
};

/* OT result codes */
enum {
    kOTNoError          = 0,
    kOTOutOfMemoryErr   = -3211,
    kOTNotFoundErr      = -3201,
    kOTFlowErr          = -3161,
    kOTBadAddressErr    = -3150,
    kOTBadOptionErr     = -3151,
    kOTAccessErr        = -3152,
    kOTBadReferenceErr  = -3153,
    kOTNoAddressErr     = -3154,
    kOTOutStateErr      = -3155,
    kOTBadSequenceErr   = -3156,
    kOTSysErrorErr      = -3157,
    kOTLookErr          = -3158,
    kOTBadDataErr       = -3159,
    kOTBufferOverflowErr = -3160,
    kOTNotSupportedErr  = -3162,
    kOTStateChangeErr   = -3163,
    kOTNoDataErr        = -3164,
    kOTNoDisconnectErr  = -3165,
    kOTNoReleaseErr     = -3166,
    kOTNoUDErr              = -3167,    /* No unit data error */
    kOTBadFlagErr           = -3168,    /* Bad flag */
    kOTNoRelErr             = -3169,    /* No release */
    kOTNotSentErr           = -3170,    /* Not sent */
    kOTNoStructureTypeErr   = -3171,    /* No structure type */
    kOTBadNameErr           = -3172,    /* Bad name */
    kOTBadQLenErr           = -3173,    /* Bad queue length */
    kOTAddressBusyErr       = -3174,    /* Address busy */
    kOTIndOutErr            = -3175,    /* Outstanding indications */
    kOTProviderMismatchErr  = -3176,    /* Provider mismatch */
    kOTResQLenErr           = -3177,    /* Reserved queue length */
    kOTResAddressErr        = -3178,    /* Reserved address */
    kOTQFullErr             = -3179,    /* Queue full */
    kOTProtocolErr          = -3180,    /* Protocol error */
    kOTPortHasDiedErr       = -3190,    /* Port has died */
    kOTPortLostConnection   = -3199,    /* Port lost connection */
    kOTBadSyncErr           = -3203,    /* Bad sync */
    kOTCanceledErr          = -3204,    /* Canceled */
    kEAGAINErr              = -3210
};

/* OT endpoint invalid ref */
#define kOTInvalidEndpointRef   ((EndpointRef)0)
#define kOTInvalidProviderRef   ((ProviderRef)0)

/* Init flags */
#define kInitOTForApplicationMask   0x00000001

/* Endpoint info structure */
typedef struct TEndpointInfo {
    SInt32  addr;
    SInt32  options;
    SInt32  tsdu;
    SInt32  etsdu;
    SInt32  connect;
    SInt32  discon;
    UInt32  servtype;
    UInt32  flags;
} TEndpointInfo;

/* Callback type */
typedef void (*OTNotifyProcPtr)(void* context, UInt32 code, OTResult result, void* cookie);

/* Core OT functions */
OSStatus InitOpenTransport(void);
void CloseOpenTransport(void);

/* Context-based variants (CFM/PPC) */
#define InitOpenTransportInContext(flags, context) InitOpenTransport()
#define CloseOpenTransportInContext(context) CloseOpenTransport()

/* Endpoint operations */
EndpointRef OTOpenEndpoint(OTConfiguration* config, OTOpenFlags flags, void* info, OSStatus* err);
OSStatus OTCloseProvider(ProviderRef ref);
OSStatus OTBind(EndpointRef ref, TBind* reqAddr, TBind* retAddr);
OSStatus OTUnbind(EndpointRef ref);
OSStatus OTConnect(EndpointRef ref, TCall* sndCall, TCall* rcvCall);
OSStatus OTListen(EndpointRef ref, TCall* call);
OSStatus OTAccept(EndpointRef ref, EndpointRef resRef, TCall* call);
OTResult OTSnd(EndpointRef ref, void* buf, OTByteCount nbytes, OTFlags flags);
OTResult OTRcv(EndpointRef ref, void* buf, OTByteCount nbytes, OTFlags* flags);
OSStatus OTSndDisconnect(EndpointRef ref, TCall* call);
OSStatus OTRcvDisconnect(EndpointRef ref, TDiscon* discon);

/* Non-blocking */
OSStatus OTSetNonBlocking(EndpointRef ref);
OSStatus OTSetBlocking(EndpointRef ref);

/* Notification */
OSStatus OTInstallNotifier(ProviderRef ref, OTNotifyProcPtr proc, void* context);

/* DNS */
OSStatus OTInetStringToAddress(void* services, char* name, InetHostInfo* hinfo);

/* Address utilities */
void OTInitInetAddress(InetAddress* addr, UInt16 port, UInt32 host);

#endif /* __OPENTRANSPORT__ */
