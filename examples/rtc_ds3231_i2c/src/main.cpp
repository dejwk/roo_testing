#include <Arduino.h>

#ifdef ROO_TESTING

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

#include "roo_testing/devices/clock/ds3231/ds3231.h"

struct Emulator {
  FakeDs3231 rtc;
  Emulator() {
    FakeEsp32().attachI2cDevice(rtc, 21, 22);
  }
} emulator;

#endif

#include "DS3231.h"

DS3231 rtc;

void setup() {
  Serial.begin(9600);
  rtc.begin();

  // Setting the clock to some arbitrary time.
  rtc.setDateTime(2022, 02, 21, 13, 55, 40);
}

void loop(void) {
  auto now = rtc.getDateTime();
  // NOTE: the dateFormat functions have a bug: they return a pointer to
  // an object that has been already destroyed.
  // Serial.println(rtc.dateFormat("d-m-Y H:i:s", now));

  Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n", now.year, now.month, now.day,
                now.hour, now.minute, now.second);

  delay(1000);
}
