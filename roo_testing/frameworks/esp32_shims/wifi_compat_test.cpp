#include <initializer_list>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_phy.h"
#include "esp_private/wifi_os_adapter.h"
#include "esp_smartconfig.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/frameworks/esp32_shims/wifi_host.h"
#include "roo_testing/transducers/wifi/wifi.h"
#include "gtest/gtest.h"

static_assert(std::is_same_v<decltype(g_wifi_osi_funcs), wifi_osi_funcs_t>);
static_assert(std::is_same_v<decltype(g_wifi_default_wpa_crypto_funcs),
                             const wpa_crypto_funcs_t>);
static_assert(
    std::is_same_v<decltype(&esp_smartconfig_start),
                   esp_err_t (*)(const smartconfig_start_config_t *)>);
static_assert(std::is_same_v<decltype(&esp_phy_set_ant_gpio),
                             esp_err_t (*)(esp_phy_ant_gpio_config_t *)>);
static_assert(std::is_same_v<decltype(&esp_phy_set_ant),
                             esp_err_t (*)(esp_phy_ant_config_t *)>);

namespace {

struct StationEventCapture {
  wifi_event_sta_connected_t connected = {};
  wifi_event_sta_disconnected_t disconnected = {};
  SemaphoreHandle_t event_received = nullptr;
};

void CaptureStationEvent(void *arg, esp_event_base_t, int32_t event_id,
                         void *event_data) {
  auto *capture = static_cast<StationEventCapture *>(arg);
  if (event_id == WIFI_EVENT_STA_CONNECTED) {
    capture->connected = *static_cast<wifi_event_sta_connected_t *>(event_data);
  } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
    capture->disconnected =
        *static_cast<wifi_event_sta_disconnected_t *>(event_data);
  } else {
    return;
  }
  xSemaphoreGive(capture->event_received);
}

struct NetworkEventCapture {
  SemaphoreHandle_t connected = nullptr;
  SemaphoreHandle_t disconnected = nullptr;
  SemaphoreHandle_t got_ip = nullptr;
  SemaphoreHandle_t lost_ip = nullptr;
  SemaphoreHandle_t scan_done = nullptr;
};

void CaptureNetworkEvent(void *arg, esp_event_base_t event_base,
                         int32_t event_id, void *) {
  auto *capture = static_cast<NetworkEventCapture *>(arg);
  SemaphoreHandle_t semaphore = nullptr;
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
    semaphore = capture->connected;
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    semaphore = capture->disconnected;
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
    semaphore = capture->scan_done;
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    semaphore = capture->got_ip;
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
    semaphore = capture->lost_ip;
  }
  if (semaphore != nullptr)
    xSemaphoreGive(semaphore);
}

enum class OrderedEvent {
  kStaStart,
  kStaStop,
  kApStart,
  kConnected,
  kDisconnected,
  kGotIp,
  kLostIp,
};

struct OrderedEventCapture {
  std::mutex mutex;
  std::vector<OrderedEvent> events;
  SemaphoreHandle_t delivered = nullptr;
};

void CaptureOrderedEvent(void *arg, esp_event_base_t event_base,
                         int32_t event_id, void *) {
  auto *capture = static_cast<OrderedEventCapture *>(arg);
  OrderedEvent event;
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    event = OrderedEvent::kStaStart;
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_STOP) {
    event = OrderedEvent::kStaStop;
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
    event = OrderedEvent::kApStart;
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
    event = OrderedEvent::kConnected;
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    event = OrderedEvent::kDisconnected;
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    event = OrderedEvent::kGotIp;
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
    event = OrderedEvent::kLostIp;
  } else {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(capture->mutex);
    capture->events.push_back(event);
  }
  xSemaphoreGive(capture->delivered);
}

// Waits for and compares one complete event transition.
void ExpectOrderedEvents(OrderedEventCapture &capture,
                         std::initializer_list<OrderedEvent> expected) {
  for (size_t i = 0; i < expected.size(); ++i) {
    ASSERT_EQ(xSemaphoreTake(capture.delivered, pdMS_TO_TICKS(1000)), pdTRUE);
  }
  std::lock_guard<std::mutex> lock(capture.mutex);
  EXPECT_EQ(capture.events,
            std::vector<OrderedEvent>(expected.begin(), expected.end()));
  capture.events.clear();
}

