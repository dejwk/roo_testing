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

}  // namespace
