#pragma once

#include "roo_testing_host_sockets.h"

// ESP-IDF/Arduino spell a small subset of socket calls with lwip_ prefixes.
// Map both current and legacy re-entrant spellings to the Linux calls.
#define lwip_accept accept
#define lwip_accept_r accept
#define lwip_bind bind
#define lwip_shutdown shutdown
#define lwip_getpeername getpeername
#define lwip_getsockname getsockname
#define lwip_setsockopt setsockopt
#define lwip_getsockopt getsockopt
#define lwip_close close
#define lwip_close_r close
#define lwip_connect connect
#define lwip_connect_r connect
#define lwip_listen listen
#define lwip_recv recv
#define lwip_recvmsg recvmsg
#define lwip_recvfrom recvfrom
#define lwip_send send
#define lwip_sendmsg sendmsg
#define lwip_sendto sendto
#define lwip_socket socket
#define lwip_select select
#define lwip_poll poll
#define lwip_ioctl ioctl
#define lwip_ioctl_r ioctl
#define lwip_read read
#define lwip_readv readv
#define lwip_write write
#define lwip_writev writev
#define lwip_inet_ntop inet_ntop
#define lwip_inet_pton inet_pton

#ifndef closesocket
#define closesocket close
#endif
