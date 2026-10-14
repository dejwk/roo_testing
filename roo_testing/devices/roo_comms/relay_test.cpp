#include "roo_testing/devices/roo_comms/relay.h"

#include <chrono>
#include <string>
#include <utility>
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

void Send(FakeRooRelay4p& relay, const std::string& packet) {
  relay.send(packet.data(), packet.size());
}

void Pair(FakeRooRelay4p& relay) {
  Send(relay, kControlMagic + std::string("\x12\x02\x08\x01", 4));
  Send(relay, kControlMagic + std::string("\x22\x00", 2));
}

// Encode fixed32 fields independently of the implementation's protobuf runtime.
std::string Request(uint32_t mask, uint32_t write) {
  std::string packet = kDataMagic + std::string("\x32\x0a\x0d", 3);
  for (int shift = 0; shift < 32; shift += 8) packet += char(mask >> shift);
  packet += char(0x15);
  for (int shift = 0; shift < 32; shift += 8) packet += char(write >> shift);
  return packet;
}

std::string Response(uint8_t state) {
  if (state == 0) return kDataMagic + std::string("\x3a\x00", 2);
  return kDataMagic + std::string("\x3a\x05\x0d", 3) + char(state) +
         std::string(3, '\0');
}

// Verifies discovery advertises four relay ports, rejected pairing retries
// discovery, accepted pairing publishes state, and start is idempotent.
TEST(FakeRelayTest, DiscoveryPairingAndPeriodicReports) {
  Inbox inbox;
  EspNowInterface radio;
  radio.setRecvCb([&](const uint8_t* mac, const uint8_t* data, size_t len) {
    inbox.receive(mac, data, len);
  });
  FakeRooRelay4p relay(0x121223233444);
  radio.attachDevice(relay);
  relay.start();
  relay.start();
  const std::string descriptor("\x0a\x08\x08\x01\x12\x04\x12\x02\x08\x04", 10);
  EXPECT_EQ(kControlMagic + std::string("\x0a\x0a", 2) + descriptor,
            inbox.next());
  Send(relay, kControlMagic + std::string("\x12\x00", 2));
  EXPECT_EQ(kControlMagic + std::string("\x1a\x0a", 2) + descriptor,
            inbox.next());
  Send(relay, kControlMagic + std::string("\x22\x02\x08\x01", 4));
  EXPECT_EQ(kControlMagic + std::string("\x0a\x0a", 2) + descriptor,
            inbox.next());
  Pair(relay);
  EXPECT_EQ(Response(0), inbox.next());
  EXPECT_EQ(Response(0), inbox.next());
}

// Verifies masked writes preserve other channels, high bits are ignored, reads
// do not write, and callbacks fire only for changes and may inspect state.
TEST(FakeRelayTest, MaskedWritesReadsAndCallbacks) {
  Inbox inbox;
  EspNowInterface radio;
  radio.setRecvCb([&](const uint8_t* mac, const uint8_t* data, size_t len) {
    inbox.receive(mac, data, len);
  });
  std::vector<std::pair<int, bool>> changes;
  FakeRooRelay4p* target = nullptr;
  FakeRooRelay4p relay(0x121223233444, [&](int idx, bool enabled) {
    EXPECT_EQ(enabled, (target->state() & (1u << idx)) != 0);
    changes.emplace_back(idx, enabled);
  });
  target = &relay;
  radio.attachDevice(relay);
  Pair(relay);
  const uint8_t mac[] = {0x12, 0x12, 0x23, 0x23, 0x34, 0x44};
  struct Sample {
    uint32_t mask;
    uint32_t write;
    uint8_t expected;
  };
  const Sample samples[] = {{0, 15, 0},
                            {5, 5, 5},
                            {2, 2, 7},
                            {1, 0, 6},
                            {0xfffffff8, 0xffffffff, 14},
                            {0, 0xffffffff, 14},
                            {15, 14, 14},
                            {15, 0, 0}};
  for (const Sample& sample : samples) {
    std::string packet = Request(sample.mask, sample.write);
    ASSERT_TRUE(radio.send(mac, packet.data(), packet.size()));
    EXPECT_EQ(Response(sample.expected), inbox.next());
    EXPECT_EQ(sample.expected, relay.state());
  }
  const std::vector<std::pair<int, bool>> expected = {
      {0, true}, {2, true},  {1, true},  {0, false},
      {3, true}, {1, false}, {2, false}, {3, false}};
  EXPECT_EQ(expected, changes);
}

// Verifies malformed traffic, unsolicited acceptance, and commands before or
// after rejected pairing cannot change state; destruction needs no start().
TEST(FakeRelayTest, IgnoresMalformedAndUnpairedCommands) {
  FakeRooRelay4p relay(0x121223233444);
  Send(relay, Request(15, 15));
  Send(relay, kControlMagic + std::string("\x22\x00", 2));
  Send(relay, Request(15, 15));
  EXPECT_EQ(0u, relay.state());
  Send(relay, kControlMagic + std::string("\x12\x00", 2));
  Send(relay, kControlMagic + std::string("\x22\x02\x08\x01", 4));
  Send(relay, Request(15, 15));
  EXPECT_EQ(0u, relay.state());
  Pair(relay);
  relay.send(nullptr, 20);
  Send(relay, "short");
  Send(relay, std::string(8, 'x') + Request(15, 15).substr(8));
  Send(relay, kDataMagic + std::string("\x32\x05\x0d", 3));
  Send(relay, kControlMagic + std::string("\x12\x02", 2));
  EXPECT_EQ(0u, relay.state());
  Send(relay, Request(15, 9));
  EXPECT_EQ(9u, relay.state());
}
}  // namespace
