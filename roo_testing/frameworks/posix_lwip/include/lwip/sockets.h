#pragma once

#include <string.h>

#include "lwip/netif.h"
#include "roo_testing_host_sockets.h"

// lwIP's POSIX socket adapter normally provides these conversions.  The host
// overlay bypasses that adapter, so bridge the byte-identical 128-bit payloads
// without depending on libc-specific in6_addr union member names.
#undef inet6_addr_from_ip6addr
#undef inet6_addr_to_ip6addr
#define inet6_addr_from_ip6addr(target, source) \
  memcpy((target)->s6_addr, (source)->addr, sizeof((target)->s6_addr))
#define inet6_addr_to_ip6addr(target, source)                    \
  do {                                                          \
    memcpy((target)->addr, (source)->s6_addr, sizeof((source)->s6_addr)); \
    ip6_addr_clear_zone(target);                                \
  } while (0)

// ESP-IDF/Arduino spell a small subset of socket calls with lwip_ prefixes.
// Map both current and legacy re-entrant spellings to the Linux calls.
// BSD-name calls are intercepted at link time instead of with public macros:
// macros named accept or connect would also rewrite Arduino C++ methods.
#ifdef __cplusplus
#define ROO_TESTING_POSIX_CALL(name) ::name
#else
#define ROO_TESTING_POSIX_CALL(name) name
#endif

#define lwip_accept ROO_TESTING_POSIX_CALL(roo_testing_accept)
#define lwip_accept_r ROO_TESTING_POSIX_CALL(roo_testing_accept)
#define lwip_bind ROO_TESTING_POSIX_CALL(roo_testing_bind)
#define lwip_shutdown ROO_TESTING_POSIX_CALL(roo_testing_shutdown)
#define lwip_getpeername ROO_TESTING_POSIX_CALL(roo_testing_getpeername)
#define lwip_getsockname ROO_TESTING_POSIX_CALL(roo_testing_getsockname)
#define lwip_setsockopt ROO_TESTING_POSIX_CALL(roo_testing_setsockopt)
#define lwip_getsockopt ROO_TESTING_POSIX_CALL(roo_testing_getsockopt)
#define lwip_close ROO_TESTING_POSIX_CALL(close)
#define lwip_close_r ROO_TESTING_POSIX_CALL(close)
#define lwip_connect ROO_TESTING_POSIX_CALL(roo_testing_connect)
#define lwip_connect_r ROO_TESTING_POSIX_CALL(roo_testing_connect)
#define lwip_listen ROO_TESTING_POSIX_CALL(roo_testing_listen)
#define lwip_recv ROO_TESTING_POSIX_CALL(roo_testing_recv)
#define lwip_recvmsg ROO_TESTING_POSIX_CALL(roo_testing_recvmsg)
#define lwip_recvfrom ROO_TESTING_POSIX_CALL(roo_testing_recvfrom)
#define lwip_send ROO_TESTING_POSIX_CALL(roo_testing_send)
#define lwip_sendmsg ROO_TESTING_POSIX_CALL(roo_testing_sendmsg)
#define lwip_sendto ROO_TESTING_POSIX_CALL(roo_testing_sendto)
#define lwip_socket ROO_TESTING_POSIX_CALL(roo_testing_socket)
#define lwip_select ROO_TESTING_POSIX_CALL(roo_testing_select)
#define lwip_poll ROO_TESTING_POSIX_CALL(roo_testing_poll)
#define lwip_ioctl ROO_TESTING_POSIX_CALL(roo_testing_ioctl)
#define lwip_ioctl_r ROO_TESTING_POSIX_CALL(roo_testing_ioctl)
#define lwip_read ROO_TESTING_POSIX_CALL(roo_testing_read)
#define lwip_readv ROO_TESTING_POSIX_CALL(roo_testing_readv)
#define lwip_write ROO_TESTING_POSIX_CALL(roo_testing_write)
#define lwip_writev ROO_TESTING_POSIX_CALL(roo_testing_writev)
#define lwip_inet_ntop ROO_TESTING_POSIX_CALL(inet_ntop)
#define lwip_inet_pton ROO_TESTING_POSIX_CALL(inet_pton)

#ifndef closesocket
#define closesocket ROO_TESTING_POSIX_CALL(close)
#endif
