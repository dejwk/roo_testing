#include "esp_event.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "lwip/ip6_addr.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

ESP_EVENT_DEFINE_BASE(WIFI_EVENT);
ESP_EVENT_DEFINE_BASE(IP_EVENT);
ESP_EVENT_DEFINE_BASE(ETH_EVENT);
ESP_EVENT_DEFINE_BASE(SC_EVENT);
ESP_EVENT_DEFINE_BASE(WIFI_PROV_EVENT);

struct esp_netif_obj {
  std::string key;
  std::string description;
  std::string hostname;
  esp_netif_ip_info_t ip_info = {};
  esp_netif_dns_info_t dns[ESP_NETIF_DNS_MAX] = {};
  std::array<uint8_t, 6> mac{};
  esp_netif_flags_t flags = ESP_NETIF_FLAG_AUTOUP;
  int32_t get_ip_event = 0;
  int32_t lost_ip_event = 0;
  int route_priority = 100;
  bool up = true;
  esp_netif_dhcp_status_t dhcp_client = ESP_NETIF_DHCP_INIT;
  esp_netif_dhcp_status_t dhcp_server = ESP_NETIF_DHCP_INIT;
};

namespace {

using AccessPoint = roo_testing_transducers::wifi::AccessPoint;
using Connection = roo_testing_transducers::wifi::Connection;
using MacAddress = roo_testing_transducers::wifi::MacAddress;

std::mutex g_mutex;
wifi_mode_t g_mode = WIFI_MODE_NULL;
wifi_config_t g_station_config = {};
wifi_config_t g_ap_config = {};
wifi_ps_type_t g_power_save = WIFI_PS_NONE;
uint8_t g_protocol = WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N;
wifi_bandwidth_t g_bandwidth = WIFI_BW20;
uint8_t g_channel = 1;
int8_t g_max_tx_power = 84;
wifi_country_t g_country = {
    .cc = "PL",
    .schan = 1,
    .nchan = 13,
    .max_tx_power = 20,
    .policy = WIFI_COUNTRY_POLICY_AUTO,
};
bool g_started = false;
std::vector<wifi_ap_record_t> g_scan_results;
std::unique_ptr<Connection> g_connection;
esp_netif_t* g_default_netif = nullptr;
std::vector<esp_netif_t*> g_netifs;

void CopyMac(const MacAddress& source, uint8_t* destination) {
  for (size_t i = 0; i < 6; ++i) destination[i] = source.get(i);
}

void CopyString(const std::string& source, uint8_t* destination,
                size_t capacity) {
  if (capacity == 0) return;
  const size_t size = std::min(source.size(), capacity - 1);
  memcpy(destination, source.data(), size);
  destination[size] = '\0';
}

wifi_auth_mode_t ToAuthMode(
    roo_testing_transducers::wifi::AuthMode auth_mode) {
  return static_cast<wifi_auth_mode_t>(auth_mode);
}

wifi_ap_record_t ToRecord(const AccessPoint& ap) {
  wifi_ap_record_t record = {};
  CopyMac(ap.macAddress(), record.bssid);
  CopyString(ap.ssid(), record.ssid, sizeof(record.ssid));
  record.primary = static_cast<uint8_t>(ap.channel());
  record.second = WIFI_SECOND_CHAN_NONE;
  record.rssi = ap.rssi();
  record.authmode = ToAuthMode(ap.auth_mode());
  record.pairwise_cipher = WIFI_CIPHER_TYPE_NONE;
  record.group_cipher = WIFI_CIPHER_TYPE_NONE;
  record.ant = WIFI_ANT_ANT0;
  record.phy_11b = 1;
  record.phy_11g = 1;
  record.phy_11n = 1;
  record.country = g_country;
  record.bandwidth = WIFI_BW20;
  return record;
}

AccessPoint* FindConfiguredAccessPoint() {
  const auto& environment = FakeEsp32().getWifiEnvironment();
  const std::string ssid(reinterpret_cast<const char*>(g_station_config.sta.ssid));
  AccessPoint* found = nullptr;
  for (const auto& entry : environment.access_points()) {
    AccessPoint* candidate = entry.second.get();
    if (candidate->ssid() != ssid) continue;
    if (g_station_config.sta.bssid_set &&
        MacAddress(g_station_config.sta.bssid) != candidate->macAddress()) {
      continue;
    }
    if (found == nullptr || candidate->rssi() > found->rssi()) found = candidate;
  }
  return found;
}

void PostDisconnect(wifi_err_reason_t reason) {
  wifi_event_sta_disconnected_t event = {};
  event.reason = reason;
  esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &event,
                 sizeof(event), portMAX_DELAY);
}

