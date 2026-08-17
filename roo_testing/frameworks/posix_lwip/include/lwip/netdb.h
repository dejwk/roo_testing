#pragma once

#include "roo_testing_host_netdb.h"

#define lwip_gethostbyname gethostbyname
#define lwip_gethostbyname_r gethostbyname_r
#define lwip_getaddrinfo getaddrinfo
#define lwip_freeaddrinfo freeaddrinfo
