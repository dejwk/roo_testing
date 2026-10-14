#include "roo_testing/devices/roo_comms/relay.h"

#include <chrono>
#include <cstring>
#include <limits>
#include <string>
#include <utility>

#include "glog/logging.h"
#include "roo_testing/devices/roo_comms/roo_comms.pb.h"

namespace {
constexpr char kControlMagic[] = "roo\x00\xe1\xb2\x88\x99";
constexpr char kDataMagic[] = "roo\x00\x5e\x0c\x15\x03";
constexpr size_t kMagicSize = 8;
constexpr uint32_t kChannelMask = 0x0f;
using roo::testing::comms::ControlMessage;
using roo::testing::comms::DataMessage;
}  // namespace

FakeRooRelay4p::FakeRooRelay4p(uint64_t addr, WriteFn write_fn)
    : FakeEspNowDevice(addr), write_fn_(std::move(write_fn)) {}

FakeRooRelay4p::~FakeRooRelay4p() {
  {
    roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
    done_ = true;
    wake_.notify_all();
  }
  if (updater_.joinable()) updater_.join();
}

void FakeRooRelay4p::start() {
  roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
  if (!updater_.joinable()) {
    updater_ = roo_testing::thread([this]() { update(); });
  }
}

uint32_t FakeRooRelay4p::state() const {
  roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
  return state_;
}

void FakeRooRelay4p::reportState() {
  DataMessage msg;
  msg.mutable_relay_response()->set_state(state_);
  std::string packet(kDataMagic, kMagicSize);
  packet += msg.SerializeAsString();
  respond(packet.data(), packet.size());
}

// Retry discovery/pairing until accepted, then publish state periodically.
void FakeRooRelay4p::update() {
  roo_testing::unique_lock<roo_testing::mutex> lock(mutex_);
  while (!done_) {
    if (pairing_ == kPaired) {
      reportState();
    } else {
      ControlMessage msg;
      auto* descriptor =
          pairing_ == kUnpaired
              ? msg.mutable_hub_discovery_request()->mutable_device_descriptor()
              : msg.mutable_hub_pairing_request()->mutable_device_descriptor();
      descriptor->set_realm_id(roo::testing::comms::kHomeAutomation);
      roo::testing::comms::HomeAutomationDeviceDescriptor home;
      home.mutable_relay()->set_port_count(4);
      CHECK(home.SerializeToString(descriptor->mutable_payload()));
      std::string packet(kControlMagic, kMagicSize);
      packet += msg.SerializeAsString();
      respond(packet.data(), packet.size());
    }
    wake_.wait_for(lock, std::chrono::seconds(1), [this]() { return done_; });
  }
}

void FakeRooRelay4p::send(const void* data, size_t len) {
  if (data == nullptr || len < kMagicSize ||
      len - kMagicSize > static_cast<size_t>(std::numeric_limits<int>::max())) {
    return;
  }
  const char* payload = static_cast<const char*>(data) + kMagicSize;
  const int size = static_cast<int>(len - kMagicSize);
  if (std::memcmp(data, kControlMagic, kMagicSize) == 0) {
    ControlMessage msg;
    if (!msg.ParseFromArray(payload, size)) return;
    roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
    if (msg.has_hub_discovery_response()) {
      pairing_ = kPairing;
    } else if (msg.has_hub_pairing_response() && pairing_ == kPairing) {
      pairing_ = msg.hub_pairing_response().status() ==
                         ControlMessage::HubPairingResponse::kOk
                     ? kPaired
                     : kUnpaired;
    }
    return;
  }
  if (std::memcmp(data, kDataMagic, kMagicSize) != 0) return;
  DataMessage msg;
  if (!msg.ParseFromArray(payload, size) || !msg.has_relay_request()) return;
  uint32_t changed;
  uint32_t current;
  {
    roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
    if (pairing_ != kPaired) return;
    const uint32_t mask = msg.relay_request().mask() & kChannelMask;
    current = (state_ & ~mask) | (msg.relay_request().write() & mask);
    changed = state_ ^ current;
    state_ = current;
    reportState();
  }
  if (write_fn_ != nullptr) {
    for (int idx = 0; idx < 4; ++idx) {
      if ((changed & (1u << idx)) != 0) {
        write_fn_(idx, (current & (1u << idx)) != 0);
      }
    }
  }
}
