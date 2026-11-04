#include "Arduino.h"

#include <chrono>
#include <vector>

#include "SPI.h"
#include "Wire.h"
#include "gtest/gtest.h"
#include "roo_testing/buses/i2c/fake_i2c.h"
#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"
#include "roo_testing/system/timer.h"
#include "runtime.h"
#include "semphr.h"
#include "task.h"

#if !ROO_TESTING_RUNTIME_FREERTOS || !defined(__FREERTOS) || !PICO_RP2040
#error Incorrect Pico FreeRTOS identity
#endif
#if defined(ESP32) || defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32)
#error Foreign kernel identity
#endif

namespace {
class Echo : public SimpleFakeSpiDevice {
 public:
  Echo() : SimpleFakeSpiDevice("task SPI") {}
  void transfer(const FakeSpiInterface& spi, uint8_t* data,
                uint16_t bits) override {
    configASSERT(isSelected());
    clocks.push_back(spi.clkHz());
    for (uint16_t i = 0; i < bits / 8; ++i) data[i] ^= 0xa5;
  }
  std::vector<uint32_t> clocks;
};

class Registers : public FakeI2cDevice {
 public:
  Registers() : FakeI2cDevice("task registers", 0x68) {}
  Result write(const uint8_t* bytes, uint16_t size, bool stop,
               uint16_t) override {
    writes.insert(writes.end(), bytes, bytes + size);
    last_stop = stop;
    return I2C_ERROR_OK;
  }
  Result read(uint8_t* bytes, uint16_t size, bool stop, uint16_t) override {
    configASSERT(!last_stop);
    last_stop = stop;
    for (uint16_t i = 0; i < size; ++i) bytes[i] = writes.back() + i;
    return I2C_ERROR_OK;
  }
  bool last_stop = true;
  std::vector<uint8_t> writes;
};

struct Wiring {
  Wiring() {
    FakePico().attachSpiDevice(echo[0], 18, 16, 19);
    FakePico().attachSpiDevice(echo[1], 18, 16, 19);
    FakePico().attachOutput(17, echo[0].cs());
    FakePico().attachOutput(20, echo[1].cs());
    FakePico().attachI2cDevice(registers, 4, 5);
  }
  Echo echo[2];
  Registers registers;
} wiring;
SemaphoreHandle_t done;
bool progressed;

void SpiWorker(void*) {
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE3));
  progressed = true;
  digitalWrite(20, LOW);
  configASSERT(SPI.transfer(0x33) == (0x33 ^ 0xa5));
  digitalWrite(20, HIGH);
  SPI.endTransaction();
  xSemaphoreGive(done);
  vTaskSuspend(nullptr);
}

void WireWorker(void* arg) {
  SemaphoreHandle_t mutex = static_cast<SemaphoreHandle_t>(arg);
  if (mutex != nullptr) configASSERT(xSemaphoreTake(mutex, portMAX_DELAY));
  Wire.beginTransmission(0x68);
  progressed = true;
  Wire.write(0x40);
  configASSERT(Wire.endTransmission() == 0);
  if (mutex != nullptr) xSemaphoreGive(mutex);
  xSemaphoreGive(done);
  vTaskSuspend(nullptr);
}

// Verifies startup runs in a task, portable core code works, and nested
// critical sections preserve scheduling before a delay gives another task the
// controller.
TEST(PicoFreeRtosArduino, IdentityAndCore) {
  EXPECT_TRUE(RooFreeRtosTaskContext());
  EXPECT_EQ(String("value=") + String(42), "value=42");
  noInterrupts();
  noInterrupts();
  interrupts();
  interrupts();
  const uint64_t before = time_us_64();
  const auto host_before = std::chrono::steady_clock::now();
  delayMicroseconds(50);
  EXPECT_GE(std::chrono::steady_clock::now() - host_before,
            std::chrono::microseconds(50));
  if (system_time_is_auto_sync_enabled()) {
    EXPECT_GE(time_us_64() - before, 50u);
  } else {
    EXPECT_EQ(time_us_64(), before);
  }
  delay(2);
  yield();
}

// Verifies reservations span settings, CS edges and task delays, so a competing
// transaction cannot reconfigure or select the bus until the owner releases it.
TEST(PicoFreeRtosArduino, CompleteSpiTransaction) {
  for (uint pin : {17, 20}) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
  }
  SPI.begin();
  done = xSemaphoreCreateBinary();
  progressed = false;
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(17, LOW);
  TaskHandle_t worker;
  ASSERT_EQ(xTaskCreate(SpiWorker, "other_spi", 16384, nullptr, 3, &worker),
            pdPASS);
  delay(2);
  EXPECT_FALSE(progressed);
  EXPECT_EQ(SPI.transfer(0x12), (0x12 ^ 0xa5));
  digitalWrite(17, HIGH);
  SPI.endTransaction();
  ASSERT_EQ(xSemaphoreTake(done, pdMS_TO_TICKS(100)), pdTRUE);
  EXPECT_TRUE(progressed);
  ASSERT_EQ(wiring.echo[0].clocks.size(), 1u);
  ASSERT_EQ(wiring.echo[1].clocks.size(), 1u);
  EXPECT_GT(wiring.echo[0].clocks[0], 980000u);
  EXPECT_LE(wiring.echo[0].clocks[0], 1000000u);
  EXPECT_GT(wiring.echo[1].clocks[0], 1900000u);
  EXPECT_LE(wiring.echo[1].clocks[0], 2000000u);
  vTaskDelete(worker);
  vSemaphoreDelete(done);
  SPI.end();
}