TEST(WifiCompatTest, DriverLifecycleIsExplicitAndResettable) {
  using roo_testing::esp32::wifi::DriverState;
  using roo_testing::esp32::wifi::StationState;

  roo_testing::esp32::wifi::Reset();
  wifi_mode_t mode = WIFI_MODE_MAX;
  EXPECT_EQ(esp_wifi_get_mode(&mode), ESP_ERR_WIFI_NOT_INIT);

  wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
  ASSERT_EQ(esp_wifi_init(&config), ESP_OK);
  EXPECT_EQ(esp_wifi_init(&config), ESP_ERR_WIFI_INIT_STATE);
  auto state = roo_testing::esp32::wifi::GetState();
  EXPECT_EQ(state.driver, DriverState::kStopped);
  EXPECT_EQ(state.station, StationState::kIdle);
  EXPECT_EQ(state.mode, WIFI_MODE_STA);

  ASSERT_EQ(esp_wifi_start(), ESP_OK);
  state = roo_testing::esp32::wifi::GetState();
  EXPECT_EQ(state.driver, DriverState::kStarted);
  EXPECT_EQ(state.station, StationState::kIdle);

  ASSERT_EQ(esp_wifi_set_mode(WIFI_MODE_AP), ESP_OK);
  state = roo_testing::esp32::wifi::GetState();
  EXPECT_EQ(state.station, StationState::kDisabled);
  EXPECT_EQ(esp_wifi_connect(), ESP_ERR_WIFI_MODE);

  EXPECT_EQ(esp_wifi_stop(), ESP_OK);
  EXPECT_EQ(esp_wifi_deinit(), ESP_OK);
  state = roo_testing::esp32::wifi::GetState();
  EXPECT_EQ(state.driver, DriverState::kUninitialized);
  EXPECT_EQ(state.station, StationState::kDisabled);
  EXPECT_EQ(state.mode, WIFI_MODE_NULL);
}

// Verifies core APIs reject calls in invalid lifecycle and interface states.
TEST(WifiCompatTest, RejectsInvalidLifecycleAndModeCalls) {
  roo_testing::esp32::wifi::Reset();
  wifi_config_t wifi_config = {};
  wifi_ap_record_t ap_record = {};
  uint16_t ap_count = 0;

  EXPECT_EQ(esp_wifi_connect(), ESP_ERR_WIFI_NOT_INIT);
  EXPECT_EQ(esp_wifi_disconnect(), ESP_ERR_WIFI_NOT_INIT);
  EXPECT_EQ(esp_wifi_scan_start(nullptr, false), ESP_ERR_WIFI_NOT_INIT);
  EXPECT_EQ(esp_wifi_scan_stop(), ESP_ERR_WIFI_NOT_INIT);
  EXPECT_EQ(esp_wifi_scan_get_ap_num(&ap_count), ESP_ERR_WIFI_NOT_INIT);
  EXPECT_EQ(esp_wifi_scan_get_ap_records(&ap_count, nullptr),
            ESP_ERR_WIFI_NOT_INIT);
  EXPECT_EQ(esp_wifi_scan_get_ap_record(&ap_record), ESP_ERR_WIFI_NOT_INIT);
  EXPECT_EQ(esp_wifi_clear_ap_list(), ESP_ERR_WIFI_NOT_INIT);
  EXPECT_EQ(esp_wifi_set_config(WIFI_IF_STA, &wifi_config),
            ESP_ERR_WIFI_NOT_INIT);
  EXPECT_EQ(esp_wifi_get_config(WIFI_IF_STA, &wifi_config),
            ESP_ERR_WIFI_NOT_INIT);
  EXPECT_EQ(esp_wifi_sta_get_ap_info(&ap_record), ESP_ERR_WIFI_CONN);

  wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
  ASSERT_EQ(esp_wifi_init(&init_config), ESP_OK);
  EXPECT_EQ(esp_wifi_connect(), ESP_ERR_WIFI_NOT_STARTED);
  EXPECT_EQ(esp_wifi_disconnect(), ESP_ERR_WIFI_NOT_STARTED);
  EXPECT_EQ(esp_wifi_scan_start(nullptr, false), ESP_ERR_WIFI_NOT_STARTED);
  EXPECT_EQ(esp_wifi_scan_stop(), ESP_ERR_WIFI_NOT_STARTED);
  EXPECT_EQ(esp_wifi_scan_get_ap_num(&ap_count), ESP_ERR_WIFI_NOT_STARTED);
  EXPECT_EQ(esp_wifi_scan_get_ap_records(&ap_count, nullptr),
            ESP_ERR_WIFI_NOT_STARTED);
  EXPECT_EQ(esp_wifi_scan_get_ap_record(&ap_record),
            ESP_ERR_WIFI_NOT_STARTED);
  EXPECT_EQ(esp_wifi_clear_ap_list(), ESP_ERR_WIFI_NOT_STARTED);
  EXPECT_EQ(esp_wifi_set_config(WIFI_IF_AP, &wifi_config),
            ESP_ERR_WIFI_MODE);
  EXPECT_EQ(esp_wifi_set_config(static_cast<wifi_interface_t>(WIFI_IF_MAX),
                                &wifi_config),
            ESP_ERR_WIFI_IF);
  EXPECT_EQ(esp_wifi_get_config(static_cast<wifi_interface_t>(WIFI_IF_MAX),
                                &wifi_config),
            ESP_ERR_WIFI_IF);

  ASSERT_EQ(esp_wifi_set_mode(WIFI_MODE_AP), ESP_OK);
  ASSERT_EQ(esp_wifi_start(), ESP_OK);
  EXPECT_EQ(esp_wifi_connect(), ESP_ERR_WIFI_MODE);
  EXPECT_EQ(esp_wifi_scan_start(nullptr, false), ESP_ERR_WIFI_MODE);
  EXPECT_EQ(esp_wifi_clear_ap_list(), ESP_ERR_WIFI_MODE);
  EXPECT_EQ(esp_wifi_set_config(WIFI_IF_STA, &wifi_config),
            ESP_ERR_WIFI_MODE);
  EXPECT_EQ(esp_wifi_set_config(WIFI_IF_AP, &wifi_config), ESP_OK);

  EXPECT_EQ(esp_wifi_stop(), ESP_OK);
  EXPECT_EQ(esp_wifi_deinit(), ESP_OK);
}

