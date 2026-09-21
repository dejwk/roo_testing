#include "esp_event.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_phy.h"
#include "esp_private/wifi_os_adapter.h"
#include "esp_smartconfig.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "lwip/ip6_addr.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <string>
#include <vector>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/frameworks/esp32_shims/wifi_host.h"

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
  bool up = false;
  esp_netif_dhcp_status_t dhcp_client = ESP_NETIF_DHCP_INIT;
  esp_netif_dhcp_status_t dhcp_server = ESP_NETIF_DHCP_INIT;
};

namespace {

using AccessPoint = roo_testing_transducers::wifi::AccessPoint;
using ConnectionAttempt = roo_testing_transducers::wifi::ConnectionAttempt;
using ConnectionOutcome = roo_testing_transducers::wifi::ConnectionOutcome;
using MacAddress = roo_testing_transducers::wifi::MacAddress;
using DriverState = roo_testing::esp32::wifi::DriverState;
using StationState = roo_testing::esp32::wifi::StationState;

std::mutex g_mutex;
DriverState g_driver_state = DriverState::kUninitialized;
StationState g_station_state = StationState::kDisabled;
wifi_mode_t g_mode = WIFI_MODE_NULL;
wifi_config_t g_station_config = {};
wifi_config_t g_ap_config = {};
bool g_smartconfig_started = false;
smartconfig_type_t g_smartconfig_type = SC_TYPE_ESPTOUCH;
esp_phy_ant_gpio_config_t g_ant_gpio_config = {};
esp_phy_ant_config_t g_ant_config = {};
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
std::vector<wifi_ap_record_t> g_scan_results;
std::vector<wifi_ap_record_t> g_pending_scan_results;
uint64_t g_scan_generation = 0;
bool g_scan_in_progress = false;
uint64_t g_connect_generation = 0;
std::optional<wifi_ap_record_t> g_connected_ap;
esp_netif_t *g_default_netif = nullptr;
esp_netif_t *g_station_netif = nullptr;
esp_netif_t *g_ap_netif = nullptr;

// Outlives Arduino global objects whose destructors release their netifs.
std::vector<esp_netif_t *> &Netifs() {
  static auto *netifs = new std::vector<esp_netif_t *>();
  return *netifs;
}

bool HasStation(wifi_mode_t mode) {
  return mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA;
}

bool HasAccessPoint(wifi_mode_t mode) {
  return mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA;
}

bool IsValidMode(wifi_mode_t mode) {
  return mode >= WIFI_MODE_NULL && mode < WIFI_MODE_MAX;
}

void ResetDriverLocked(DriverState driver_state) {
  ++g_scan_generation;
  ++g_connect_generation;
  g_driver_state = driver_state;
  g_station_state = driver_state == DriverState::kUninitialized
                        ? StationState::kDisabled
                        : StationState::kIdle;
  g_mode = driver_state == DriverState::kUninitialized ? WIFI_MODE_NULL
                                                        : WIFI_MODE_STA;
  g_station_config = {};
  g_ap_config = {};
  g_smartconfig_started = false;
  g_smartconfig_type = SC_TYPE_ESPTOUCH;
  g_power_save = WIFI_PS_MIN_MODEM;
  g_protocol = WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N;
  g_bandwidth = WIFI_BW20;
  g_channel = 1;
  g_max_tx_power = 78;
  g_country = {
      .cc = "01",
      .schan = 1,
      .nchan = 11,
      .max_tx_power = 20,
      .policy = WIFI_COUNTRY_POLICY_AUTO,
  };
  g_scan_results.clear();
  g_pending_scan_results.clear();
  g_scan_in_progress = false;
  g_connected_ap.reset();
  if (g_station_netif != nullptr) {
    g_station_netif->ip_info = {};
    g_station_netif->up = driver_state == DriverState::kStarted;
  }
  if (g_ap_netif != nullptr)
    g_ap_netif->up = false;
}

constexpr wifi_osi_funcs_t MakeHostWifiOsiFuncs() {
  wifi_osi_funcs_t funcs = {};
  funcs._version = ESP_WIFI_OS_ADAPTER_VERSION;
  funcs._magic = ESP_WIFI_OS_ADAPTER_MAGIC;
  return funcs;
}

constexpr wpa_crypto_funcs_t MakeHostWpaCryptoFuncs() {
  wpa_crypto_funcs_t funcs = {};
  funcs.size = sizeof(wpa_crypto_funcs_t);
  funcs.version = ESP_WIFI_CRYPTO_VERSION;
  return funcs;
}

void CopyMac(const MacAddress &source, uint8_t *destination) {
  for (size_t i = 0; i < 6; ++i)
    destination[i] = source.get(i);
}

// Reads a possibly unterminated fixed-width IDF byte-string field.
std::string ReadStringField(const uint8_t *source, size_t capacity) {
  return std::string(reinterpret_cast<const char *>(source),
                     strnlen(reinterpret_cast<const char *>(source), capacity));
}

// Copies a string into a fixed-width field and returns its represented length.
size_t CopyStringField(const std::string &source, uint8_t *destination,
                       size_t capacity) {
  const size_t size = std::min(source.size(), capacity);
  memcpy(destination, source.data(), size);
  if (size < capacity)
    destination[size] = '\0';
  return size;
}

wifi_auth_mode_t ToAuthMode(roo_testing_transducers::wifi::AuthMode auth_mode) {
  return static_cast<wifi_auth_mode_t>(auth_mode);
}

wifi_ap_record_t ToRecord(const AccessPoint &ap) {
  wifi_ap_record_t record = {};
  CopyMac(ap.macAddress(), record.bssid);
  CopyStringField(ap.ssid(), record.ssid, sizeof(record.ssid));
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

AccessPoint *FindConfiguredAccessPoint(
    const roo_testing_transducers::wifi::Environment &environment,
    const wifi_config_t &config) {
  const std::string ssid =
      ReadStringField(config.sta.ssid, sizeof(config.sta.ssid));
  AccessPoint *found = nullptr;
  for (const auto &entry : environment.access_points()) {
    AccessPoint *candidate = entry.second.get();
    if (candidate->ssid() != ssid)
      continue;
    if (config.sta.bssid_set &&
        MacAddress(config.sta.bssid) != candidate->macAddress()) {
      continue;
    }
    if (found == nullptr || candidate->rssi() > found->rssi())
      found = candidate;
  }
  return found;
}

void PostDisconnect(wifi_err_reason_t reason, const wifi_config_t &config) {
  wifi_event_sta_disconnected_t event = {};
  const std::string ssid =
      ReadStringField(config.sta.ssid, sizeof(config.sta.ssid));
  event.ssid_len = static_cast<uint8_t>(
      CopyStringField(ssid, event.ssid, sizeof(event.ssid)));
  event.reason = reason;
  esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &event, sizeof(event),
                 portMAX_DELAY);
}

void PostDisconnect(wifi_err_reason_t reason) {
  PostDisconnect(reason, g_station_config);
}

void PostLostIp() {
  ip_event_got_ip_t event = {};
  event.esp_netif = g_station_netif;
  esp_event_post(IP_EVENT, IP_EVENT_STA_LOST_IP, &event, sizeof(event),
                 portMAX_DELAY);
}

void PostLostIpIfCurrent(uint64_t generation) {
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (generation != g_connect_generation || g_station_netif == nullptr ||
        g_station_netif->ip_info.ip.addr != 0) {
      return;
    }
  }
  PostLostIp();
}

