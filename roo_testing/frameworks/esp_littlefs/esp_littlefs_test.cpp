#include "esp_littlefs.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {

class ScopedRoot {
 public:
  ScopedRoot() : previous_(FakeEsp32().fs_root()) {
    char path[] = "/tmp/roo_littlefs_testXXXXXX";
    root_ = mkdtemp(path);
    FakeEsp32().set_fs_root(root_);
  }

  ~ScopedRoot() {
    FakeEsp32().set_fs_root(previous_);
    std::error_code error;
    std::filesystem::remove_all(root_, error);
  }

  const std::string& path() const { return root_; }

 private:
  std::string previous_;
  std::string root_;
};

TEST(EspLittlefsTest, RegistersExistingDirectoryAndUnregisters) {
  ScopedRoot root;
  const std::string path = root.path() + "/littlefs";
  ASSERT_TRUE(std::filesystem::create_directory(path));
  const esp_vfs_littlefs_conf_t conf = {.base_path = path.c_str()};
  EXPECT_EQ(ESP_OK, esp_vfs_littlefs_register(&conf));
  EXPECT_TRUE(esp_littlefs_mounted(nullptr));
  EXPECT_EQ(ESP_ERR_INVALID_STATE, esp_vfs_littlefs_register(&conf));
  EXPECT_EQ(ESP_OK, esp_vfs_littlefs_unregister(nullptr));
  EXPECT_FALSE(esp_littlefs_mounted(nullptr));
  EXPECT_EQ(ESP_ERR_INVALID_STATE, esp_vfs_littlefs_unregister(nullptr));
}

TEST(EspLittlefsTest, MissingDirectoryCanBeFormatted) {
  ScopedRoot root;
  const std::string path = root.path() + "/littlefs";
  esp_vfs_littlefs_conf_t conf = {.base_path = path.c_str()};
  EXPECT_EQ(ESP_FAIL, esp_vfs_littlefs_register(&conf));
  conf.format_if_mount_failed = true;
  EXPECT_EQ(ESP_OK, esp_vfs_littlefs_register(&conf));
  EXPECT_TRUE(std::filesystem::is_directory(path));
  EXPECT_EQ(ESP_OK, esp_vfs_littlefs_unregister(nullptr));
}

TEST(EspLittlefsTest, FormattingClearsContentsAndKeepsMountPoint) {
  ScopedRoot root;
  const std::string path = root.path() + "/littlefs";
  ASSERT_TRUE(std::filesystem::create_directories(path + "/subdir"));
  std::ofstream(path + "/subdir/file") << "content";
  const esp_vfs_littlefs_conf_t conf = {.base_path = path.c_str()};
  ASSERT_EQ(ESP_OK, esp_vfs_littlefs_register(&conf));
  EXPECT_EQ(ESP_OK, esp_littlefs_format(nullptr));
  EXPECT_TRUE(std::filesystem::is_directory(path));
  EXPECT_TRUE(std::filesystem::is_empty(path));
  EXPECT_EQ(ESP_OK, esp_vfs_littlefs_unregister(nullptr));
}

}  // namespace
