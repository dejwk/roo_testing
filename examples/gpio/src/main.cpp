#include <Arduino.h>

#ifdef ROO_TESTING

#include "roo_testing/transducers/voltage/voltage.h"

using namespace roo_testing_transducers;

ConstVoltage digital_input(0.0);

SimpleVoltageSource sawtooth([]() -> float {
  // Generate chainsaw from 0 to 2V, period of 10s.
  return ((millis() / 10 % 1000) / 500.0);
});

SimpleDigitalSink trigger("trigger", [](DigitalLevel level) {
  Serial.printf("Trigger detected; value: %d\n", level);
  digital_input.set(level);
});

class Trigger : public SimpleFakeGpioPin {
 public:
  Trigger() : SimpleFakeGpioPin("trigger") {}
};

// Here we attach signals to ESP32 pins.
struct Emulator {
  Emulator() {
    FakeEsp32().gpio.attachInput(33, digital_input);
    FakeEsp32().gpio.attachInput(34, Ground());
    FakeEsp32().gpio.attachInput(35, Vcc33());
    FakeEsp32().gpio.attachInput(36, sawtooth);

    // When this pin gets written to, we update the value attached to pin 33.
    // Essentially, the pin #4 is 'soldered' to pin #33.
    FakeEsp32().gpio.attach(4, trigger);
  }
} emulator;

#endif  // ROO_TESTING

void setup() {
  Serial.begin(9600);
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);

  pinMode(33, INPUT);
  pinMode(34, INPUT);
  pinMode(35, INPUT);
  pinMode(36, ANALOG);
}

void loop() {
  Serial.println();
  // Trivial output pins do not need to be configured.
  digitalWrite(2, HIGH);
  Serial.printf("digital value on pin #2: %d\n", digitalRead(2));
  Serial.println();

  // Now we're updating the value on the pin that is 'soldered'
  // (via the trigger attachment) to pin #33, so it will change
  // the digital readout we see in the following line.
  digitalWrite(4, digitalRead(4) == HIGH ? LOW : HIGH);
  Serial.printf("digital value on pin #4: %d\n", digitalRead(4));

  // The same value should be now on the input:
  Serial.printf("digital input:           %d\n", digitalRead(33));

  Serial.println();
  Serial.printf("ground: %d\n", digitalRead(34));
  Serial.printf("VCC: %d\n", digitalRead(35));
  Serial.printf("analog input: %d\n", analogRead(36));
  delay(1000);
}
