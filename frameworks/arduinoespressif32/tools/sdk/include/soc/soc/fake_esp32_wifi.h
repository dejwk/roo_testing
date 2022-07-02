#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <thread>

#include "esp_err.h"
#include "fake_esp32_nvs.h"
#include "roo_testing/buses/gpio/fake_gpio.h"
#include "roo_testing/buses/i2c/fake_i2c.h"
#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/transducers/time/clock.h"
#include "roo_testing/transducers/wifi/wifi.h"

class Esp32WifiAdapter {
 public:
  Esp32WifiAdapter()
      : scan_completed_(false),
        env_(nullptr),
        connected_(false) {}

  void setEnvironment(roo_testing_transducers::wifi::Environment& env) { 
    env_ = &env; 
  }

  WifiOperationResult startScan(const WifiScanConfig& config,
                                std::function<void()> done_fn) {
    std::thread t([this, done_fn, config]() {
      auto scan_result = radio_->scan(config);
      {
        std::unique_lock<std::mutex> lock(mutex_);
        known_networks_ = std::move(scan_result.networks);
        scan_completed_ = true;
        scan_result_ = scan_result.result;
      }
      done_fn();
    });
    t.detach();
    return roo_testing_transducers::WifiOperationResult::OK;
  }

  WifiOperationResult getApCount(uint16_t* number) {
    std::unique_lock<std::mutex> lock(mutex_);
    *number = known_networks_.size();
    return OK;
  }

  WifiOperationResult getApRecords(std::vector<WifiApRecord>& records) {
    std::unique_lock<std::mutex> lock(mutex_);
    records = known_networks_;
    return OK;
  }

  WifiOperationResult connect(const WifiStaConfig& config) {
    auto result = radio_->connect(config, &current_network_);
    // connected_ = (result == OK);
    return result;
  }

  // WifiOperationResult disconnect() {
  //   connected_ = false;
  //   return OK;
  // }

  roo_testing_transducers::WifiOperationResult getApInfo(
      roo_testing_transducers::WifiApRecord& result) {
    if (!connected_) {
      return ERR_WIFI_NOT_CONNECT;
    }
    result = current_network_;
    return roo_testing_transducers::WifiOperationResult::OK;
  }

 private:
  roo_testing_transducers::wifi::Environment* env_;
  


  std::vector<WifiApRecord> known_networks_;
  WifiApRecord current_network_;
  bool scan_completed_;
  WifiOperationResult scan_result_;

  std::mutex mutex_;
  bool connected_;
  WifiStaConfig sta_config_;
};
