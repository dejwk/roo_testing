#pragma once

#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "roo_testing/buses/roo_transceivers/fake_transceivers.h"
#include "roo_testing/transducers/temperature/temperature.h"
#include "roo_transceivers/id.h"
#include "roo_transceivers/universe.h"

const roo_transceivers_Descriptor* getFakeEnvironmentalSensorDescriptor();

class FakeEnvironmentalSensor : public FakeTransceiver {
 public:
  FakeEnvironmentalSensor(
      const roo_transceivers::DeviceLocator& locator,
      const roo_testing_transducers::Thermometer* thermometer)
      : FakeTransceiver(locator, getFakeEnvironmentalSensorDescriptor()),
        thermometer_(thermometer) {}

  roo_transceivers::Measurement read(std::string_view sensor_id) const override;

  virtual bool write(std::string_view actuator_id, float value) override {
    return false;
  }

 private:
  const roo_testing_transducers::Thermometer* thermometer_;
};
