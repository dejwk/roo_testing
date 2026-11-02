#pragma once
#include <FS.h>

#include <memory>
#include <string>
namespace roo_testing {
struct PicoFilesystemState;

/// Behavioral Pico FS fake over a uniquely owned native directory. It uses
/// the imported FS/File/Dir API; it does not model SD signals or flash wear.
/// Calls require external serialization, as do the Arduino-Pico FS wrappers.
class PicoFilesystem : public fs::FS {
 public:
  /// Creates isolated empty storage. When @p prune_empty_parents is true,
  /// remove/rmdir also delete empty ancestors, matching Arduino-Pico LittleFS.
  explicit PicoFilesystem(bool prune_empty_parents = false);
  /// Ends the backend and removes its isolated directory after file references
  /// are released. Close files before destroying the FS object (core contract).
  ~PicoFilesystem();
  /// Storage identity cannot be copied.
  PicoFilesystem(const PicoFilesystem&) = delete;
  PicoFilesystem& operator=(const PicoFilesystem&) = delete;
  /// Injects media removal; operations on existing handles then fail.
  void setMediaPresent(bool present);
  /// Injects begin() failure without changing persistent contents.
  void setMountFailure(bool fail);
  /// Reports whether the emulated backend is mounted.
  bool mounted() const;
  /// Returns the isolated host root for assertions and test fixture setup.
  const std::string& hostRoot() const;

 private:
  explicit PicoFilesystem(std::shared_ptr<PicoFilesystemState> state);
  std::shared_ptr<PicoFilesystemState> state_;
};
}  // namespace roo_testing
