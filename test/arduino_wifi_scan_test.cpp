#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include "WiFi.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "gtest/gtest.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/transducers/wifi/wifi.h"

namespace {

struct ScanDeliveryState {
  std::thread::id caller;
  std::atomic<bool> delivered{false};
  std::atomic<bool> delivered_on_caller{false};
};

void OnScanDone(void* arg, esp_event_base_t, int32_t, void*) {
  auto* state = static_cast<ScanDeliveryState*>(arg);
  state->delivered_on_caller = std::this_thread::get_id() == state->caller;
  state->delivered = true;
}

struct ReconnectDeliveryState {
  std::atomic<int> connected{0};
  std::atomic<int> disconnected{0};
  std::atomic<int> got_ip{0};
};

void OnReconnectEvent(void* arg, esp_event_base_t event_base, int32_t event_id,
                      void*) {
  auto* state = static_cast<ReconnectDeliveryState*>(arg);
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
    ++state->connected;
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    ++state->disconnected;
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ++state->got_ip;
  }
}

// Pumps Arduino tasks until the requested WiFi status arrives or times out.
bool WaitForWifiStatus(wl_status_t expected,
                       std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (WiFi.status() != expected &&
         std::chrono::steady_clock::now() < deadline) {
    delay(1);
  }
  return WiFi.status() == expected;
}

TEST(ArduinoWifiScanTest, AsyncScanCompletesOutsideCallingThread) {
  using roo_testing_transducers::wifi::AccessPoint;
  using roo_testing_transducers::wifi::Environment;
  using roo_testing_transducers::wifi::MacAddress;

  static Environment environment;
  environment.addAccessPoint(std::make_unique<AccessPoint>(
      MacAddress(0x02, 0x00, 0x00, 0x00, 0x00, 0x01), "roo-test"));
  FakeEsp32().setWifiEnvironment(environment);

  ASSERT_TRUE(WiFi.mode(WIFI_STA));

  ScanDeliveryState delivery{std::this_thread::get_id()};
  esp_event_handler_instance_t instance = nullptr;
  ASSERT_EQ(
      esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE,
                                          OnScanDone, &delivery, &instance),
      ESP_OK);

  const int16_t start_result = WiFi.scanNetworks(true, false);
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
  int16_t scan_result = WiFi.scanComplete();
  while ((scan_result == WIFI_SCAN_RUNNING || !delivery.delivered) &&
         std::chrono::steady_clock::now() < deadline) {
    delay(1);
    scan_result = WiFi.scanComplete();
  }

  EXPECT_EQ(esp_event_handler_instance_unregister(
                WIFI_EVENT, WIFI_EVENT_SCAN_DONE, instance),
            ESP_OK);
  EXPECT_EQ(start_result, WIFI_SCAN_RUNNING);
  EXPECT_TRUE(delivery.delivered);
  EXPECT_FALSE(delivery.delivered_on_caller);
  ASSERT_EQ(scan_result, 1);
  EXPECT_STREQ(WiFi.SSID(0).c_str(), "roo-test");
  WiFi.scanDelete();
}

// Verifies Arduino status and IP state across begin, disconnect, and reconnect.
TEST(ArduinoWifiScanTest, ConnectsAndReconnectsThroughArduinoApi) {
  using roo_testing_transducers::wifi::AccessPoint;
  using roo_testing_transducers::wifi::Environment;
  using roo_testing_transducers::wifi::MacAddress;

  ASSERT_TRUE(WiFi.mode(WIFI_OFF));
  Environment environment;
  auto access_point = std::make_unique<AccessPoint>(
      MacAddress(0x02, 0x00, 0x00, 0x00, 0x00, 0x02), "arduino-test");
  access_point
      ->setAuthMode(roo_testing_transducers::wifi::AUTH_WPA2_PSK)
      ->setPasswd("secret");
  environment.addAccessPoint(std::move(access_point));
  FakeEsp32().setWifiEnvironment(environment);

  WiFi.begin("arduino-test", "secret");
  ASSERT_TRUE(WaitForWifiStatus(WL_CONNECTED, std::chrono::seconds(1)));
  EXPECT_NE(static_cast<uint32_t>(WiFi.localIP()), 0U);
  EXPECT_STREQ(WiFi.SSID().c_str(), "arduino-test");

  ASSERT_TRUE(WiFi.disconnect(false, false, 1000));
  EXPECT_NE(WiFi.status(), WL_CONNECTED);
  EXPECT_EQ(static_cast<uint32_t>(WiFi.localIP()), 0U);

  ASSERT_TRUE(WiFi.reconnect());
  ASSERT_TRUE(WaitForWifiStatus(WL_CONNECTED, std::chrono::seconds(1)));
  EXPECT_NE(static_cast<uint32_t>(WiFi.localIP()), 0U);

  EXPECT_TRUE(WiFi.disconnect(true, true, 1000));
}

// Verifies a scripted link loss exercises Arduino's automatic reconnect path.
TEST(ArduinoWifiScanTest, AutoReconnectsAfterBeaconTimeout) {
  using roo_testing_transducers::wifi::AccessPoint;
  using roo_testing_transducers::wifi::ConnectionAttempt;
  using roo_testing_transducers::wifi::ConnectionOutcome;
  using roo_testing_transducers::wifi::Environment;
  using roo_testing_transducers::wifi::MacAddress;

  ASSERT_TRUE(WiFi.mode(WIFI_OFF));
  auto environment = std::make_shared<Environment>();
  environment->addAccessPoint(std::make_unique<AccessPoint>(
      MacAddress(0x02, 0x00, 0x00, 0x00, 0x00, 0x03), "auto-reconnect"));
  environment->queueConnectionAttempt(ConnectionAttempt{
      ConnectionOutcome::kBeaconTimeout, 0, 0, 20});
  environment->queueConnectionAttempt(
      ConnectionAttempt{ConnectionOutcome::kSuccess});
  FakeEsp32().setWifiEnvironment(environment);

  ReconnectDeliveryState delivery;
  esp_event_handler_instance_t wifi_instance = nullptr;
  esp_event_handler_instance_t ip_instance = nullptr;
  ASSERT_EQ(esp_event_handler_instance_register(
                WIFI_EVENT, ESP_EVENT_ANY_ID, OnReconnectEvent, &delivery,
                &wifi_instance),
            ESP_OK);
  ASSERT_EQ(esp_event_handler_instance_register(
                IP_EVENT, ESP_EVENT_ANY_ID, OnReconnectEvent, &delivery,
                &ip_instance),
            ESP_OK);

  ASSERT_TRUE(WiFi.setAutoReconnect(true));
  WiFi.begin("auto-reconnect");
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while ((delivery.disconnected < 1 || delivery.got_ip < 2) &&
         std::chrono::steady_clock::now() < deadline) {
    delay(1);
  }

  EXPECT_GE(delivery.connected.load(), 2);
  EXPECT_GE(delivery.disconnected.load(), 1);
  EXPECT_GE(delivery.got_ip.load(), 2);
  EXPECT_EQ(WiFi.status(), WL_CONNECTED);
  EXPECT_NE(static_cast<uint32_t>(WiFi.localIP()), 0U);

  EXPECT_EQ(esp_event_handler_instance_unregister(
                WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_instance),
            ESP_OK);
  EXPECT_EQ(esp_event_handler_instance_unregister(
                IP_EVENT, ESP_EVENT_ANY_ID, ip_instance),
            ESP_OK);
  EXPECT_TRUE(WiFi.disconnect(true, true, 1000));
}

}  // namespace
