#pragma once

// ESP-IDF installs a Linux sys/socket.h forwarding header ahead of glibc's
// headers. This switch tells that forwarding header to select the host ABI.
#define LWIP_HDR_LINUX_SYS_SOCKETS_H 1

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <unistd.h>

// arpa/inet.h is shadowed by ESP-IDF's lwIP compatibility header. The socket
// users in Arduino need only the standard conversion routines, whose ABI is
// declared here using the types supplied by netinet/in.h.
#ifdef __cplusplus
extern "C" {
#endif

in_addr_t inet_addr(const char *address);
int inet_aton(const char *address, struct in_addr *result);
char *inet_ntoa(struct in_addr address);
const char *inet_ntop(int family, const void *address, char *output,
                      socklen_t output_size);
int inet_pton(int family, const char *address, void *output);

#ifdef __cplusplus
}  // extern "C"
#endif
