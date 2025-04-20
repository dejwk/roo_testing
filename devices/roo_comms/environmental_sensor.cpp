#include "roo_testing/devices/roo_comms/environmental_sensor.h"

#include <cstring>

#include "glog/logging.h"
#include "roo_testing/devices/roo_comms/roo_comms.pb.h"

namespace {

static const unsigned char kControlMagic[] = {'r',  'o',  'o',  0x0,
                                              0xE1, 0xB2, 0x88, 0x99};

roo::comms::HomeAutomationDeviceDescriptor BuildHomeAutomationDescriptor() {
  roo::comms::HomeAutomationDeviceDescriptor desc;
  desc.mutable_environmental_sensor()->set_has_aht20(true);
  desc.mutable_environmental_sensor()->set_has_bmp280(true);
  return desc;
}

}  // namespace

void FakeRooEnvironmentalSensor::update() {
  while (true) {
    {
      std::scoped_lock<std::mutex> lock(mutex_);
      if (done_) break;
      if (!paired_) {
        LOG(INFO) << "Sending broadcast discovery message";
        roo::comms::ControlMessage msg;
        auto* req = msg.mutable_hub_discovery_request();
        auto* desc = req->mutable_device_descriptor();
        desc->set_realm_id(roo::comms::kHomeAutomation);
        CHECK(BuildHomeAutomationDescriptor().SerializeToString(
            desc->mutable_payload()));
        std::string serialized;
        CHECK(msg.SerializeToString(&serialized));
        std::string result;
        result.append(&kControlMagic[0], &kControlMagic[8]);
        result.append(serialized);
        respond(result.data(), result.size());
      } else {
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

void update_fn(FakeRooEnvironmentalSensor* target) { target->update(); }

FakeRooEnvironmentalSensor::FakeRooEnvironmentalSensor(
    uint64_t addr, const roo_testing_transducers::Thermometer* thermometer)
    : FakeEspNowDevice(addr),
      thermometer_(thermometer),
      paired_(false),
      done_(false) {}

void FakeRooEnvironmentalSensor::start() {
  updater_ = std::thread(update_fn, this);
}

void FakeRooEnvironmentalSensor::send(const void* data, size_t len) {}

FakeRooEnvironmentalSensor::~FakeRooEnvironmentalSensor() {
  {
    std::scoped_lock<std::mutex> lock(mutex_);
    done_ = true;
  }
  updater_.join();
}