esp_netif_t* NewNetif(const char* key, const char* description,
                      esp_netif_flags_t flags, int32_t get_ip_event = 0,
                      int32_t lost_ip_event = 0) {
  auto* netif = new esp_netif_t();
  netif->key = key;
  netif->description = description;
  netif->hostname = "roo-testing";
  netif->flags = flags;
  netif->get_ip_event = get_ip_event;
  netif->lost_ip_event = lost_ip_event;
  netif->ip_info.ip.addr = 0x6401A8C0U;      // 192.168.1.100
  netif->ip_info.netmask.addr = 0x00FFFFFFU; // 255.255.255.0
  netif->ip_info.gw.addr = 0x0101A8C0U;      // 192.168.1.1
  esp_read_mac(netif->mac.data(), ESP_MAC_WIFI_STA);
  g_netifs.push_back(netif);
  if (g_default_netif == nullptr) g_default_netif = netif;
  return netif;
}

}  // namespace

extern "C" {

esp_err_t esp_wifi_init(const wifi_init_config_t*) { return ESP_OK; }
esp_err_t esp_wifi_deinit(void) { return ESP_OK; }

esp_err_t esp_wifi_set_mode(wifi_mode_t mode) {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_mode = mode;
  return ESP_OK;
}
esp_err_t esp_wifi_get_mode(wifi_mode_t* mode) {
  if (mode == nullptr) return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  *mode = g_mode;
  return ESP_OK;
}

esp_err_t esp_wifi_start(void) {
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_started = true;
  }
  if (g_mode == WIFI_MODE_STA || g_mode == WIFI_MODE_APSTA) {
    esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_START, nullptr, 0, portMAX_DELAY);
  }
  if (g_mode == WIFI_MODE_AP || g_mode == WIFI_MODE_APSTA) {
    esp_event_post(WIFI_EVENT, WIFI_EVENT_AP_START, nullptr, 0, portMAX_DELAY);
  }
  return ESP_OK;
}

esp_err_t esp_wifi_stop(void) {
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_started = false;
    g_connection.reset();
  }
  esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_STOP, nullptr, 0, portMAX_DELAY);
  return ESP_OK;
}
esp_err_t esp_wifi_restore(void) { return ESP_OK; }
esp_err_t esp_wifi_clear_fast_connect(void) { return ESP_OK; }

esp_err_t esp_wifi_connect(void) {
  AccessPoint* ap;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_started) return ESP_ERR_WIFI_NOT_STARTED;
    ap = FindConfiguredAccessPoint();
    if (ap == nullptr) {
      // Post outside the lock because handlers may call back into Wi-Fi APIs.
    } else {
      const char* password =
          reinterpret_cast<const char*>(g_station_config.sta.password);
      if (ap->passwd() != password) ap = reinterpret_cast<AccessPoint*>(1);
    }
  }
  if (ap == nullptr) {
    PostDisconnect(WIFI_REASON_NO_AP_FOUND);
    return ESP_OK;
  }
  if (ap == reinterpret_cast<AccessPoint*>(1)) {
    PostDisconnect(WIFI_REASON_AUTH_FAIL);
    return ESP_OK;
  }

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_connection = ap->createConnection(MacAddress(mac));
  }
  wifi_event_sta_connected_t connected = {};
  CopyMac(ap->macAddress(), connected.bssid);
  CopyString(ap->ssid(), connected.ssid, sizeof(connected.ssid));
  connected.ssid_len = static_cast<uint8_t>(
      std::min(ap->ssid().size(), sizeof(connected.ssid)));
  connected.channel = static_cast<uint8_t>(ap->channel());
  connected.authmode = ToAuthMode(ap->auth_mode());
  esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &connected,
                 sizeof(connected), portMAX_DELAY);

  ip_event_got_ip_t got_ip = {};
  got_ip.esp_netif = g_default_netif;
  if (g_default_netif != nullptr) got_ip.ip_info = g_default_netif->ip_info;
  esp_event_post(IP_EVENT, IP_EVENT_STA_GOT_IP, &got_ip, sizeof(got_ip),
                 portMAX_DELAY);
  return ESP_OK;
}

