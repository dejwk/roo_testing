#include "roo_testing/buses/esp_now/fake_esp_now.h"

#include <cstring>

#include "roo_testing/sys/mutex.h"

namespace {
void GetMac(uint64_t mac, uint8_t* dest) {
  dest[5] = mac >> 0;
  dest[4] = mac >> 8;
  dest[3] = mac >> 16;
  dest[2] = mac >> 24;
  dest[1] = mac >> 32;
  dest[0] = mac >> 40;
}

uint64_t FromMac(const uint8_t* addr) {
  return ((uint64_t)addr[0] << 40) | ((uint64_t)addr[1] << 32) |
         ((uint64_t)addr[2] << 24) | ((uint64_t)addr[3] << 16) |
         ((uint64_t)addr[4] << 8) | ((uint64_t)addr[5] << 0);
}
}  // namespace

bool FakeEspNowInterface::send(const uint8_t* addr, const void* data,
                               size_t len) {
  uint64_t dest_mac = FromMac(addr);
  if (dest_mac == 0xFFFFFFFFFFFFL) {
    for (auto& itr : devices_) {
      itr.second->send(data, len);
    }
    OnSentCb cb;
    {
      roo_testing::lock_guard<roo_testing::mutex> guard(mutex_);
      cb = on_sent_cb_;
    }
    if (cb != nullptr) {
      uint8_t mac[6];
      GetMac(dest_mac, mac);
      cb(mac, true);
    }
    return true;
  } else {
    auto itr = devices_.find(dest_mac);
    if (itr == devices_.end()) return false;
    itr->second->send(data, len);
    OnSentCb cb;
    {
      roo_testing::lock_guard<roo_testing::mutex> guard(mutex_);
      cb = on_sent_cb_;
    }
    if (cb != nullptr) {
      uint8_t mac[6];
      GetMac(dest_mac, mac);
      cb(mac, true);
    }
    return true;
  }
}

FakeEspNowInterface::FakeEspNowInterface()
    : devices_(), inbox_(), on_sent_cb_(nullptr) {
      start();
    }

// FakeEspNowInterface::FakeEspNowInterface(
//     std::vector<std::pair<uint64_t, std::unique_ptr<FakeEspNowDevice>>>
//     devices) : FakeEspNowInterface() {
//   for (std::pair<uint64_t, std::unique_ptr<FakeEspNowDevice>>& d : devices) {
//     addDevice(d.first, std::move(d).second);
//   }
// }

void FakeEspNowInterface::attachDevice(FakeEspNowDevice& device) {
  uint64_t addr = device.mac_addr();
  device.set_receive_cb([this, addr](const void* data, size_t len) {
    incoming(addr, data, len);
  });
  devices_[addr] = &device;
}

void FakeEspNowInterface::incoming(uint64_t peer, const void* data,
                                   size_t len) {
  if (len == 0) return;
  roo_testing::lock_guard<roo_testing::mutex> guard(mutex_);
  std::unique_ptr<uint8_t[]> payload(new uint8_t[len]);
  memcpy(payload.get(), data, len);
  inbox_.push(
      Packet{.peer_mac = peer, .size = (int)len, .payload = std::move(payload)});
  nonempty_.notify_all();
}

void FakeEspNowInterface::setSendCb(OnSentCb send_cb) {
  roo_testing::lock_guard<roo_testing::mutex> guard(mutex_);
  on_sent_cb_ = std::move(send_cb);
}

void FakeEspNowInterface::setRecvCb(OnRecvCb recv_cb) {
  roo_testing::lock_guard<roo_testing::mutex> guard(mutex_);
  on_recv_cb_ = std::move(recv_cb);
}

void FakeEspNowInterface::processInbox() {
  Packet packet;
  OnRecvCb cb;
  while (true) {
    {
      roo_testing::unique_lock<roo_testing::mutex> guard(mutex_);
      while (inbox_.empty()) {
        nonempty_.wait(guard);
      }
      packet = std::move(inbox_.front());
      inbox_.pop();
      cb = on_recv_cb_;
    }
    if (packet.peer_mac == 0 && packet.size == -1) {
      // Sentinel.
      break;
    }
    if (cb != nullptr) {
      uint8_t mac[6];
      GetMac(packet.peer_mac, mac);
      cb(mac, packet.payload.get(), packet.size);
    }
  }
}

void process_inbox_fn(FakeEspNowInterface* interface) {
  interface->processInbox();
}

void FakeEspNowInterface::start() {
  roo_testing::thread::attributes attrs;
  attrs.set_name("roo_t/espnow");
  inbox_processor_ = roo_testing::thread(attrs, process_inbox_fn, this);
}

FakeEspNowInterface::~FakeEspNowInterface() {
  {
    roo_testing::lock_guard<roo_testing::mutex> guard(mutex_);
    inbox_.push(Packet{.peer_mac = 0, .size = -1, .payload = nullptr});
    nonempty_.notify_all();
  }
  inbox_processor_.join();
}