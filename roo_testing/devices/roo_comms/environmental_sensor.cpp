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
      roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
      if (done_) break;
      switch (state_) {
        case kUnpaired: {
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
          break;
        }
        case kPairing: {
          LOG(INFO) << "Sending pairing request message";
          roo::comms::ControlMessage msg;
          auto* req = msg.mutable_hub_pairing_request();
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
          break;
        }
        case kPaired: {
          // Send periodic sensor updates...
          break;
        }
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
      state_(kUnpaired),
      done_(false) {}

void FakeRooEnvironmentalSensor::start() {
  updater_ = roo_testing::thread(update_fn, this);
}

void FakeRooEnvironmentalSensor::send(const void* data, size_t len) {
  LOG(INFO) << "Received data of length " << len;
  // Check magic.
  if (len < 8 || std::memcmp(data, kControlMagic, 8) != 0) {
    LOG(WARNING) << "Invalid control message received";
    return;
  }
  data = (const unsigned char*)data + 8;
  len -= 8;
  roo::comms::ControlMessage msg;
  if (!msg.ParseFromArray(data, (int)len)) {
    LOG(WARNING) << "Failed to parse control message";
    return;
  }
  switch (msg.contents_case()) {
    case roo::comms::ControlMessage::kHubDiscoveryResponse: {
      LOG(INFO) << "Received discovery response";
      roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
      state_ = kPairing;
      break;
    }
    case roo::comms::ControlMessage::kHubPairingResponse: {
      LOG(INFO) << "Received pairing response";
      roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
      state_ = kPaired;
      break;
    }
    default: {
      LOG(WARNING) << "Unexpected control message received";
    }
  }
}

FakeRooEnvironmentalSensor::~FakeRooEnvironmentalSensor() {
  {
    roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
    done_ = true;
  }
  updater_.join();
}
