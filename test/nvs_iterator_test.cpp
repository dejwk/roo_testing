#include <nvs.h>

#include <set>
#include <string>

#include "gtest/gtest.h"

namespace {

TEST(NvsIteratorTest, EnumeratesNamespaceAndFiltersByType) {
  nvs_handle_t handle = 0;
  ASSERT_EQ(ESP_OK, nvs_open("iter_test", NVS_READWRITE, &handle));
  ASSERT_EQ(ESP_OK, nvs_set_u32(handle, "count", 7));
  ASSERT_EQ(ESP_OK, nvs_set_str(handle, "name", "device"));
  ASSERT_EQ(ESP_OK, nvs_commit(handle));

  nvs_iterator_t iterator = nullptr;
  ASSERT_EQ(ESP_OK,
            nvs_entry_find("nvs", "iter_test", NVS_TYPE_ANY, &iterator));
  std::set<std::string> keys;
  esp_err_t result = ESP_OK;
  while (result == ESP_OK) {
    nvs_entry_info_t info;
    ASSERT_EQ(ESP_OK, nvs_entry_info(iterator, &info));
    EXPECT_STREQ("iter_test", info.namespace_name);
    keys.insert(info.key);
    result = nvs_entry_next(&iterator);
  }
  EXPECT_EQ(ESP_ERR_NVS_NOT_FOUND, result);
  EXPECT_EQ((std::set<std::string>{"count", "name"}), keys);
  EXPECT_EQ(nullptr, iterator);
  nvs_release_iterator(iterator);

  ASSERT_EQ(ESP_OK, nvs_entry_find_in_handle(handle, NVS_TYPE_STR, &iterator));
  nvs_entry_info_t info;
  ASSERT_EQ(ESP_OK, nvs_entry_info(iterator, &info));
  EXPECT_STREQ("name", info.key);
  EXPECT_EQ(NVS_TYPE_STR, info.type);
  EXPECT_EQ(ESP_ERR_NVS_NOT_FOUND, nvs_entry_next(&iterator));
  EXPECT_EQ(nullptr, iterator);

  ASSERT_EQ(ESP_OK, nvs_erase_all(handle));
  ASSERT_EQ(ESP_OK, nvs_commit(handle));
  nvs_close(handle);
}

TEST(NvsIteratorTest, ReportsNoMatchingEntries) {
  nvs_iterator_t iterator = reinterpret_cast<nvs_iterator_t>(1);
  EXPECT_EQ(ESP_ERR_NVS_NOT_FOUND,
            nvs_entry_find("nvs", "missing_iter", NVS_TYPE_ANY, &iterator));
  EXPECT_EQ(nullptr, iterator);
}

} // namespace