esp_err_t esp_wifi_disconnect(void) {
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_connection.reset();
  }
  PostDisconnect(WIFI_REASON_UNSPECIFIED);
  return ESP_OK;
}

esp_err_t esp_wifi_scan_start(const wifi_scan_config_t* config, bool) {
  std::vector<wifi_ap_record_t> results;
  const auto& environment = FakeEsp32().getWifiEnvironment();
  for (const auto& entry : environment.access_points()) {
    const AccessPoint& ap = *entry.second;
    if (config != nullptr) {
      if (!ap.isVisible() && !config->show_hidden) continue;
      if (config->channel != 0 && config->channel != ap.channel()) continue;
      if (config->ssid != nullptr &&
          strcmp(reinterpret_cast<const char*>(config->ssid),
                 ap.ssid().c_str()) != 0) {
        continue;
      }
      if (config->bssid != nullptr &&
          MacAddress(config->bssid) != ap.macAddress()) {
        continue;
      }
    }
    results.push_back(ToRecord(ap));
  }
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_scan_results = std::move(results);
  }
  wifi_event_sta_scan_done_t event = {};
  event.status = 0;
  event.number = static_cast<uint16_t>(g_scan_results.size());
  esp_event_post(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &event, sizeof(event),
                 portMAX_DELAY);
  return ESP_OK;
}

esp_err_t esp_wifi_scan_stop(void) { return ESP_OK; }
esp_err_t esp_wifi_scan_get_ap_num(uint16_t* number) {
  if (number == nullptr) return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  *number = static_cast<uint16_t>(g_scan_results.size());
  return ESP_OK;
}
esp_err_t esp_wifi_scan_get_ap_records(uint16_t* number,
                                       wifi_ap_record_t* records) {
  if (number == nullptr || (records == nullptr && *number != 0)) {
    return ESP_ERR_INVALID_ARG;
  }
  std::lock_guard<std::mutex> lock(g_mutex);
  const size_t count = std::min<size_t>(*number, g_scan_results.size());
  std::copy_n(g_scan_results.begin(), count, records);
  *number = static_cast<uint16_t>(count);
  return ESP_OK;
}
esp_err_t esp_wifi_scan_get_ap_record(wifi_ap_record_t* record) {
  if (record == nullptr) return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_scan_results.empty()) return ESP_ERR_NOT_FOUND;
  *record = g_scan_results.front();
  return ESP_OK;
}
esp_err_t esp_wifi_clear_ap_list(void) {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_scan_results.clear();
  return ESP_OK;
}

esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t* config) {
  if (config == nullptr) return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  if (interface == WIFI_IF_STA) {
    g_station_config = *config;
  } else if (interface == WIFI_IF_AP) {
    g_ap_config = *config;
  } else {
    return ESP_ERR_INVALID_ARG;
  }
  return ESP_OK;
}
esp_err_t esp_wifi_get_config(wifi_interface_t interface, wifi_config_t* config) {
  if (config == nullptr) return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  if (interface == WIFI_IF_STA) {
    *config = g_station_config;
  } else if (interface == WIFI_IF_AP) {
    *config = g_ap_config;
  } else {
    return ESP_ERR_INVALID_ARG;
  }
  return ESP_OK;
}

esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t* info) {
  if (info == nullptr) return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_connection == nullptr) return ESP_ERR_WIFI_NOT_CONNECT;
  *info = ToRecord(g_connection->access_point());
  return ESP_OK;
}
esp_err_t esp_wifi_sta_get_rssi(int* rssi) {
  if (rssi == nullptr) return ESP_ERR_INVALID_ARG;
  wifi_ap_record_t record;
  const esp_err_t result = esp_wifi_sta_get_ap_info(&record);
  if (result == ESP_OK) *rssi = record.rssi;
  return result;
}

