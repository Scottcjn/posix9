/*
 * OpenTransport.h - Stub for Mac OS Open Transport
 * For cross-compilation only - actual implementation on Mac OS 9
 *
 * Provides all types, constants, and function declarations needed
 * by posix9_socket.c to compile against the Retro68 toolchain.
 */
#ifndef __OPENTRANSPORT__
#define __OPENTRANSPORT__

#include <MacTypes.h>

/* OSStatus - needed for OT function return types */
#ifndef OSStatus
typedef SInt32 OSStatus;
#endif

/* ============================================================
 * Basic OT Types
 * ============================================================ */

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
typedef SInt32 OTSequence;

/* InetHost - 32-bit IPv4 address in host byte order */
typedef UInt32 InetHost;

/* OT Configuration */
typedef void* OTConfiguration;
typedef OTConfiguration* OTConfigurationRef;

#define kOTInvalidConfigurationRef  ((OTConfigurationRef)0)

/* ============================================================
 * TNetbuf - Core XTI buffer descriptor
 * ============================================================ */

typedef struct TNetbuf {
    UInt32  maxlen;     /* Maximum buffer length */
    UInt32  len;        /* Actual data length */
    UInt8  *buf;        /* Buffer pointer */
} TNetbuf;

/* ============================================================
 * OT Address
 * ============================================================ */

typedef struct OTAddress {
    UInt16 fAddressType;
    UInt8  fAddress[1];
} OTAddress;

/* ============================================================
 * XTI Structures using TNetbuf
 * ============================================================ */

/* TBind - bind request/response */
typedef struct TBind {
    TNetbuf addr;       /* Address buffer */
    OTQLen  qlen;       /* Connection queue length */
} TBind;

/* TCall - connection request/response */
typedef struct TCall {
    TNetbuf     addr;       /* Address buffer */
    TNetbuf     opt;        /* Options buffer */
    TNetbuf     udata;      /* User data buffer */
    OTSequence  sequence;   /* Sequence number */
} TCall;

/* TDiscon - disconnect indication */
typedef struct TDiscon {
    TNetbuf     udata;      /* User data buffer */
    OTReason    reason;     /* Disconnect reason */
    OTSequence  sequence;   /* Sequence number */
} TDiscon;

/* TUnitData - unitdata (UDP) send/receive */
typedef struct TUnitData {
    TNetbuf addr;       /* Address buffer */
    TNetbuf opt;        /* Options buffer */
    TNetbuf udata;      /* User data buffer */
} TUnitData;

/* ============================================================
 * InetAddress
 * ============================================================ */

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

/* ============================================================
 * OT Notification Events
 * ============================================================ */

enum {
    T_LISTEN        = 0x0001,
    T_CONNECT       = 0x0002,
    T_DATA          = 0x0004,
    T_EXDATA        = 0x0008,
    T_DISCONNECT    = 0x0010,
    T_ORDREL        = 0x0080,
    T_GODATA        = 0x0100,
    T_PASSCON       = 0x0200,
    T_UDERR         = 0x0400
};

/* OT flags */
enum {
    T_EXPEDITED     = 0x0020    /* Expedited (OOB) data flag */
};

/* ============================================================
 * OT Result Codes
 * ============================================================ */

