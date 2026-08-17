#pragma once

// ESP-IDF installs a Linux sys/socket.h forwarding header ahead of glibc's
// headers. This switch tells that forwarding header to select the host ABI.
#define LWIP_HDR_LINUX_SYS_SOCKETS_H 1

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <unistd.h>

// IPAddress.h may have loaded lwIP's byte-order and inet convenience macros
// before Network asks for sockets.  Remove those aliases before declaring the
// host ABI, then forward past ESP-IDF's shadowing netinet headers.
#undef htonl
#undef htons
#undef ntohl
#undef ntohs
#undef inet_addr
#undef inet_aton
#undef inet_ntoa
#undef inet_ntoa_r
#undef inet_ntop
#undef inet_pton

#include "roo_testing_host_netinet.h"

// The Linux FreeRTOS port delivers its scheduler tick with SIGALRM. Even when
// the handler requests SA_RESTART, Linux never restarts select/poll and timed
// socket operations can still report EINTR. The host bridge protects current
// nonblocking Arduino socket operations and gives select/poll monotonic retry
// deadlines without changing blocking-call timeout semantics.
#ifdef __cplusplus
extern "C" {
#endif

int roo_testing_accept(int socket, struct sockaddr *address,
                       socklen_t *address_size);
int roo_testing_bind(int socket, const struct sockaddr *address,
                     socklen_t address_size);
int roo_testing_shutdown(int socket, int how);
int roo_testing_getpeername(int socket, struct sockaddr *address,
                            socklen_t *address_size);
int roo_testing_getsockname(int socket, struct sockaddr *address,
                            socklen_t *address_size);
int roo_testing_setsockopt(int socket, int level, int option,
                           const void *value, socklen_t value_size);
int roo_testing_getsockopt(int socket, int level, int option, void *value,
                           socklen_t *value_size);
int roo_testing_connect(int socket, const struct sockaddr *address,
                        socklen_t address_size);
int roo_testing_listen(int socket, int backlog);
ssize_t roo_testing_recv(int socket, void *buffer, size_t size, int flags);
ssize_t roo_testing_recvmsg(int socket, struct msghdr *message, int flags);
ssize_t roo_testing_recvfrom(int socket, void *buffer, size_t size, int flags,
                             struct sockaddr *address,
                             socklen_t *address_size);
ssize_t roo_testing_send(int socket, const void *buffer, size_t size,
                         int flags);
ssize_t roo_testing_sendmsg(int socket, const struct msghdr *message,
                            int flags);
ssize_t roo_testing_sendto(int socket, const void *buffer, size_t size,
                           int flags, const struct sockaddr *address,
                           socklen_t address_size);
int roo_testing_socket(int domain, int type, int protocol);
int roo_testing_select(int descriptor_count, fd_set *read_descriptors,
                       fd_set *write_descriptors, fd_set *error_descriptors,
                       struct timeval *timeout);
int roo_testing_poll(struct pollfd *descriptors, nfds_t descriptor_count,
                     int timeout_ms);
int roo_testing_ioctl(int descriptor, unsigned long request, void *argument);
ssize_t roo_testing_read(int descriptor, void *buffer, size_t size);
ssize_t roo_testing_readv(int descriptor, const struct iovec *buffers,
                          int buffer_count);
ssize_t roo_testing_write(int descriptor, const void *buffer, size_t size);
ssize_t roo_testing_writev(int descriptor, const struct iovec *buffers,
                           int buffer_count);

#ifdef __cplusplus
}  // extern "C"
#endif