esp_err_t esp_wifi_set_storage(wifi_storage_t) { return ESP_OK; }
esp_err_t esp_wifi_set_ps(wifi_ps_type_t power_save) {
  g_power_save = power_save;
  return ESP_OK;
}
esp_err_t esp_wifi_get_ps(wifi_ps_type_t* power_save) {
  if (power_save == nullptr) return ESP_ERR_INVALID_ARG;
  *power_save = g_power_save;
  return ESP_OK;
}
esp_err_t esp_wifi_set_protocol(wifi_interface_t, uint8_t protocol) {
  g_protocol = protocol;
  return ESP_OK;
}
esp_err_t esp_wifi_get_protocol(wifi_interface_t, uint8_t* protocol) {
  if (protocol == nullptr) return ESP_ERR_INVALID_ARG;
  *protocol = g_protocol;
  return ESP_OK;
}
esp_err_t esp_wifi_set_protocols(wifi_interface_t,
                                 wifi_protocols_t* protocols) {
  if (protocols == nullptr) return ESP_ERR_INVALID_ARG;
  g_protocol = static_cast<uint8_t>(protocols->ghz_2g);
  return ESP_OK;
}
esp_err_t esp_wifi_get_protocols(wifi_interface_t,
                                 wifi_protocols_t* protocols) {
  if (protocols == nullptr) return ESP_ERR_INVALID_ARG;
  protocols->ghz_2g = g_protocol;
  protocols->ghz_5g = 0;
  return ESP_OK;
}
esp_err_t esp_wifi_set_bandwidth(wifi_interface_t, wifi_bandwidth_t bandwidth) {
  g_bandwidth = bandwidth;
  return ESP_OK;
}
esp_err_t esp_wifi_get_bandwidth(wifi_interface_t,
                                 wifi_bandwidth_t* bandwidth) {
  if (bandwidth == nullptr) return ESP_ERR_INVALID_ARG;
  *bandwidth = g_bandwidth;
  return ESP_OK;
}
esp_err_t esp_wifi_set_channel(uint8_t primary, wifi_second_chan_t) {
  g_channel = primary;
  return ESP_OK;
}
esp_err_t esp_wifi_get_channel(uint8_t* primary, wifi_second_chan_t* second) {
  if (primary == nullptr || second == nullptr) return ESP_ERR_INVALID_ARG;
  *primary = g_channel;
  *second = WIFI_SECOND_CHAN_NONE;
  return ESP_OK;
}
esp_err_t esp_wifi_set_country(const wifi_country_t* country) {
  if (country == nullptr) return ESP_ERR_INVALID_ARG;
  g_country = *country;
  return ESP_OK;
}
esp_err_t esp_wifi_get_country(wifi_country_t* country) {
  if (country == nullptr) return ESP_ERR_INVALID_ARG;
  *country = g_country;
  return ESP_OK;
}
esp_err_t esp_wifi_set_country_code(const char* country, bool) {
  if (country == nullptr) return ESP_ERR_INVALID_ARG;
  strncpy(g_country.cc, country, sizeof(g_country.cc));
  return ESP_OK;
}
esp_err_t esp_wifi_get_country_code(char* country) {
  if (country == nullptr) return ESP_ERR_INVALID_ARG;
  memcpy(country, g_country.cc, sizeof(g_country.cc));
  return ESP_OK;
}
esp_err_t esp_wifi_set_mac(wifi_interface_t, const uint8_t mac[6]) {
  return esp_base_mac_addr_set(mac);
}
esp_err_t esp_wifi_get_mac(wifi_interface_t interface, uint8_t mac[6]) {
  return esp_read_mac(mac, interface == WIFI_IF_STA ? ESP_MAC_WIFI_STA
                                                    : ESP_MAC_WIFI_SOFTAP);
}
esp_err_t esp_wifi_set_max_tx_power(int8_t power) {
  g_max_tx_power = power;
  return ESP_OK;
}
esp_err_t esp_wifi_get_max_tx_power(int8_t* power) {
  if (power == nullptr) return ESP_ERR_INVALID_ARG;
  *power = g_max_tx_power;
  return ESP_OK;
}
esp_err_t esp_wifi_ap_get_sta_list(wifi_sta_list_t* stations) {
  if (stations == nullptr) return ESP_ERR_INVALID_ARG;
  memset(stations, 0, sizeof(*stations));
  return ESP_OK;
}
esp_err_t esp_wifi_ftm_initiate_session(wifi_ftm_initiator_cfg_t*) {
  return ESP_ERR_NOT_SUPPORTED;
}
esp_err_t esp_wifi_ftm_end_session(void) { return ESP_OK; }
esp_err_t esp_wifi_set_band(wifi_band_t band) {
  return band == WIFI_BAND_2G ? ESP_OK : ESP_ERR_NOT_SUPPORTED;
}
esp_err_t esp_wifi_get_band(wifi_band_t* band) {
  if (band == nullptr) return ESP_ERR_INVALID_ARG;
  *band = WIFI_BAND_2G;
  return ESP_OK;
}
esp_err_t esp_wifi_set_band_mode(wifi_band_mode_t mode) {
  return mode == WIFI_BAND_MODE_2G_ONLY ? ESP_OK : ESP_ERR_NOT_SUPPORTED;
}
esp_err_t esp_wifi_get_band_mode(wifi_band_mode_t* mode) {
  if (mode == nullptr) return ESP_ERR_INVALID_ARG;
  *mode = WIFI_BAND_MODE_2G_ONLY;
  return ESP_OK;
}

