#include <Arduino.h>

#ifdef ROO_TESTING

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

#include "roo_testing/buses/onewire/OneWire.h"
#include "roo_testing/buses/onewire/fake_onewire.h"
#include "roo_testing/devices/onewire/thermometer/thermometer.h"

using namespace roo_testing_transducers;

FunctionThermometer t1([]() -> Temperature {
  return Temperature::FromC(25 + 20 * sin(millis() / 500000.0));
});

FixedThermometer t2(Temperature::FromC(18.0));

FakeOneWireInterface fake_onewire({
    new FakeOneWireThermometer("28884B9B0A0000D3", t1),
    new FakeOneWireThermometer("286E1D990A000046", t2),
});

FakeOneWire oneWire(&fake_onewire);

#else

// Note: OneWire is not (yet) emulated at the electric signal level, and
// therefore we must use a customized OneWire header "fake_onewire.h", above,
// rather than the standard header, below. If you include "OneWire.h" in files
// other than main.cpp, you also need to include the #ifdef guards to
// conditionally replace it with "fake_onewire.h".

#include "OneWire.h"
OneWire oneWire(25);

#endif

#include "DallasTemperature.h"

DallasTemperature dt(&oneWire);

void setup() {
  Serial.begin(9600);
  dt.begin();
  Serial.printf("Detected %d thermometers.\n", dt.getDeviceCount());
  dt.setWaitForConversion(true);
}

void loop() {
  int count = dt.getDeviceCount();
  dt.requestTemperatures();
  for (int i = 0; i < count; ++i) {
    Serial.printf("Temperature %d is: %f°C\n", i, dt.getTempCByIndex(i));
  }
  Serial.println();
}