TEST(WifiCompatTest, ExposesSmartconfigEventBaseAndInitTables) {
  ASSERT_NE(SC_EVENT, nullptr);
  EXPECT_STREQ(SC_EVENT, "SC_EVENT");

  wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
  EXPECT_EQ(config.osi_funcs, &g_wifi_osi_funcs);
  EXPECT_EQ(config.osi_funcs->_version, ESP_WIFI_OS_ADAPTER_VERSION);
  EXPECT_EQ(static_cast<uint32_t>(config.osi_funcs->_magic),
            static_cast<uint32_t>(ESP_WIFI_OS_ADAPTER_MAGIC));
  EXPECT_EQ(config.wpa_crypto_funcs.size, sizeof(wpa_crypto_funcs_t));
  EXPECT_EQ(config.wpa_crypto_funcs.version,
            static_cast<uint32_t>(ESP_WIFI_CRYPTO_VERSION));
  EXPECT_EQ(config.wpa_crypto_funcs.hmac_sha256_vector, nullptr);
}

TEST(WifiCompatTest, SmartconfigHasSafeHostLifecycle) {
  ASSERT_EQ(esp_smartconfig_stop(), ESP_OK);
  EXPECT_EQ(esp_smartconfig_start(nullptr), ESP_ERR_INVALID_ARG);
  EXPECT_EQ(esp_smartconfig_set_type(
                static_cast<smartconfig_type_t>(SC_TYPE_ESPTOUCH_V2 + 1)),
            ESP_ERR_INVALID_ARG);
  EXPECT_EQ(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2), ESP_OK);

  smartconfig_start_config_t config = {};
  EXPECT_EQ(esp_smartconfig_start(&config), ESP_OK);
  EXPECT_EQ(esp_smartconfig_start(&config), ESP_ERR_INVALID_STATE);
  EXPECT_EQ(esp_smartconfig_set_type(SC_TYPE_AIRKISS), ESP_ERR_INVALID_STATE);
  EXPECT_EQ(esp_smartconfig_stop(), ESP_OK);
  EXPECT_EQ(esp_smartconfig_stop(), ESP_OK);

  config.esp_touch_v2_enable_crypt = true;
  EXPECT_EQ(esp_smartconfig_start(&config), ESP_ERR_INVALID_ARG);
}