bool ClearStationIpLocked() {
  if (g_station_netif == nullptr || g_station_netif->ip_info.ip.addr == 0)
    return false;
  g_station_netif->ip_info = {};
  return true;
}

void AssignDhcpLeaseLocked() {
  g_station_netif->ip_info.ip.addr = 0x6401A8C0U;      // 192.168.1.100
  g_station_netif->ip_info.netmask.addr = 0x00FFFFFFU; // 255.255.255.0
  g_station_netif->ip_info.gw.addr = 0x0101A8C0U;      // 192.168.1.1
}

esp_netif_t *NewNetif(const char *key, const char *description,
                      esp_netif_flags_t flags, int32_t get_ip_event = 0,
                      int32_t lost_ip_event = 0) {
  auto *netif = new esp_netif_t();
  netif->key = key;
  netif->description = description;
  netif->hostname = "roo-testing";
  netif->flags = flags;
  netif->get_ip_event = get_ip_event;
  netif->lost_ip_event = lost_ip_event;
  esp_read_mac(netif->mac.data(), ESP_MAC_WIFI_STA);
  Netifs().push_back(netif);
  if (g_default_netif == nullptr)
    g_default_netif = netif;
  return netif;
}

struct PendingScanCompletion {
  uint64_t generation;
  uint32_t duration_ms;
};

struct PendingConnection {
  uint64_t generation;
  wifi_config_t config;
  std::shared_ptr<const roo_testing_transducers::wifi::Environment> environment;
  ConnectionAttempt attempt;
  bool lost_previous_ip;
};

bool IsConnectionCurrentLocked(uint64_t generation) {
  return generation == g_connect_generation &&
         g_driver_state == DriverState::kStarted && HasStation(g_mode) &&
         g_station_state != StationState::kIdle &&
         g_station_state != StationState::kDisabled;
}

bool DelayConnection(uint64_t generation, uint32_t delay_ms) {
  if (delay_ms != 0)
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
  std::lock_guard<std::mutex> lock(g_mutex);
  return IsConnectionCurrentLocked(generation);
}

wifi_err_reason_t ResolveAssociation(
    const roo_testing_transducers::wifi::Environment &environment,
    const wifi_config_t &config, ConnectionOutcome outcome, AccessPoint **ap) {
  *ap = FindConfiguredAccessPoint(environment, config);
  if (outcome == ConnectionOutcome::kNoAccessPoint || *ap == nullptr)
    return WIFI_REASON_NO_AP_FOUND;
  if (outcome == ConnectionOutcome::kAuthenticationFailure)
    return WIFI_REASON_AUTH_FAIL;
  if (outcome == ConnectionOutcome::kAutomatic) {
    const std::string password =
        ReadStringField(config.sta.password, sizeof(config.sta.password));
    if ((*ap)->passwd() != password)
      return WIFI_REASON_AUTH_FAIL;
  }
  return WIFI_REASON_UNSPECIFIED;
}

void PostConnected(const AccessPoint &ap) {
  wifi_event_sta_connected_t connected = {};
  CopyMac(ap.macAddress(), connected.bssid);
  connected.ssid_len = static_cast<uint8_t>(
      CopyStringField(ap.ssid(), connected.ssid, sizeof(connected.ssid)));
  connected.channel = static_cast<uint8_t>(ap.channel());
  connected.authmode = ToAuthMode(ap.auth_mode());
  esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &connected,
                 sizeof(connected), portMAX_DELAY);
}

void CompleteConnection(PendingConnection *pending) {
  ConnectionAttempt attempt = pending->attempt;
  const uint16_t attempt_count = pending->config.sta.failure_retry_cnt + 1;
  AccessPoint *ap = nullptr;
  wifi_err_reason_t failure = WIFI_REASON_UNSPECIFIED;

  for (uint16_t index = 0; index < attempt_count; ++index) {
    if (!DelayConnection(pending->generation, attempt.association_delay_ms))
      return;
    failure = ResolveAssociation(*pending->environment, pending->config,
                                 attempt.outcome, &ap);
    if (failure == WIFI_REASON_UNSPECIFIED)
      break;
    if (index + 1 < attempt_count)
      attempt = pending->environment->nextConnectionAttempt();
  }

  if (failure != WIFI_REASON_UNSPECIFIED) {
    {
      std::lock_guard<std::mutex> lock(g_mutex);
      if (!IsConnectionCurrentLocked(pending->generation))
        return;
      g_station_state = StationState::kIdle;
    }
    PostDisconnect(failure, pending->config);
    if (pending->lost_previous_ip)
      PostLostIpIfCurrent(pending->generation);
    return;
  }

  if (pending->lost_previous_ip)
    PostLostIpIfCurrent(pending->generation);
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!IsConnectionCurrentLocked(pending->generation))
      return;
    g_connected_ap = ToRecord(*ap);
    g_station_state = StationState::kAssociated;
  }
  PostConnected(*ap);

  if (attempt.outcome == ConnectionOutcome::kDhcpTimeout)
    return;
  if (!DelayConnection(pending->generation, attempt.dhcp_delay_ms))
    return;

  ip_event_got_ip_t got_ip = {};
  bool has_ip = false;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!IsConnectionCurrentLocked(pending->generation))
      return;
    // Arduino creates its AP netif before its station netif. Always deliver
    // the lease to the station that completed this connection attempt.
    got_ip.esp_netif = g_station_netif;
    if (g_station_netif != nullptr) {
      g_station_netif->up = true;
      if (g_station_netif->dhcp_client == ESP_NETIF_DHCP_STARTED &&
          g_station_netif->ip_info.ip.addr == 0) {
        AssignDhcpLeaseLocked();
      }
      got_ip.ip_info = g_station_netif->ip_info;
      has_ip = got_ip.ip_info.ip.addr != 0;
    }
    if (has_ip)
      g_station_state = StationState::kGotIp;
  }
  if (has_ip) {
    esp_event_post(IP_EVENT, IP_EVENT_STA_GOT_IP, &got_ip, sizeof(got_ip),
                   portMAX_DELAY);
  }

  if (attempt.outcome != ConnectionOutcome::kBeaconTimeout &&
      attempt.outcome != ConnectionOutcome::kRoaming) {
    return;
  }
  if (!DelayConnection(pending->generation, attempt.connected_duration_ms))
    return;

  bool lost_ip;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!IsConnectionCurrentLocked(pending->generation))
      return;
    g_connected_ap.reset();
    lost_ip = ClearStationIpLocked();
    g_station_state = StationState::kIdle;
  }
  PostDisconnect(attempt.outcome == ConnectionOutcome::kRoaming
                     ? WIFI_REASON_ROAMING
                     : WIFI_REASON_BEACON_TIMEOUT,
                 pending->config);
  if (lost_ip)
    PostLostIpIfCurrent(pending->generation);
}

