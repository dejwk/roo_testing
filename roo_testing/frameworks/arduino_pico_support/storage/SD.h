#pragma once
#include <SDFS.h>
#include <SPI.h>

/// Supported Arduino-Pico SD shim: SPI begin/end and FS operations use SDFS.
/// The host model records configuration without emulating SD wire traffic.
class SDClass {
 public:
  /// Mounts SDFS with @p cs, maximum @p frequency Hz and borrowed @p spi.
  bool begin(uint8_t cs, uint32_t frequency = 4000000, HardwareSPI& spi = SPI) {
    cs_ = cs;
    frequency_ = frequency;
    spi_ = &spi;
    return frequency != 0 && SDFS.begin();
  }
  /// Uses the core's compatibility overload and default frequency.
  bool begin(uint8_t cs, HardwareSPI& spi) { return begin(cs, 4000000, spi); }
  /// Unmounts SDFS, optionally ending the borrowed controller.
  void end(bool end_spi = true) {
    SDFS.end();
    if (end_spi && spi_ != nullptr) spi_->end();
  }
  /// Returns the last chip select used by begin().
  uint8_t chipSelect() const { return cs_; }
  /// Returns the last requested frequency.
  uint32_t frequency() const { return frequency_; }
  /// Returns the borrowed SPI controller from the last begin().
  HardwareSPI* spi() const { return spi_; }

 private:
  uint8_t cs_ = 0;
  uint32_t frequency_ = 0;
  HardwareSPI* spi_ = nullptr;
};

extern SDClass SD;
