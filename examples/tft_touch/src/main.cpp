#include <Arduino.h>

#ifdef ROO_TESTING

#include "roo_testing/buses/gpio/fake_gpio.h"
#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/devices/display_ili9486/ili9486spi.h"
#include "roo_testing/devices/touch_xpt2046/touch_xpt2046spi.h"
#include "roo_testing/transducers/ui/viewport/flex_viewport.h"
#include "roo_testing/transducers/ui/viewport/fltk/fltk_viewport.h"

using namespace roo_testing_transducers;

struct Emulator {
  FltkViewport viewport;
  FlexViewport flexViewport;
  FakeIli9486Spi display;
  FakeXpt2046Spi touch;

  Emulator()
      : viewport(),
        flexViewport(viewport, 2),
        display(5, 2, 4, flexViewport),
        touch(15, flexViewport,
              FakeXpt2046Spi::Calibration(322, 196, 3899, 3691)) {
    FakeEsp32().attachSpiDevice(display, 18, 19, 23);
    FakeEsp32().attachSpiDevice(touch, 18, 19, 23);
  }
} emulator;

#endif

#include "SPI.h"
#include "roo_display.h"
#include "roo_display/driver/ili9486.h"
#include "roo_display/driver/touch_xpt2046.h"
#include "roo_display/shape/basic_shapes.h"

using namespace roo_display;

Ili9486spi<5, 2, 4> device;
TouchXpt2046Uncalibrated<15> touch_raw;
TouchXpt2046<15> touch(TouchCalibration(322, 196, 3899, 3691));

Display display(&device, &touch);

int16_t x, y;
bool was_touched;

void setup() {
  // SPI.begin(18, 19, 23, -1);
  SPI.begin();
  Serial.begin(9600);
  display.init(color::DarkGray);
  x = -1;
  y = -1;
  was_touched = false;
}

void loop(void) {
  int16_t old_x = x;
  int16_t old_y = y;
  unsigned long start = millis();
  bool touched = display.getTouch(&x, &y);
  if (touched) {
    was_touched = true;
    DrawingContext dc(&display);
    dc.draw(Line(0, y, display.width() - 1, y, color::Red));
    dc.draw(Line(x, 0, x, display.height() - 1, color::Red));
    if (x != old_x) {
      dc.draw(Line(old_x, 0, old_x, display.height() - 1, color::DarkGray));
    }
    if (y != old_y) {
      dc.draw(Line(0, old_y, display.width() - 1, old_y, color::DarkGray));
    }
    dc.draw(Line(0, y, display.width() - 1, y, color::Red));
    dc.draw(Line(x, 0, x, display.height() - 1, color::Red));

    Serial.println(String("Touched: ") + x + ", " + y);
  } else {
    if (was_touched) {
      Serial.println("Released!");
      was_touched = false;
      DrawingContext dc(&display);
      dc.draw(Line(0, old_y, display.width() - 1, old_y, color::DarkGray));
      dc.draw(Line(old_x, 0, old_x, display.height() - 1, color::DarkGray));
    }
  }
}
