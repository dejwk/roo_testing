#pragma once
#include "roo_testing/frameworks/arduino_pico_support/filesystem.h"
/// Matches the core's LittleFS configuration identity and auto-format flag.
class LittleFSConfig : public fs::FSConfig {
 public:
  /// Constructs a LittleFS config; host storage never formats implicitly.
  explicit LittleFSConfig(bool auto_format = true)
      : FSConfig(0x4c495454, auto_format) {}
};
/// Isolated flash storage; it persists through end()/begin() within the
/// process.
extern roo_testing::PicoFilesystem LittleFS;
