#include "esp_wifi.h"

#include <netdb.h>
#include <signal.h>

#include <vector>

#include "esp_private/wifi.h"  // for wifi_log_level_t
#include "freertos/timers.h"
#include "lwip/dns.h"
#include "roo_testing/devices/microcontroller/esp32/fake_esp32.h"
#include "roo_testing/sys/mutex.h"

using namespace roo_testing_transducers;

namespace {

inline wifi_auth_mode_t toAuthMode(
    const roo_testing_transducers::wifi::AuthMode auth_mode) {
  return (wifi_auth_mode_t)auth_mode;
}

inline void toBSSID(
    const roo_testing_transducers::wifi::MacAddress &mac_address,
    uint8_t *bssid) {
  bssid[0] = mac_address.get(0);
  bssid[1] = mac_address.get(1);
  bssid[2] = mac_address.get(2);
  bssid[3] = mac_address.get(3);
  bssid[4] = mac_address.get(4);
  bssid[5] = mac_address.get(5);
}

inline std::string fromCharArray(const char *cstr) {
  return std::string(cstr, strlen(cstr));
}

inline void toCharArray(const std::string &in, uint8_t *out, int maxlen) {
  size_t len = in.size() + 1;
  if (len > maxlen) len = maxlen;
  memcpy(out, in.c_str(), len);
}

inline wifi_ap_record_t toApRecord(
    const roo_testing_transducers::wifi::AccessPoint &ap) {
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

extern "C" {
void asyncScan(TimerHandle_t xTimer);
void asyncDisconnect(TimerHandle_t xTimer);
void asyncConnect(TimerHandle_t xTimer);
void asyncGotIp(TimerHandle_t xTimer);
}

class Esp32WifiAdapter {
 public:
  enum State {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
  };

  Esp32WifiAdapter() : scan_completed_(false), state_(DISCONNECTED) {}

  esp_err_t scan(const wifi_scan_config_t &config, bool block) {
    scan_config_ = config;
    if (block) {
      asyncScan(nullptr);
    } else {
      static StaticTimer_t static_timer;
      static TimerHandle_t timer = xTimerCreateStatic(
          "wifiScan", 2000, false, nullptr, &asyncScan, &static_timer);
      xTimerStart(timer, portMAX_DELAY);
    }
    return ESP_OK;
  }

  esp_err_t getApCount(uint16_t *number) {
    roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
    *number = known_networks_.size();
    return ESP_OK;
  }

  esp_err_t getApRecords(std::vector<wifi_ap_record_t> &records) {
    roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
    records = known_networks_;
    return ESP_OK;
  }

  void setStaConfig(const wifi_sta_config_t &config) {
    roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
    sta_config_ = config;
  }

  wifi_sta_config_t getStaConfig() {
    roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
    return sta_config_;
  }

  esp_err_t connect() {
    roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
    if (state_ == CONNECTED || state_ == CONNECTING) return ESP_OK;
    if (state_ == DISCONNECTING) {
      state_ = CONNECTED;
      return ESP_OK;
    }
    state_ = CONNECTING;
    auto ap = findAP();
    if (ap == nullptr) {
      disconnect_reason_ = WIFI_REASON_NO_AP_FOUND;
      static StaticTimer_t static_timer;
      static TimerHandle_t timer = xTimerCreateStatic(
          "wifiNoAp", 2000, false, nullptr, &asyncDisconnect, &static_timer);
      xTimerStart(timer, portMAX_DELAY);
    } else {
      uint8_t mac[6];
      esp_efuse_mac_get_default(mac);
      connection_ =
          ap->createConnection(roo_testing_transducers::wifi::MacAddress(mac));
      if (ap->passwd() != fromCharArray((const char *)sta_config_.password)) {
        disconnect_reason_ = WIFI_REASON_AUTH_FAIL;
        static StaticTimer_t static_timer;
        static TimerHandle_t timer =
            xTimerCreateStatic("wifiAuthFail", 2000, false, nullptr,
                               &asyncDisconnect, &static_timer);
        xTimerStart(timer, portMAX_DELAY);
      } else {
        static StaticTimer_t static_timer;
        static TimerHandle_t timer =
            xTimerCreateStatic("wifiConnected", 1200, false, nullptr,
                               &asyncConnect, &static_timer);
        xTimerStart(timer, portMAX_DELAY);
      }
    }
    return ESP_OK;
  }

  esp_err_t disconnect() {
    {
      roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
      if (state_ == DISCONNECTED || state_ == DISCONNECTING) return ESP_OK;
      if (state_ == CONNECTING) {
        state_ = DISCONNECTED;
        connection_ = nullptr;
        return ESP_OK;
      }
      state_ = DISCONNECTING;
    }
    disconnect_reason_ = WIFI_REASON_UNSPECIFIED;
    // Called synchronously so that reconnect blocks.
    asyncDisconnect(nullptr);
    return ESP_OK;
  }

  esp_err_t getApInfo(wifi_ap_record_t &result) {
    roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
    if (state_ != CONNECTED) {
      return ESP_ERR_WIFI_NOT_CONNECT;
    }
    result = toApRecord(connection_->access_point());
    return ESP_OK;
  }

  void setLogLevel(wifi_log_level_t log_level) { log_level_ = log_level; }

  wifi_log_level_t getLogLevel() const { return log_level_; }

  void setProtocol(uint8_t protocol) { wifi_protocol_ = protocol; }

  uint8_t getProtocol() const { return wifi_protocol_; }

  void finalizeScan() {
    const roo_testing_transducers::wifi::Environment &env =
        FakeEsp32().getWifiEnvironment();
    std::vector<wifi_ap_record_t> found;

    for (const auto &i : env.access_points()) {
      const roo_testing_transducers::wifi::AccessPoint &ap = *i.second;
      if (!ap.isVisible() && !scan_config_.show_hidden) continue;
      if (scan_config_.channel > 0 && scan_config_.channel != ap.channel())
        continue;
      if (ap.rssi() < -80) continue;
      if (scan_config_.ssid != nullptr &&
          strcmp((const char *)scan_config_.ssid, ap.ssid().c_str()) != 0) {
        continue;
      }
      if (scan_config_.bssid != nullptr &&
          roo_testing_transducers::wifi::MacAddress(scan_config_.bssid) !=
              ap.macAddress()) {
        continue;
      }
      found.push_back(toApRecord(ap));
    }
    {
      roo_testing::lock_guard<roo_testing::mutex> lock(mutex_);
      known_networks_ = std::move(found);
      scan_completed_ = true;
      scan_result_ = ESP_OK;
    }
    wifi_event_sta_scan_done_t event_data = {
        .status = 0,
        .number = known_networks_.size(),
        .scan_id = 0,
    };
    esp_event_post(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &event_data,
                   sizeof(event_data), portMAX_DELAY);
  }

  void finalizeDisconnect() {
    if (state_ != CONNECTING && state_ != DISCONNECTING) return;
    state_ = DISCONNECTED;
    connection_ = nullptr;
    wifi_event_sta_disconnected_t event_data;
    event_data.reason = disconnect_reason_;
    if (connection_ != nullptr) {
      const roo_testing_transducers::wifi::AccessPoint &ap =
          connection_->access_point();
      toCharArray(ap.ssid(), event_data.ssid, 33);
      toBSSID(ap.macAddress(), event_data.bssid);
    }
    esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &event_data,
                   sizeof(event_data), portMAX_DELAY);
  }

  void finalizeConnect() {
    if (state_ != CONNECTING) return;
    state_ = CONNECTED;
    const roo_testing_transducers::wifi::AccessPoint &ap =
        connection_->access_point();
    wifi_event_sta_connected_t event_data;
    event_data.authmode = toAuthMode(ap.auth_mode());
    event_data.channel = ap.channel();
    toCharArray(ap.ssid(), event_data.ssid, 33);
    event_data.ssid_len = ap.ssid().size();
    toBSSID(ap.macAddress(), event_data.bssid);
    esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &event_data,
                   sizeof(event_data), portMAX_DELAY);
    static StaticTimer_t static_timer;
    static TimerHandle_t timer = xTimerCreateStatic(
        "wifiGotIp", 300, false, nullptr, &asyncGotIp, &static_timer);
    xTimerStart(timer, portMAX_DELAY);
  }