TEST(WifiCompatTest, AntennaConfigurationRoundTripsOnHost) {
  EXPECT_EQ(esp_phy_set_ant_gpio(nullptr), ESP_ERR_INVALID_ARG);
  EXPECT_EQ(esp_phy_get_ant_gpio(nullptr), ESP_ERR_INVALID_ARG);
  EXPECT_EQ(esp_phy_set_ant(nullptr), ESP_ERR_INVALID_ARG);
  EXPECT_EQ(esp_phy_get_ant(nullptr), ESP_ERR_INVALID_ARG);

  esp_phy_ant_gpio_config_t gpio_config = {};
  gpio_config.gpio_cfg[0].gpio_select = 1;
  gpio_config.gpio_cfg[0].gpio_num = 2;
  gpio_config.gpio_cfg[1].gpio_select = 1;
  gpio_config.gpio_cfg[1].gpio_num = 25;
  ASSERT_EQ(esp_phy_set_ant_gpio(&gpio_config), ESP_OK);

  esp_phy_ant_gpio_config_t actual_gpio_config = {};
  ASSERT_EQ(esp_phy_get_ant_gpio(&actual_gpio_config), ESP_OK);
  EXPECT_EQ(actual_gpio_config.gpio_cfg[0].gpio_select, 1);
  EXPECT_EQ(actual_gpio_config.gpio_cfg[0].gpio_num, 2);
  EXPECT_EQ(actual_gpio_config.gpio_cfg[1].gpio_select, 1);
  EXPECT_EQ(actual_gpio_config.gpio_cfg[1].gpio_num, 25);

  esp_phy_ant_config_t ant_config = {};
  ant_config.rx_ant_mode = ESP_PHY_ANT_MODE_AUTO;
  ant_config.rx_ant_default = ESP_PHY_ANT_ANT1;
  ant_config.tx_ant_mode = ESP_PHY_ANT_MODE_ANT0;
  ant_config.enabled_ant0 = 1;
  ant_config.enabled_ant1 = 2;
  ASSERT_EQ(esp_phy_set_ant(&ant_config), ESP_OK);

  esp_phy_ant_config_t actual_ant_config = {};
  ASSERT_EQ(esp_phy_get_ant(&actual_ant_config), ESP_OK);
  EXPECT_EQ(actual_ant_config.rx_ant_mode, ESP_PHY_ANT_MODE_AUTO);
  EXPECT_EQ(actual_ant_config.rx_ant_default, ESP_PHY_ANT_ANT1);
  EXPECT_EQ(actual_ant_config.tx_ant_mode, ESP_PHY_ANT_MODE_ANT0);
  EXPECT_EQ(actual_ant_config.enabled_ant0, 1);
  EXPECT_EQ(actual_ant_config.enabled_ant1, 2);
}

