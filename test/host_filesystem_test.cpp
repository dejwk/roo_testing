#include <cstdlib>
#include <filesystem>
#include <string>

#include "LittleFS.h"
#include "SPIFFS.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "gtest/gtest.h"

namespace {

class ScopedFilesystemRoot {
public:
  ScopedFilesystemRoot() : previous_(FakeEsp32().fs_root()) {
    char path[] = "/tmp/roo_testing_fsXXXXXX";
    root_ = mkdtemp(path);
    FakeEsp32().set_fs_root(root_);
  }

  ~ScopedFilesystemRoot() {
    FakeEsp32().set_fs_root(previous_);
    std::error_code error;
    std::filesystem::remove_all(root_, error);
  }

  const std::string &root() const { return root_; }

private:
  std::string previous_;
  std::string root_;
};

TEST(HostFilesystemTest, RedirectsSpiffsOperationsUnderFakeEsp32Root) {
  ScopedFilesystemRoot root;
  ASSERT_TRUE(std::filesystem::create_directory(root.root() + "/spiffs"));
  ASSERT_TRUE(SPIFFS.begin());

  File file = SPIFFS.open("/config/settings.txt", FILE_WRITE, true);
  ASSERT_TRUE(file);
  const std::string contents = "host-backed";
  EXPECT_EQ(contents.size(),
            file.write(reinterpret_cast<const uint8_t *>(contents.data()),
                       contents.size()));
  file.close();

  EXPECT_TRUE(std::filesystem::is_regular_file(root.root() +
                                               "/spiffs/config/settings.txt"));
  EXPECT_TRUE(SPIFFS.exists("/config/settings.txt"));
  EXPECT_TRUE(SPIFFS.rename("/config/settings.txt", "/config/current.txt"));

  file = SPIFFS.open("/config/current.txt", FILE_READ);
  ASSERT_TRUE(file);
  std::string actual(contents.size(), '\0');
  EXPECT_EQ(actual.size(), file.read(reinterpret_cast<uint8_t *>(actual.data()),
                                     actual.size()));
  EXPECT_EQ(contents, actual);
  file.close();

  EXPECT_TRUE(SPIFFS.remove("/config/current.txt"));
  SPIFFS.end();
  EXPECT_FALSE(SPIFFS.open("/anything", FILE_READ));
}

TEST(HostFilesystemTest, ProvidesHostLittleFsMountAndDirectorySemantics) {
  ScopedFilesystemRoot root;
  ASSERT_TRUE(std::filesystem::create_directory(root.root() + "/littlefs"));
  ASSERT_TRUE(LittleFS.begin());

  EXPECT_TRUE(LittleFS.mkdir("/cache"));
  EXPECT_TRUE(std::filesystem::is_directory(root.root() + "/littlefs/cache"));
  EXPECT_TRUE(LittleFS.rmdir("/cache"));
  EXPECT_TRUE(LittleFS.format());
  EXPECT_EQ(0, LittleFS.totalBytes());
  EXPECT_EQ(0, LittleFS.usedBytes());

  LittleFS.end();
}

} // namespace