enum {
    kOTNoError              = 0,
    kOTOutOfMemoryErr       = -3211,
    kOTNotFoundErr          = -3201,
    kOTFlowErr              = -3161,
    kOTBadAddressErr        = -3150,
    kOTBadOptionErr         = -3151,
    kOTAccessErr            = -3152,
    kOTBadReferenceErr      = -3153,
    kOTNoAddressErr         = -3154,
    kOTOutStateErr          = -3155,
    kOTBadSequenceErr       = -3156,
    kOTSysErrorErr          = -3157,
    kOTLookErr              = -3158,
    kOTBadDataErr           = -3159,
    kOTBufferOverflowErr    = -3160,
    kOTNotSupportedErr      = -3162,
    kOTStateChangeErr       = -3163,
    kOTNoDataErr            = -3164,
    kOTNoDisconnectErr      = -3165,
    kOTNoReleaseErr         = -3166,
    kOTNoUDErr              = -3167,
    kOTBadFlagErr           = -3168,
    kOTNoRelErr             = -3169,
    kOTNotSentErr           = -3170,
    kOTNoStructureTypeErr   = -3171,
    kOTBadNameErr           = -3172,
    kOTBadQLenErr           = -3173,
    kOTAddressBusyErr       = -3174,
    kOTIndOutErr            = -3175,
    kOTProviderMismatchErr  = -3176,
    kOTResQLenErr           = -3177,
    kOTResAddressErr        = -3178,
    kOTQFullErr             = -3179,
    kOTProtocolErr          = -3180,
    kOTPortHasDiedErr       = -3190,
    kOTPortLostConnection   = -3199,
    kOTBadSyncErr           = -3203,
    kOTCanceledErr          = -3204
};

/* ============================================================
 * POSIX-mapped OT Error Codes (kE*Err)
 *
 * These map BSD errno values to OT result codes.
 * Used by posix9_socket.c to translate OT errors back to errno.
 * ============================================================ */

enum {
    kEPERMErr               = -3200,
    kENOENTErr              = -3201,
    kEINTRErr               = -3203,
    kEIOErr                 = -3204,
    kENXIOErr               = -3205,
    kEBADFErr               = -3208,
    kEAGAINErr              = -3210,
    kENOMEMErr              = -3211,
    kEACCESErr              = -3212,
    kEFAULTErr              = -3213,
    kEBUSYErr               = -3215,
    kEEXISTErr              = -3216,
    kENODEVErr              = -3218,
    kEINVALErr              = -3221,
    kENOTTYErr              = -3224,
    kEPIPEErr               = -3231,
    kERANGEErr              = -3233,
    kEWOULDBLOCKErr         = -3234,
    kEDEADLKErr             = -3235,
    kEALREADYErr            = -3236,
    kENOTSOCKErr            = -3237,
    kEDESTADDRREQErr        = -3238,
    kEMSGSIZEErr            = -3239,
    kEPROTOTYPEErr          = -3240,
    kENOPROTOOPTErr         = -3241,
    kEPROTONOSUPPORTErr     = -3242,
    kESOCKTNOSUPPORTErr     = -3243,
    kEOPNOTSUPPErr          = -3244,
    kEADDRINUSEErr          = -3247,
    kEADDRNOTAVAILErr       = -3248,
    kENETDOWNErr            = -3249,
    kENETUNREACHErr         = -3250,
    kENETRESETErr           = -3251,
    kECONNABORTEDErr        = -3252,
    kECONNRESETErr          = -3253,
    kENOBUFSErr             = -3254,
    kEISCONNErr             = -3255,
    kENOTCONNErr            = -3256,
    kESHUTDOWNErr           = -3257,
    kETIMEDOUTErr           = -3259,
    kECONNREFUSEDErr        = -3260,
    kEHOSTDOWNErr           = -3263,
    kEHOSTUNREACHErr        = -3264,
    kEINPROGRESSErr         = -3270
};

/* ============================================================
 * Endpoint References
 * ============================================================ */

#define kOTInvalidEndpointRef   ((EndpointRef)0)
#define kOTInvalidProviderRef   ((ProviderRef)0)

/* Init flags */
#define kInitOTForApplicationMask   0x00000001

/* ============================================================
 * Endpoint Info
 * ============================================================ */

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

/* ============================================================
 * Callback / Notifier Types
 * ============================================================ */

typedef void (*OTNotifyProcPtr)(void* context, UInt32 code, OTResult result, void* cookie);
typedef OTNotifyProcPtr OTNotifyUPP;

