#include "roo_testing_host_sockets.h"

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cerrno>
#include <pthread.h>
#include <signal.h>

extern "C" {

int __real_accept(int socket, struct sockaddr *address,
                  socklen_t *address_size);
int __real_bind(int socket, const struct sockaddr *address,
                socklen_t address_size);
int __real_shutdown(int socket, int how);
int __real_getpeername(int socket, struct sockaddr *address,
                       socklen_t *address_size);
int __real_getsockname(int socket, struct sockaddr *address,
                       socklen_t *address_size);
int __real_setsockopt(int socket, int level, int option, const void *value,
                      socklen_t value_size);
int __real_getsockopt(int socket, int level, int option, void *value,
                      socklen_t *value_size);
int __real_connect(int socket, const struct sockaddr *address,
                   socklen_t address_size);
int __real_listen(int socket, int backlog);
ssize_t __real_recv(int socket, void *buffer, size_t size, int flags);
ssize_t __real_recvmsg(int socket, struct msghdr *message, int flags);
ssize_t __real_recvfrom(int socket, void *buffer, size_t size, int flags,
                        struct sockaddr *address, socklen_t *address_size);
ssize_t __real_send(int socket, const void *buffer, size_t size, int flags);
ssize_t __real_sendmsg(int socket, const struct msghdr *message, int flags);
ssize_t __real_sendto(int socket, const void *buffer, size_t size, int flags,
                      const struct sockaddr *address, socklen_t address_size);
int __real_socket(int domain, int type, int protocol);
int __real_select(int descriptor_count, fd_set *read_descriptors,
                  fd_set *write_descriptors, fd_set *error_descriptors,
                  struct timeval *timeout);
int __real_poll(struct pollfd *descriptors, nfds_t descriptor_count,
                int timeout_ms);
}  // extern "C"

namespace {

template <typename Operation>
auto WithoutSchedulerTick(Operation operation) -> decltype(operation()) {
  sigset_t scheduler_tick;
  sigset_t previous_mask;
  sigemptyset(&scheduler_tick);
  sigaddset(&scheduler_tick, SIGALRM);
  const bool blocked =
      pthread_sigmask(SIG_BLOCK, &scheduler_tick, &previous_mask) == 0;
  auto result = operation();
  const int operation_errno = errno;
  if (blocked) {
    pthread_sigmask(SIG_SETMASK, &previous_mask, nullptr);
  }
  errno = operation_errno;
  return result;
}

bool IsNonBlocking(int descriptor) {
  const int flags = WithoutSchedulerTick(
      [&] { return ::fcntl(descriptor, F_GETFL, 0); });
  return flags >= 0 && (flags & O_NONBLOCK) != 0;
}

using MonotonicClock = std::chrono::steady_clock;

MonotonicClock::time_point SelectDeadline(const timeval &timeout) {
  return MonotonicClock::now() + std::chrono::seconds(timeout.tv_sec) +
         std::chrono::microseconds(timeout.tv_usec);
}

timeval SelectTimeRemaining(MonotonicClock::time_point deadline) {
  auto remaining = deadline - MonotonicClock::now();
  if (remaining <= MonotonicClock::duration::zero()) {
    return {};
  }
  auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(
      remaining);
  if (microseconds < remaining) {
    ++microseconds;
  }
  timeval timeout = {};
  timeout.tv_sec = microseconds.count() / 1000000;
  timeout.tv_usec = microseconds.count() % 1000000;
  return timeout;
}

int PollTimeRemaining(MonotonicClock::time_point deadline) {
  auto remaining = deadline - MonotonicClock::now();
  if (remaining <= MonotonicClock::duration::zero()) {
    return 0;
  }
  auto milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(remaining);
  if (milliseconds < remaining) {
    ++milliseconds;
  }
  return static_cast<int>(
      std::min<int64_t>(milliseconds.count(), INT_MAX));
}

}  // namespace

