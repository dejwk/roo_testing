#include "roo_testing/devices/roo_transceivers/environmental_sensor.h"

#include <cstring>

#include "roo_time.h"
#include "roo_transceivers/universe.h"

const roo_transceivers_Descriptor* getFakeEnvironmentalSensorDescriptor() {
  static roo_transceivers_Descriptor descriptor = {
      .sensors_count = 1,
      .sensors =
          {
              {.id = "temperature",
               .quantity = roo_transceivers_Quantity_kTemperature},
          },
      .actuators_count = 0,
      .actuators = {}};
  return &descriptor;
}

namespace {

roo_time::Uptime rounded_now() {
  return roo_time::Uptime::Start() +
         roo_time::Seconds(
             (roo_time::Uptime::Now() - roo_time::Uptime::Start()).inSeconds());
}

roo_transceivers::Measurement measurement(
    const roo_testing_transducers::Thermometer& thermometer) {
  return roo_transceivers::Measurement(roo_transceivers_Quantity_kTemperature,
                                       rounded_now(), thermometer.read().AsC());
}

}  // namespace

roo_transceivers::Measurement FakeEnvironmentalSensor::read(
    std::string_view sensor_id) const {
  if (sensor_id == "temperature") {
    return measurement(*thermometer_);
  } else {
    return roo_transceivers::Measurement();
  }
}
