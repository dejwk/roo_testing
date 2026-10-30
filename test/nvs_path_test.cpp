#include "roo_testing/microcontrollers/esp32/nvs_path.h"

#include <cstdlib>
#include <filesystem>
#include <string>

#include "gtest/gtest.h"

namespace {
class NvsPathTest : public testing::Test {
 protected:
  void SetUp() override {
    cwd_ = std::filesystem::current_path();
    const char* workspace = std::getenv("BUILD_WORKSPACE_DIRECTORY");
    had_workspace_ = workspace != nullptr;
    if (had_workspace_) workspace_ = workspace;
  }

  void TearDown() override {
    std::filesystem::current_path(cwd_);
    if (had_workspace_) {
      setenv("BUILD_WORKSPACE_DIRECTORY", workspace_.c_str(), 1);
    } else {
      unsetenv("BUILD_WORKSPACE_DIRECTORY");
    }
  }

  std::filesystem::path cwd_;
  std::string workspace_;
  bool had_workspace_ = false;
};

// Verifies rebuilds and configuration changes cannot redirect saved pairings
// into a different disposable runfiles tree.
TEST_F(NvsPathTest, WorkspaceSurvivesRunfilesDirectoryChanges) {
  const std::filesystem::path root =
      std::filesystem::path(testing::TempDir()) / "nvs_path";
  const std::filesystem::path workspace = root / "workspace";
  setenv("BUILD_WORKSPACE_DIRECTORY", workspace.c_str(), 1);
  for (const char* directory : {"fastbuild.runfiles", "dbg.runfiles"}) {
    std::filesystem::create_directories(root / directory);
    std::filesystem::current_path(root / directory);
    EXPECT_EQ((workspace / "nvs.txt").string(),
              roo_testing::internal::DefaultNvsPath());
  }
}

// Verifies direct launches and test sandboxes retain cwd-local preferences.
TEST_F(NvsPathTest, DirectLaunchUsesWorkingDirectory) {
  unsetenv("BUILD_WORKSPACE_DIRECTORY");
  EXPECT_EQ((cwd_ / "nvs.txt").string(),
            roo_testing::internal::DefaultNvsPath());
  setenv("BUILD_WORKSPACE_DIRECTORY", "", 1);
  EXPECT_EQ((cwd_ / "nvs.txt").string(),
            roo_testing::internal::DefaultNvsPath());
}
}  // namespace