TEST(WifiCompatTest, ScanFiltersSortsAndConsumesResults) {
  using roo_testing_transducers::wifi::AccessPoint;
  using roo_testing_transducers::wifi::Environment;
  using roo_testing_transducers::wifi::MacAddress;
  using roo_testing_transducers::wifi::RSSI;

  static Environment environment;
  environment.setScanDurationMs(0);
  auto weak =
      std::make_unique<AccessPoint>(MacAddress(0x02, 0, 0, 0, 0, 1), "weak");
  weak->setRSSI(RSSI(-80));
  environment.addAccessPoint(std::move(weak));
  auto strong =
      std::make_unique<AccessPoint>(MacAddress(0x02, 0, 0, 0, 0, 2), "strong");
  strong->setRSSI(RSSI(-40));
  environment.addAccessPoint(std::move(strong));
  auto hidden =
      std::make_unique<AccessPoint>(MacAddress(0x02, 0, 0, 0, 0, 3), "hidden");
  hidden->setRSSI(RSSI(-20))->setVisible(false);
  environment.addAccessPoint(std::move(hidden));
  FakeEsp32().setWifiEnvironment(environment);

  roo_testing::esp32::wifi::Reset();
  wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
  ASSERT_EQ(esp_wifi_init(&init_config), ESP_OK);
  ASSERT_EQ(esp_wifi_set_mode(WIFI_MODE_STA), ESP_OK);
  ASSERT_EQ(esp_wifi_start(), ESP_OK);
  wifi_scan_config_t config = {};
  ASSERT_EQ(esp_wifi_scan_start(&config, true), ESP_OK);

  uint16_t count = 0;
  ASSERT_EQ(esp_wifi_scan_get_ap_num(&count), ESP_OK);
  ASSERT_EQ(count, 2);
  wifi_ap_record_t record = {};
  ASSERT_EQ(esp_wifi_scan_get_ap_record(&record), ESP_OK);
  EXPECT_STREQ(reinterpret_cast<const char *>(record.ssid), "strong");
  ASSERT_EQ(esp_wifi_scan_get_ap_record(&record), ESP_OK);
  EXPECT_STREQ(reinterpret_cast<const char *>(record.ssid), "weak");
  EXPECT_EQ(esp_wifi_scan_get_ap_record(&record), ESP_FAIL);

  config.show_hidden = true;
  ASSERT_EQ(esp_wifi_scan_start(&config, true), ESP_OK);
  count = 3;
  wifi_ap_record_t records[3] = {};
  ASSERT_EQ(esp_wifi_scan_get_ap_records(&count, records), ESP_OK);
  ASSERT_EQ(count, 3);
  EXPECT_STREQ(reinterpret_cast<const char *>(records[0].ssid), "hidden");
  ASSERT_EQ(esp_wifi_scan_get_ap_num(&count), ESP_OK);
  EXPECT_EQ(count, 0);
  EXPECT_EQ(esp_wifi_stop(), ESP_OK);
  EXPECT_EQ(esp_wifi_deinit(), ESP_OK);
}

// Verifies fixed-width IDF fields do not require spare null-terminator bytes.
TEST(WifiCompatTest, SupportsMaximumLengthSsidAndPassword) {
  using roo_testing_transducers::wifi::AccessPoint;
  using roo_testing_transducers::wifi::Environment;
  using roo_testing_transducers::wifi::MacAddress;

  static Environment environment;
  const MacAddress ap_mac(0x02, 0, 0, 0, 0, 4);
  const std::string ssid(32, 's');
  const std::string password(64, 'p');
  auto ap = std::make_unique<AccessPoint>(ap_mac, ssid);
  ap->setAuthMode(roo_testing_transducers::wifi::AUTH_WPA2_PSK)
      ->setPasswd(password);
  environment.addAccessPoint(std::move(ap));
  FakeEsp32().setWifiEnvironment(environment);

  ASSERT_EQ(esp_event_loop_create_default(), ESP_OK);
  StationEventCapture capture;
  capture.event_received = xSemaphoreCreateBinary();
  ASSERT_NE(capture.event_received, nullptr);
  ASSERT_EQ(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                       CaptureStationEvent, &capture),
            ESP_OK);

  roo_testing::esp32::wifi::Reset();
  wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
  ASSERT_EQ(esp_wifi_init(&init_config), ESP_OK);
  ASSERT_EQ(esp_wifi_set_mode(WIFI_MODE_STA), ESP_OK);
  ASSERT_EQ(esp_wifi_start(), ESP_OK);

  wifi_config_t station_config = {};
  memcpy(station_config.sta.ssid, ssid.data(), ssid.size());
  memcpy(station_config.sta.password, password.data(), password.size());
  station_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
  station_config.sta.bssid_set = true;
  for (size_t i = 0; i < sizeof(station_config.sta.bssid); ++i) {
    station_config.sta.bssid[i] = ap_mac.get(i);
  }
  ASSERT_EQ(esp_wifi_set_config(WIFI_IF_STA, &station_config), ESP_OK);
  ASSERT_EQ(esp_wifi_connect(), ESP_OK);
  ASSERT_EQ(xSemaphoreTake(capture.event_received, pdMS_TO_TICKS(1000)),
            pdTRUE);
  EXPECT_EQ(capture.connected.ssid_len, ssid.size());
  EXPECT_EQ(memcmp(capture.connected.ssid, ssid.data(), ssid.size()), 0);

  wifi_ap_record_t info = {};
  ASSERT_EQ(esp_wifi_sta_get_ap_info(&info), ESP_OK);
  EXPECT_EQ(memcmp(info.ssid, ssid.data(), ssid.size()), 0);
  EXPECT_EQ(info.ssid[ssid.size()], '\0');

  ASSERT_EQ(esp_wifi_disconnect(), ESP_OK);
  ASSERT_EQ(xSemaphoreTake(capture.event_received, pdMS_TO_TICKS(1000)),
            pdTRUE);
  EXPECT_EQ(capture.disconnected.ssid_len, ssid.size());
  EXPECT_EQ(memcmp(capture.disconnected.ssid, ssid.data(), ssid.size()), 0);

  EXPECT_EQ(esp_wifi_stop(), ESP_OK);
  EXPECT_EQ(esp_wifi_deinit(), ESP_OK);
  EXPECT_EQ(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                         CaptureStationEvent),
            ESP_OK);
  EXPECT_EQ(esp_event_loop_delete_default(), ESP_OK);
  vSemaphoreDelete(capture.event_received);
}

