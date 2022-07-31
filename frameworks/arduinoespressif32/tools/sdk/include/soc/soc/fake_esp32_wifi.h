#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <thread>

#include "WiFiScan.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "fake_esp32_nvs.h"
#include "roo_testing/buses/gpio/fake_gpio.h"
#include "roo_testing/buses/i2c/fake_i2c.h"
#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/transducers/time/clock.h"
#include "roo_testing/transducers/wifi/wifi.h"

namespace {

inline wifi_auth_mode_t toAuthMode(
    const roo_testing_transducers::wifi::AuthMode auth_mode) {
  return (wifi_auth_mode_t)auth_mode;
}

inline void toBSSID(
    const roo_testing_transducers::wifi::MacAddress& mac_address,
    uint8_t* bssid) {
  bssid[0] = mac_address.get(0);
  bssid[1] = mac_address.get(1);
  bssid[2] = mac_address.get(2);
  bssid[3] = mac_address.get(3);
  bssid[4] = mac_address.get(4);
  bssid[5] = mac_address.get(5);
}

inline std::string fromCharArray(const char* cstr) {
  return std::string(cstr, strlen(cstr));
}

inline void toCharArray(const std::string& in, uint8_t* out, int maxlen) {
  size_t len = in.size() + 1;
  if (len > maxlen) len = maxlen;
  memcpy(out, in.c_str(), len);
}

inline wifi_ap_record_t toApRecord(
    const roo_testing_transducers::wifi::AccessPoint& ap) {
  wifi_ap_record_t record;
  toBSSID(ap.macAddress(), record.bssid);
  toCharArray(ap.ssid(), record.ssid, 33);
  record.primary = ap.channel();
  record.second = WIFI_SECOND_CHAN_NONE;
  record.rssi = ap.rssi();
  record.authmode = toAuthMode(ap.auth_mode());
  record.pairwise_cipher = WIFI_CIPHER_TYPE_NONE;
  record.group_cipher = WIFI_CIPHER_TYPE_NONE;
  record.ant = WIFI_ANT_ANT0;
  record.phy_11b = 1;
  record.phy_11g = 1;
  record.phy_11n = 1;
  record.phy_lr = 0;
  record.wps = 0;
  // record.ftm_responder = 0;
  // record.ftm_initiator = 0;
  record.country = {.cc = "PL",
                    .schan = 11,
                    .nchan = 1,
                    .max_tx_power = 0,
                    .policy = WIFI_COUNTRY_POLICY_AUTO};
  return record;
}

}  // namespace

class Esp32WifiAdapter {
 public:
  enum State {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
  };

  Esp32WifiAdapter();

  void setEnvironment(roo_testing_transducers::wifi::Environment& env) {
    env_ = &env;
  }

  esp_err_t scan(const wifi_scan_config_t& config, bool block) {
    std::vector<wifi_ap_record_t> found;

    for (const auto& i : env_->access_points()) {
      const roo_testing_transducers::wifi::AccessPoint& ap = *i.second;
      if (!ap.isVisible() && !config.show_hidden) continue;
      if (config.channel > 0 && config.channel != ap.channel()) continue;
      if (ap.rssi() < -80) continue;
      if (config.ssid != nullptr &&
          strcmp((const char*)config.ssid, ap.ssid().c_str()) != 0) {
        continue;
      }
      if (config.bssid != nullptr && roo_testing_transducers::wifi::MacAddress(
                                         config.bssid) != ap.macAddress()) {
        continue;
      }
      found.push_back(toApRecord(ap));
    }
    if (block) {
      delayedScanResults(std::move(found), 2000);
    } else {
      std::thread t(
          [this, found]() { delayedScanResults(std::move(found), 2000); });
      t.detach();
    }
    return ESP_OK;
  }

  esp_err_t getApCount(uint16_t* number) {
    std::unique_lock<std::mutex> lock(mutex_);
    *number = known_networks_.size();
    return ESP_OK;
  }

  esp_err_t getApRecords(std::vector<wifi_ap_record_t>& records) {
    std::unique_lock<std::mutex> lock(mutex_);
    records = known_networks_;
    return ESP_OK;
  }

