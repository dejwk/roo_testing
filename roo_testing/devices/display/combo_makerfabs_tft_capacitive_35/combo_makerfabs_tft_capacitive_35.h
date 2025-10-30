#pragma once

#include "roo_testing/devices/display/ili9488/ili9488spi.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/transducers/ui/viewport/flex_viewport.h"
#include "roo_testing/devices/touch/ft6x36/ft6x36.h"

class FakeMakerfabsTftCapacitive35 {
 public:
  FakeMakerfabsTftCapacitive35(roo_testing_transducers::FlexViewport& viewport)
      : viewport_(viewport),
        display_(viewport_),
        touch_(viewport, FakeFt6x36::Calibration(0, 20, 309, 454)) {
    FakeEsp32().attachSpiDevice(display_, 14, 12, 13);
    FakeEsp32().gpio.attachOutput(15, display_.cs());
    FakeEsp32().gpio.attachOutput(33, display_.dc());
    FakeEsp32().gpio.attachOutput(26, display_.rst());
    FakeEsp32().attachI2cDevice(touch_, 26, 27);
  }

 private:
  roo_testing_transducers::FlexViewport& viewport_;
  FakeIli9488Spi display_;
  FakeFt6x36 touch_;
};