void CompleteConnectionTask(void *arg) {
  std::unique_ptr<PendingConnection> pending(
      static_cast<PendingConnection *>(arg));
  CompleteConnection(pending.get());
  pending.reset();
  vTaskDelete(nullptr);
}

void CompleteAndPostScan(uint64_t generation) {
  uint16_t result_count;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (generation != g_scan_generation || !g_scan_in_progress)
      return;
    g_scan_results = std::move(g_pending_scan_results);
    g_pending_scan_results.clear();
    g_scan_in_progress = false;
    result_count = static_cast<uint16_t>(g_scan_results.size());
  }
  wifi_event_sta_scan_done_t event = {};
  event.status = 0;
  event.number = result_count;
  esp_event_post(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &event, sizeof(event),
                 portMAX_DELAY);
}

void CompleteScan(void *arg) {
  std::unique_ptr<PendingScanCompletion> completion(
      static_cast<PendingScanCompletion *>(arg));
  // Real asynchronous scans complete after esp_wifi_scan_start() returns.
  // Keep the scanning state visible long enough for UI examples to render it.
  vTaskDelay(pdMS_TO_TICKS(completion->duration_ms));
  CompleteAndPostScan(completion->generation);
  completion.reset();
  vTaskDelete(nullptr);
}

} // namespace

namespace roo_testing {
namespace esp32 {
namespace wifi {

void Reset() {
  std::lock_guard<std::mutex> lock(g_mutex);
  ResetDriverLocked(DriverState::kUninitialized);
  if (g_station_netif != nullptr) {
    g_station_netif->up = false;
    g_station_netif->ip_info = {};
  }
  if (g_ap_netif != nullptr)
    g_ap_netif->up = false;
}

StateSnapshot GetState() {
  std::lock_guard<std::mutex> lock(g_mutex);
  return {g_driver_state, g_station_state, g_mode};
}

}  // namespace wifi
}  // namespace esp32
}  // namespace roo_testing

extern "C" {

// WIFI_INIT_CONFIG_DEFAULT() embeds these ESP-IDF-owned adapter tables. The
// emulator handles Wi-Fi above that adapter boundary, so callbacks stay null;
// retaining the exact version, size, and magic fields keeps the public config
// contract valid without exposing target-only RTOS or crypto implementations.
wifi_osi_funcs_t g_wifi_osi_funcs = MakeHostWifiOsiFuncs();
const wpa_crypto_funcs_t g_wifi_default_wpa_crypto_funcs =
    MakeHostWpaCryptoFuncs();

esp_err_t esp_wifi_init(const wifi_init_config_t *config) {
  if (config == nullptr) return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_driver_state != DriverState::kUninitialized) {
    return ESP_ERR_WIFI_INIT_STATE;
  }
  ResetDriverLocked(DriverState::kStopped);
  return ESP_OK;
}

esp_err_t esp_wifi_deinit(void) {
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_driver_state == DriverState::kUninitialized) {
    return ESP_ERR_WIFI_NOT_INIT;
  }
  if (g_driver_state == DriverState::kStarted) {
    return ESP_ERR_WIFI_INIT_STATE;
  }
  ResetDriverLocked(DriverState::kUninitialized);
  return ESP_OK;
}

esp_err_t esp_wifi_set_mode(wifi_mode_t mode) {
  if (!IsValidMode(mode)) return ESP_ERR_INVALID_ARG;
  wifi_mode_t previous_mode;
  bool started;
  bool disconnected = false;
  bool lost_ip = false;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_driver_state == DriverState::kUninitialized) {
      return ESP_ERR_WIFI_NOT_INIT;
    }
    previous_mode = g_mode;
    started = g_driver_state == DriverState::kStarted;
    g_mode = mode;
    if (!HasStation(mode)) {
      ++g_scan_generation;
      ++g_connect_generation;
      g_scan_in_progress = false;
      g_pending_scan_results.clear();
      disconnected = g_connected_ap.has_value();
      g_connected_ap.reset();
      lost_ip = ClearStationIpLocked();
      if (g_station_netif != nullptr)
        g_station_netif->up = false;
      g_station_state = StationState::kDisabled;
    } else if (!HasStation(previous_mode)) {
      if (g_station_netif != nullptr)
        g_station_netif->up = started;
      g_station_state = StationState::kIdle;
    }
  }
  if (disconnected)
    PostDisconnect(WIFI_REASON_ASSOC_LEAVE);
  if (lost_ip)
    PostLostIp();
  if (started && HasStation(previous_mode) != HasStation(mode)) {
    esp_event_post(WIFI_EVENT, HasStation(mode) ? WIFI_EVENT_STA_START
                                                : WIFI_EVENT_STA_STOP,
                   nullptr, 0, portMAX_DELAY);
  }
  if (started && HasAccessPoint(previous_mode) != HasAccessPoint(mode)) {
    esp_event_post(WIFI_EVENT, HasAccessPoint(mode) ? WIFI_EVENT_AP_START
                                                    : WIFI_EVENT_AP_STOP,
                   nullptr, 0, portMAX_DELAY);
  }
  return ESP_OK;
}
esp_err_t esp_wifi_get_mode(wifi_mode_t *mode) {
  if (mode == nullptr)
    return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_driver_state == DriverState::kUninitialized) {
    return ESP_ERR_WIFI_NOT_INIT;
  }
  *mode = g_mode;
  return ESP_OK;
}

