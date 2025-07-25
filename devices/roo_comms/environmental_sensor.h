#pragma once

#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "roo_testing/buses/esp_now/fake_esp_now.h"
#include "roo_testing/transducers/temperature/temperature.h"
#include "roo_testing/sys/mutex.h"
#include "roo_testing/sys/thread.h"

// const roo_transceivers_Descriptor* getFakeEnvironmentalSensorDescriptor();

class FakeRooEnvironmentalSensor : public FakeEspNowDevice {
 public:
  FakeRooEnvironmentalSensor(
      uint64_t addr, const roo_testing_transducers::Thermometer* thermometer);

  ~FakeRooEnvironmentalSensor();

  void start();

  void send(const void* data, size_t len) override;

 private:
  friend void update_fn(FakeRooEnvironmentalSensor*);

  void update();

  const roo_testing_transducers::Thermometer* thermometer_;
  bool paired_;
  bool done_;
  roo_testing::thread updater_;
  roo_testing::mutex mutex_;
};
