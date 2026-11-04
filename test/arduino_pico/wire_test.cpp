#include "Wire.h"

#include <vector>

#include "Arduino.h"
#include "gtest/gtest.h"
#include "roo_testing/buses/i2c/fake_i2c.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

namespace {

class Device : public FakeI2cDevice {
 public:
  Device() : FakeI2cDevice("Wire parity", 0x68) {}

  Result write(const uint8_t* bytes, uint16_t size, bool, uint16_t) override {
    writes.emplace_back(bytes, bytes + size);
    return result;
  }

  Result read(uint8_t* bytes, uint16_t size, bool, uint16_t) override {
    ++reads;
    for (uint16_t i = 0; i < size; ++i) bytes[i] = 0x20 + i;
    return result;
  }

  Result result = I2C_ERROR_OK;
  int reads = 0;
  std::vector<std::vector<uint8_t>> writes;
};

struct Wiring {
  Wiring() { FakePico().attachI2cDevice(device, 4, 5); }
  Device device;
} wiring;

class PicoWire : public testing::Test {
 protected:
  void SetUp() override {
    Wire.end();
    ASSERT_EQ(Wire.setBufferSize(32), 32u);
    Wire.setTimeout(25);
    Wire.clearTimeoutFlag();
    Wire.clearWriteError();
    wiring.device.result = FakeI2cDevice::I2C_ERROR_OK;
    wiring.device.reads = 0;
    wiring.device.writes.clear();
    Wire.begin();
  }

