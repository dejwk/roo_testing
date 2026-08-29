#pragma once

#include <cstdint>
#include <optional>

/// Private, value-owned LEDC fade scheduler. Public LEDC APIs deliberately do
/// not use this engine until the interrupt and blocking integration phase.
class LedcFadeEngine {
 public:
  struct Request {
    uint8_t channel;
    int pin;
    uint32_t frequency_hz;
    uint8_t resolution;
    uint32_t start_duty;
    uint32_t target_duty;
    int64_t carrier_origin_us;
    double pulse_start_phase = 0;
    bool inverted = false;
    uint64_t duration_us = 0;
  };

  struct Completion {
    uint8_t channel;
    uint64_t generation;
    uint32_t duty;
  };

  struct Snapshot {
    uint64_t generation = 0;
    uint32_t duty = 0;
    bool gate_held = false;
    bool completion_pending = false;
  };

  LedcFadeEngine();
  ~LedcFadeEngine();
  LedcFadeEngine(const LedcFadeEngine&) = delete;
  LedcFadeEngine& operator=(const LedcFadeEngine&) = delete;

  bool Start(const Request& request);
  bool Cancel(uint8_t channel);
  Snapshot snapshot(uint8_t channel) const;
  std::optional<Completion> TakeCompletion(uint8_t channel);

 private:
  struct Impl;
  Impl* impl_;
};
