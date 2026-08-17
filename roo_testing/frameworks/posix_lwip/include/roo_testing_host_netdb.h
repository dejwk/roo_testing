#pragma once

#include "roo_testing_host_sockets.h"

// Keep the lwIP spellings used by both old and current Arduino releases while
// resolving them through the Linux resolver.  The global qualifier prevents a
// similarly named C++ member from intercepting an unqualified macro expansion.
#ifdef __cplusplus
#define lwip_gethostbyname ::gethostbyname
#define lwip_gethostbyname_r ::gethostbyname_r
#define lwip_getaddrinfo ::getaddrinfo
#define lwip_freeaddrinfo ::freeaddrinfo
#else
#define lwip_gethostbyname gethostbyname
#define lwip_gethostbyname_r gethostbyname_r
#define lwip_getaddrinfo getaddrinfo
#define lwip_freeaddrinfo freeaddrinfo
#endif
