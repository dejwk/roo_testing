#include "sd_diskio.h"

#include "gtest/gtest.h"

TEST(ArduinoSdDiskioLinkTest, RejectsUnregisteredHostDrive) {
  EXPECT_EQ(1, sdcard_unmount(0));
  EXPECT_EQ(1, sdcard_uninit(0));
}