esp_err_t esp_wifi_start(void) {
  wifi_mode_t mode;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_driver_state == DriverState::kUninitialized) {
      return ESP_ERR_WIFI_NOT_INIT;
    }
    if (g_mode == WIFI_MODE_NULL) return ESP_ERR_WIFI_MODE;
    if (g_driver_state == DriverState::kStarted) return ESP_OK;
    g_driver_state = DriverState::kStarted;
    g_station_state = HasStation(g_mode) ? StationState::kIdle
                                         : StationState::kDisabled;
    if (HasStation(g_mode) && g_station_netif != nullptr)
      g_station_netif->up = true;
    if (HasAccessPoint(g_mode) && g_ap_netif != nullptr)
      g_ap_netif->up = true;
    mode = g_mode;
  }
  if (HasStation(mode)) {
    esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_START, nullptr, 0, portMAX_DELAY);
  }
  if (HasAccessPoint(mode)) {
    esp_event_post(WIFI_EVENT, WIFI_EVENT_AP_START, nullptr, 0, portMAX_DELAY);
  }
  return ESP_OK;
}

esp_err_t esp_wifi_stop(void) {
  wifi_mode_t mode;
  bool disconnected;
  bool lost_ip;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_driver_state == DriverState::kUninitialized) {
      return ESP_ERR_WIFI_NOT_INIT;
    }
    if (g_driver_state == DriverState::kStopped) return ESP_OK;
    mode = g_mode;
    g_driver_state = DriverState::kStopped;
    g_station_state = HasStation(mode) ? StationState::kIdle
                                       : StationState::kDisabled;
    ++g_scan_generation;
    ++g_connect_generation;
    g_scan_in_progress = false;
    g_pending_scan_results.clear();
    disconnected = g_connected_ap.has_value();
    g_connected_ap.reset();
    lost_ip = ClearStationIpLocked();
    if (g_station_netif != nullptr)
      g_station_netif->up = false;
    if (g_ap_netif != nullptr)
      g_ap_netif->up = false;
  }
  if (disconnected)
    PostDisconnect(WIFI_REASON_ASSOC_LEAVE);
  if (lost_ip)
    PostLostIp();
  if (HasStation(mode)) {
    esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_STOP, nullptr, 0, portMAX_DELAY);
  }
  if (HasAccessPoint(mode)) {
    esp_event_post(WIFI_EVENT, WIFI_EVENT_AP_STOP, nullptr, 0, portMAX_DELAY);
  }
  return ESP_OK;
}
esp_err_t esp_wifi_restore(void) {
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_driver_state == DriverState::kUninitialized) {
    return ESP_ERR_WIFI_NOT_INIT;
  }
  const DriverState state = g_driver_state;
  ResetDriverLocked(state);
  return ESP_OK;
}
esp_err_t esp_wifi_clear_fast_connect(void) { return ESP_OK; }

esp_err_t esp_wifi_connect(void) {
  std::unique_ptr<PendingConnection> pending(
      new (std::nothrow) PendingConnection());
  if (pending == nullptr)
    return ESP_ERR_NO_MEM;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_driver_state == DriverState::kUninitialized)
      return ESP_ERR_WIFI_NOT_INIT;
    if (g_driver_state != DriverState::kStarted)
      return ESP_ERR_WIFI_NOT_STARTED;
    if (!HasStation(g_mode)) return ESP_ERR_WIFI_MODE;
    if (g_scan_in_progress || g_station_state == StationState::kConnecting) {
      return ESP_ERR_WIFI_STATE;
    }
    if (g_station_config.sta.ssid[0] == '\0') return ESP_ERR_WIFI_SSID;
    g_connected_ap.reset();
    // A configured static address belongs to the interface, not the previous
    // association. Only discard an old DHCP lease when starting a connection.
    pending->lost_previous_ip =
        g_station_netif != nullptr &&
        g_station_netif->dhcp_client == ESP_NETIF_DHCP_STARTED &&
        ClearStationIpLocked();
    g_station_state = StationState::kConnecting;
    pending->generation = ++g_connect_generation;
    pending->config = g_station_config;
    pending->environment = FakeEsp32().acquireWifiEnvironment();
    pending->attempt = pending->environment->nextConnectionAttempt();
  }

  const bool asynchronous =
      pending->attempt.association_delay_ms != 0 ||
      pending->attempt.dhcp_delay_ms != 0 ||
      pending->attempt.connected_duration_ms != 0 ||
      pending->config.sta.failure_retry_cnt != 0 ||
      pending->attempt.outcome == ConnectionOutcome::kBeaconTimeout ||
      pending->attempt.outcome == ConnectionOutcome::kRoaming;
  if (!asynchronous) {
    CompleteConnection(pending.get());
    return ESP_OK;
  }
  if (xTaskCreate(CompleteConnectionTask, "wifi_connect", 4096, pending.get(),
                  tskIDLE_PRIORITY + 2, nullptr) != pdPASS) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (pending->generation == g_connect_generation)
      g_station_state = StationState::kIdle;
    return ESP_ERR_NO_MEM;
  }
  pending.release();
  return ESP_OK;
}

esp_err_t esp_wifi_disconnect(void) {
  bool connected;
  bool lost_ip;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_driver_state == DriverState::kUninitialized) {
      return ESP_ERR_WIFI_NOT_INIT;
    }
    if (g_driver_state != DriverState::kStarted) {
      return ESP_ERR_WIFI_NOT_STARTED;
    }
    if (!HasStation(g_mode)) return ESP_ERR_WIFI_MODE;
    ++g_connect_generation;
    connected = g_connected_ap.has_value();
    g_connected_ap.reset();
    lost_ip = ClearStationIpLocked();
    g_station_state = StationState::kIdle;
  }
  if (connected)
    PostDisconnect(WIFI_REASON_ASSOC_LEAVE);
  if (lost_ip)
    PostLostIp();
  return ESP_OK;
}

esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config, bool block) {
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_driver_state == DriverState::kUninitialized)
      return ESP_ERR_WIFI_NOT_INIT;
    if (g_driver_state != DriverState::kStarted)
      return ESP_ERR_WIFI_NOT_STARTED;
    if (g_mode != WIFI_MODE_STA && g_mode != WIFI_MODE_APSTA) {
      return ESP_ERR_WIFI_MODE;
    }
    if (g_station_state == StationState::kConnecting ||
        g_scan_in_progress)
      return ESP_ERR_WIFI_STATE;
  }
  std::vector<wifi_ap_record_t> results;
  const auto environment = FakeEsp32().acquireWifiEnvironment();
  for (const auto &entry : environment->access_points()) {
    const AccessPoint &ap = *entry.second;
    if (!ap.isVisible() && (config == nullptr || !config->show_hidden))
      continue;
    if (config != nullptr) {
      if (config->channel != 0 && config->channel != ap.channel())
        continue;
      if (config->ssid != nullptr &&
          strcmp(reinterpret_cast<const char *>(config->ssid),
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
  std::sort(results.begin(), results.end(),
            [](const wifi_ap_record_t &lhs, const wifi_ap_record_t &rhs) {
              if (lhs.rssi != rhs.rssi)
                return lhs.rssi > rhs.rssi;
              return std::lexicographical_compare(lhs.bssid, lhs.bssid + 6,
                                                  rhs.bssid, rhs.bssid + 6);
            });
  uint64_t generation;
  const uint32_t duration_ms = environment->scanDurationMs();
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_pending_scan_results = std::move(results);
    g_scan_in_progress = true;
    generation = ++g_scan_generation;
  }
  if (block) {
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    CompleteAndPostScan(generation);
  } else {
    auto completion = std::unique_ptr<PendingScanCompletion>(
        new (std::nothrow) PendingScanCompletion{generation, duration_ms});
    if (completion == nullptr) {
      std::lock_guard<std::mutex> lock(g_mutex);
      g_scan_in_progress = false;
      g_pending_scan_results.clear();
      return ESP_ERR_NO_MEM;
    }
    if (xTaskCreate(CompleteScan, "wifi_scan", 4096, completion.get(),
                    tskIDLE_PRIORITY + 2, nullptr) != pdPASS) {
      std::lock_guard<std::mutex> lock(g_mutex);
      g_scan_in_progress = false;
      g_pending_scan_results.clear();
      return ESP_ERR_NO_MEM;
    }
    completion.release();
  }
  return ESP_OK;
}

esp_err_t esp_wifi_scan_stop(void) {
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_driver_state == DriverState::kUninitialized)
    return ESP_ERR_WIFI_NOT_INIT;
  if (g_driver_state != DriverState::kStarted)
    return ESP_ERR_WIFI_NOT_STARTED;
  if (!g_scan_in_progress)
    return ESP_OK;
  ++g_scan_generation;
  g_scan_in_progress = false;
  g_pending_scan_results.clear();
  return ESP_OK;
}
esp_err_t esp_wifi_scan_get_ap_num(uint16_t *number) {
  if (number == nullptr)
    return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_driver_state == DriverState::kUninitialized)
    return ESP_ERR_WIFI_NOT_INIT;
  if (g_driver_state != DriverState::kStarted)
    return ESP_ERR_WIFI_NOT_STARTED;
  *number = static_cast<uint16_t>(g_scan_results.size());
  return ESP_OK;
}
esp_err_t esp_wifi_scan_get_ap_records(uint16_t *number,
                                       wifi_ap_record_t *records) {
  if (number == nullptr || (records == nullptr && *number != 0)) {
    return ESP_ERR_INVALID_ARG;
  }
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_driver_state == DriverState::kUninitialized)
    return ESP_ERR_WIFI_NOT_INIT;
  if (g_driver_state != DriverState::kStarted)
    return ESP_ERR_WIFI_NOT_STARTED;
  const size_t count = std::min<size_t>(*number, g_scan_results.size());
  std::copy_n(g_scan_results.begin(), count, records);
  *number = static_cast<uint16_t>(count);
  g_scan_results.clear();
  return ESP_OK;
}
esp_err_t esp_wifi_scan_get_ap_record(wifi_ap_record_t *record) {
  if (record == nullptr)
    return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_driver_state == DriverState::kUninitialized)
    return ESP_ERR_WIFI_NOT_INIT;
  if (g_driver_state != DriverState::kStarted)
    return ESP_ERR_WIFI_NOT_STARTED;
  if (g_scan_results.empty())
    return ESP_FAIL;
  *record = g_scan_results.front();
  g_scan_results.erase(g_scan_results.begin());
  return ESP_OK;
}
esp_err_t esp_wifi_clear_ap_list(void) {
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_driver_state == DriverState::kUninitialized)
    return ESP_ERR_WIFI_NOT_INIT;
  if (g_driver_state != DriverState::kStarted)
    return ESP_ERR_WIFI_NOT_STARTED;
  if (!HasStation(g_mode))
    return ESP_ERR_WIFI_MODE;
  g_scan_results.clear();
  return ESP_OK;
}

esp_err_t esp_wifi_set_config(wifi_interface_t interface,
                              wifi_config_t *config) {
  if (config == nullptr)
    return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_driver_state == DriverState::kUninitialized)
    return ESP_ERR_WIFI_NOT_INIT;
  if (interface == WIFI_IF_STA) {
    if (!HasStation(g_mode))
      return ESP_ERR_WIFI_MODE;
    if (g_station_state == StationState::kConnecting)
      return ESP_ERR_WIFI_STATE;
    g_station_config = *config;
  } else if (interface == WIFI_IF_AP) {
    if (!HasAccessPoint(g_mode))
      return ESP_ERR_WIFI_MODE;
    g_ap_config = *config;
  } else {
    return ESP_ERR_WIFI_IF;
  }
  return ESP_OK;
}
esp_err_t esp_wifi_get_config(wifi_interface_t interface,
                              wifi_config_t *config) {
  if (config == nullptr)
    return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_driver_state == DriverState::kUninitialized)
    return ESP_ERR_WIFI_NOT_INIT;
  if (interface == WIFI_IF_STA) {
    *config = g_station_config;
  } else if (interface == WIFI_IF_AP) {
    *config = g_ap_config;
  } else {
    return ESP_ERR_WIFI_IF;
  }
  return ESP_OK;
}

esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *info) {
  if (info == nullptr)
    return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_driver_state != DriverState::kStarted || !HasStation(g_mode))
    return ESP_ERR_WIFI_CONN;
  if (!g_connected_ap.has_value())
    return ESP_ERR_WIFI_NOT_CONNECT;
  *info = *g_connected_ap;
  return ESP_OK;
}
esp_err_t esp_wifi_sta_get_rssi(int *rssi) {
  if (rssi == nullptr)
    return ESP_ERR_INVALID_ARG;
  wifi_ap_record_t record;
  const esp_err_t result = esp_wifi_sta_get_ap_info(&record);
  if (result == ESP_OK)
    *rssi = record.rssi;
  return result;
}

esp_err_t esp_wifi_set_storage(wifi_storage_t) { return ESP_OK; }
esp_err_t esp_wifi_set_ps(wifi_ps_type_t power_save) {
  g_power_save = power_save;
  return ESP_OK;
}
esp_err_t esp_wifi_get_ps(wifi_ps_type_t *power_save) {
  if (power_save == nullptr)
    return ESP_ERR_INVALID_ARG;
  *power_save = g_power_save;
  return ESP_OK;
}
esp_err_t esp_wifi_set_protocol(wifi_interface_t, uint8_t protocol) {
  g_protocol = protocol;
  return ESP_OK;
}
esp_err_t esp_wifi_get_protocol(wifi_interface_t, uint8_t *protocol) {
  if (protocol == nullptr)
    return ESP_ERR_INVALID_ARG;
  *protocol = g_protocol;
  return ESP_OK;
}
esp_err_t esp_wifi_set_protocols(wifi_interface_t,
                                 wifi_protocols_t *protocols) {
  if (protocols == nullptr)
    return ESP_ERR_INVALID_ARG;
  g_protocol = static_cast<uint8_t>(protocols->ghz_2g);
  return ESP_OK;
}
esp_err_t esp_wifi_get_protocols(wifi_interface_t,
                                 wifi_protocols_t *protocols) {
  if (protocols == nullptr)
    return ESP_ERR_INVALID_ARG;
  protocols->ghz_2g = g_protocol;
  protocols->ghz_5g = 0;
  return ESP_OK;
}
esp_err_t esp_wifi_set_bandwidth(wifi_interface_t, wifi_bandwidth_t bandwidth) {
  g_bandwidth = bandwidth;
  return ESP_OK;
}
esp_err_t esp_wifi_get_bandwidth(wifi_interface_t,
                                 wifi_bandwidth_t *bandwidth) {
  if (bandwidth == nullptr)
    return ESP_ERR_INVALID_ARG;
  *bandwidth = g_bandwidth;
  return ESP_OK;
}
esp_err_t esp_wifi_set_channel(uint8_t primary, wifi_second_chan_t) {
  g_channel = primary;
  return ESP_OK;
}
esp_err_t esp_wifi_get_channel(uint8_t *primary, wifi_second_chan_t *second) {
  if (primary == nullptr || second == nullptr)
    return ESP_ERR_INVALID_ARG;
  *primary = g_channel;
  *second = WIFI_SECOND_CHAN_NONE;
  return ESP_OK;
}
esp_err_t esp_wifi_set_country(const wifi_country_t *country) {
  if (country == nullptr)
    return ESP_ERR_INVALID_ARG;
  g_country = *country;
  return ESP_OK;
}
esp_err_t esp_wifi_get_country(wifi_country_t *country) {
  if (country == nullptr)
    return ESP_ERR_INVALID_ARG;
  *country = g_country;
  return ESP_OK;
}
esp_err_t esp_wifi_set_country_code(const char *country, bool) {
  if (country == nullptr)
    return ESP_ERR_INVALID_ARG;
  strncpy(g_country.cc, country, sizeof(g_country.cc));
  return ESP_OK;
}
esp_err_t esp_wifi_get_country_code(char *country) {
  if (country == nullptr)
    return ESP_ERR_INVALID_ARG;
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
esp_err_t esp_wifi_get_max_tx_power(int8_t *power) {
  if (power == nullptr)
    return ESP_ERR_INVALID_ARG;
  *power = g_max_tx_power;
  return ESP_OK;
}
esp_err_t esp_wifi_ap_get_sta_list(wifi_sta_list_t *stations) {
  if (stations == nullptr)
    return ESP_ERR_INVALID_ARG;
  memset(stations, 0, sizeof(*stations));
  return ESP_OK;
}
esp_err_t esp_wifi_ftm_initiate_session(wifi_ftm_initiator_cfg_t *) {
  return ESP_ERR_NOT_SUPPORTED;
}
esp_err_t esp_wifi_ftm_end_session(void) { return ESP_OK; }
esp_err_t esp_wifi_set_band(wifi_band_t band) {
  return band == WIFI_BAND_2G ? ESP_OK : ESP_ERR_NOT_SUPPORTED;
}
esp_err_t esp_wifi_get_band(wifi_band_t *band) {
  if (band == nullptr)
    return ESP_ERR_INVALID_ARG;
  *band = WIFI_BAND_2G;
  return ESP_OK;
}
esp_err_t esp_wifi_set_band_mode(wifi_band_mode_t mode) {
  return mode == WIFI_BAND_MODE_2G_ONLY ? ESP_OK : ESP_ERR_NOT_SUPPORTED;
}
esp_err_t esp_wifi_get_band_mode(wifi_band_mode_t *mode) {
  if (mode == nullptr)
    return ESP_ERR_INVALID_ARG;
  *mode = WIFI_BAND_MODE_2G_ONLY;
  return ESP_OK;
}

esp_err_t esp_smartconfig_set_type(smartconfig_type_t type) {
  if (type < SC_TYPE_ESPTOUCH || type > SC_TYPE_ESPTOUCH_V2) {
    return ESP_ERR_INVALID_ARG;
  }
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_smartconfig_started)
    return ESP_ERR_INVALID_STATE;
  g_smartconfig_type = type;
  return ESP_OK;
}

esp_err_t esp_smartconfig_start(const smartconfig_start_config_t *config) {
  if (config == nullptr || (config->esp_touch_v2_enable_crypt &&
                            config->esp_touch_v2_key == nullptr)) {
    return ESP_ERR_INVALID_ARG;
  }
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_smartconfig_started)
    return ESP_ERR_INVALID_STATE;
  g_smartconfig_started = true;
  return ESP_OK;
}

