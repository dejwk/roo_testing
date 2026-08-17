#include "esp_arduino_version.h"
#include "esp_idf_version.h"
#include "gtest/gtest.h"

static_assert(ESP_ARDUINO_VERSION_MAJOR == 3);
static_assert(ESP_ARDUINO_VERSION_MINOR == 3);
static_assert(ESP_ARDUINO_VERSION_PATCH == 8);
static_assert(ESP_IDF_VERSION_MAJOR == 5);
static_assert(ESP_IDF_VERSION_MINOR == 5);
static_assert(ESP_IDF_VERSION_PATCH == 4);

TEST(FrameworkVersionTest, UsesPinnedCompatiblePair) {
  EXPECT_STREQ(ESP_ARDUINO_VERSION_STR, "3.3.8");
  EXPECT_EQ(ESP_IDF_VERSION, ESP_IDF_VERSION_VAL(5, 5, 4));
}