// Verifies an application mutex protects TX, repeated START, and RX consumption
// across task delays, as required by the real core's shared Wire buffer.
TEST(PicoFreeRtosArduino, CallerSerializesWireTransaction) {
  wiring.registers.writes.clear();
  Wire.begin();
  done = xSemaphoreCreateBinary();
  progressed = false;
  SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
  ASSERT_EQ(xSemaphoreTake(mutex, portMAX_DELAY), pdTRUE);
  Wire.beginTransmission(0x68);
  Wire.write(0x20);
  TaskHandle_t worker;
  ASSERT_EQ(xTaskCreate(WireWorker, "other_wire", 16384, mutex, 3, &worker),
            pdPASS);
  delay(2);
  EXPECT_FALSE(progressed);
  ASSERT_EQ(Wire.endTransmission(false), 0);
  delay(1);
  EXPECT_FALSE(progressed);
  ASSERT_EQ(Wire.requestFrom(0x68, size_t{2}), 2u);
  delay(1);
  EXPECT_FALSE(progressed);
  EXPECT_EQ(Wire.read(), 0x20);
  EXPECT_FALSE(progressed);
  EXPECT_EQ(Wire.read(), 0x21);
  EXPECT_FALSE(progressed);
  xSemaphoreGive(mutex);
  ASSERT_EQ(xSemaphoreTake(done, pdMS_TO_TICKS(100)), pdTRUE);
  EXPECT_TRUE(progressed);
  EXPECT_EQ(wiring.registers.writes, (std::vector<uint8_t>{0x20, 0x40}));
  vTaskDelete(worker);
  vSemaphoreDelete(done);
  vSemaphoreDelete(mutex);
  Wire.end();
}
// Verifies unread RX, rejected reads, and no-STOP bus state impose no task
// reservation. Another task can replace the shared buffer and finish the bus.
TEST(PicoFreeRtosArduino, UnreadReceiveDoesNotReserveController) {
  for (bool stop : {true, false}) {
    wiring.registers.writes.clear();
    Wire.begin();
    Wire.beginTransmission(0x68);
    Wire.write(0x60);
    ASSERT_EQ(Wire.endTransmission(false), 0);
    ASSERT_EQ(Wire.requestFrom(0x68, size_t{2}, stop), 2u);
    EXPECT_EQ(Wire.busIdle(), stop);
    EXPECT_EQ(Wire.requestFrom(0x68, size_t{WIRE_BUFFER_SIZE + 1}), 0u);
    Wire.flush();
    EXPECT_EQ(Wire.available(), 2);
    EXPECT_EQ(Wire.busIdle(), stop);
    done = xSemaphoreCreateBinary();
    progressed = false;
    TaskHandle_t worker;
    ASSERT_EQ(xTaskCreate(WireWorker, "other_wire", 16384, nullptr, 3, &worker),
              pdPASS);
    ASSERT_EQ(xSemaphoreTake(done, pdMS_TO_TICKS(100)), pdTRUE);
    EXPECT_TRUE(progressed);
    EXPECT_EQ(wiring.registers.writes, (std::vector<uint8_t>{0x60, 0x40}));
    EXPECT_EQ(Wire.available(), 0);
    EXPECT_TRUE(Wire.busIdle());
    vTaskDelete(worker);
    vSemaphoreDelete(done);
    Wire.end();
  }
}

// Verifies beginTransmission does not acquire a cross-call mutex. Without
// application locking, a second task appends to and sends the first task's TX.
TEST(PicoFreeRtosArduino, TransmitBufferDoesNotReserveController) {
  wiring.registers.writes.clear();
  Wire.begin();
  Wire.beginTransmission(0x68);
  Wire.write(0x20);
  done = xSemaphoreCreateBinary();
  progressed = false;
  TaskHandle_t worker;
  ASSERT_EQ(xTaskCreate(WireWorker, "other_wire", 16384, nullptr, 3, &worker),
            pdPASS);
  ASSERT_EQ(xSemaphoreTake(done, pdMS_TO_TICKS(100)), pdTRUE);
  EXPECT_TRUE(progressed);
  EXPECT_EQ(wiring.registers.writes, (std::vector<uint8_t>{0x20, 0x40}));
  EXPECT_EQ(Wire.endTransmission(), 4);
  vTaskDelete(worker);
  vSemaphoreDelete(done);
  Wire.end();
}
}  // namespace

// Verifies the task-backed GoogleTest runner propagates its exact failure
// status.
TEST(PicoFreeRtosArduino, DISABLED_ExpectedFailure) {
  ADD_FAILURE() << "intentional runner failure";
}
