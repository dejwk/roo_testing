#include <nvs.h>
#include <nvs_flash.h>

#include <algorithm>
#include <array>
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

TEST(NvsIteratorTest, VariableLengthReadsValidateBufferCapacity) {
  nvs_handle_t handle = 0;
  ASSERT_EQ(ESP_OK, nvs_open("read_lengths", NVS_READWRITE, &handle));
  ASSERT_EQ(ESP_OK, nvs_set_str(handle, "text", "hello"));
  const std::array<uint8_t, 5> blob = {{1, 2, 3, 4, 5}};
  ASSERT_EQ(ESP_OK, nvs_set_blob(handle, "blob", blob.data(), blob.size()));

  size_t length = 0;
  EXPECT_EQ(ESP_OK, nvs_get_str(handle, "text", nullptr, &length));
  EXPECT_EQ(6u, length);

  std::array<char, 8> text_buffer;
  text_buffer.fill('x');
  length = 4;
  EXPECT_EQ(ESP_ERR_NVS_INVALID_LENGTH,
            nvs_get_str(handle, "text", text_buffer.data(), &length));
  EXPECT_EQ(6u, length);
  EXPECT_EQ('x', text_buffer[0]);

  length = text_buffer.size();
  EXPECT_EQ(ESP_OK, nvs_get_str(handle, "text", text_buffer.data(), &length));
  EXPECT_EQ(6u, length);
  EXPECT_STREQ("hello", text_buffer.data());
  EXPECT_EQ('x', text_buffer[6]);

  std::array<uint8_t, 8> blob_buffer;
  blob_buffer.fill(0xEE);
  length = 3;
  EXPECT_EQ(ESP_ERR_NVS_INVALID_LENGTH,
            nvs_get_blob(handle, "blob", blob_buffer.data(), &length));
  EXPECT_EQ(blob.size(), length);
  EXPECT_EQ(0xEE, blob_buffer[0]);

  length = blob_buffer.size();
  EXPECT_EQ(ESP_OK, nvs_get_blob(handle, "blob", blob_buffer.data(), &length));
  EXPECT_EQ(blob.size(), length);
  EXPECT_TRUE(std::equal(blob.begin(), blob.end(), blob_buffer.begin()));
  EXPECT_EQ(0xEE, blob_buffer[5]);

  EXPECT_EQ(ESP_ERR_NVS_INVALID_LENGTH,
            nvs_get_str(handle, "text", nullptr, nullptr));
  EXPECT_EQ(ESP_ERR_INVALID_ARG, nvs_get_u32(handle, "value", nullptr));

  ASSERT_EQ(ESP_OK, nvs_erase_all(handle));
  ASSERT_EQ(ESP_OK, nvs_commit(handle));
  nvs_close(handle);
}

TEST(NvsIteratorTest, ValidatesNamesAndInputPointers) {
  nvs_handle_t handle = 0;
  EXPECT_EQ(ESP_ERR_INVALID_ARG,
            nvs_open("invalid_args", NVS_READWRITE, nullptr));
  EXPECT_EQ(ESP_ERR_NVS_INVALID_NAME, nvs_open("", NVS_READWRITE, &handle));
  EXPECT_EQ(ESP_ERR_NVS_INVALID_NAME,
            nvs_open("namespace_too_long", NVS_READWRITE, &handle));

  ASSERT_EQ(ESP_OK, nvs_open("valid_args", NVS_READWRITE, &handle));
  EXPECT_EQ(ESP_ERR_NVS_INVALID_NAME, nvs_set_u32(handle, nullptr, 1));
  EXPECT_EQ(ESP_ERR_NVS_INVALID_NAME, nvs_set_u32(handle, "", 1));
  EXPECT_EQ(ESP_ERR_INVALID_ARG, nvs_set_str(handle, "text", nullptr));
  EXPECT_EQ(ESP_ERR_INVALID_ARG, nvs_set_blob(handle, "blob", nullptr, 1));

  ASSERT_EQ(ESP_OK, nvs_erase_all(handle));
  ASSERT_EQ(ESP_OK, nvs_commit(handle));
  nvs_close(handle);
}

TEST(NvsIteratorTest, DeinitInvalidatesHandlesAndPreservesData) {
  nvs_handle_t handle = 0;
  ASSERT_EQ(ESP_OK, nvs_open("deinit_test", NVS_READWRITE, &handle));
  ASSERT_EQ(ESP_OK, nvs_set_u32(handle, "value", 42));
  ASSERT_EQ(ESP_OK, nvs_commit(handle));

  ASSERT_EQ(ESP_OK, nvs_flash_deinit());
  uint32_t value = 0;
  EXPECT_EQ(ESP_ERR_NVS_INVALID_HANDLE, nvs_get_u32(handle, "value", &value));
  EXPECT_EQ(ESP_ERR_NVS_NOT_INITIALIZED,
            nvs_open("deinit_test", NVS_READONLY, &handle));

  ASSERT_EQ(ESP_OK, nvs_flash_init());
  ASSERT_EQ(ESP_OK, nvs_open("deinit_test", NVS_READONLY, &handle));
  EXPECT_EQ(ESP_OK, nvs_get_u32(handle, "value", &value));
  EXPECT_EQ(42u, value);
  nvs_close(handle);
}

TEST(NvsIteratorTest, ErasePartitionClearsDataAndInvalidatesHandles) {
  nvs_handle_t handle = 0;
  ASSERT_EQ(ESP_OK, nvs_open("erase_part", NVS_READWRITE, &handle));
  ASSERT_EQ(ESP_OK, nvs_set_u32(handle, "value", 42));
  ASSERT_EQ(ESP_OK, nvs_commit(handle));

  ASSERT_EQ(ESP_OK, nvs_flash_erase());
  uint32_t value = 0;
  EXPECT_EQ(ESP_ERR_NVS_INVALID_HANDLE, nvs_get_u32(handle, "value", &value));
  EXPECT_EQ(ESP_ERR_NVS_NOT_INITIALIZED,
            nvs_open("erase_part", NVS_READONLY, &handle));

  ASSERT_EQ(ESP_OK, nvs_flash_init());
  EXPECT_EQ(ESP_ERR_NVS_NOT_FOUND,
            nvs_open("erase_part", NVS_READONLY, &handle));
}

} // namespace