esp_err_t esp_netif_init(void) { return ESP_OK; }
esp_err_t esp_netif_deinit(void) { return ESP_OK; }
esp_netif_t* esp_netif_new(const esp_netif_config_t* config) {
  const esp_netif_inherent_config_t* base =
      config == nullptr ? nullptr : config->base;
  return NewNetif(
      base != nullptr && base->if_key != nullptr ? base->if_key : "HOST_NETIF",
      base != nullptr && base->if_desc != nullptr
          ? base->if_desc
          : "host network interface",
      base != nullptr ? base->flags : ESP_NETIF_FLAG_AUTOUP,
      base != nullptr ? static_cast<int32_t>(base->get_ip_event) : 0,
      base != nullptr ? static_cast<int32_t>(base->lost_ip_event) : 0);
}
void esp_netif_destroy(esp_netif_t* netif) {
  if (netif == nullptr) return;
  g_netifs.erase(std::remove(g_netifs.begin(), g_netifs.end(), netif),
                 g_netifs.end());
  if (g_default_netif == netif) {
    g_default_netif = g_netifs.empty() ? nullptr : g_netifs.front();
  }
  delete netif;
}
esp_netif_t* esp_netif_create_default_wifi_sta(void) {
  return NewNetif("WIFI_STA_DEF", "sta", static_cast<esp_netif_flags_t>(
      ESP_NETIF_DHCP_CLIENT | ESP_NETIF_FLAG_AUTOUP),
      IP_EVENT_STA_GOT_IP, IP_EVENT_STA_LOST_IP);
}
esp_netif_t* esp_netif_create_default_wifi_ap(void) {
  return NewNetif("WIFI_AP_DEF", "ap", static_cast<esp_netif_flags_t>(
      ESP_NETIF_DHCP_SERVER | ESP_NETIF_FLAG_AUTOUP));
}
void esp_netif_destroy_default_wifi(void* netif) {
  esp_netif_destroy(static_cast<esp_netif_t*>(netif));
}
int32_t esp_netif_get_event_id(esp_netif_t* netif,
                               esp_netif_ip_event_type_t event_type) {
  if (netif == nullptr) return -1;
  switch (event_type) {
    case ESP_NETIF_IP_EVENT_GOT_IP:
      return netif->get_ip_event;
    case ESP_NETIF_IP_EVENT_LOST_IP:
      return netif->lost_ip_event;
    default:
      return -1;
  }
}
esp_err_t esp_netif_attach_wifi_station(esp_netif_t* netif) {
  return netif == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}
esp_err_t esp_netif_attach_wifi_ap(esp_netif_t* netif) {
  return netif == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}
