#include <Arduino.h>

#if CONFIG_IDF_TARGET_ESP32
constexpr uint8_t kInputPin = 33;
constexpr uint8_t kGroundPin = 34;
constexpr uint8_t kVccPin = 35;
constexpr uint8_t kAnalogPin = 36;
#else
constexpr uint8_t kInputPin = 5;
constexpr uint8_t kGroundPin = 6;
constexpr uint8_t kVccPin = 7;
constexpr uint8_t kAnalogPin = 1;
#endif

#ifdef ROO_TESTING

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/transducers/voltage/voltage.h"

using namespace roo_testing_transducers;

ConstVoltage digital_input(0.0);

SimpleVoltageSource sawtooth([]() -> float {
  // Generate chainsaw from 0 to 2V, period of 10s.
  return ((millis() / 10 % 1000) / 500.0);
});

SimpleDigitalSink trigger = SimpleDigitalSink::WithSignalCallback(
    "trigger", [](const VoltageSignal& signal) {
      const DigitalLevel level =
          DigitalLevelFromVoltage(AverageDcVoltage(signal));
      Serial.printf("Trigger detected; value: %d\n", level);
      digital_input.set(level);
    });

// Here we attach signals to ESP32 pins.
struct Emulator {
  Emulator() {
    FakeEsp32().gpio.attachInput(kInputPin, digital_input);
    FakeEsp32().gpio.attachInput(kGroundPin, Ground());
    FakeEsp32().gpio.attachInput(kVccPin, Vcc33());
    FakeEsp32().gpio.attachInput(kAnalogPin, sawtooth);

    // When this pin gets written to, we update the value attached to the input
    // pin. Essentially, the pin #4 is 'soldered' to the input pin.
    FakeEsp32().gpio.attachOutput(4, trigger);
  }
} emulator;

#endif  // ROO_TESTING

void setup() {
  Serial.begin(9600);
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);

  pinMode(kInputPin, INPUT);
  pinMode(kGroundPin, INPUT);
  pinMode(kVccPin, INPUT);
  pinMode(kAnalogPin, ANALOG);
}

void loop() {
  Serial.println();
  // Trivial output pins do not need to be configured.
  digitalWrite(2, HIGH);
  Serial.printf("digital value on pin #2: %d\n", digitalRead(2));
  Serial.println();

  // Now we're updating the value on the pin that is 'soldered'
  // (via the trigger attachment) to the input pin, so it will change
  // the digital readout we see in the following line.
  digitalWrite(4, digitalRead(4) == HIGH ? LOW : HIGH);
  Serial.printf("digital value on pin #4: %d\n", digitalRead(4));

  // The same value should be now on the input:
  Serial.printf("digital input:           %d\n", digitalRead(kInputPin));

  Serial.println();
  Serial.printf("ground: %d\n", digitalRead(kGroundPin));
  Serial.printf("VCC: %d\n", digitalRead(kVccPin));
  Serial.printf("analog input: %d\n", analogRead(kAnalogPin));
  delay(1000);
}