esp_err_t esp_smartconfig_stop(void) {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_smartconfig_started = false;
  return ESP_OK;
}

esp_err_t esp_phy_set_ant_gpio(esp_phy_ant_gpio_config_t *config) {
  if (config == nullptr)
    return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  g_ant_gpio_config = *config;
  return ESP_OK;
}

esp_err_t esp_phy_get_ant_gpio(esp_phy_ant_gpio_config_t *config) {
  if (config == nullptr)
    return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  *config = g_ant_gpio_config;
  return ESP_OK;
}

esp_err_t esp_phy_set_ant(esp_phy_ant_config_t *config) {
  if (config == nullptr)
    return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  g_ant_config = *config;
  return ESP_OK;
}

esp_err_t esp_phy_get_ant(esp_phy_ant_config_t *config) {
  if (config == nullptr)
    return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  *config = g_ant_config;
  return ESP_OK;
}

esp_err_t esp_netif_init(void) { return ESP_OK; }
esp_err_t esp_netif_deinit(void) { return ESP_OK; }
esp_netif_t *esp_netif_new(const esp_netif_config_t *config) {
  const esp_netif_inherent_config_t *base =
      config == nullptr ? nullptr : config->base;
  return NewNetif(
      base != nullptr && base->if_key != nullptr ? base->if_key : "HOST_NETIF",
      base != nullptr && base->if_desc != nullptr ? base->if_desc
                                                  : "host network interface",
      base != nullptr ? base->flags : ESP_NETIF_FLAG_AUTOUP,
      base != nullptr ? static_cast<int32_t>(base->get_ip_event) : 0,
      base != nullptr ? static_cast<int32_t>(base->lost_ip_event) : 0);
}
void esp_netif_destroy(esp_netif_t *netif) {
  if (netif == nullptr)
    return;
  auto &netifs = Netifs();
  netifs.erase(std::remove(netifs.begin(), netifs.end(), netif), netifs.end());
  if (g_default_netif == netif) {
    g_default_netif = netifs.empty() ? nullptr : netifs.front();
  }
  if (g_station_netif == netif)
    g_station_netif = nullptr;
  if (g_ap_netif == netif)
    g_ap_netif = nullptr;
  delete netif;
}
esp_netif_t *esp_netif_create_default_wifi_sta(void) {
  g_station_netif = NewNetif("WIFI_STA_DEF", "sta",
                             static_cast<esp_netif_flags_t>(
                                 ESP_NETIF_DHCP_CLIENT | ESP_NETIF_FLAG_AUTOUP),
                             IP_EVENT_STA_GOT_IP, IP_EVENT_STA_LOST_IP);
  g_station_netif->dhcp_client = ESP_NETIF_DHCP_STARTED;
  g_station_netif->up = g_driver_state == DriverState::kStarted &&
                        HasStation(g_mode);
  return g_station_netif;
}
esp_netif_t *esp_netif_create_default_wifi_ap(void) {
  g_ap_netif = NewNetif("WIFI_AP_DEF", "ap",
                        static_cast<esp_netif_flags_t>(ESP_NETIF_DHCP_SERVER |
                                                       ESP_NETIF_FLAG_AUTOUP));
  g_ap_netif->dhcp_server = ESP_NETIF_DHCP_STARTED;
  g_ap_netif->ip_info.ip.addr = 0x0104A8C0U;      // 192.168.4.1
  g_ap_netif->ip_info.netmask.addr = 0x00FFFFFFU; // 255.255.255.0
  g_ap_netif->ip_info.gw.addr = 0x0104A8C0U;      // 192.168.4.1
  g_ap_netif->up = g_driver_state == DriverState::kStarted &&
                   HasAccessPoint(g_mode);
  return g_ap_netif;
}
void esp_netif_destroy_default_wifi(void *netif) {
  esp_netif_destroy(static_cast<esp_netif_t *>(netif));
}
int32_t esp_netif_get_event_id(esp_netif_t *netif,
                               esp_netif_ip_event_type_t event_type) {
  if (netif == nullptr)
    return -1;
  switch (event_type) {
  case ESP_NETIF_IP_EVENT_GOT_IP:
    return netif->get_ip_event;
  case ESP_NETIF_IP_EVENT_LOST_IP:
    return netif->lost_ip_event;
  default:
    return -1;
  }
}
esp_err_t esp_netif_attach_wifi_station(esp_netif_t *netif) {
  return netif == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}
esp_err_t esp_netif_attach_wifi_ap(esp_netif_t *netif) {
  return netif == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}
