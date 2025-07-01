#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <queue>

#include "roo_testing/sys/mutex.h"
#include "roo_testing/sys/thread.h"
#include "roo_testing/sys/condition_variable.h"

class FakeEspNowDevice {
 public:
  using ReceiveCb = std::function<void(const void*, size_t)>;

  FakeEspNowDevice(uint64_t addr) : addr_(addr) {}

  virtual ~FakeEspNowDevice() {}

  virtual void send(const void* data, size_t len) = 0;

 protected:
  uint64_t mac_addr() const { return addr_; }

  void respond(const void* data, size_t len) { 
    if (receive_cb_ != nullptr) receive_cb_(data, len); }

 private:
  friend class FakeEspNowInterface;

  void set_receive_cb(ReceiveCb receive_cb) {
    receive_cb_ = std::move(receive_cb);
  }

  uint64_t addr_;
  ReceiveCb receive_cb_;
};

class FakeEspNowInterface {
 public:
  using OnSentCb = std::function<void(const uint8_t*, bool)>;
  using OnRecvCb = std::function<void(const uint8_t*, const uint8_t*, size_t)>;

  struct Packet {
    uint64_t peer_mac;
    int size;
    std::unique_ptr<uint8_t[]> payload;
  };

  FakeEspNowInterface();

  FakeEspNowInterface(
      std::vector<std::pair<uint64_t, std::unique_ptr<FakeEspNowDevice>>>
          devices);

  ~FakeEspNowInterface();

  void start();

  //   void addDevice(uint64_t mac, FakeEspNowDevice* device) {
  //     addDevice(mac, std::unique_ptr<FakeEspNowDevice>(device));
  //   }

  void attachDevice(FakeEspNowDevice& device);

  bool send(const uint8_t* dest_mac, const void* data, size_t len);

  void setSendCb(OnSentCb send_cb);
  void setRecvCb(OnRecvCb recv_cb);

 private:
  friend void process_inbox_fn(FakeEspNowInterface*);

  void incoming(uint64_t peer, const void* data, size_t len);

  void processInbox();

  std::map<uint64_t, FakeEspNowDevice*> devices_;
  std::queue<Packet> inbox_;
  roo_testing::mutex mutex_;
  roo_testing::condition_variable nonempty_;
  OnSentCb on_sent_cb_;
  OnRecvCb on_recv_cb_;

  roo_testing::thread inbox_processor_;
};