// Verifies scan, reconnect, netif, and environment lifetimes stay coherent.
TEST(WifiCompatTest, MaintainsConnectionAndNetifStateAcrossOperations) {
  using roo_testing::esp32::wifi::StationState;
  using roo_testing_transducers::wifi::AccessPoint;
  using roo_testing_transducers::wifi::Environment;
  using roo_testing_transducers::wifi::MacAddress;

  {
    Environment temporary_environment;
    temporary_environment.addAccessPoint(std::make_unique<AccessPoint>(
        MacAddress(0x02, 0, 0, 0, 0, 5), "owned"));
    FakeEsp32().setWifiEnvironment(temporary_environment);
  }

  ASSERT_EQ(esp_event_loop_create_default(), ESP_OK);
  NetworkEventCapture capture;
  capture.connected = xSemaphoreCreateBinary();
  capture.disconnected = xSemaphoreCreateBinary();
  capture.got_ip = xSemaphoreCreateBinary();
  capture.lost_ip = xSemaphoreCreateBinary();
  capture.scan_done = xSemaphoreCreateBinary();
  ASSERT_NE(capture.connected, nullptr);
  ASSERT_NE(capture.disconnected, nullptr);
  ASSERT_NE(capture.got_ip, nullptr);
  ASSERT_NE(capture.lost_ip, nullptr);
  ASSERT_NE(capture.scan_done, nullptr);
  ASSERT_EQ(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                       CaptureNetworkEvent, &capture),
            ESP_OK);
  ASSERT_EQ(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID,
                                       CaptureNetworkEvent, &capture),
            ESP_OK);

  roo_testing::esp32::wifi::Reset();
  wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
  ASSERT_EQ(esp_wifi_init(&init_config), ESP_OK);
  ASSERT_EQ(esp_wifi_set_mode(WIFI_MODE_STA), ESP_OK);
  esp_netif_t *station_netif = esp_netif_create_default_wifi_sta();
  ASSERT_NE(station_netif, nullptr);
  EXPECT_FALSE(esp_netif_is_netif_up(station_netif));
  ASSERT_EQ(esp_wifi_start(), ESP_OK);
  EXPECT_TRUE(esp_netif_is_netif_up(station_netif));

  wifi_config_t station_config = {};
  memcpy(station_config.sta.ssid, "owned", sizeof("owned"));
  ASSERT_EQ(esp_wifi_set_config(WIFI_IF_STA, &station_config), ESP_OK);
  ASSERT_EQ(esp_wifi_connect(), ESP_OK);
  ASSERT_EQ(xSemaphoreTake(capture.connected, pdMS_TO_TICKS(1000)), pdTRUE);
  ASSERT_EQ(xSemaphoreTake(capture.got_ip, pdMS_TO_TICKS(1000)), pdTRUE);
  auto state = roo_testing::esp32::wifi::GetState();
  EXPECT_EQ(state.station, StationState::kGotIp);

  esp_netif_ip_info_t ip_info = {};
  ASSERT_EQ(esp_netif_get_ip_info(station_netif, &ip_info), ESP_OK);
  EXPECT_NE(ip_info.ip.addr, 0U);

  auto scan_environment = std::make_shared<Environment>();
  scan_environment->setScanDurationMs(20);
  FakeEsp32().setWifiEnvironment(scan_environment);
  ASSERT_EQ(esp_wifi_scan_start(nullptr, false), ESP_OK);
  state = roo_testing::esp32::wifi::GetState();
  EXPECT_EQ(state.station, StationState::kGotIp);
  EXPECT_EQ(esp_wifi_scan_start(nullptr, false), ESP_ERR_WIFI_STATE);
  ASSERT_EQ(xSemaphoreTake(capture.scan_done, pdMS_TO_TICKS(1000)), pdTRUE);
  state = roo_testing::esp32::wifi::GetState();
  EXPECT_EQ(state.station, StationState::kGotIp);

  wifi_ap_record_t connected_ap = {};
  ASSERT_EQ(esp_wifi_sta_get_ap_info(&connected_ap), ESP_OK);
  EXPECT_STREQ(reinterpret_cast<const char *>(connected_ap.ssid), "owned");

  memset(&station_config, 0, sizeof(station_config));
  memcpy(station_config.sta.ssid, "missing", sizeof("missing"));
  ASSERT_EQ(esp_wifi_set_config(WIFI_IF_STA, &station_config), ESP_OK);
  ASSERT_EQ(esp_wifi_connect(), ESP_OK);
  ASSERT_EQ(xSemaphoreTake(capture.disconnected, pdMS_TO_TICKS(1000)), pdTRUE);
  ASSERT_EQ(xSemaphoreTake(capture.lost_ip, pdMS_TO_TICKS(1000)), pdTRUE);
  EXPECT_EQ(esp_wifi_sta_get_ap_info(&connected_ap),
            ESP_ERR_WIFI_NOT_CONNECT);
  state = roo_testing::esp32::wifi::GetState();
  EXPECT_EQ(state.station, StationState::kIdle);
  ASSERT_EQ(esp_netif_get_ip_info(station_netif, &ip_info), ESP_OK);
  EXPECT_EQ(ip_info.ip.addr, 0U);
  EXPECT_TRUE(esp_netif_is_netif_up(station_netif));

  scan_environment->addAccessPoint(std::make_unique<AccessPoint>(
      MacAddress(0x02, 0, 0, 0, 0, 6), "missing"));
  ASSERT_EQ(esp_wifi_connect(), ESP_OK);
  ASSERT_EQ(xSemaphoreTake(capture.connected, pdMS_TO_TICKS(1000)), pdTRUE);
  ASSERT_EQ(xSemaphoreTake(capture.got_ip, pdMS_TO_TICKS(1000)), pdTRUE);
  ASSERT_EQ(esp_wifi_sta_get_ap_info(&connected_ap), ESP_OK);
  EXPECT_STREQ(reinterpret_cast<const char *>(connected_ap.ssid), "missing");
  state = roo_testing::esp32::wifi::GetState();
  EXPECT_EQ(state.station, StationState::kGotIp);

  ASSERT_EQ(esp_wifi_disconnect(), ESP_OK);
  ASSERT_EQ(xSemaphoreTake(capture.disconnected, pdMS_TO_TICKS(1000)), pdTRUE);
  ASSERT_EQ(xSemaphoreTake(capture.lost_ip, pdMS_TO_TICKS(1000)), pdTRUE);

  EXPECT_EQ(esp_wifi_stop(), ESP_OK);
  EXPECT_FALSE(esp_netif_is_netif_up(station_netif));
  EXPECT_EQ(esp_wifi_deinit(), ESP_OK);
  esp_netif_destroy_default_wifi(station_netif);
  EXPECT_EQ(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                         CaptureNetworkEvent),
            ESP_OK);
  EXPECT_EQ(esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID,
                                         CaptureNetworkEvent),
            ESP_OK);
  EXPECT_EQ(esp_event_loop_delete_default(), ESP_OK);
  vSemaphoreDelete(capture.connected);
  vSemaphoreDelete(capture.disconnected);
  vSemaphoreDelete(capture.got_ip);
  vSemaphoreDelete(capture.lost_ip);
  vSemaphoreDelete(capture.scan_done);
}

