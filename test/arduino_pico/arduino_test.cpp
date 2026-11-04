#include "Arduino.h"

#include <vector>

#include "SPI.h"
#include "Wire.h"
#include "gtest/gtest.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/uart.h"
#include "roo_testing/buses/uart/fake_uart.h"
#include "roo_testing/devices/clock/ds3231/ds3231.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"
#include "roo_testing/system/timer.h"

#if !ARDUINO_ARCH_RP2040 || !PICO_RP2040 || PICO_RP2350 || \
    !ROO_TESTING_RUNTIME_BARE_METAL
#error Incorrect Arduino Pico identity
#endif
#if defined(ESP32) || defined(ESP_PLATFORM) || defined(__FREERTOS) || \
    defined(ARDUINO_ARCH_ESP32)
#error Foreign frontend/runtime leaked into Pico
#endif

namespace {
/// Supplies deterministic wire traffic through the existing SPI device model.
class Echo : public SimpleFakeSpiDevice {
 public:
  /// Creates an SPI model which records and XORs transmitted bytes.
  Echo() : SimpleFakeSpiDevice("Arduino SPI") {}
  /// Replies under software CS, without knowing the calling frontend.
  void transfer(const FakeSpiInterface&, uint8_t* bytes,
                uint16_t bits) override {
    EXPECT_TRUE(isSelected());
    sent.insert(sent.end(), bytes, bytes + bits / 8);
    for (uint i = 0; i < bits / 8; ++i) bytes[i] ^= 0xa5;
  }
  std::vector<uint8_t> sent;
};

/// Owns immutable borrowed connections before any pin is configured.
struct Wiring {
  Wiring() : input(1.65f), led("LED"), pwm("PWM") {
    FakePico().attachInput(26, input);
    FakePico().attachOutput(25, led);
    FakePico().attachOutput(22, pwm);
    FakePico().attachUartDevice(cable.end_a(), 0, 1);
    FakePico().attachUartDevice(cable.end_b(), 8, 9);
    FakePico().attachI2cDevice(rtc, 4, 5);
    FakePico().attachSpiDevice(spi, 18, 16, 19);
    FakePico().attachOutput(17, spi.cs());
  }
  roo_testing_transducers::ConstVoltage input;
  roo_testing_transducers::SimpleVoltageSink led;
  roo_testing_transducers::SimpleVoltageSink pwm;
  FakeUartCable cable;
  FakeDs3231 rtc;
  Echo spi;
};
Wiring& Connections() {
  static Wiring wiring;
  return wiring;
}

class ArduinoPicoTest : public testing::Test {
 protected:
  void SetUp() override { Connections(); }
};

// Verifies illegal controller/pin mappings fail explicitly as the real core
// panics, rather than returning false and allowing the app to continue.
TEST(PicoArduinoWire, RejectIllegalPins) {
  Wire.end();
  EXPECT_DEATH(Wire.setSDA(6), "SDA to illegal pin");
  EXPECT_DEATH(Wire.setSCL(7), "SCL to illegal pin");
}

// Verifies real core String, Print, and Stream behavior over the shared UART,
// including timeout progress when the deterministic clock is stopped.
TEST_F(ArduinoPicoTest, PortableCoreAndUart) {
  Serial1.begin(115200);
  Serial2.begin(115200);
  const String message = String("value=") + String(42);
  EXPECT_EQ(Serial1.println(message), 10u);
  EXPECT_EQ(Serial2.peek(), 'v');
  EXPECT_EQ(Serial2.readStringUntil('\n'), "value=42\r");
  const uint8_t byte = 0x71;
  uart_write_blocking(uart1, &byte, 1);
  EXPECT_EQ(Serial1.read(), byte);
  Serial2.setTimeout(3);
  const uint64_t before = time_us_64();
  char buffer;
  EXPECT_EQ(Serial2.readBytes(&buffer, 1), 0u);
  EXPECT_GE(time_us_64() - before, 3000u);
  EXPECT_FALSE(Serial1.setTX(4));
  EXPECT_FALSE(Serial1.setPollingMode(false));
  Serial1.end();
  Serial2.end();
}

// Verifies Arduino and SDK APIs share the same pad, clock, ADC and PWM state.
TEST_F(ArduinoPicoTest, MixedGpioTimeAndAnalog) {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  EXPECT_TRUE(gpio_get(25));
  gpio_put(25, false);
  EXPECT_EQ(digitalRead(25), LOW);
  const uint64_t before = time_us_64();
  delay(7);
  EXPECT_EQ(time_us_64() - before, 7000u);
  EXPECT_EQ(micros(), static_cast<uint32_t>(time_us_64()));
  analogReadResolution(12);
  EXPECT_EQ(analogRead(A0), 2048);
  EXPECT_EQ(adc_read(), 2048);
  analogWriteRange(255);
  analogWriteFreq(1000);
  analogWrite(22, 64);
  EXPECT_TRUE(FakePico().pwmState(3).enabled);
  pwm_set_gpio_level(22, 128);
  EXPECT_EQ(FakePico().pwmState(3).levels[0], 128);
  EXPECT_GT(Connections().pwm.sampleAtUptimeMicros(time_us_64()), 3.0f);
  pwm_set_enabled(3, false);
}

// Verifies Arduino Wire shares the DS3231 model and repeated-START state with
// SDK.
TEST_F(ArduinoPicoTest, SharedRtcAndWireErrors) {
  Wire.begin();
  Wire.setClock(400000);
  Wire.beginTransmission(0x68);
  Wire.write(7);
  Wire.write(0x12);
  Wire.write(0x34);
  EXPECT_EQ(Wire.endTransmission(), 0);
  Wire.beginTransmission(0x68);
  Wire.write(7);
  ASSERT_EQ(Wire.endTransmission(false), 0);
  EXPECT_TRUE(FakePico().i2cState(0).restart_on_next);
  uint8_t response[2];
  ASSERT_EQ(i2c_read_blocking(i2c0, 0x68, response, 2, false), 2);
  EXPECT_EQ(response[0], 0x12);
  EXPECT_EQ(response[1], 0x34);
  Wire.beginTransmission(0x68);
  Wire.write(0x11);
  ASSERT_EQ(Wire.endTransmission(false), 0);
  ASSERT_EQ(Wire.requestFrom(0x68, size_t{2}), 2u);
  EXPECT_EQ(Wire.peek(), 25);
  EXPECT_EQ(Wire.read(), 25);
  EXPECT_EQ(Wire.read(), 0);
  EXPECT_EQ(Wire.read(), -1);
  Wire.beginTransmission(0x70);
  Wire.write(1);
  EXPECT_NE(Wire.endTransmission(), 0);
  Wire.setTimeout(0);
  Wire.beginTransmission(0x68);
  Wire.write(7);
  EXPECT_EQ(Wire.endTransmission(), 5);
  EXPECT_TRUE(Wire.getTimeoutFlag());
  Wire.clearTimeoutFlag();
  EXPECT_FALSE(Wire.getTimeoutFlag());
  Wire.setTimeout(25);
  EXPECT_TRUE(Wire.setSDA(4));
  EXPECT_TRUE(Wire.setSCL(5));
  EXPECT_DEATH(Wire.setSDA(8), "SDA while running");
  EXPECT_DEATH(Wire.setSCL(9), "SCL while running");
  Wire.end();
}

// Verifies Arduino transfers preserve source bytes and software bit reversal,
// while ordinary SDK transfers reach the same attached model and CS signal.
TEST_F(ArduinoPicoTest, SharedSpi) {
  pinMode(17, OUTPUT);
  digitalWrite(17, HIGH);
  SPI.begin();
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
  digitalWrite(17, LOW);
  EXPECT_EQ(SPI.transfer(0x12), 0x12 ^ 0xa5);
  const uint8_t source[] = {1, 2, 3};
  uint8_t result[3];
  SPI.transfer(source, result, 3);
  EXPECT_EQ(source[0], 1);
  EXPECT_EQ(result[2], 3 ^ 0xa5);
  uint8_t byte = 5;
  ASSERT_EQ(spi_write_read_blocking(spi0, &byte, &byte, 1), 1);
  EXPECT_EQ(byte, 5 ^ 0xa5);
  digitalWrite(17, HIGH);
  SPI.endTransaction();
  SPI.beginTransaction(SPISettings(1000000, LSBFIRST, SPI_MODE0));
  digitalWrite(17, LOW);
  EXPECT_EQ(SPI.transfer(1), 1 ^ 0xa5);
  EXPECT_EQ(Connections().spi.sent.back(), 0x80);
  digitalWrite(17, HIGH);
  SPI.endTransaction();
  SPI.end();
}

// Verifies absent interrupt, DMA, USB and slave hardware cannot report success.
TEST_F(ArduinoPicoTest, UnsupportedFeatures) {
  EXPECT_DEATH(noInterrupts(), "Unimplemented");
  EXPECT_DEATH(attachInterrupt(25, [] {}, RISING), "Unimplemented");
  EXPECT_DEATH(Wire.begin(0x30), "slave");
  EXPECT_FALSE(Wire.writeAsync(0x30, nullptr, 0));
  EXPECT_FALSE(SPI.transferAsync(nullptr, nullptr, 0));
  EXPECT_DEATH(Serial.dtr(), "USB");
  EXPECT_DEATH(analogReadTemp(), "temperature");
  EXPECT_DEATH(SPI.usingInterrupt(1), "interrupts");
  EXPECT_DEATH(
      {
        SPI.begin();
        SPI.beginTransaction(SPISettings(1000000, MSBFIRST, 4));
      },
      "Invalid Arduino-Pico SPI mode");
  EXPECT_DEATH(
      {
        SPI.begin();
        SPI.beginTransaction(
            SPISettings(1000000, static_cast<BitOrder>(2), SPI_MODE0));
      },
      "Invalid Arduino-Pico SPI bit order");
}
}  // namespace