/* NewOTNotifyUPP - on PPC/CFM this is identity (no mixed-mode) */
#define NewOTNotifyUPP(proc)    ((OTNotifyUPP)(proc))
#define DisposeOTNotifyUPP(upp) /* no-op */

/* ============================================================
 * Core OT Functions
 * ============================================================ */

/* Initialization
 * Import library exports InitOpenTransportPriv, not InitOpenTransport.
 * The public API functions are macros wrapping the Priv variants. */
OSStatus InitOpenTransportPriv(OTOpenFlags flags);
void CloseOpenTransportPriv(OTClientContextPtr context);

#define InitOpenTransport()                     InitOpenTransportPriv(0)
#define InitOpenTransportInContext(flags, ctx)   InitOpenTransportPriv(flags)
#define CloseOpenTransport()                    CloseOpenTransportPriv(NULL)
#define CloseOpenTransportInContext(ctx)         CloseOpenTransportPriv(ctx)

/* Endpoint operations
 * Import library exports OTOpenEndpointPriv and OTCloseProviderPriv. */
EndpointRef OTOpenEndpointPriv(OTConfigurationRef config, OTOpenFlags flags,
                                TEndpointInfo* info, OSStatus* err);
OSStatus OTCloseProviderPriv(ProviderRef ref);

#define OTOpenEndpoint(config, flags, info, err) \
    OTOpenEndpointPriv((OTConfigurationRef)(config), (flags), (info), (err))
#define OTOpenEndpointInContext(config, flags, info, err, ctx) \
    OTOpenEndpointPriv((config), (flags), (info), (err))
#define OTCloseProvider(ref) OTCloseProviderPriv(ref)

/* Binding */
OSStatus OTBind(EndpointRef ref, TBind* reqAddr, TBind* retAddr);
OSStatus OTUnbind(EndpointRef ref);

/* Connection */
OSStatus OTConnect(EndpointRef ref, TCall* sndCall, TCall* rcvCall);
OSStatus OTListen(EndpointRef ref, TCall* call);
OSStatus OTAccept(EndpointRef ref, EndpointRef resRef, TCall* call);

/* Data transfer (TCP) */
OTResult OTSnd(EndpointRef ref, void* buf, OTByteCount nbytes, OTFlags flags);
OTResult OTRcv(EndpointRef ref, void* buf, OTByteCount nbytes, OTFlags* flags);

/* Data transfer (UDP) */
OSStatus OTSndUData(EndpointRef ref, TUnitData* udata);
OSStatus OTRcvUData(EndpointRef ref, TUnitData* udata, OTFlags* flags);

/* Disconnect */
OSStatus OTSndDisconnect(EndpointRef ref, TCall* call);
OSStatus OTRcvDisconnect(EndpointRef ref, TDiscon* discon);
OSStatus OTSndOrderlyDisconnect(EndpointRef ref);

/* Mode setting */
OSStatus OTSetNonBlocking(EndpointRef ref);
OSStatus OTSetBlocking(EndpointRef ref);
OSStatus OTSetSynchronous(EndpointRef ref);
OSStatus OTSetAsynchronous(EndpointRef ref);

/* Event polling */
OTResult OTLook(EndpointRef ref);

/* Notification */
OSStatus OTInstallNotifier(ProviderRef ref, OTNotifyUPP proc, void* context);

/* ============================================================
 * DNS / Address Functions
 * ============================================================ */

OSStatus OTInetStringToAddress(void* services, char* name, InetHostInfo* hinfo);
OSStatus OTInetStringToHost(const char* str, InetHost* host);
void     OTInetHostToString(InetHost host, char* str);
OSStatus OTInetAddressToName(void* services, InetHost host, char* name);

/* Address utilities */
void OTInitInetAddress(InetAddress* addr, UInt16 port, UInt32 host);

/* ============================================================
 * Mac OS Cooperative Multitasking
 * ============================================================ */

#ifndef SystemTask
void SystemTask(void);
#endif

#endif /* __OPENTRANSPORT__ */
