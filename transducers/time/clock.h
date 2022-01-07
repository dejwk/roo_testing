#pragma once

#include <stdint.h>

#include "roo_testing/transducers/transducer.h"

namespace roo_testing_transducers {

class Clock : public Transducer {
 public:
  // Returns microseconds since Unix epoch.
  virtual int64_t getTimeMicros() const = 0;

  virtual void delayMicros(uint64_t delay) = 0;
};

class SystemClock : public Clock {
 public:
  int64_t getTimeMicros() const override;
  void delayMicros(uint64_t delay) override;

  const std::string& name() const override {
    static std::string name = "System Clock";
    return name;
  }
};

class FakeClock : public Clock {
 public:
  FakeClock() : time_(0) {}
  int64_t getTimeMicros() const override { return time_; }
  void delayMicros(uint64_t delay) override { time_ += (int64_t)delay; }

  const std::string& name() const override {
    static std::string name = "Fake Clock";
    return name;
  }

 private:
  int64_t time_;
};

Clock* getDefaultSystemClock();

}  // namespace roo_testing_transducers
