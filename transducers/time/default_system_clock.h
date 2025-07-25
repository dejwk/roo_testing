#pragma once

#include <stdint.h>

#include "roo_testing/transducers/time/clock.h"

class SystemClock : public Clock {
 public:
  int64_t getTimeMicros() const override;
  void delayMicros(uint64_t delay) override;

  const std::string& name() const override {
    static std::string name = "System Clock";
    return name;
  }
};
