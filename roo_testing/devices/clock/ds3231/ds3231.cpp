#include <assert.h>
#include <time.h>

#include <cstring>

#include "roo_testing/devices/clock/ds3231/ds3231.h"
#include "roo_testing/transducers/time/clock.h"

using roo_testing_transducers::FixedThermometer;
using roo_testing_transducers::Temperature;
using roo_testing_transducers::Thermometer;

inline int64_t getSystemTimeMicros() {
  return roo_testing_transducers::getDefaultSystemClock()->getTimeMicros();
}

static const uint64_t kMicrosPerSec = 1000000LL;

uint8_t bcd2dec(uint8_t bcd) { return ((bcd / 16) * 10) + (bcd % 16); }

uint8_t dec2bcd(uint8_t dec) { return ((dec / 10) * 16) + (dec % 10); }

enum Ds3231Reg {
  REG_TIME = 0x00,
  REG_ALARM_1 = 0x07,
  REG_ALARM_2 = 0x0B,
  REG_CONTROL = 0x0E,
  REG_STATUS = 0x0F,
  REG_TEMPERATURE = 0x11
};

FakeDs3231::FakeDs3231()
    : FakeDs3231(new FixedThermometer(Temperature::FromC(25))) {}

FakeDs3231::FakeDs3231(Thermometer *thermometer)
    : FakeI2cDevice("DS3231", 0x68),
      thermometer_(thermometer),
      register_address_(0xFF),
      time_offset_(0),
      weekday_offset_(0) {
  for (int i = 0; i < 0x12; ++i) registers_[i] = 0;
}

FakeI2cDevice::Result FakeDs3231::write(const uint8_t *buf, uint16_t size,
                                        bool sendStop, uint16_t timeOutMillis) {
  assert(size > 0);
  register_address_ = buf[0];
  tick();
  for (int i = 0; i < size - 1; ++i) {
    register_write((register_address_ + i) % 0x12, buf[i + 1]);
  }
  flush();
  return FakeI2cDevice::I2C_ERROR_OK;
}

void FakeDs3231::tick() {
  // Set the time.
  time_t device_time = ((getSystemTimeMicros() + time_offset_) / kMicrosPerSec);
  struct tm *t = gmtime(&device_time);
  registers_[0] = dec2bcd(t->tm_sec);
  registers_[1] = dec2bcd(t->tm_min);
  switch (get_hour_mode()) {
    case H24: {
      registers_[2] = dec2bcd(t->tm_hour);
      break;
    }
    case H12: {
      bool pm = (t->tm_hour >= 12);
      uint8_t h12 = t->tm_hour == 0    ? 12
                    : t->tm_hour >= 13 ? t->tm_hour - 12
                                       : t->tm_hour;
      registers_[2] = dec2bcd(h12) | 0x40 | (pm ? 0x20 : 0);
      break;
    }
  }
  registers_[3] = ((t->tm_wday + weekday_offset_) % 7) + 1;
  registers_[4] = dec2bcd(t->tm_mday);
  registers_[5] = dec2bcd(t->tm_mon + 1) |
                  ((((t->tm_year + 1900) / 100) % 2) == 0 ? 0 : 0x80);
  registers_[6] = dec2bcd(t->tm_year % 100);

  // Set the temperature reading.
  double tempC = thermometer_->read().AsC();
  if (tempC < -128.0) {
    tempC = -128.0;
  } else if (tempC > 127.75) {
    tempC = 127.75;
  }
  registers_[0x11] = (int8_t)tempC;
  registers_[0x12] = ((int)(tempC + 128.0 * 4.0) % 4) << 6;

  for (int i = 0; i < 0x12; ++i) {
    registers_written_[i] = false;
  }
}

uint8_t FakeDs3231::register_read(int index) const { return registers_[index]; }

void FakeDs3231::register_write(int index, uint8_t value) {
  registers_[index] = value;
  registers_written_[index] = true;
}

void FakeDs3231::flush() {
  bool seconds_written = registers_written_[0];
  bool time_written = registers_written_[0x00] || registers_written_[0x01] ||
                      registers_written_[0x02] || registers_written_[0x03] ||
                      registers_written_[0x04] || registers_written_[0x05] ||
                      registers_written_[0x06];
  bool alarm1_written = registers_written_[0x07] || registers_written_[0x08] ||
                        registers_written_[0x09] || registers_written_[0x0A];
  bool alarm2_written = registers_written_[0x0B] || registers_written_[0x0C] ||
                        registers_written_[0x0D];
  bool control_written = registers_written_[0x0E] || registers_written_[0x0F];
  if (time_written) {
    struct tm t;
    t.tm_sec = bcd2dec(registers_[0x00]);
    t.tm_min = bcd2dec(registers_[0x01]);
    t.tm_hour = bcd2dec(registers_[0x02]);
    t.tm_mday = bcd2dec(registers_[0x04]);
    t.tm_mon = bcd2dec(registers_[0x05] & 0x7F) - 1;
    // If the century bit is set, we interpret the internal date as being
    // in the range of 1900-1999, for the purpose of calculating the offset.
    // Otherwise, we interpret it as 2000-2099 (which means we need to +100,
    // because tm_year expects offset since 1900).
    t.tm_year =
        bcd2dec(registers_[0x06]) + ((registers_[0x05] & 0x80) == 0 ? 100 : 0);
    time_t device_time = mktime(&t);
    // Re-read to calculate the day-of-week offset.
    int real_wday = gmtime(&device_time)->tm_wday;
    weekday_offset_ = ((bcd2dec(registers_[3]) - real_wday) + 8) % 7;
    int64_t device_time_micros = (int64_t)(device_time)*kMicrosPerSec;
    int64_t system_time_micros = getSystemTimeMicros();
    int64_t new_time_offset = device_time_micros - system_time_micros;
    if (seconds_written) {
      // Reset the internal clock to full seconds.
      time_offset_ = new_time_offset;
    } else {
      // Preserve the sub-second offset, to not delay next full-second tick.
      time_offset_ = ((new_time_offset / kMicrosPerSec) * kMicrosPerSec) +
                     time_offset_ % kMicrosPerSec;
    }
  }
}

FakeI2cDevice::Result FakeDs3231::read(uint8_t *buff, uint16_t size,
                                       bool sendStop, uint16_t timeOutMillis) {
  assert(register_address_ + size < 0x12);
  tick();
  for (int i = 0; i < size; ++i) {
    buff[i] = register_read(i);
  }
  return I2C_ERROR_OK;
}
