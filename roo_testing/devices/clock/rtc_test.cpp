#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "roo_testing/devices/clock/ds1307/ds1307.h"
#include "roo_testing/devices/clock/ds3231/ds3231.h"
#include "roo_testing/devices/clock/ds3232/ds3232.h"
#include "roo_testing/devices/clock/ds3234/ds3234.h"
#include "roo_testing/devices/clock/pcf8523/pcf8523.h"
#include "roo_testing/devices/clock/rv3028/rv3028.h"
#include "roo_testing/devices/clock/rv8803/rv8803.h"

namespace {
using roo_testing_rtc::Chip;
using roo_testing_rtc::I2cRtc;
using roo_testing_transducers::FakeClock;

class I2cRtcTest : public testing::TestWithParam<Chip> {
 protected:
  void SetUp() override {
    switch (GetParam()) {
      case Chip::kDs3231:
        rtc_.reset(new FakeDs3231(time_));
        break;
      case Chip::kDs3232:
        rtc_.reset(new FakeDs3232(&time_));
        break;
      case Chip::kDs1307:
        rtc_.reset(new FakeDs1307(&time_));
        break;
      case Chip::kPcf8523:
        rtc_.reset(new FakePcf8523(&time_));
        break;
      case Chip::kRv3028:
        rtc_.reset(new FakeRv3028(&time_));
        break;
      case Chip::kRv8803:
        rtc_.reset(new FakeRv8803(&time_));
        break;
      default:
        FAIL() << "Not an I2C chip";
    }
  }
  int base() const { return GetParam() == Chip::kPcf8523 ? 3 : 0; }
  int dayIndex() const { return GetParam() == Chip::kPcf8523 ? 3 : 4; }
  bool ds() const {
    return GetParam() == Chip::kDs3231 || GetParam() == Chip::kDs3232;
  }
  void write(uint8_t reg, std::vector<uint8_t> bytes) {
    bytes.insert(bytes.begin(), reg);
    ASSERT_EQ(FakeI2cDevice::I2C_ERROR_OK,
              rtc_->write(bytes.data(), bytes.size(), true, 100));
  }
  std::vector<uint8_t> read(uint8_t reg, size_t size) {
    EXPECT_EQ(FakeI2cDevice::I2C_ERROR_OK, rtc_->write(&reg, 1, false, 100));
    std::vector<uint8_t> bytes(size);
    EXPECT_EQ(FakeI2cDevice::I2C_ERROR_OK,
              rtc_->read(bytes.data(), size, true, 100));
    return bytes;
  }
  std::vector<uint8_t> calendar(uint8_t second = 0x59) {
    const uint8_t weekday = GetParam() == Chip::kRv8803           ? 2
                            : ds() || GetParam() == Chip::kDs1307 ? 2
                                                                  : 1;
    return GetParam() == Chip::kPcf8523
               ? std::vector<uint8_t>{second, 0x59, 0x23, 0x28, weekday, 2, 0}
               : std::vector<uint8_t>{second, 0x59, 0x23, weekday, 0x28, 2, 0};
  }
  FakeClock time_;
  std::unique_ptr<I2cRtc> rtc_;
};

// Verifies factory validity bits and correct public I2C addresses.
TEST_P(I2cRtcTest, InitialState) {
  EXPECT_EQ(GetParam() == Chip::kRv3028   ? 0x52
            : GetParam() == Chip::kRv8803 ? 0x32
                                          : 0x68,
            rtc_->address());
  if (ds())
    EXPECT_EQ(0x80, read(0x0f, 1)[0] & 0x80);
  else if (GetParam() == Chip::kDs1307 || GetParam() == Chip::kPcf8523)
    EXPECT_EQ(0x80, read(base(), 1)[0] & 0x80);
  else
    EXPECT_NE(0, read(0x0e, 1)[0] & (GetParam() == Chip::kRv3028 ? 1 : 3));
}

// Verifies independent golden calendar bytes across leap day and midnight.
TEST_P(I2cRtcTest, CalendarAdvancesAcrossLeapDay) {
  std::vector<uint8_t> want = calendar();
  write(base(), want);
  time_.delayMicros(1000000);
  want[0] = 0;
  want[1] = 0;
  want[2] = 0;
  want[dayIndex()] = 0x29;
  want[dayIndex() == 3 ? 4 : 3] = GetParam() == Chip::kRv8803           ? 4
                                  : ds() || GetParam() == Chip::kDs1307 ? 3
                                                                        : 2;
  EXPECT_EQ(want, read(base(), 7));
  time_.delayMicros(86400000000ULL);
  want[dayIndex()] = 1;
  want[5] = 3;
  want[dayIndex() == 3 ? 4 : 3] = GetParam() == Chip::kRv8803           ? 8
                                  : ds() || GetParam() == Chip::kDs1307 ? 4
                                                                        : 3;
  EXPECT_EQ(want, read(base(), 7));
}

// Verifies halted clocks hold their calendar and resume without a time jump.
TEST_P(I2cRtcTest, HaltAndResume) {
  if (ds() || GetParam() == Chip::kRv3028) GTEST_SKIP();
  std::vector<uint8_t> want = calendar(0x50);
  write(base(), want);
  const uint8_t control = GetParam() == Chip::kDs1307    ? 0
                          : GetParam() == Chip::kPcf8523 ? 0
                                                         : 0x0f;
  const uint8_t mask = GetParam() == Chip::kDs1307    ? 0x80
                       : GetParam() == Chip::kPcf8523 ? 0x20
                                                      : 1;
  const uint8_t before = read(control, 1)[0];
  write(control, {static_cast<uint8_t>(before | mask)});
  if (GetParam() == Chip::kDs1307) want[0] |= 0x80;
  time_.delayMicros(3000000);
  EXPECT_EQ(want, read(base(), 7));
  write(control, {static_cast<uint8_t>(before & ~mask)});
  time_.delayMicros(1000000);
  want[0] = 0x51;
  EXPECT_EQ(want, read(base(), 7));
}

// Verifies 12-hour noon/midnight carries use the chip's actual PM bit encoding.
TEST_P(I2cRtcTest, TwelveHourRollover) {
  if (GetParam() == Chip::kRv8803) GTEST_SKIP();
  if (GetParam() == Chip::kPcf8523) write(0, {8});
  if (GetParam() == Chip::kRv3028) write(0x10, {2});
  std::vector<uint8_t> want = calendar();
  want[2] = ds() || GetParam() == Chip::kDs1307 ? 0x71 : 0x31;
  write(base(), want);
  time_.delayMicros(2000000);
  want[0] = 1;
  want[1] = 0;
  want[2] = ds() || GetParam() == Chip::kDs1307 ? 0x52 : 0x12;
  want[dayIndex()] = 0x29;
  want[dayIndex() == 3 ? 4 : 3] = ds() || GetParam() == Chip::kDs1307 ? 3 : 2;
  EXPECT_EQ(want, read(base(), 7));
}

// Verifies status writes cannot set sticky flags, clear unrelated events, or
// overwrite read-only bits; invalid BCD injection remains observable.
TEST_P(I2cRtcTest, FlagsAndRawFaults) {
  if (ds()) {
    rtc_->injectRegister(0x0f, 0x87);
    write(0x0f, {3});
    EXPECT_EQ(7, read(0x0f, 1)[0]);
    write(0x0f, {0x80});
    EXPECT_EQ(4, read(0x0f, 1)[0]);
  } else if (GetParam() == Chip::kRv3028 || GetParam() == Chip::kRv8803) {
    rtc_->injectRegister(0x0e, GetParam() == Chip::kRv3028 ? 0xbf : 0x3f);
    write(0x0e,
          {static_cast<uint8_t>(GetParam() == Chip::kRv3028 ? 0xfe : 0x3c)});
    EXPECT_EQ(GetParam() == Chip::kRv3028 ? 0xbe : 0x3c, read(0x0e, 1)[0]);
  }
  rtc_->injectRegister(base() + 1, 0x6a);
  time_.delayMicros(1000000);
  EXPECT_EQ(0x6a, read(base() + 1, 1)[0]);
  rtc_->simulateTimeLoss();
  if (ds()) {
    EXPECT_NE(0, read(0x0f, 1)[0] & 0x80);
  }
}

// Verifies selected pointers advance across calls and reject invalid input.
TEST_P(I2cRtcTest, AddressPointerAndValidation) {
  write(base(), calendar());
  const uint8_t address = base() + 4;
  ASSERT_EQ(FakeI2cDevice::I2C_ERROR_OK, rtc_->write(&address, 1, false, 100));
  uint8_t value = 0;
  ASSERT_EQ(FakeI2cDevice::I2C_ERROR_OK, rtc_->read(&value, 1, true, 100));
  EXPECT_EQ(calendar()[4], value);
  ASSERT_EQ(FakeI2cDevice::I2C_ERROR_OK, rtc_->read(&value, 1, true, 100));
  EXPECT_EQ(2, value);
  EXPECT_NE(FakeI2cDevice::I2C_ERROR_OK, rtc_->write(nullptr, 1, true, 100));
  EXPECT_NE(FakeI2cDevice::I2C_ERROR_OK, rtc_->read(nullptr, 1, true, 100));
}

INSTANTIATE_TEST_SUITE_P(AllI2c, I2cRtcTest,
                         testing::Values(Chip::kDs3231, Chip::kDs3232,
                                         Chip::kDs1307, Chip::kPcf8523,
                                         Chip::kRv3028, Chip::kRv8803));

class Spi : public FakeSpiInterface {
 public:
  Spi() : FakeSpiInterface("rtc-test") {}
  uint32_t clkHz() const override { return frequency; }
  SpiDataMode dataMode() const override { return mode; }
  SpiBitOrder bitOrder() const override { return order; }
  uint32_t frequency = 1000000;
  SpiDataMode mode = kSpiMode1;
  SpiBitOrder order = kSpiMsbFirst;
};

class Ds3234Test : public testing::Test {
 protected:
  std::vector<uint8_t> frame(std::vector<uint8_t> bytes) {
    rtc_.cs().write(0.0f);
    rtc_.transfer(spi_, bytes.data(), bytes.size() * 8);
    rtc_.cs().write(3.3f);
    return bytes;
  }
  FakeClock time_;
  FakeDs3234 rtc_{&time_};
  Spi spi_;
};

// Verifies command/data streams survive bytewise transfers while CS stays low,
// reset at the next CS edge, and keep the snapshot coherent across a rollover.
TEST_F(Ds3234Test, CsFramingAndLatchedCalendar) {
  frame({0x80, 0x59, 0x59, 0x23, 3, 0x31, 0x12, 0x24});
  rtc_.cs().write(0.0f);
  uint8_t byte = 0;
  rtc_.transfer(spi_, &byte, 8);
  byte = 0;
  rtc_.transfer(spi_, &byte, 8);
  EXPECT_EQ(0x59, byte);
  time_.delayMicros(2000000);
  // Repeating the same CS level must not start a new command.
  rtc_.cs().write(0.0f);
  byte = 0;
  rtc_.transfer(spi_, &byte, 8);
  EXPECT_EQ(0x59, byte);
  rtc_.cs().write(3.3f);
  EXPECT_EQ((std::vector<uint8_t>{0, 1, 0, 0, 4, 1, 1, 0x25}),
            frame({0, 0, 0, 0, 0, 0, 0, 0}));
  EXPECT_FALSE(rtc_.protocolError());
}

// Verifies the snapshot precedes the command and is refreshed when the
// calendar bank wraps from 0x13 to 0x00 during one asserted CS frame.
TEST_F(Ds3234Test, SnapshotAtCsAndRegisterWrap) {
  frame({0x80, 0x50, 0x34, 0x12, 7, 0x15, 0x06, 0x24});
  rtc_.cs().write(0.0f);
  time_.delayMicros(2000000);
  uint8_t bytes[] = {0, 0};
  rtc_.transfer(spi_, bytes, 16);
  EXPECT_EQ(0x50, bytes[1]);
  rtc_.cs().write(3.3f);
  rtc_.cs().write(0.0f);
  uint8_t command = 0x13;
  rtc_.transfer(spi_, &command, 8);
  time_.delayMicros(2000000);
  uint8_t read[] = {0, 0};
  rtc_.transfer(spi_, read, 16);
  EXPECT_EQ(0x54, read[1]);
  rtc_.cs().write(3.3f);
}

// Verifies bad SPI modes/order/frequency/partial bytes are rejected rather
// than silently satisfying a driver that uses the wrong protocol.
TEST_F(Ds3234Test, RejectsUnsupportedProtocol) {
  spi_.mode = kSpiMode0;
  EXPECT_EQ((std::vector<uint8_t>{0xff, 0xff}), frame({0x0f, 0}));
  EXPECT_TRUE(rtc_.protocolError());
  rtc_.clearProtocolError();
  spi_.mode = kSpiMode3;
  EXPECT_EQ(0x80, frame({0x0f, 0})[1]);
  EXPECT_FALSE(rtc_.protocolError());
  spi_.order = kSpiLsbFirst;
  frame({0x8f, 0});
  EXPECT_TRUE(rtc_.protocolError());
  rtc_.clearProtocolError();
  spi_.order = kSpiMsbFirst;
  spi_.frequency = 4000001;
  frame({0, 0});
  EXPECT_TRUE(rtc_.protocolError());
  rtc_.clearProtocolError();
  spi_.frequency = 1000000;
  rtc_.cs().write(0.0f);
  uint8_t byte = 0;
  rtc_.transfer(spi_, &byte, 7);
  rtc_.cs().write(3.3f);
  EXPECT_TRUE(rtc_.protocolError());
}

// Verifies independently modeled oscillator-stop and alarm W0C behavior.
TEST_F(Ds3234Test, StatusFlags) {
  rtc_.injectRegister(0x0f, 0x83);
  frame({0x8f, 3});
  EXPECT_EQ(3, frame({0x0f, 0})[1]);
  frame({0x8f, 0x80});
  EXPECT_EQ(0, frame({0x0f, 0})[1]);
  rtc_.simulateTimeLoss();
  EXPECT_EQ(0x80, frame({0x0f, 0})[1]);
}
}  // namespace
