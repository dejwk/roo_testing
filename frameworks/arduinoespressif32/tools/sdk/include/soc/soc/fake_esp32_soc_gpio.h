#pragma once

#include <stdint.h>

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

  explicit operator uint32_t() const {
    return uint32_t(Esp32GpioInReadSpec());
  }

 private:
  uint32_t dummy;
};

class Esp32GpioIn1ReadSpec {
 public:
  Esp32GpioIn1ReadSpec() : mask(0x000000FF), shift(0) {}

  Esp32GpioIn1ReadSpec(uint32_t mask, int8_t shift) : mask(mask), shift(shift) {}

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
