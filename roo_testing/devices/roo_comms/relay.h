#pragma once

#include <cstdint>
#include <functional>

#include "roo_testing/buses/esp_now/fake_esp_now.h"
#include "roo_testing/sys/condition_variable.h"
#include "roo_testing/sys/mutex.h"
#include "roo_testing/sys/thread.h"

/// Four-channel ESP-NOW relay with discovery, pairing, and state feedback.
/// Channels start off. Pairing and channel state are not persisted.
class FakeRooRelay4p : public FakeEspNowDevice {
 public:
  /// Receives a zero-based channel index and its new on/off state.
  using WriteFn = std::function<void(int, bool)>;

  /// Creates a relay at MAC address @p addr with optional change callback.
  /// The callback runs on the sending thread, outside the state lock; it may
  /// inspect state(), but must not destroy the device.
  explicit FakeRooRelay4p(uint64_t addr, WriteFn write_fn = {});

  /// Stops and joins the background broadcaster, if started.
  ~FakeRooRelay4p() override;

  /// Starts one-second discovery/pairing retries and paired state reports.
  /// Attach to the fake radio before calling. Repeated calls are harmless.
  void start();

  /// Handles control packets and paired relay read/write requests.
  /// Malformed packets and data received before pairing are ignored.
  void send(const void* data, size_t len) override;

  /// Returns the four channel states as a bitmask (bit zero is channel one).
  uint32_t state() const;

 private:
  enum PairingState { kUnpaired, kPairing, kPaired };

  void update();

  // Called with mutex_ held; emits the current four-bit state.
  void reportState();

  WriteFn write_fn_;
  PairingState pairing_ = kUnpaired;
  uint32_t state_ = 0;
  bool done_ = false;
  mutable roo_testing::mutex mutex_;
  roo_testing::condition_variable wake_;
  roo_testing::thread updater_;
};