esp_err_t esp_wifi_set_default_wifi_sta_handlers(void) { return ESP_OK; }
esp_err_t esp_wifi_set_default_wifi_ap_handlers(void) { return ESP_OK; }
esp_err_t esp_wifi_clear_default_wifi_driver_and_handlers(void*) {
  return ESP_OK;
}
esp_err_t esp_netif_set_default_netif(esp_netif_t* netif) {
  g_default_netif = netif;
  return ESP_OK;
}
esp_netif_t* esp_netif_get_default_netif(void) { return g_default_netif; }
bool esp_netif_is_netif_up(esp_netif_t* netif) {
  return netif != nullptr && netif->up;
}
esp_err_t esp_netif_set_hostname(esp_netif_t* netif, const char* hostname) {
  if (netif == nullptr || hostname == nullptr) return ESP_ERR_INVALID_ARG;
  netif->hostname = hostname;
  return ESP_OK;
}
esp_err_t esp_netif_get_hostname(esp_netif_t* netif, const char** hostname) {
  if (netif == nullptr || hostname == nullptr) return ESP_ERR_INVALID_ARG;
  *hostname = netif->hostname.c_str();
  return ESP_OK;
}
esp_err_t esp_netif_set_ip_info(esp_netif_t* netif,
                                const esp_netif_ip_info_t* info) {
  if (netif == nullptr || info == nullptr) return ESP_ERR_INVALID_ARG;
  netif->ip_info = *info;
  return ESP_OK;
}
esp_err_t esp_netif_get_ip_info(esp_netif_t* netif,
                                esp_netif_ip_info_t* info) {
  if (netif == nullptr || info == nullptr) return ESP_ERR_INVALID_ARG;
  *info = netif->ip_info;
  return ESP_OK;
}
esp_err_t esp_netif_set_mac(esp_netif_t* netif, uint8_t mac[]) {
  if (netif == nullptr || mac == nullptr) return ESP_ERR_INVALID_ARG;
  std::copy_n(mac, 6, netif->mac.begin());
  return ESP_OK;
}
esp_err_t esp_netif_get_mac(esp_netif_t* netif, uint8_t mac[]) {
  if (netif == nullptr || mac == nullptr) return ESP_ERR_INVALID_ARG;
  std::copy(netif->mac.begin(), netif->mac.end(), mac);
  return ESP_OK;
}
esp_err_t esp_netif_dhcpc_start(esp_netif_t* netif) {
  if (netif == nullptr) return ESP_ERR_INVALID_ARG;
  netif->dhcp_client = ESP_NETIF_DHCP_STARTED;
  return ESP_OK;
}
esp_err_t esp_netif_dhcpc_stop(esp_netif_t* netif) {
  if (netif == nullptr) return ESP_ERR_INVALID_ARG;
  netif->dhcp_client = ESP_NETIF_DHCP_STOPPED;
  return ESP_OK;
}
esp_err_t esp_netif_dhcpc_get_status(esp_netif_t* netif,
                                     esp_netif_dhcp_status_t* status) {
  if (netif == nullptr || status == nullptr) return ESP_ERR_INVALID_ARG;
  *status = netif->dhcp_client;
  return ESP_OK;
}
esp_err_t esp_netif_dhcps_start(esp_netif_t* netif) {
  if (netif == nullptr) return ESP_ERR_INVALID_ARG;
  netif->dhcp_server = ESP_NETIF_DHCP_STARTED;
  return ESP_OK;
}
esp_err_t esp_netif_dhcps_stop(esp_netif_t* netif) {
  if (netif == nullptr) return ESP_ERR_INVALID_ARG;
  netif->dhcp_server = ESP_NETIF_DHCP_STOPPED;
  return ESP_OK;
}
esp_err_t esp_netif_dhcps_get_status(esp_netif_t* netif,
                                     esp_netif_dhcp_status_t* status) {
  if (netif == nullptr || status == nullptr) return ESP_ERR_INVALID_ARG;
  *status = netif->dhcp_server;
  return ESP_OK;
}
esp_err_t esp_netif_dhcps_option(esp_netif_t* netif,
                                 esp_netif_dhcp_option_mode_t,
                                 esp_netif_dhcp_option_id_t, void*, uint32_t) {
  return netif == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}
esp_err_t esp_netif_set_dns_info(esp_netif_t* netif,
                                 esp_netif_dns_type_t type,
                                 esp_netif_dns_info_t* dns) {
  if (netif == nullptr || dns == nullptr || type >= ESP_NETIF_DNS_MAX) {
    return ESP_ERR_INVALID_ARG;
  }
  netif->dns[type] = *dns;
  return ESP_OK;
}
esp_err_t esp_netif_get_dns_info(esp_netif_t* netif,
                                 esp_netif_dns_type_t type,
                                 esp_netif_dns_info_t* dns) {
  if (netif == nullptr || dns == nullptr || type >= ESP_NETIF_DNS_MAX) {
    return ESP_ERR_INVALID_ARG;
  }
  *dns = netif->dns[type];
  return ESP_OK;
}
esp_err_t esp_netif_create_ip6_linklocal(esp_netif_t* netif) {
  return netif == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}
