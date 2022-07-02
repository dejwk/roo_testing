#include "esp_wifi.h"

#include <condition_variable>
#include <mutex>
#include <vector>

#include "WiFiScan.h"
#include "fake_esp32.h"

using namespace roo_testing_transducers;

wifi_sta_config_t esp32_sta_config;

extern "C" {

esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config, bool block) {
  WifiScanConfig &cfg = *(WifiScanConfig *)config;
  if (block) {
    bool done = false;
    std::mutex mutex;
    std::condition_variable condvar;
    auto result = (esp_err_t)FakeEsp32().wifi.startScan(cfg, [&]() {
      WiFiScanClass::_scanDone();
      std::unique_lock<std::mutex> lock(mutex);
      done = true;
      condvar.notify_one();
    });
    std::unique_lock<std::mutex> lock(mutex);
    while (!done) condvar.wait(lock);
    return ESP_OK;
  } else {
    return (esp_err_t)FakeEsp32().wifi.startScan(cfg, WiFiScanClass::_scanDone);
  }
}

esp_err_t esp_wifi_scan_get_ap_num(uint16_t *number) {
  return (esp_err_t)FakeEsp32().wifi.getApCount(number);
}

esp_err_t esp_wifi_scan_get_ap_records(uint16_t *number,
                                       wifi_ap_record_t *ap_records) {
  std::vector<WifiApRecord> records;
  esp_err_t result = (esp_err_t)FakeEsp32().wifi.getApRecords(records);
  if (result != OK) {
    return result;
  }
  if (*number > records.size()) *number = records.size();
  std::copy(records.begin(), records.begin() + *number,
            (WifiApRecord *)ap_records);
  return result;
}

esp_err_t esp_wifi_get_config(wifi_interface_t interface, wifi_config_t *conf) {
  if (interface != ESP_IF_WIFI_STA) {
    return ESP_ERR_INVALID_ARG;
  }
  conf->sta = esp32_sta_config;
  return ESP_OK;
}

esp_err_t esp_wifi_connect(void) {
  return FakeEsp32().wifi.connect(*((WifiStaConfig*)&esp32_sta_config));
}

esp_err_t esp_wifi_disconnect(void) { return FakeEsp32().wifi.disconnect(); }

esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf) {
  if (interface != ESP_IF_WIFI_STA) {
    return ESP_ERR_INVALID_ARG;
  }
  esp32_sta_config = conf->sta;
  return ESP_OK;
}

esp_err_t esp_wifi_get_mac(wifi_interface_t ifx, uint8_t mac[6]) {
  return ESP_OK;
}

esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap_info) {
  return (esp_err_t)FakeEsp32().wifi.getApInfo(*(WifiApRecord *)ap_info);
}

esp_err_t esp_wifi_get_mode(wifi_mode_t *mode) {
  *mode = WIFI_MODE_STA;
  return ESP_OK;
}

esp_err_t esp_wifi_set_mode(wifi_mode_t mode) {
  if (mode != WIFI_MODE_STA) return ESP_ERR_INVALID_ARG;
  return ESP_OK;
}

esp_err_t esp_wifi_set_storage(wifi_storage_t storage) { return ESP_OK; }

esp_err_t esp_wifi_init(const wifi_init_config_t *config) { return ESP_OK; }

esp_err_t esp_wifi_start(void) { return ESP_OK; }

esp_err_t esp_wifi_stop(void) { return ESP_OK; }

esp_err_t esp_wifi_get_channel(uint8_t *primary, wifi_second_chan_t *second) {
  *primary = 1;
  *second = ::WIFI_SECOND_CHAN_NONE;
  return ESP_OK;
}

esp_err_t esp_wifi_set_ps(wifi_ps_type_t type) { return ESP_OK; }

esp_err_t esp_wifi_get_ps(wifi_ps_type_t *type) {
  *type = WIFI_PS_NONE;
  return ESP_OK;
}

esp_err_t esp_wifi_set_max_tx_power(int8_t power) { return ESP_OK; }

esp_err_t esp_wifi_get_max_tx_power(int8_t *power) {
  *power = 84;
  return ESP_OK;
}

}  // extern "C"