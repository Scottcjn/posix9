/*
 * OpenTransportProviders.h - Stub for OT Providers
 * For cross-compilation only
 */
#ifndef __OPENTRANSPORTPROVIDERS__
#define __OPENTRANSPORTPROVIDERS__

#include "OpenTransport.h"

/* TCP/IP endpoint configuration */
#define kTCPName    "tcp"
#define kUDPName    "udp"

/* Create TCP endpoint config */
OTConfiguration* OTCreateConfiguration(const char* path);

/* Inet services ref */
typedef void* InetSvcRef;

/* Open inet services */
InetSvcRef OTOpenInternetServices(OTConfiguration* config, OTOpenFlags flags, OSStatus* err);

#endif /* __OPENTRANSPORTPROVIDERS__ */
