#pragma once
#include <stdint.h>

#include "roo_testing/system/timer.h"
#if defined(__FREERTOS)
#include "runtime.h"
#endif

namespace roo_testing {
namespace pico {
/// Reserves a controller across an Arduino conversation: SPI=0..1, I2C=2..3.
inline void ReserveController(uint32_t resource) {
#if defined(__FREERTOS)
  RooFreeRtosReserveResource(resource);
#else
  (void)resource;
#endif
}

/// Releases a controller reservation owned by the current task.
inline void ReleaseController(uint32_t resource) {
#if defined(__FREERTOS)
  RooFreeRtosReleaseResource(resource);
#else
  (void)resource;
#endif
}

/// Holds a recursive controller lock for an individual supported SDK operation.
class ControllerLock {
 public:
  /// Acquires @p resource until this operation has completed.
  explicit ControllerLock(uint32_t resource) : resource_(resource) {
#if defined(__FREERTOS)
    RooFreeRtosLockResource(resource_);
#endif
  }

  /// Releases the operation while preserving an enclosing Arduino reservation.
  ~ControllerLock() {
#if defined(__FREERTOS)
    RooFreeRtosUnlockResource(resource_);
#endif
  }

  /// Controller operations cannot be copied.
  ControllerLock(const ControllerLock&) = delete;
  /// Controller operations cannot be assigned.
  ControllerLock& operator=(const ControllerLock&) = delete;

 private:
  uint32_t resource_;
};

/// Parks a task for a transfer interval, preserving the bare-metal time path.
inline void TransferDelayNs(uint64_t ns) {
#if defined(__FREERTOS)
  system_time_delay_micros(ns / 1000 + (ns % 1000 != 0));
#else
  system_time_lag_ns(ns);
  system_time_sync();
#endif
}
}  // namespace pico
}  // namespace roo_testing
