#include <memory>
#include <type_traits>

#include "esp_phy.h"
#include "esp_private/wifi_os_adapter.h"
#include "esp_smartconfig.h"
#include "esp_wifi.h"
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

} // namespace
