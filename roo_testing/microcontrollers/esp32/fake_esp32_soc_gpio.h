#pragma once

#include <stdint.h>

#include "esp_bit_defs.h"
#include "soc/soc_caps.h"

class Esp32GpioOutW1tc {
 public:
  uint32_t operator=(uint32_t val) const;

 private:
  uint32_t dummy;
};

class Esp32GpioOutW1ts {
 public:
  uint32_t operator=(uint32_t val) const;

 private:
  uint32_t dummy;
};

class Esp32GpioOut1W1tc {
 public:
  uint32_t operator=(uint32_t val) const;

 private:
  uint32_t dummy;
};

class Esp32GpioOut1W1ts {
 public:
  uint32_t operator=(uint32_t val) const;

 private:
  uint32_t dummy;
};

class Esp32GpioInReadSpec {
 public:
  Esp32GpioInReadSpec() : mask(0xFFFFFFFF), shift(0) {}

  Esp32GpioInReadSpec(uint32_t mask, int8_t shift) : mask(mask), shift(shift) {}

  Esp32GpioInReadSpec operator>>(uint32_t shift) const {
    int8_t newshift = this->shift + shift;
    return Esp32GpioInReadSpec((mask >> newshift) << newshift, newshift);
  }

  Esp32GpioInReadSpec operator&(uint32_t sel) const {
    return Esp32GpioInReadSpec(((mask >> shift) & sel) << shift, shift);
  }

  explicit operator uint32_t() const;

 private:
  uint32_t mask;
  int8_t shift;
};

class Esp32GpioIn {
 public:
  Esp32GpioIn() {}

  Esp32GpioInReadSpec operator>>(uint32_t shift) const {
    return Esp32GpioInReadSpec() >> shift;
  }

  Esp32GpioInReadSpec operator&(uint32_t sel) const {
    return Esp32GpioInReadSpec() & sel;
  }

  explicit operator uint32_t() const { return uint32_t(Esp32GpioInReadSpec()); }

 private:
  uint32_t dummy;
};

class Esp32GpioIn1ReadSpec {
 public:
  Esp32GpioIn1ReadSpec()
      : mask(static_cast<uint32_t>(uint64_t{SOC_GPIO_VALID_GPIO_MASK} >> 32)),
        shift(0) {}

  Esp32GpioIn1ReadSpec(uint32_t mask, int8_t shift)
      : mask(mask), shift(shift) {}

  Esp32GpioIn1ReadSpec operator>>(uint32_t shift) const {
    int8_t newshift = this->shift + shift;
    return Esp32GpioIn1ReadSpec((mask >> newshift) << newshift, newshift);
  }

  Esp32GpioIn1ReadSpec operator&(uint32_t sel) const {
    return Esp32GpioIn1ReadSpec(((mask >> shift) & sel) << shift, shift);
  }

  explicit operator uint32_t() const;

 private:
  uint32_t mask;
  int8_t shift;
};

class Esp32GpioIn1 {
 public:
  Esp32GpioIn1() {}

  Esp32GpioIn1ReadSpec operator>>(uint32_t shift) const {
    return Esp32GpioIn1ReadSpec() >> shift;
  }

  Esp32GpioIn1ReadSpec operator&(uint32_t sel) const {
    return Esp32GpioIn1ReadSpec() & sel;
  }

  explicit operator uint32_t() const {
    return uint32_t(Esp32GpioIn1ReadSpec());
  }

 private:
  uint32_t dummy;
};

/// Intercepts both scalar and union-style GPIO output register assignments.
template <typename Proxy>
struct Esp32GpioWriteRegister {
  /// Drives the GPIO proxy using a write-one-to-set or write-one-to-clear mask.
  uint32_t operator=(uint32_t value) const { return val.operator=(value); }

  Proxy val;
};

/// Samples both scalar and union-style GPIO input register expressions.
template <typename Proxy>
struct Esp32GpioReadRegister {
  /// Defers a shifted input sample to the underlying GPIO proxy.
  auto operator>>(uint32_t shift) const { return val >> shift; }

  /// Defers a masked input sample to the underlying GPIO proxy.
  auto operator&(uint32_t mask) const { return val & mask; }

  /// Samples the current GPIO input levels.
  explicit operator uint32_t() const { return static_cast<uint32_t>(val); }

  Proxy val;
};