esp_err_t esp_netif_get_ip6_linklocal(esp_netif_t* netif,
                                      esp_ip6_addr_t* address) {
  if (netif == nullptr || address == nullptr) return ESP_ERR_INVALID_ARG;
  memset(address, 0, sizeof(*address));
  return ESP_OK;
}
esp_err_t esp_netif_get_ip6_global(esp_netif_t* netif,
                                   esp_ip6_addr_t* address) {
  return esp_netif_get_ip6_linklocal(netif, address);
}
int esp_netif_get_all_ip6(esp_netif_t*, esp_ip6_addr_t[]) { return 0; }
esp_ip6_addr_type_t esp_netif_ip6_get_addr_type(
    const esp_ip6_addr_t* address) {
  if (address == nullptr) return ESP_IP6_ADDR_IS_UNKNOWN;
  const auto* lwip_address = reinterpret_cast<const ip6_addr_t*>(address);
  if (ip6_addr_isglobal(lwip_address)) {
    return ESP_IP6_ADDR_IS_GLOBAL;
  }
  if (ip6_addr_islinklocal(lwip_address)) {
    return ESP_IP6_ADDR_IS_LINK_LOCAL;
  }
  if (ip6_addr_issitelocal(lwip_address)) {
    return ESP_IP6_ADDR_IS_SITE_LOCAL;
  }
  if (ip6_addr_isuniquelocal(lwip_address)) {
    return ESP_IP6_ADDR_IS_UNIQUE_LOCAL;
  }
  if (ip6_addr_isipv4mappedipv6(lwip_address)) {
    return ESP_IP6_ADDR_IS_IPV4_MAPPED_IPV6;
  }
  return ESP_IP6_ADDR_IS_UNKNOWN;
}
int esp_netif_get_netif_impl_index(esp_netif_t* netif) {
  if (netif == nullptr) return -1;
  const auto it = std::find(g_netifs.begin(), g_netifs.end(), netif);
  return it == g_netifs.end() ? -1
                             : static_cast<int>(it - g_netifs.begin()) + 1;
}
esp_err_t esp_netif_get_netif_impl_name(esp_netif_t* netif, char* name) {
  if (netif == nullptr || name == nullptr) return ESP_ERR_INVALID_ARG;
  strcpy(name, "lo");
  return ESP_OK;
}
const char* esp_netif_get_ifkey(esp_netif_t* netif) {
  return netif == nullptr ? nullptr : netif->key.c_str();
}
const char* esp_netif_get_desc(esp_netif_t* netif) {
  return netif == nullptr ? nullptr : netif->description.c_str();
}
esp_netif_t* esp_netif_get_handle_from_ifkey(const char* key) {
  if (key == nullptr) return nullptr;
  for (auto* netif : g_netifs) {
    if (netif->key == key) return netif;
  }
  return nullptr;
}
esp_netif_flags_t esp_netif_get_flags(esp_netif_t* netif) {
  return netif == nullptr ? static_cast<esp_netif_flags_t>(0) : netif->flags;
}
int esp_netif_get_route_prio(esp_netif_t* netif) {
  return netif == nullptr ? -1 : netif->route_priority;
}
int esp_netif_set_route_prio(esp_netif_t* netif, int priority) {
  if (netif == nullptr) return -1;
  netif->route_priority = priority;
  return 0;
}
esp_netif_t* esp_netif_next(esp_netif_t* current) {
  if (g_netifs.empty()) return nullptr;
  if (current == nullptr) return g_netifs.front();
  auto it = std::find(g_netifs.begin(), g_netifs.end(), current);
  if (it == g_netifs.end()) return nullptr;
  ++it;
  return it == g_netifs.end() ? nullptr : *it;
}
esp_err_t esp_netif_napt_enable(esp_netif_t* netif) {
  return netif == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}
esp_err_t esp_netif_napt_disable(esp_netif_t* netif) {
  return netif == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

}  // extern "C"
