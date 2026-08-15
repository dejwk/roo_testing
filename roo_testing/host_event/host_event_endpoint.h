#pragma once

#include <atomic>
#include <cstdint>

namespace roo_testing {

enum class HostEventConnectResult : uint8_t {
  kConnected,
  kAlreadyConnected,
  kNoCapacity,
  kWrongContext,
};

namespace internal {
class HostEventGateway;
void hostEventTickHook() noexcept;
} // namespace internal

/// Transfers payload-free readiness from a native host thread to FreeRTOS.
///
/// `connect()`, `disconnect()`, and `isConnected()` are FreeRTOS-task
/// operations. A native producer must publish its payload before calling
/// `notifyFromHost()`, and must quiesce its callbacks before disconnecting.
class HostEventEndpoint {
public:
  using Handler = void (*)(void *context);

  HostEventEndpoint() = default;

  HostEventConnectResult connect(Handler handler, void *context);
  void disconnect();
  bool isConnected() const;

  // Safe only from a native host thread. Coalesces readiness and never blocks.
  void notifyFromHost() noexcept;

  HostEventEndpoint(const HostEventEndpoint &) = delete;
  HostEventEndpoint &operator=(const HostEventEndpoint &) = delete;

private:
  friend class internal::HostEventGateway;

  std::atomic<bool> pending_{false};
  Handler handler_ = nullptr;
  void *context_ = nullptr;
  int8_t slot_ = -1;
};

} // namespace roo_testing
