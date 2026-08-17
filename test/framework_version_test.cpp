#include "esp_arduino_version.h"
#include "esp_idf_version.h"
#include "gtest/gtest.h"

static_assert(ESP_ARDUINO_VERSION_MAJOR == 3);
static_assert(ESP_ARDUINO_VERSION_MINOR == 3);
static_assert(ESP_ARDUINO_VERSION_PATCH == 11);
static_assert(ESP_IDF_VERSION_MAJOR == 6);
static_assert(ESP_IDF_VERSION_MINOR == 0);
static_assert(ESP_IDF_VERSION_PATCH == 2);

TEST(FrameworkVersionTest, UsesPinnedCompatiblePair) {
  EXPECT_STREQ(ESP_ARDUINO_VERSION_STR, "3.3.11");
  EXPECT_EQ(ESP_IDF_VERSION, ESP_IDF_VERSION_VAL(6, 0, 2));
}
