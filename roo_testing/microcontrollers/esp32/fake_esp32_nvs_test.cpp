#include "fake_esp32_nvs.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace {

std::string MakeTempFile() {
  char path[] = "/tmp/nvs_testXXXXXX";
  int fd = mkstemp(path);
  if (fd >= 0) {
    close(fd);
  }
  std::remove(path);
  return std::string(path);
}

}  // namespace

TEST(FakeEsp32NvsJsonTest, RoundTripJsonStorage) {
  std::string path = MakeTempFile();
  const unsigned char blob_data[] = {0x01, 0x02, 0x03, 0x04};

  {
    Nvs nvs(path);
    ASSERT_EQ(nvs.init("nvs"), 0);
    nvs_handle_t handle = 0;
    ASSERT_EQ(nvs.open("nvs", "settings", false, &handle), 0);
    ASSERT_EQ(nvs.set_i8(handle, "i8", -8), 0);
    ASSERT_EQ(nvs.set_u8(handle, "u8", 8), 0);
    ASSERT_EQ(nvs.set_i16(handle, "i16", -16), 0);
    ASSERT_EQ(nvs.set_u16(handle, "u16", 16), 0);
    ASSERT_EQ(nvs.set_i32(handle, "i32", -32), 0);
    ASSERT_EQ(nvs.set_u32(handle, "u32", 32), 0);
    ASSERT_EQ(nvs.set_i64(handle, "i64", -64), 0);
    ASSERT_EQ(nvs.set_u64(handle, "u64", 64), 0);
    ASSERT_EQ(nvs.set_u32(handle, "volume", 7), 0);
    ASSERT_EQ(nvs.set_str(handle, "device_name", "demo-board"), 0);
    ASSERT_EQ(nvs.set_blob(handle, "calib", blob_data, sizeof(blob_data)), 0);
    ASSERT_EQ(nvs.commit(), 0);
    nvs.close(handle);
  }

  std::string file_contents;
  {
    std::ifstream input(path);
    std::string data((std::istreambuf_iterator<char>(input)),
                     std::istreambuf_iterator<char>());
    file_contents = std::move(data);
  }
  EXPECT_NE(file_contents.find("\"type\": \"U32\""), std::string::npos);
  EXPECT_NE(file_contents.find("\"type\": \"STR\""), std::string::npos);
  EXPECT_NE(file_contents.find("\"type\": \"BLOB\""), std::string::npos);
  EXPECT_NE(file_contents.find("\"device_name\""), std::string::npos);

  {
    Nvs nvs(path);
    nvs_handle_t handle = 0;
    ASSERT_EQ(nvs.open("nvs", "settings", false, &handle), 0);

    int8_t i8 = 0;
    ASSERT_EQ(nvs.get_i8(handle, "i8", &i8), 0);
    EXPECT_EQ(i8, -8);

    uint8_t u8 = 0;
    ASSERT_EQ(nvs.get_u8(handle, "u8", &u8), 0);
    EXPECT_EQ(u8, 8u);

    int16_t i16 = 0;
    ASSERT_EQ(nvs.get_i16(handle, "i16", &i16), 0);
    EXPECT_EQ(i16, -16);

    uint16_t u16 = 0;
    ASSERT_EQ(nvs.get_u16(handle, "u16", &u16), 0);
    EXPECT_EQ(u16, 16u);

    int32_t i32 = 0;
    ASSERT_EQ(nvs.get_i32(handle, "i32", &i32), 0);
    EXPECT_EQ(i32, -32);

    uint32_t u32 = 0;
    ASSERT_EQ(nvs.get_u32(handle, "u32", &u32), 0);
    EXPECT_EQ(u32, 32u);

    int64_t i64 = 0;
    ASSERT_EQ(nvs.get_i64(handle, "i64", &i64), 0);
    EXPECT_EQ(i64, -64);

    uint64_t u64 = 0;
    ASSERT_EQ(nvs.get_u64(handle, "u64", &u64), 0);
    EXPECT_EQ(u64, 64u);

    uint32_t volume = 0;
    ASSERT_EQ(nvs.get_u32(handle, "volume", &volume), 0);
    EXPECT_EQ(volume, 7u);

    size_t name_len = 0;
    ASSERT_EQ(nvs.get_str(handle, "device_name", nullptr, &name_len), 0);
    std::vector<char> name_buf(name_len, '\0');
    ASSERT_EQ(nvs.get_str(handle, "device_name", name_buf.data(), &name_len),
              0);
    EXPECT_STREQ(name_buf.data(), "demo-board");

    size_t blob_len = 0;
    ASSERT_EQ(nvs.get_blob(handle, "calib", nullptr, &blob_len), 0);
    std::vector<char> blob_buf(blob_len);
    ASSERT_EQ(nvs.get_blob(handle, "calib", blob_buf.data(), &blob_len), 0);
    ASSERT_EQ(blob_len, sizeof(blob_data));
    EXPECT_EQ(std::memcmp(blob_buf.data(), blob_data, blob_len), 0);

    nvs.close(handle);
  }

  std::remove(path.c_str());
}

TEST(FakeEsp32NvsJsonTest, LoadFromGoldenJson) {
  const char* kGoldenJson = R"json({
  "partitions": {
    "nvs": {
      "name_spaces": {
        "settings": {
          "entries": {
            "volume": {
              "type": "U32",
              "uint": 7
            },
            "device_name": {
              "type": "STR",
              "str": "demo-board"
            },
            "calib": {
              "type": "BLOB",
              "blob_hex": "01020304"
            }
          }
        }
      }
    }
  }
})json";

  std::string path = MakeTempFile();
  {
    std::ofstream output(path);
    output << kGoldenJson;
  }

  Nvs nvs(path);
  nvs_handle_t handle = 0;
  ASSERT_EQ(nvs.open("nvs", "settings", false, &handle), 0);

  uint32_t volume = 0;
  ASSERT_EQ(nvs.get_u32(handle, "volume", &volume), 0);
  EXPECT_EQ(volume, 7u);

  size_t name_len = 0;
  ASSERT_EQ(nvs.get_str(handle, "device_name", nullptr, &name_len), 0);
  std::vector<char> name_buf(name_len, '\0');
  ASSERT_EQ(nvs.get_str(handle, "device_name", name_buf.data(), &name_len), 0);
  EXPECT_STREQ(name_buf.data(), "demo-board");

  size_t blob_len = 0;
  ASSERT_EQ(nvs.get_blob(handle, "calib", nullptr, &blob_len), 0);
  std::vector<char> blob_buf(blob_len);
  ASSERT_EQ(nvs.get_blob(handle, "calib", blob_buf.data(), &blob_len), 0);
  ASSERT_EQ(blob_len, 4u);
  const unsigned char blob_data[] = {0x01, 0x02, 0x03, 0x04};
  EXPECT_EQ(std::memcmp(blob_buf.data(), blob_data, blob_len), 0);

  nvs.close(handle);
  std::remove(path.c_str());
}
