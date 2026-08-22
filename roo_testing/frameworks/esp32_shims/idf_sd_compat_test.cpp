#include <cstdlib>
#include <filesystem>
#include <string>

#include "driver/sdmmc_types.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "gtest/gtest.h"

namespace {

class TempDirectory {
 public:
  TempDirectory() {
    char path[] = "/tmp/roo_testing_sdXXXXXX";
    root_ = mkdtemp(path);
  }

  ~TempDirectory() {
    std::error_code error;
    std::filesystem::remove_all(root_, error);
  }

  const char* path() const { return root_.c_str(); }

 private:
  std::string root_;
};

TEST(IdfSdCompatTest, MountsAndUnmountsHostDirectories) {
  TempDirectory spi_root;
  TempDirectory mmc_root;
  sdmmc_host_t host = {};
  sdspi_device_config_t spi_slot = {};
  int mmc_slot = 0;
  esp_vfs_fat_mount_config_t config = {};
  sdmmc_card_t* spi_card = nullptr;
  sdmmc_card_t* mmc_card = nullptr;

  EXPECT_EQ(ESP_OK, esp_vfs_fat_sdspi_mount(spi_root.path(), &host, &spi_slot,
                                            &config, &spi_card));
  ASSERT_NE(nullptr, spi_card);
  EXPECT_EQ(ESP_OK, esp_vfs_fat_sdmmc_mount(mmc_root.path(), &host, &mmc_slot,
                                            &config, &mmc_card));
  ASSERT_NE(nullptr, mmc_card);

  sdmmc_card_t* duplicate = nullptr;
  EXPECT_EQ(ESP_ERR_INVALID_STATE,
            esp_vfs_fat_sdspi_mount(spi_root.path(), &host, &spi_slot, &config,
                                    &duplicate));
  EXPECT_EQ(nullptr, duplicate);

  EXPECT_EQ(ESP_OK, esp_vfs_fat_sdcard_unmount(spi_root.path(), spi_card));
  EXPECT_EQ(ESP_OK, esp_vfs_fat_sdcard_unmount(mmc_root.path(), mmc_card));
}

TEST(IdfSdCompatTest, RejectsMissingDirectoriesAndInvalidArguments) {
  TempDirectory root;
  const std::string missing = std::string(root.path()) + "/missing";
  sdmmc_host_t host = {};
  sdspi_device_config_t slot = {};
  esp_vfs_fat_mount_config_t config = {};
  sdmmc_card_t* card = nullptr;

  EXPECT_EQ(ESP_FAIL, esp_vfs_fat_sdspi_mount(missing.c_str(), &host, &slot,
                                              &config, &card));
  EXPECT_EQ(nullptr, card);
  EXPECT_EQ(ESP_ERR_INVALID_ARG,
            esp_vfs_fat_sdspi_mount(nullptr, &host, &slot, &config, &card));
  EXPECT_EQ(ESP_ERR_INVALID_ARG, esp_vfs_fat_sdcard_unmount("/tmp", nullptr));
}

}  // namespace
