#include <gtest/gtest.h>

#include "esp32-hal-periman.h"

namespace {

constexpr uint8_t kEarlyPin = 4;
constexpr char kEarlyExtraType[] = "global-constructor";
bool g_set_pin_bus_result;
bool g_set_extra_type_result;

// Arduino libraries commonly configure pins from constructors of global
// objects.  Force this constructor to run before ordinary namespace-scope
// objects so the emulator's peripheral manager cannot depend on translation
// unit initialization order.
struct EarlyPerimanUser {
  EarlyPerimanUser() {
    g_set_pin_bus_result = perimanSetPinBus(
        kEarlyPin, ESP32_BUS_TYPE_GPIO,
        reinterpret_cast<void*>(kEarlyPin + 1), -1, -1);
    g_set_extra_type_result =
        perimanSetPinBusExtraType(kEarlyPin, kEarlyExtraType);
  }
};

EarlyPerimanUser g_early_periman_user __attribute__((init_priority(101)));

TEST(ArduinoPerimanStaticInitializationTest,
     SupportsCallsFromGlobalConstructors) {
  EXPECT_TRUE(g_set_pin_bus_result);
  EXPECT_TRUE(g_set_extra_type_result);
  EXPECT_EQ(ESP32_BUS_TYPE_GPIO, perimanGetPinBusType(kEarlyPin));
  EXPECT_STREQ(kEarlyExtraType, perimanGetPinBusExtraType(kEarlyPin));
}

}  // namespace
