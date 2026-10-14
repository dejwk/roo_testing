#include "roo_testing/devices/roo_comms/environmental_sensor.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32_espnow.h"

namespace {
const std::string kControlMagic("roo\x00\xe1\xb2\x88\x99", 8);
const std::string kDataMagic("roo\x00\x5e\x0c\x15\x03", 8);

class Inbox {
 public:
  void receive(const uint8_t*, const uint8_t* data, size_t length) {
    roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
    messages_.emplace_back(reinterpret_cast<const char*>(data), length);
    ready_.notify_all();
  }

  std::string next() {
    roo_testing::unique_lock<roo_testing::mutex> lock(mutex_);
    if (!ready_.wait_for(lock, std::chrono::seconds(3),
                         [this]() { return !messages_.empty(); }))
      return {};
    std::string message = std::move(messages_.front());
    messages_.erase(messages_.begin());
    return message;
  }

 private:
  roo_testing::mutex mutex_;
  roo_testing::condition_variable ready_;
  std::vector<std::string> messages_;
};

void SendControl(FakeRooEnvironmentalSensor& sensor,
                 const std::string& payload) {
  std::string packet = kControlMagic + payload;
  sensor.send(packet.data(), packet.size());
}

// Verifies successful pairing starts periodic AHT20/BMP280 temperature packets,
// resamples the transducer, and leaves unsupported or missing values absent.
TEST(FakeEnvironmentalSensorTest, PairedSensorPublishesChangingTemperatures) {
  Inbox inbox;
  EspNowInterface radio;
  radio.setRecvCb([&](const uint8_t* mac, const uint8_t* data, size_t length) {
    inbox.receive(mac, data, length);
  });
  std::atomic<double> temperature(21.5);
  roo_testing_transducers::FunctionThermometer thermometer([&]() {
    return roo_testing_transducers::Temperature::FromC(temperature.load());
  });
  FakeRooEnvironmentalSensor sensor(0x112233FFFFEE, &thermometer);
  radio.attachDevice(sensor);
  sensor.start();
  std::string packet = inbox.next();
  ASSERT_EQ(kControlMagic, packet.substr(0, 8));
  ASSERT_GT(packet.size(), 8u);
  EXPECT_EQ(0x0a, static_cast<unsigned char>(packet[8]));
  // HubDiscoveryResponse { hub_channel: 1 }.
  SendControl(sensor, std::string("\x12\x02\x08\x01", 4));
  packet = inbox.next();
  ASSERT_EQ(kControlMagic, packet.substr(0, 8));
  ASSERT_GT(packet.size(), 8u);
  EXPECT_EQ(0x1a, static_cast<unsigned char>(packet[8]));
  // HubPairingResponse { status: kOk } (default status is omitted).
  SendControl(sensor, std::string("\x22\x00", 2));

  struct Sample {
    double temperature;
    std::string payload;
  };
  const Sample samples[] = {
      {21.5, std::string("\x2a\x0e\x12\x05\x0d\x00\x00\xac\x41"
                         "\x1a\x05\x0d\x00\x00\xac\x41",
                         16)},
      {32.0, std::string("\x2a\x0e\x12\x05\x0d\x00\x00\x00\x42"
                         "\x1a\x05\x0d\x00\x00\x00\x42",
                         16)},
      {std::nan(""), std::string("\x2a\x00", 2)},
  };
  for (const Sample& sample : samples) {
    temperature.store(sample.temperature);
    packet = inbox.next();
    ASSERT_EQ(kDataMagic, packet.substr(0, 8));
    // Fixed wire bytes independently verify both optional float fields and
    // absence of humidity/pressure values for which no transducer exists.
    EXPECT_EQ(kDataMagic + sample.payload, packet);
  }
}
}  // namespace