// Verifies connection teardown events precede IP loss and interface stop.
TEST(WifiCompatTest, DeliversLifecycleEventsInOrder) {
  using roo_testing_transducers::wifi::AccessPoint;
  using roo_testing_transducers::wifi::Environment;
  using roo_testing_transducers::wifi::MacAddress;

  auto environment = std::make_shared<Environment>();
  environment->addAccessPoint(std::make_unique<AccessPoint>(
      MacAddress(0x02, 0, 0, 0, 0, 7), "ordered"));
  FakeEsp32().setWifiEnvironment(environment);

  ASSERT_EQ(esp_event_loop_create_default(), ESP_OK);
  roo_testing::esp32::wifi::Reset();
  wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
  ASSERT_EQ(esp_wifi_init(&init_config), ESP_OK);
  ASSERT_EQ(esp_wifi_set_mode(WIFI_MODE_STA), ESP_OK);
  esp_netif_t *station_netif = esp_netif_create_default_wifi_sta();
  ASSERT_NE(station_netif, nullptr);
  ASSERT_EQ(esp_wifi_start(), ESP_OK);

  OrderedEventCapture capture;
  capture.delivered = xSemaphoreCreateCounting(32, 0);
  ASSERT_NE(capture.delivered, nullptr);
  ASSERT_EQ(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                       CaptureOrderedEvent, &capture),
            ESP_OK);
  ASSERT_EQ(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID,
                                       CaptureOrderedEvent, &capture),
            ESP_OK);

  wifi_config_t station_config = {};
  memcpy(station_config.sta.ssid, "ordered", sizeof("ordered"));
  ASSERT_EQ(esp_wifi_set_config(WIFI_IF_STA, &station_config), ESP_OK);
  ASSERT_EQ(esp_wifi_connect(), ESP_OK);
  ExpectOrderedEvents(
      capture, {OrderedEvent::kConnected, OrderedEvent::kGotIp});

  ASSERT_EQ(esp_wifi_disconnect(), ESP_OK);
  ExpectOrderedEvents(
      capture, {OrderedEvent::kDisconnected, OrderedEvent::kLostIp});

  ASSERT_EQ(esp_wifi_connect(), ESP_OK);
  ExpectOrderedEvents(
      capture, {OrderedEvent::kConnected, OrderedEvent::kGotIp});
  ASSERT_EQ(esp_wifi_stop(), ESP_OK);
  ExpectOrderedEvents(capture, {OrderedEvent::kDisconnected,
                                OrderedEvent::kLostIp,
                                OrderedEvent::kStaStop});

  ASSERT_EQ(esp_wifi_start(), ESP_OK);
  ExpectOrderedEvents(capture, {OrderedEvent::kStaStart});
  ASSERT_EQ(esp_wifi_connect(), ESP_OK);
  ExpectOrderedEvents(
      capture, {OrderedEvent::kConnected, OrderedEvent::kGotIp});

  auto empty_environment = std::make_shared<Environment>();
  FakeEsp32().setWifiEnvironment(empty_environment);
  ASSERT_EQ(esp_wifi_connect(), ESP_OK);
  ExpectOrderedEvents(
      capture, {OrderedEvent::kDisconnected, OrderedEvent::kLostIp});

  empty_environment->addAccessPoint(std::make_unique<AccessPoint>(
      MacAddress(0x02, 0, 0, 0, 0, 8), "ordered"));
  ASSERT_EQ(esp_wifi_connect(), ESP_OK);
  ExpectOrderedEvents(
      capture, {OrderedEvent::kConnected, OrderedEvent::kGotIp});

  ASSERT_EQ(esp_wifi_set_mode(WIFI_MODE_AP), ESP_OK);
  ExpectOrderedEvents(capture, {OrderedEvent::kDisconnected,
                                OrderedEvent::kLostIp,
                                OrderedEvent::kStaStop,
                                OrderedEvent::kApStart});

  EXPECT_EQ(esp_wifi_stop(), ESP_OK);
  EXPECT_EQ(esp_wifi_deinit(), ESP_OK);
  esp_netif_destroy_default_wifi(station_netif);
  EXPECT_EQ(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                         CaptureOrderedEvent),
            ESP_OK);
  EXPECT_EQ(esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID,
                                         CaptureOrderedEvent),
            ESP_OK);
  EXPECT_EQ(esp_event_loop_delete_default(), ESP_OK);
  vSemaphoreDelete(capture.delivered);
}

} // namespace