  void finalizeGotIP() {
    const roo_testing_transducers::wifi::AccessPoint &ap =
        connection_->access_point();
    ip_event_got_ip_t event_data;
    event_data.if_index = 0;
    esp_event_post(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_data,
                   sizeof(event_data), portMAX_DELAY);
  }

 private:
  // Checks if there is an AP matching the sta_config_ criteria.
  roo_testing_transducers::wifi::AccessPoint *findAP() {
    const roo_testing_transducers::wifi::Environment &env =
        FakeEsp32().getWifiEnvironment();
    roo_testing_transducers::wifi::AccessPoint *found = nullptr;
    std::string ssid = fromCharArray((const char *)sta_config_.ssid);
    for (auto &i : env.access_points()) {
      roo_testing_transducers::wifi::AccessPoint &ap = *i.second;
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

  std::unique_ptr<roo_testing_transducers::wifi::Connection> connection_;

  std::vector<wifi_ap_record_t> known_networks_;
  // wifi_ap_record_t current_network_;
  bool scan_completed_;
  esp_err_t scan_result_;

  roo_testing::mutex mutex_;
  State state_;
  wifi_log_level_t log_level_;
  uint8_t wifi_protocol_;
  wifi_scan_config_t scan_config_;
  wifi_sta_config_t sta_config_;
  wifi_err_reason_t disconnect_reason_;
};

Esp32WifiAdapter &adapter() {
  static Esp32WifiAdapter adapter;
  return adapter;
}

}  // namespace

extern "C" {

esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config, bool block) {
  return adapter().scan(*config, block);
}

esp_err_t esp_wifi_scan_get_ap_num(uint16_t *number) {
  return adapter().getApCount(number);
}

esp_err_t esp_wifi_scan_get_ap_records(uint16_t *number,
                                       wifi_ap_record_t *ap_records) {
  std::vector<wifi_ap_record_t> records;
  esp_err_t result = adapter().getApRecords(records);
  if (result != ESP_OK) {
    return result;
  }
  if (*number > records.size()) *number = records.size();
  std::copy(records.begin(), records.begin() + *number, ap_records);
  return result;
}

esp_err_t esp_wifi_get_config(wifi_interface_t interface, wifi_config_t *conf) {
  if (interface != ESP_IF_WIFI_STA) {
    return ESP_ERR_INVALID_ARG;
  }
  conf->sta = adapter().getStaConfig();
  return ESP_OK;
}

esp_err_t esp_wifi_connect(void) { return adapter().connect(); }

esp_err_t esp_wifi_disconnect(void) { return adapter().disconnect(); }

esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf) {
  if (interface != ESP_IF_WIFI_STA) {
    return ESP_ERR_INVALID_ARG;
  }
  adapter().setStaConfig(conf->sta);
  return ESP_OK;
}

