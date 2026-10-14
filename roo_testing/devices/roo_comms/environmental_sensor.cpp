#include "roo_testing/devices/roo_comms/environmental_sensor.h"

#include <cmath>
#include <cstring>

#include "glog/logging.h"
#include "roo_testing/devices/roo_comms/roo_comms.pb.h"

namespace {

static const unsigned char kControlMagic[] = {'r',  'o',  'o',  0x0,
                                              0xE1, 0xB2, 0x88, 0x99};

static const unsigned char kDataMagic[] = {'r',  'o',  'o',  0x00,
                                           0x5E, 0x0C, 0x15, 0x03};

roo::testing::comms::HomeAutomationDeviceDescriptor
BuildHomeAutomationDescriptor() {
  roo::testing::comms::HomeAutomationDeviceDescriptor desc;
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
          roo::testing::comms::ControlMessage msg;
          auto* req = msg.mutable_hub_discovery_request();
          auto* desc = req->mutable_device_descriptor();
          desc->set_realm_id(roo::testing::comms::kHomeAutomation);
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
          roo::testing::comms::ControlMessage msg;
          auto* req = msg.mutable_hub_pairing_request();
          auto* desc = req->mutable_device_descriptor();
          desc->set_realm_id(roo::testing::comms::kHomeAutomation);
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
          roo::testing::comms::DataMessage msg;
          auto* readings = msg.mutable_environmental_sensor_readings();
          const double temperature = thermometer_ == nullptr
                                         ? std::nan("")
                                         : thermometer_->read().AsC();
          if (std::isfinite(temperature)) {
            readings->mutable_aht20()->set_temperature_celsius(temperature);
            readings->mutable_bmp280()->set_temperature_celsius(temperature);
          }
          // No humidity or pressure transducers are attached to this fake.
          // Leave those optional readings absent instead of inventing values.
          std::string payload;
          CHECK(msg.SerializeToString(&payload));
          std::string packet(reinterpret_cast<const char*>(kDataMagic),
                             sizeof(kDataMagic));
          packet.append(payload);
          respond(packet.data(), packet.size());
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
  roo::testing::comms::ControlMessage msg;
  if (!msg.ParseFromArray(data, (int)len)) {
    LOG(WARNING) << "Failed to parse control message";
    return;
  }
  switch (msg.contents_case()) {
    case roo::testing::comms::ControlMessage::kHubDiscoveryResponse: {
      LOG(INFO) << "Received discovery response";
      roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
      state_ = kPairing;
      break;
    }
    case roo::testing::comms::ControlMessage::kHubPairingResponse: {
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