  void TearDown() override {
    Wire.end();
    Wire.setBufferSize(WIRE_BUFFER_SIZE);
  }
};

// Verifies invalid sizes issue no transfer and preserve unread bytes, while a
// request exactly at the configured limit succeeds without truncation.
TEST_F(PicoWire, RejectReadSizesWithoutChangingBuffer) {
  ASSERT_EQ(Wire.requestFrom(0x68, size_t{2}), 2u);
  EXPECT_EQ(Wire.read(), 0x20);
  EXPECT_EQ(Wire.requestFrom(0x68, size_t{33}), 0u);
  EXPECT_EQ(Wire.requestFrom(0x68, size_t{0}), 0u);
  EXPECT_EQ(wiring.device.reads, 1);
  EXPECT_EQ(Wire.available(), 1);
  EXPECT_EQ(Wire.read(), 0x21);
  ASSERT_EQ(Wire.requestFrom(0x68, size_t{32}), 32u);
  EXPECT_EQ(Wire.available(), 32);
}

// Verifies flush preserves both unread RX and queued TX, as in the pinned
// Arduino-Pico core; only endTransmission sends queued bytes.
TEST_F(PicoWire, FlushIsNoOp) {
  ASSERT_EQ(Wire.requestFrom(0x68, size_t{2}), 2u);
  Wire.flush();
  EXPECT_EQ(Wire.peek(), 0x20);
  EXPECT_EQ(Wire.available(), 2);
  Wire.beginTransmission(0x68);
  ASSERT_EQ(Wire.write(0x42), 1u);
  Wire.flush();
  EXPECT_TRUE(wiring.device.writes.empty());
  ASSERT_EQ(Wire.endTransmission(), 0);
  EXPECT_EQ(wiring.device.writes, (std::vector<std::vector<uint8_t>>{{0x42}}));
  EXPECT_EQ(Wire.available(), 0);
}

// Verifies repeated begin calls and requests during TX leave the active
// transmission intact, including its address and already queued data.
TEST_F(PicoWire, PreserveActiveTransmission) {
  Wire.beginTransmission(0x68);
  ASSERT_EQ(Wire.write(0x42), 1u);
  Wire.begin();
  Wire.beginTransmission(0x70);
  EXPECT_EQ(Wire.requestFrom(0x68, size_t{2}), 0u);
  ASSERT_EQ(Wire.write(0x43), 1u);
  ASSERT_EQ(Wire.endTransmission(), 0);
  EXPECT_EQ(wiring.device.writes,
            (std::vector<std::vector<uint8_t>>{{0x42, 0x43}}));
  EXPECT_EQ(wiring.device.reads, 0);
}

// Verifies resizing reports the retained size while running, clamps small
// requests to the core minimum, and limits writes without setting Print errors.
TEST_F(PicoWire, BufferSizeAndWriteLimit) {
  EXPECT_EQ(Wire.setBufferSize(64), 32u);
  Wire.beginTransmission(0x68);
  const std::vector<uint8_t> bytes(33, 0x42);
  EXPECT_EQ(Wire.write(bytes.data(), bytes.size()), 32u);
  EXPECT_EQ(Wire.write(0x43), 0u);
  EXPECT_EQ(Wire.getWriteError(), 0);
  ASSERT_EQ(Wire.endTransmission(), 0);
  EXPECT_EQ(wiring.device.writes.front().size(), 32u);
  Wire.end();
  EXPECT_EQ(Wire.setBufferSize(1), 32u);
  EXPECT_EQ(Wire.setBufferSize(64), 64u);
  Wire.begin();
  EXPECT_EQ(Wire.requestFrom(0x68, size_t{64}), 64u);
}

// Verifies stopping hides buffered bytes, stopped calls fail without delivery,
// and begin restores the master's pin functions and internal pull-ups.
TEST_F(PicoWire, StoppedAndRestartedMaster) {
  ASSERT_EQ(Wire.requestFrom(0x68, size_t{2}), 2u);
  Wire.end();
  EXPECT_EQ(Wire.available(), 0);
  EXPECT_EQ(Wire.read(), -1);
  EXPECT_EQ(Wire.peek(), -1);
  Wire.beginTransmission(0x68);
  EXPECT_EQ(Wire.write(0x42), 0u);
  EXPECT_EQ(Wire.endTransmission(), 4);
  EXPECT_EQ(Wire.requestFrom(0x68, size_t{1}), 0u);
  EXPECT_EQ(gpio_get_function(4), GPIO_FUNC_SIO);
  Wire.begin();
  EXPECT_EQ(gpio_get_function(4), GPIO_FUNC_I2C);
  EXPECT_TRUE(gpio_is_pulled_up(4));
  EXPECT_TRUE(gpio_is_pulled_up(5));
  EXPECT_EQ(Wire.available(), 0);
}

// Verifies the existing pin route can be selected again while running; changing
// routes while stopped succeeds and does not depend on the prior route.
TEST_F(PicoWire, SelectPinsAndRepeatRunningSelection) {
  EXPECT_TRUE(Wire.setSDA(4));
  EXPECT_TRUE(Wire.setSCL(5));
  Wire.end();
  EXPECT_TRUE(Wire.setSDA(8));
  EXPECT_TRUE(Wire.setSCL(9));
  Wire.begin();
  EXPECT_TRUE(Wire.setSDA(8));
  EXPECT_TRUE(Wire.setSCL(9));
  EXPECT_EQ(gpio_get_function(8), GPIO_FUNC_I2C);
  EXPECT_EQ(gpio_get_function(9), GPIO_FUNC_I2C);
  Wire.end();
  EXPECT_TRUE(Wire.setSDA(4));
  EXPECT_TRUE(Wire.setSCL(5));
}

// Verifies failed receives replace prior RX state, timeout flags are sticky,
// and timeout recovery leaves the master usable for the next transfer.
TEST_F(PicoWire, ReceiveErrorsAndTimeoutRecovery) {
  ASSERT_EQ(Wire.requestFrom(0x68, size_t{2}), 2u);
  EXPECT_EQ(Wire.requestFrom(0x70, size_t{1}), 0u);
  EXPECT_EQ(Wire.available(), 0);
  EXPECT_FALSE(Wire.getTimeoutFlag());
  wiring.device.result = FakeI2cDevice::I2C_ERROR_TIMEOUT;
  Wire.setTimeout(25, true);
  EXPECT_EQ(Wire.requestFrom(0x68, size_t{1}), 0u);
  EXPECT_TRUE(Wire.getTimeoutFlag());
  EXPECT_EQ(Wire.available(), 0);
  wiring.device.result = FakeI2cDevice::I2C_ERROR_OK;
  EXPECT_EQ(Wire.requestFrom(0x68, size_t{1}), 1u);
  EXPECT_TRUE(Wire.getTimeoutFlag());
  Wire.clearTimeoutFlag();
  EXPECT_FALSE(Wire.getTimeoutFlag());
}

// Verifies scanner-style address probes ACK attached devices, NACK missing
// addresses, deliver no payload and always emit STOP, even with stop=false.
TEST_F(PicoWire, AddressOnlyProbe) {
  for (uint8_t address : {0x68, 0x70}) {
    Wire.beginTransmission(0x68);
    Wire.write(0x42);
    ASSERT_EQ(Wire.endTransmission(false), 0);
    EXPECT_FALSE(Wire.busIdle());
    const size_t writes = wiring.device.writes.size();
    Wire.setTimeout(0);
    Wire.beginTransmission(address);
    EXPECT_EQ(Wire.endTransmission(false), address == 0x68 ? 0 : 2);
    EXPECT_EQ(wiring.device.writes.size(), writes);
    EXPECT_EQ(wiring.device.reads, 0);
    EXPECT_FALSE(Wire.getTimeoutFlag());
    EXPECT_TRUE(Wire.busIdle());
    Wire.setTimeout(25);
  }
}

// Verifies TX buffers clear on success/NACK, but survive a timeout when bus
// reset was not requested, matching the core's early timeout return.
TEST_F(PicoWire, TransmitResultAndBufferState) {
  Wire.beginTransmission(0x68);
  Wire.write(0x42);
  EXPECT_EQ(Wire.endTransmission(), 0);
  EXPECT_EQ(Wire.available(), 0);
  Wire.beginTransmission(0x70);
  Wire.write(0x43);
  EXPECT_EQ(Wire.endTransmission(), 4);
  EXPECT_EQ(Wire.available(), 0);
  Wire.beginTransmission(0x68);
  Wire.write(0x44);
  wiring.device.result = FakeI2cDevice::I2C_ERROR_TIMEOUT;
  EXPECT_EQ(Wire.endTransmission(), 5);
  EXPECT_EQ(Wire.available(), 1);
  EXPECT_EQ(Wire.read(), 0x44);
  EXPECT_TRUE(Wire.getTimeoutFlag());
}

}  // namespace