esp_err_t esp_wifi_get_mac(wifi_interface_t ifx, uint8_t mac[6]) {
  return ESP_OK;
}

esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap_info) {
  return adapter().getApInfo(*ap_info);
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

// esp_err_t esp_wifi_init(const wifi_init_config_t *config) { return ESP_OK; }

esp_err_t esp_wifi_start(void) { return ESP_OK; }

esp_err_t esp_wifi_stop(void) { return ESP_OK; }

esp_err_t esp_wifi_restore(void) { return ESP_OK; }

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

// NETIF

esp_err_t esp_netif_init(void) { return ESP_OK; }

esp_err_t esp_netif_deinit(void) { return ESP_OK; }

esp_err_t esp_netif_set_hostname(esp_netif_t *esp_netif, const char *hostname) {
  return ESP_OK;
}

esp_err_t esp_netif_dhcpc_start(esp_netif_t *esp_netif) { return ESP_OK; }

esp_err_t esp_netif_dhcpc_stop(esp_netif_t *esp_netif) { return ESP_OK; }

esp_err_t esp_netif_set_ip_info(esp_netif_t *esp_netif,
                                const esp_netif_ip_info_t *ip_info) {
  return ESP_OK;
}

esp_err_t esp_netif_dhcps_get_status(esp_netif_t *esp_netif,
                                     esp_netif_dhcp_status_t *status) {
  *status = ESP_NETIF_DHCP_STARTED;
  return ESP_OK;
}

esp_err_t esp_netif_dhcps_start(esp_netif_t *esp_netif) { return ESP_OK; }

esp_err_t esp_netif_dhcps_stop(esp_netif_t *esp_netif) { return ESP_OK; }

esp_err_t esp_netif_set_dns_info(esp_netif_t *esp_netif,
                                 esp_netif_dns_type_t type,
                                 esp_netif_dns_info_t *dns) {
  return ESP_OK;
}

esp_err_t esp_netif_create_ip6_linklocal(esp_netif_t *esp_netif) {
  return true;
}

// DNS

int8_t dns_gethostbyname(const char *hostname, ip_addr_t *addr,
                         dns_found_callback found, void *callback_arg) {
  //   struct hostent *hent;
  //   struct in_addr **addr_list;
  //   int i;
  //   if ((hent = gethostbyname(hostname)) == nullptr) {
  //     found(hostname, nullptr, callback_arg);
  //     return 0;
  //   }
  //   addr_list = (struct in_addr **)hent->h_addr_list;
  //   for (i = 0; addr_list[i] != nullptr; i++) {
  //     addr->type = addr_list[i]->s_addr
  //     strcpy(ip, inet_ntoa(*addr_list[i]));
  //     return 0;
  //   }
  //   found(hostname, nullptr, callback_arg);
  return 0;
}

const ip_addr_t *dns_getserver(u8_t numdns) {
  static ip_addr_t addr = {
      .u_addr = {.ip4 = {.addr = 0x08080808}},
      .type = IPADDR_TYPE_V4,
  };
  return &addr;
}

// Logging

esp_err_t esp_wifi_internal_set_log_level(wifi_log_level_t log_level) {
  adapter().setLogLevel(log_level);
  return ESP_OK;
}

// WIFI protocol

esp_err_t esp_wifi_set_protocol(wifi_interface_t ifx, uint8_t protocol_bitmap) {
  adapter().setProtocol(protocol_bitmap);
  return ESP_OK;
}

// FTM

esp_err_t esp_wifi_ftm_initiate_session(wifi_ftm_initiator_cfg_t *cfg) {
  return ESP_OK;
}

esp_err_t esp_wifi_ftm_end_session(void) { return ESP_OK; }

// AP

esp_err_t esp_wifi_ap_get_sta_list(wifi_sta_list_t *sta) {
  sta->num = 0;
  return ESP_OK;
}

namespace {

void asyncScan(TimerHandle_t xTimer) { adapter().finalizeScan(); }

void asyncDisconnect(TimerHandle_t xTimer) { adapter().finalizeDisconnect(); }

void asyncConnect(TimerHandle_t xTimer) { adapter().finalizeConnect(); }

void asyncGotIp(TimerHandle_t xTimer) { adapter().finalizeGotIP(); }

}  // namespace

esp_err_t esp_wifi_set_channel(uint8_t primary, wifi_second_chan_t second) {
  return ESP_OK;
}

}  // extern "C"