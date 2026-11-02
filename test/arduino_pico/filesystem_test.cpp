#include "roo_testing/frameworks/arduino_pico_support/filesystem.h"

#include <LittleFS.h>
#include <SD.h>
#include <unistd.h>

#include "gtest/gtest.h"

namespace {
// Verifies the real upstream FS/File/Dir wrappers retain data through remount,
// isolate roots and reject paths that escape through parents or symlinks.
TEST(PicoFilesystem, FileDirectoryAndIsolation) {
  roo_testing::PicoFilesystem a;
  roo_testing::PicoFilesystem b;
  ASSERT_NE(a.hostRoot(), b.hostRoot());
  ASSERT_TRUE(a.begin());
  ASSERT_TRUE(b.begin());
  ASSERT_TRUE(a.mkdir("/dir"));
  fs::File file = a.open("/dir/data", "w+");
  ASSERT_TRUE(static_cast<bool>(file));
  ASSERT_EQ(file.write(reinterpret_cast<const uint8_t*>("abcd"), 4), 4u);
  EXPECT_TRUE(file.truncate(3));
  EXPECT_EQ(file.size(), 3u);
  EXPECT_TRUE(file.seek(0));
  EXPECT_EQ(file.read(), 'a');
  file.close();
  fs::FSInfo info;
  ASSERT_TRUE(a.info(info));
  EXPECT_EQ(info.usedBytes, 3u);
  EXPECT_FALSE(b.exists("/dir/data"));
  a.end();
  ASSERT_TRUE(a.begin());
  EXPECT_TRUE(a.exists("/dir/data"));
  fs::Dir dir = a.openDir("/dir");
  ASSERT_TRUE(dir.next());
  EXPECT_STREQ(dir.fileName().c_str(), "/dir/data");
  EXPECT_FALSE(dir.next());
  EXPECT_TRUE(dir.rewind());
  EXPECT_TRUE(dir.next());
  EXPECT_FALSE(a.exists("/../data"));
  ASSERT_EQ(symlink(b.hostRoot().c_str(), (a.hostRoot() + "/escape").c_str()),
            0);
  EXPECT_FALSE(static_cast<bool>(a.open("/escape/data", "w")));
  EXPECT_TRUE(a.rename("/dir/data", "/dir/new"));
  EXPECT_FALSE(a.rmdir("/dir"));
  EXPECT_TRUE(a.remove("/dir/new"));
  EXPECT_TRUE(a.rmdir("/dir"));
}

// Verifies injected media loss invalidates open I/O, mount errors preserve
// data, formatting is explicit and roots disappear with the last owner.
TEST(PicoFilesystem, FailuresAndLifetime) {
  std::string root;
  {
    roo_testing::PicoFilesystem fs;
    root = fs.hostRoot();
    fs.setMountFailure(true);
    EXPECT_FALSE(fs.begin());
    fs.setMountFailure(false);
    ASSERT_TRUE(fs.begin());
    fs::File file = fs.open("/data", "w");
    ASSERT_TRUE(static_cast<bool>(file));
    fs.setMediaPresent(false);
    EXPECT_EQ(file.write(uint8_t{1}), 0u);
    EXPECT_FALSE(fs.mounted());
    file.close();
    fs.end();
    fs.setMediaPresent(true);
    ASSERT_TRUE(fs.begin());
    EXPECT_TRUE(fs.exists("/data"));
    EXPECT_FALSE(fs.format());
    fs.end();
    EXPECT_TRUE(fs.format());
    EXPECT_TRUE(fs.begin());
    EXPECT_FALSE(fs.exists("/data"));
  }
  EXPECT_NE(access(root.c_str(), F_OK), 0);
}

// Verifies LittleFS's native deletion side effect differs from SDFS: the last
// file removal prunes empty ancestors while the generic fake retains them.
TEST(PicoFilesystem, LittleFsPrunesEmptyParents) {
  roo_testing::PicoFilesystem flash(true);
  ASSERT_TRUE(flash.begin());
  ASSERT_TRUE(flash.mkdir("/parent"));
  ASSERT_TRUE(flash.mkdir("/parent/child"));
  fs::File file = flash.open("/parent/child/data", "w");
  ASSERT_TRUE(static_cast<bool>(file));
  file.close();
  ASSERT_TRUE(flash.remove("/parent/child/data"));
  EXPECT_FALSE(flash.exists("/parent"));
  ASSERT_TRUE(flash.mkdir("/parent"));
  ASSERT_TRUE(flash.mkdir("/parent/child"));
  ASSERT_TRUE(flash.rmdir("/parent/child"));
  EXPECT_FALSE(flash.exists("/parent"));
  file = flash.open("/file", "w");
  ASSERT_TRUE(static_cast<bool>(file));
  file.close();
  // LittleFS aliases raw rmdir to remove; Roo validates the entry type first.
  EXPECT_TRUE(flash.rmdir("/file"));
}

}  // namespace