  void setStaConfig(const wifi_sta_config_t& config) {
    std::unique_lock<std::mutex> lock(mutex_);
    sta_config_ = config;
  }

  wifi_sta_config_t getStaConfig() {
    std::unique_lock<std::mutex> lock(mutex_);
    return sta_config_;
  }

  esp_err_t connect() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (state_ == CONNECTED || state_ == CONNECTING) return ESP_OK;
    if (state_ == DISCONNECTING) {
      state_ = CONNECTED;
      return ESP_OK;
    }
    state_ = CONNECTING;
    auto ap = findAP();
    if (ap != nullptr) {
      connection_ = ap->createConnection(mac_address_);
      if (ap->passwd() != fromCharArray((const char*)sta_config_.password)) {
        std::thread t(
            [this]() { delayedNotifyWrongPassword(*connection_, 1000); });
        t.detach();
      } else {
        std::thread t([this]() { delayedNotifyConnected(*connection_, 1000); });
        t.detach();
      }
    } else {
      std::thread t([this]() {
        delayedNotifyNoAP(fromCharArray((const char*)sta_config_.ssid), nullptr,
                          1000);
      });
      t.detach();
    }
    // auto result = radio_->connect(config, &current_network_);
    // connected_ = (result == OK);
    return ESP_OK;
  }

  esp_err_t disconnect() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (state_ == DISCONNECTED || state_ == DISCONNECTING) return ESP_OK;
    if (state_ == CONNECTING) {
      state_ = DISCONNECTED;
      connection_ = nullptr;
      return ESP_OK;
    }
    state_ = DISCONNECTING;
    std::thread t([this]() {
      delayedNotifyDisconnected(*connection_, 200, WIFI_REASON_UNSPECIFIED);
    });
    t.detach();
    return ESP_OK;
  }

  esp_err_t getApInfo(wifi_ap_record_t& result) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (state_ != CONNECTED) {
      return ESP_ERR_WIFI_NOT_CONNECT;
    }
    result = toApRecord(connection_->access_point());
    return ESP_OK;
  }

 private:
  void delayedScanResults(std::vector<wifi_ap_record_t> records,
                          uint32_t delay_ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    {
      std::unique_lock<std::mutex> lock(mutex_);
      known_networks_ = std::move(records);
      scan_completed_ = true;
      scan_result_ = ESP_OK;
    }
    system_event_t event;
    event.event_id = SYSTEM_EVENT_SCAN_DONE;
    event.event_info.scan_done = {
        .status = 0, .number = (uint8_t)known_networks_.size(), .scan_id = 0};
    WiFiGenericClass::_eventCallback(nullptr, &event);
  }

  void delayedNotifyNoAP(
      const std::string& ssid,
      const roo_testing_transducers::wifi::MacAddress* mac_address,
      uint32_t delay_ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    system_event_t event;
    event.event_id = SYSTEM_EVENT_STA_DISCONNECTED;
    event.event_info.disconnected.reason = WIFI_REASON_NO_AP_FOUND;
    toCharArray(ssid, event.event_info.disconnected.ssid, 33);
    event.event_info.disconnected.ssid_len = ssid.size();
    if (mac_address != nullptr) {
      toBSSID(*mac_address, event.event_info.disconnected.bssid);
    }
    bool notify = false;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      notify = (state_ == CONNECTING);
      if (state_ == CONNECTED) return;
      state_ = DISCONNECTED;
    }
    if (notify) {
      WiFiGenericClass::_eventCallback(nullptr, &event);
    }
  }

  void delayedNotifyWrongPassword(
      const roo_testing_transducers::wifi::Connection& conn,
      uint32_t delay_ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    const roo_testing_transducers::wifi::AccessPoint& ap = conn.access_point();
    system_event_t event;
    event.event_id = SYSTEM_EVENT_STA_DISCONNECTED;
    event.event_info.disconnected.reason = WIFI_REASON_AUTH_FAIL;
    toCharArray(ap.ssid(), event.event_info.disconnected.ssid, 33);
    event.event_info.disconnected.ssid_len = ap.ssid().size();
    toBSSID(ap.macAddress(), event.event_info.disconnected.bssid);
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (state_ != CONNECTING) return;
      state_ = DISCONNECTED;
    }
    WiFiGenericClass::_eventCallback(nullptr, &event);
  }

  void delayedNotifyConnected(
      const roo_testing_transducers::wifi::Connection& conn,
      uint32_t delay_ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    const roo_testing_transducers::wifi::AccessPoint& ap = conn.access_point();
    system_event_t event;
    event.event_id = SYSTEM_EVENT_STA_CONNECTED;
    event.event_info.connected.authmode = toAuthMode(ap.auth_mode());
    event.event_info.connected.channel = ap.channel();
    toCharArray(ap.ssid(), event.event_info.connected.ssid, 33);
    event.event_info.connected.ssid_len = ap.ssid().size();
    toBSSID(ap.macAddress(), event.event_info.connected.bssid);
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (state_ != CONNECTING) return;
      state_ = CONNECTED;
    }
    WiFiGenericClass::_eventCallback(nullptr, &event);
    std::thread t([this]() { delayedNotifyGotIP(*connection_, 300); });
    t.detach();
  }

  void delayedNotifyGotIP(const roo_testing_transducers::wifi::Connection& conn,
                          uint32_t delay_ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    const roo_testing_transducers::wifi::AccessPoint& ap = conn.access_point();
    system_event_t event;
    event.event_id = SYSTEM_EVENT_STA_GOT_IP;
    // event.event_info.got_ip.if_index = 0;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (state_ != CONNECTED) return;
    }
    WiFiGenericClass::_eventCallback(nullptr, &event);
  }

  void delayedNotifyDisconnected(
      const roo_testing_transducers::wifi::Connection& conn, uint32_t delay_ms,
      uint8_t reason) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    const roo_testing_transducers::wifi::AccessPoint& ap = conn.access_point();
    system_event_t event;
    event.event_id = SYSTEM_EVENT_STA_DISCONNECTED;
    toCharArray(ap.ssid(), event.event_info.disconnected.ssid, 33);
    event.event_info.disconnected.ssid_len = ap.ssid().size();
    toBSSID(ap.macAddress(), event.event_info.disconnected.bssid);
    event.event_info.disconnected.reason = reason;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (state_ != DISCONNECTING) return;
      state_ = DISCONNECTED;
    }
    WiFiGenericClass::_eventCallback(nullptr, &event);
  }

  // Checks if there is an AP matching the sta_config_ criteria.
  roo_testing_transducers::wifi::AccessPoint* findAP() {
    roo_testing_transducers::wifi::AccessPoint* found = nullptr;
    std::string ssid = fromCharArray((const char*)sta_config_.ssid);
    for (auto& i : env_->access_points()) {
      roo_testing_transducers::wifi::AccessPoint& ap = *i.second;
      if (sta_config_.bssid_set && roo_testing_transducers::wifi::MacAddress(
                                       sta_config_.bssid) != ap.macAddress()) {
        continue;
      }
      if (ssid != ap.ssid()) {
        continue;
      }
      // Found a match.
      if (found != nullptr) {
        switch (sta_config_.sort_method) {
          case WIFI_CONNECT_AP_BY_SIGNAL: {
            if (found->rssi() > ap.rssi()) continue;
            break;
          }
          case WIFI_CONNECT_AP_BY_SECURITY: {
            break;
          }
        }
      }
      found = &ap;
    }
    return found;
  }

  roo_testing_transducers::wifi::MacAddress mac_address_;
  roo_testing_transducers::wifi::Environment* env_;
  std::unique_ptr<roo_testing_transducers::wifi::Connection> connection_;

  std::vector<wifi_ap_record_t> known_networks_;
  // wifi_ap_record_t current_network_;
  bool scan_completed_;
  esp_err_t scan_result_;

  std::mutex mutex_;
  State state_;
  wifi_sta_config_t sta_config_;
};