esp_err_t esp_wifi_set_default_wifi_sta_handlers(void) { return ESP_OK; }
esp_err_t esp_wifi_set_default_wifi_ap_handlers(void) { return ESP_OK; }
esp_err_t esp_wifi_clear_default_wifi_driver_and_handlers(void *) {
  return ESP_OK;
}
esp_err_t esp_netif_set_default_netif(esp_netif_t *netif) {
  g_default_netif = netif;
  return ESP_OK;
}
esp_netif_t *esp_netif_get_default_netif(void) { return g_default_netif; }
bool esp_netif_is_netif_up(esp_netif_t *netif) {
  return netif != nullptr && netif->up;
}
esp_err_t esp_netif_set_hostname(esp_netif_t *netif, const char *hostname) {
  if (netif == nullptr || hostname == nullptr)
    return ESP_ERR_INVALID_ARG;
  netif->hostname = hostname;
  return ESP_OK;
}
esp_err_t esp_netif_get_hostname(esp_netif_t *netif, const char **hostname) {
  if (netif == nullptr || hostname == nullptr)
    return ESP_ERR_INVALID_ARG;
  *hostname = netif->hostname.c_str();
  return ESP_OK;
}
esp_err_t esp_netif_set_ip_info(esp_netif_t *netif,
                                const esp_netif_ip_info_t *info) {
  if (netif == nullptr || info == nullptr)
    return ESP_ERR_INVALID_ARG;
  netif->ip_info = *info;
  return ESP_OK;
}
esp_err_t esp_netif_get_ip_info(esp_netif_t *netif, esp_netif_ip_info_t *info) {
  if (netif == nullptr || info == nullptr)
    return ESP_ERR_INVALID_ARG;
  *info = netif->ip_info;
  return ESP_OK;
}
esp_err_t esp_netif_set_mac(esp_netif_t *netif, uint8_t mac[]) {
  if (netif == nullptr || mac == nullptr)
    return ESP_ERR_INVALID_ARG;
  std::copy_n(mac, 6, netif->mac.begin());
  return ESP_OK;
}
esp_err_t esp_netif_get_mac(esp_netif_t *netif, uint8_t mac[]) {
  if (netif == nullptr || mac == nullptr)
    return ESP_ERR_INVALID_ARG;
  std::copy(netif->mac.begin(), netif->mac.end(), mac);
  return ESP_OK;
}
esp_err_t esp_netif_dhcpc_start(esp_netif_t *netif) {
  if (netif == nullptr)
    return ESP_ERR_INVALID_ARG;
  netif->dhcp_client = ESP_NETIF_DHCP_STARTED;
  return ESP_OK;
}
esp_err_t esp_netif_dhcpc_stop(esp_netif_t *netif) {
  if (netif == nullptr)
    return ESP_ERR_INVALID_ARG;
  netif->dhcp_client = ESP_NETIF_DHCP_STOPPED;
  return ESP_OK;
}
esp_err_t esp_netif_dhcpc_get_status(esp_netif_t *netif,
                                     esp_netif_dhcp_status_t *status) {
  if (netif == nullptr || status == nullptr)
    return ESP_ERR_INVALID_ARG;
  *status = netif->dhcp_client;
  return ESP_OK;
}
esp_err_t esp_netif_dhcps_start(esp_netif_t *netif) {
  if (netif == nullptr)
    return ESP_ERR_INVALID_ARG;
  netif->dhcp_server = ESP_NETIF_DHCP_STARTED;
  return ESP_OK;
}
esp_err_t esp_netif_dhcps_stop(esp_netif_t *netif) {
  if (netif == nullptr)
    return ESP_ERR_INVALID_ARG;
  netif->dhcp_server = ESP_NETIF_DHCP_STOPPED;
  return ESP_OK;
}
esp_err_t esp_netif_dhcps_get_status(esp_netif_t *netif,
                                     esp_netif_dhcp_status_t *status) {
  if (netif == nullptr || status == nullptr)
    return ESP_ERR_INVALID_ARG;
  *status = netif->dhcp_server;
  return ESP_OK;
}
esp_err_t esp_netif_dhcps_option(esp_netif_t *netif,
                                 esp_netif_dhcp_option_mode_t,
                                 esp_netif_dhcp_option_id_t, void *, uint32_t) {
  return netif == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}
esp_err_t esp_netif_set_dns_info(esp_netif_t *netif, esp_netif_dns_type_t type,
                                 esp_netif_dns_info_t *dns) {
  if (netif == nullptr || dns == nullptr || type >= ESP_NETIF_DNS_MAX) {
    return ESP_ERR_INVALID_ARG;
  }
  netif->dns[type] = *dns;
  return ESP_OK;
}
esp_err_t esp_netif_get_dns_info(esp_netif_t *netif, esp_netif_dns_type_t type,
                                 esp_netif_dns_info_t *dns) {
  if (netif == nullptr || dns == nullptr || type >= ESP_NETIF_DNS_MAX) {
    return ESP_ERR_INVALID_ARG;
  }
  *dns = netif->dns[type];
  return ESP_OK;
}
esp_err_t esp_netif_create_ip6_linklocal(esp_netif_t *netif) {
  return netif == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}
esp_err_t esp_netif_get_ip6_linklocal(esp_netif_t *netif,
                                      esp_ip6_addr_t *address) {
  if (netif == nullptr || address == nullptr)
    return ESP_ERR_INVALID_ARG;
  memset(address, 0, sizeof(*address));
  return ESP_OK;
}
esp_err_t esp_netif_get_ip6_global(esp_netif_t *netif,
                                   esp_ip6_addr_t *address) {
  return esp_netif_get_ip6_linklocal(netif, address);
}
int esp_netif_get_all_ip6(esp_netif_t *, esp_ip6_addr_t[]) { return 0; }
esp_ip6_addr_type_t esp_netif_ip6_get_addr_type(const esp_ip6_addr_t *address) {
  if (address == nullptr)
    return ESP_IP6_ADDR_IS_UNKNOWN;
  const auto *lwip_address = reinterpret_cast<const ip6_addr_t *>(address);
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
int esp_netif_get_netif_impl_index(esp_netif_t *netif) {
  if (netif == nullptr)
    return -1;
  const auto &netifs = Netifs();
  const auto it = std::find(netifs.begin(), netifs.end(), netif);
  return it == netifs.end() ? -1
                            : static_cast<int>(it - netifs.begin()) + 1;
}
esp_err_t esp_netif_get_netif_impl_name(esp_netif_t *netif, char *name) {
  if (netif == nullptr || name == nullptr)
    return ESP_ERR_INVALID_ARG;
  strcpy(name, "lo");
  return ESP_OK;
}
const char *esp_netif_get_ifkey(esp_netif_t *netif) {
  return netif == nullptr ? nullptr : netif->key.c_str();
}
const char *esp_netif_get_desc(esp_netif_t *netif) {
  return netif == nullptr ? nullptr : netif->description.c_str();
}
esp_netif_t *esp_netif_get_handle_from_ifkey(const char *key) {
  if (key == nullptr)
    return nullptr;
  for (auto *netif : Netifs()) {
    if (netif->key == key)
      return netif;
  }
  return nullptr;
}
esp_netif_flags_t esp_netif_get_flags(esp_netif_t *netif) {
  return netif == nullptr ? static_cast<esp_netif_flags_t>(0) : netif->flags;
}
int esp_netif_get_route_prio(esp_netif_t *netif) {
  return netif == nullptr ? -1 : netif->route_priority;
}
int esp_netif_set_route_prio(esp_netif_t *netif, int priority) {
  if (netif == nullptr)
    return -1;
  netif->route_priority = priority;
  return 0;
}
esp_netif_t *esp_netif_next(esp_netif_t *current) {
  auto &netifs = Netifs();
  if (netifs.empty())
    return nullptr;
  if (current == nullptr)
    return netifs.front();
  auto it = std::find(netifs.begin(), netifs.end(), current);
  if (it == netifs.end())
    return nullptr;
  ++it;
  return it == netifs.end() ? nullptr : *it;
}
esp_err_t esp_netif_napt_enable(esp_netif_t *netif) {
  return netif == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}
esp_err_t esp_netif_napt_disable(esp_netif_t *netif) {
  return netif == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

} // extern "C"
