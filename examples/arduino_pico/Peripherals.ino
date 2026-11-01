#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#ifdef ROO_TESTING
#include "glog/logging.h"
#include "roo_testing/buses/uart/fake_uart.h"
#include "roo_testing/devices/clock/ds3231/ds3231.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

namespace {
/// Supplies a deterministic full-duplex SPI response for this host example.
class Echo : public SimpleFakeSpiDevice {
 public:
  /// Creates a loopback SPI model.
  Echo() : SimpleFakeSpiDevice("loopback") {}
  /// Leaves transmitted bytes as the response.
  void transfer(const FakeSpiInterface&, uint8_t*, uint16_t) override {}
};
FakeUartCable cable;
FakeDs3231 rtc;
Echo spi;
}  // namespace
#endif

void setup() {
#ifdef ROO_TESTING
  FakePico().attachUartDevice(cable.end_a(), 0, 1);
  FakePico().attachUartDevice(cable.end_b(), 8, 9);
  FakePico().attachI2cDevice(rtc, 4, 5);
  FakePico().attachSpiDevice(spi, 18, 16, 19);
  FakePico().attachOutput(17, spi.cs());
#endif
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(17, OUTPUT);
  digitalWrite(17, HIGH);
  Serial1.begin(115200);
  Serial2.begin(115200);
  Wire.begin();
  SPI.begin();
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  Serial1.println("Pico");
  const String received = Serial2.readStringUntil('\n');
  Wire.beginTransmission(0x68);
  Wire.write(0x11);
  const uint8_t status = Wire.endTransmission(false);
  const size_t count = Wire.requestFrom(0x68, size_t{2});
  const int temperature = Wire.read();
  Wire.read();
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(17, LOW);
  const uint8_t reply = SPI.transfer(0x5a);
  digitalWrite(17, HIGH);
  SPI.endTransaction();
#ifdef ROO_TESTING
  CHECK_EQ(received, "Pico\r");
  CHECK_EQ(status, 0);
  CHECK_EQ(count, 2u);
  CHECK_EQ(temperature, 25);
  CHECK_EQ(reply, 0x5a);
#endif
  Serial.println("Pico blink/UART/DS3231/SPI cycle complete");
  delay(10);
  digitalWrite(LED_BUILTIN, LOW);
  delay(10);
}