extern "C" {

int roo_testing_accept(int socket, struct sockaddr *address,
                       socklen_t *address_size) {
  if (IsNonBlocking(socket)) {
    return WithoutSchedulerTick(
        [&] { return __real_accept(socket, address, address_size); });
  }
  // A blocking accept may have SO_RCVTIMEO. Retrying would restart that
  // timeout on every scheduler tick, so leave blocking-call policy to its
  // caller. Arduino's server sockets are nonblocking and use the path above.
  return __real_accept(socket, address, address_size);
}

int roo_testing_bind(int socket, const struct sockaddr *address,
                     socklen_t address_size) {
  return WithoutSchedulerTick(
      [&] { return __real_bind(socket, address, address_size); });
}

int roo_testing_shutdown(int socket, int how) {
  return WithoutSchedulerTick([&] { return __real_shutdown(socket, how); });
}

int roo_testing_getpeername(int socket, struct sockaddr *address,
                            socklen_t *address_size) {
  return WithoutSchedulerTick(
      [&] { return __real_getpeername(socket, address, address_size); });
}

int roo_testing_getsockname(int socket, struct sockaddr *address,
                            socklen_t *address_size) {
  return WithoutSchedulerTick(
      [&] { return __real_getsockname(socket, address, address_size); });
}

int roo_testing_setsockopt(int socket, int level, int option,
                           const void *value, socklen_t value_size) {
  return WithoutSchedulerTick([&] {
    return __real_setsockopt(socket, level, option, value, value_size);
  });
}

int roo_testing_getsockopt(int socket, int level, int option, void *value,
                           socklen_t *value_size) {
  return WithoutSchedulerTick([&] {
    return __real_getsockopt(socket, level, option, value, value_size);
  });
}

int roo_testing_connect(int socket, const struct sockaddr *address,
                        socklen_t address_size) {
  if (IsNonBlocking(socket)) {
    return WithoutSchedulerTick(
        [&] { return __real_connect(socket, address, address_size); });
  }
  // After EINTR, Linux may continue a blocking connect asynchronously; a
  // second connect is not a valid retry. Arduino always uses nonblocking
  // sockets, while other callers retain libc's exact blocking semantics.
  return __real_connect(socket, address, address_size);
}

int roo_testing_listen(int socket, int backlog) {
  return WithoutSchedulerTick(
      [&] { return __real_listen(socket, backlog); });
}

ssize_t roo_testing_recv(int socket, void *buffer, size_t size, int flags) {
  if ((flags & MSG_DONTWAIT) != 0 || IsNonBlocking(socket)) {
    return WithoutSchedulerTick(
        [&] { return __real_recv(socket, buffer, size, flags); });
  }
  return __real_recv(socket, buffer, size, flags);
}

ssize_t roo_testing_recvmsg(int socket, struct msghdr *message, int flags) {
  if ((flags & MSG_DONTWAIT) != 0 || IsNonBlocking(socket)) {
    return WithoutSchedulerTick(
        [&] { return __real_recvmsg(socket, message, flags); });
  }
  return __real_recvmsg(socket, message, flags);
}

ssize_t roo_testing_recvfrom(int socket, void *buffer, size_t size, int flags,
                             struct sockaddr *address,
                             socklen_t *address_size) {
  if ((flags & MSG_DONTWAIT) != 0 || IsNonBlocking(socket)) {
    return WithoutSchedulerTick([&] {
      return __real_recvfrom(socket, buffer, size, flags, address,
                             address_size);
    });
  }
  return __real_recvfrom(socket, buffer, size, flags, address, address_size);
}

ssize_t roo_testing_send(int socket, const void *buffer, size_t size,
                         int flags) {
  if ((flags & MSG_DONTWAIT) != 0 || IsNonBlocking(socket)) {
    return WithoutSchedulerTick(
        [&] { return __real_send(socket, buffer, size, flags); });
  }
  return __real_send(socket, buffer, size, flags);
}

ssize_t roo_testing_sendmsg(int socket, const struct msghdr *message,
                            int flags) {
  if ((flags & MSG_DONTWAIT) != 0 || IsNonBlocking(socket)) {
    return WithoutSchedulerTick(
        [&] { return __real_sendmsg(socket, message, flags); });
  }
  return __real_sendmsg(socket, message, flags);
}

ssize_t roo_testing_sendto(int socket, const void *buffer, size_t size,
                           int flags, const struct sockaddr *address,
                           socklen_t address_size) {
  if ((flags & MSG_DONTWAIT) != 0 || IsNonBlocking(socket)) {
    return WithoutSchedulerTick([&] {
      return __real_sendto(socket, buffer, size, flags, address, address_size);
    });
  }
  return __real_sendto(socket, buffer, size, flags, address, address_size);
}

int roo_testing_socket(int domain, int type, int protocol) {
  return WithoutSchedulerTick(
      [&] { return __real_socket(domain, type, protocol); });
}

int roo_testing_select(int descriptor_count, fd_set *read_descriptors,
                       fd_set *write_descriptors, fd_set *error_descriptors,
                       struct timeval *timeout) {
  if (timeout != nullptr &&
      (timeout->tv_sec < 0 || timeout->tv_usec < 0 ||
       timeout->tv_usec >= 1000000)) {
    errno = EINVAL;
    return -1;
  }
  const bool has_timeout = timeout != nullptr;
  const auto deadline =
      has_timeout ? SelectDeadline(*timeout) : MonotonicClock::time_point{};
  const fd_set original_read =
      read_descriptors == nullptr ? fd_set{} : *read_descriptors;
  const fd_set original_write =
      write_descriptors == nullptr ? fd_set{} : *write_descriptors;
  const fd_set original_error =
      error_descriptors == nullptr ? fd_set{} : *error_descriptors;

  while (true) {
    fd_set attempt_read = original_read;
    fd_set attempt_write = original_write;
    fd_set attempt_error = original_error;
    timeval attempt_timeout =
        has_timeout ? SelectTimeRemaining(deadline) : timeval{};
    int result = __real_select(
        descriptor_count,
        read_descriptors == nullptr ? nullptr : &attempt_read,
        write_descriptors == nullptr ? nullptr : &attempt_write,
        error_descriptors == nullptr ? nullptr : &attempt_error,
        has_timeout ? &attempt_timeout : nullptr);

    if (result >= 0) {
      if (read_descriptors != nullptr) *read_descriptors = attempt_read;
      if (write_descriptors != nullptr) *write_descriptors = attempt_write;
      if (error_descriptors != nullptr) *error_descriptors = attempt_error;
      if (timeout != nullptr) *timeout = SelectTimeRemaining(deadline);
      return result;
    }
    if (errno != EINTR) {
      return result;
    }
    if (has_timeout && MonotonicClock::now() >= deadline) {
      if (read_descriptors != nullptr) FD_ZERO(read_descriptors);
      if (write_descriptors != nullptr) FD_ZERO(write_descriptors);
      if (error_descriptors != nullptr) FD_ZERO(error_descriptors);
      *timeout = {};
      return 0;
    }
  }
}

int roo_testing_poll(struct pollfd *descriptors, nfds_t descriptor_count,
                     int timeout_ms) {
  const bool has_timeout = timeout_ms >= 0;
  const auto deadline = has_timeout
                            ? MonotonicClock::now() +
                                  std::chrono::milliseconds(timeout_ms)
                            : MonotonicClock::time_point{};
  int remaining_ms = timeout_ms;
  while (true) {
    int result = __real_poll(descriptors, descriptor_count, remaining_ms);
    if (result >= 0 || errno != EINTR) {
      return result;
    }
    if (has_timeout) {
      remaining_ms = PollTimeRemaining(deadline);
      if (remaining_ms == 0) {
        if (descriptors != nullptr) {
          for (nfds_t i = 0; i < descriptor_count; ++i) {
            descriptors[i].revents = 0;
          }
        }
        return 0;
      }
    }
  }
}

int roo_testing_ioctl(int descriptor, unsigned long request, void *argument) {
  return WithoutSchedulerTick(
      [&] { return ::ioctl(descriptor, request, argument); });
}

ssize_t roo_testing_read(int descriptor, void *buffer, size_t size) {
  return ::read(descriptor, buffer, size);
}

ssize_t roo_testing_readv(int descriptor, const struct iovec *buffers,
                          int buffer_count) {
  return ::readv(descriptor, buffers, buffer_count);
}

ssize_t roo_testing_write(int descriptor, const void *buffer, size_t size) {
  return ::write(descriptor, buffer, size);
}

ssize_t roo_testing_writev(int descriptor, const struct iovec *buffers,
                           int buffer_count) {
  return ::writev(descriptor, buffers, buffer_count);
}

// GNU ld redirects bare BSD socket references to these entry points. Keeping
// the redirect out of the preprocessor prevents names such as
// NetworkClient::connect and NetworkServer::accept from being rewritten.
int __wrap_accept(int socket, struct sockaddr *address,
                  socklen_t *address_size) {
  return roo_testing_accept(socket, address, address_size);
}

int __wrap_bind(int socket, const struct sockaddr *address,
                socklen_t address_size) {
  return roo_testing_bind(socket, address, address_size);
}

int __wrap_shutdown(int socket, int how) {
  return roo_testing_shutdown(socket, how);
}

int __wrap_getpeername(int socket, struct sockaddr *address,
                       socklen_t *address_size) {
  return roo_testing_getpeername(socket, address, address_size);
}

int __wrap_getsockname(int socket, struct sockaddr *address,
                       socklen_t *address_size) {
  return roo_testing_getsockname(socket, address, address_size);
}

int __wrap_setsockopt(int socket, int level, int option, const void *value,
                      socklen_t value_size) {
  return roo_testing_setsockopt(socket, level, option, value, value_size);
}

int __wrap_getsockopt(int socket, int level, int option, void *value,
                      socklen_t *value_size) {
  return roo_testing_getsockopt(socket, level, option, value, value_size);
}

int __wrap_connect(int socket, const struct sockaddr *address,
                   socklen_t address_size) {
  return roo_testing_connect(socket, address, address_size);
}

int __wrap_listen(int socket, int backlog) {
  return roo_testing_listen(socket, backlog);
}

ssize_t __wrap_recv(int socket, void *buffer, size_t size, int flags) {
  return roo_testing_recv(socket, buffer, size, flags);
}

ssize_t __wrap_recvmsg(int socket, struct msghdr *message, int flags) {
  return roo_testing_recvmsg(socket, message, flags);
}

ssize_t __wrap_recvfrom(int socket, void *buffer, size_t size, int flags,
                        struct sockaddr *address, socklen_t *address_size) {
  return roo_testing_recvfrom(socket, buffer, size, flags, address,
                              address_size);
}

ssize_t __wrap_send(int socket, const void *buffer, size_t size, int flags) {
  return roo_testing_send(socket, buffer, size, flags);
}

ssize_t __wrap_sendmsg(int socket, const struct msghdr *message, int flags) {
  return roo_testing_sendmsg(socket, message, flags);
}

ssize_t __wrap_sendto(int socket, const void *buffer, size_t size, int flags,
                      const struct sockaddr *address,
                      socklen_t address_size) {
  return roo_testing_sendto(socket, buffer, size, flags, address,
                            address_size);
}

int __wrap_socket(int domain, int type, int protocol) {
  return roo_testing_socket(domain, type, protocol);
}

int __wrap_select(int descriptor_count, fd_set *read_descriptors,
                  fd_set *write_descriptors, fd_set *error_descriptors,
                  struct timeval *timeout) {
  return roo_testing_select(descriptor_count, read_descriptors,
                            write_descriptors, error_descriptors, timeout);
}

int __wrap_poll(struct pollfd *descriptors, nfds_t descriptor_count,
                int timeout_ms) {
  return roo_testing_poll(descriptors, descriptor_count, timeout_ms);
}

}  // extern "C"
