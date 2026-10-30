#pragma once

#include <limits.h>
#include <unistd.h>

#include <cstdlib>
#include <string>

namespace roo_testing::internal {

/// Keeps bazel-run preferences in the workspace, with cwd as the fallback.
/// Returns an empty path if the working directory cannot be resolved.
inline std::string DefaultNvsPath() {
  const char* workspace = std::getenv("BUILD_WORKSPACE_DIRECTORY");
  if (workspace != nullptr && workspace[0] != '\0') {
    return std::string(workspace) + "/nvs.txt";
  }
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == nullptr) return {};
  return std::string(cwd) + "/nvs.txt";
}

}  // namespace roo_testing::internal
