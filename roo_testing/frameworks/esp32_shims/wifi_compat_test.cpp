#include <type_traits>

#include "esp_phy.h"
#include "esp_private/wifi_os_adapter.h"
#include "esp_smartconfig.h"
#include "esp_wifi.h"
#include "gtest/gtest.h"

static_assert(std::is_same_v<decltype(g_wifi_osi_funcs), wifi_osi_funcs_t>);
static_assert(std::is_same_v<decltype(g_wifi_default_wpa_crypto_funcs),
                             const wpa_crypto_funcs_t>);
static_assert(std::is_same_v<decltype(&esp_smartconfig_start),
                             esp_err_t (*)(const smartconfig_start_config_t*)>);
static_assert(std::is_same_v<decltype(&esp_phy_set_ant_gpio),
                             esp_err_t (*)(esp_phy_ant_gpio_config_t*)>);
static_assert(std::is_same_v<decltype(&esp_phy_set_ant),
                             esp_err_t (*)(esp_phy_ant_config_t*)>);

namespace {

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
  EXPECT_EQ(esp_smartconfig_set_type(SC_TYPE_AIRKISS),
            ESP_ERR_INVALID_STATE);
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

}  // namespace
