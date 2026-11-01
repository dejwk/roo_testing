#include "roo_testing/devices/clock/internal/rtc_registers.h"

namespace roo_testing_rtc {
namespace {
uint8_t Bcd(int value) {
  return static_cast<uint8_t>((value / 10) * 16 + value % 10);
}

// Decode actual register digits, without depending on a driver conversion API.
int Decimal(uint8_t value) {
  if ((value & 15) > 9 || (value >> 4) > 9) return -1;
  return (value >> 4) * 10 + (value & 15);
}

// These calendars implement a four-year leap cycle, including the DS century
// rollover limitation. No host libc timezone or DST conversion is involved.
int DaysInMonth(int year, int month) {
  static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return days[month - 1] + (month == 2 && year % 4 == 0);
}
}  // namespace

RtcRegisters::RtcRegisters(Chip chip, roo_testing_transducers::Clock* clock)
    : chip_(chip),
      clock_(clock == nullptr ? roo_testing_transducers::getDefaultSystemClock()
                              : clock) {
  reset();
}

void RtcRegisters::reset() {
  data_.fill(0);
  fraction_us_ = 0;
  sampled_ = false;
  reading_ = false;
  const int base = timeBase();
  data_[base + (chip_ == Chip::kPcf8523 ? 3 : 4)] = 1;
  data_[base + 5] = 1;
  uint8_t weekday = 7;
  if (chip_ == Chip::kRv8803) {
    weekday = 0x40;
  } else if (chip_ == Chip::kPcf8523 || chip_ == Chip::kRv3028) {
    weekday = 6;
  }
  data_[base + (chip_ == Chip::kPcf8523 ? 4 : 3)] = weekday;
  if (chip_ == Chip::kPcf8523) data_[2] = 0xe0;
  if (chip_ == Chip::kRv3028) data_[0x37] = 0x10;
  if (isDs323x()) setTemperature(25);
  simulateTimeLoss();
}

uint16_t RtcRegisters::size() const {
  switch (chip_) {
    case Chip::kDs3231:
      return 0x13;
    case Chip::kDs3232:
      return 0x100;
    case Chip::kDs3234:
      return 0x14;  // Calendar bank wraps at 0x13; indirect SRAM is unmodeled.
    case Chip::kDs1307:
      return 0x40;
    case Chip::kPcf8523:
      return 0x14;
    case Chip::kRv3028:
      return 0x40;
    case Chip::kRv8803:
      return 0x30;
  }
  return 0;
}

uint8_t RtcRegisters::nextAddress(uint8_t address) const {
  return static_cast<uint8_t>((static_cast<uint16_t>(address) + 1) % size());
}

uint8_t RtcRegisters::canonicalAddress(uint8_t address) const {
  if (chip_ == Chip::kRv8803 && address >= 0x11 && address <= 0x17) {
    return address - 0x11;
  }
  if (chip_ == Chip::kRv8803 && address >= 0x18 && address <= 0x1f) {
    return address - 0x10;
  }
  return address;
}

int RtcRegisters::timeBase() const { return chip_ == Chip::kPcf8523 ? 3 : 0; }

bool RtcRegisters::isDs323x() const {
  return chip_ == Chip::kDs3231 || chip_ == Chip::kDs3232 ||
         chip_ == Chip::kDs3234;
}

bool RtcRegisters::stopped() const {
  if (chip_ == Chip::kDs1307) return (data_[0] & 0x80) != 0;
  if (chip_ == Chip::kPcf8523) return (data_[0] & 0x20) != 0;
  if (chip_ == Chip::kRv8803) return (data_[0x0f] & 1) != 0;
  return false;  // DS EOSC applies on battery only; supplies are not modeled.
}

bool RtcRegisters::twelveHour() const {
  if (isDs323x() || chip_ == Chip::kDs1307) return (data_[2] & 0x40) != 0;
  if (chip_ == Chip::kPcf8523) return (data_[0] & 8) != 0;
  return chip_ == Chip::kRv3028 && (data_[0x10] & 2) != 0;
}

bool RtcRegisters::decode(Calendar& c) const {
  const int base = timeBase();
  c.second = Decimal(data_[base] & 0x7f);
  c.minute = Decimal(data_[base + 1]);
  c.hour =
      twelveHour() ? Decimal(data_[base + 2] & 0x1f) : Decimal(data_[base + 2]);
  if (twelveHour()) {
    if (c.hour < 1 || c.hour > 12) return false;
    c.hour = c.hour % 12 + ((data_[base + 2] & 0x20) != 0 ? 12 : 0);
  }
  c.day = Decimal(data_[base + (chip_ == Chip::kPcf8523 ? 3 : 4)]);
  c.month = Decimal(data_[base + 5] & (isDs323x() ? 0x1f : 0xff));
  const int year = Decimal(data_[base + 6]);
  c.year =
      2000 + year + (isDs323x() && (data_[base + 5] & 0x80) != 0 ? 100 : 0);
  return c.second >= 0 && c.second < 60 && c.minute >= 0 && c.minute < 60 &&
         c.hour >= 0 && c.hour < 24 && year >= 0 && c.month >= 1 &&
         c.month <= 12 && c.day >= 1 && c.day <= DaysInMonth(c.year, c.month);
}

void RtcRegisters::encode(const Calendar& c) {
  const int base = timeBase();
  data_[base] = Bcd(c.second) | (data_[base] & 0x80);
  data_[base + 1] = Bcd(c.minute);
  if (twelveHour()) {
    data_[base + 2] = Bcd(c.hour % 12 == 0 ? 12 : c.hour % 12) |
                      (c.hour >= 12 ? 0x20 : 0) |
                      (isDs323x() || chip_ == Chip::kDs1307 ? 0x40 : 0);
  } else {
    data_[base + 2] = Bcd(c.hour);
  }
  data_[base + (chip_ == Chip::kPcf8523 ? 3 : 4)] = Bcd(c.day);
  data_[base + 5] = Bcd(c.month) | (isDs323x() && c.year >= 2100 ? 0x80 : 0);
  data_[base + 6] = Bcd(c.year % 100);
}

// Carry seconds through the calendar independently of any driver or roo_time.
void RtcRegisters::advance(uint64_t seconds) {
  Calendar c;
  if (!decode(c)) return;
  uint64_t value = c.second + seconds;
  c.second = value % 60;
  value = c.minute + value / 60;
  c.minute = value % 60;
  value = c.hour + value / 60;
  c.hour = value % 24;
  uint64_t days = value / 24;
  const int index = timeBase() + (chip_ == Chip::kPcf8523 ? 4 : 3);
  if (days != 0) {
    if (chip_ == Chip::kRv8803) {
      int weekday = data_[index] & 0x7f;
      const int shift = days % 7;
      data_[index] = ((weekday << shift) | (weekday >> (7 - shift))) & 0x7f;
    } else {
      const int origin = isDs323x() || chip_ == Chip::kDs1307 ? 1 : 0;
      data_[index] = (data_[index] - origin + days) % 7 + origin;
    }
  }
  days %= isDs323x() ? 73050 : 36525;
  while (days != 0) {
    const int remaining = DaysInMonth(c.year, c.month) - c.day + 1;
    if (days < static_cast<uint64_t>(remaining)) {
      c.day += days;
      break;
    }
    days -= remaining;
    c.day = 1;
    if (++c.month == 13) {
      c.month = 1;
      if (++c.year == (isDs323x() ? 2200 : 2100)) c.year = 2000;
    }
  }
  encode(c);
}

void RtcRegisters::sync() {
  const int64_t now = clock_->getTimeMicros();
  if (!sampled_) {
    sampled_at_ = now;
    sampled_ = true;
  }
  const int64_t elapsed = now >= sampled_at_ ? now - sampled_at_ : 0;
  sampled_at_ = now;
  if (stopped()) return;
  fraction_us_ += elapsed;
  const uint64_t seconds = fraction_us_ / 1000000;
  fraction_us_ %= 1000000;
  if (seconds != 0) advance(seconds);
}

void RtcRegisters::beginRead() {
  sync();
  for (int i = 0; i < 7; ++i) snapshot_[i] = data_[timeBase() + i];
  reading_ = true;
}

void RtcRegisters::endRead() { reading_ = false; }

uint8_t RtcRegisters::read(uint8_t address) {
  sync();
  address = canonicalAddress(address);
  if (reading_ && chip_ != Chip::kRv8803 && address >= timeBase() &&
      address < timeBase() + 7) {
    return snapshot_[address - timeBase()];
  }
  if (chip_ == Chip::kRv8803 && address == 0x10) {
    return Bcd(fraction_us_ / 10000);
  }
  return data_[address];
}

uint8_t RtcRegisters::statusWrite(uint8_t old, uint8_t value) const {
  if (isDs323x()) {
    const uint8_t settings = chip_ == Chip::kDs3231 ? 8 : 0x78;
    return (old & value & 0x83) | (value & settings) | (old & 4);
  }
  if (chip_ == Chip::kRv3028) return (old & value & 0x7f) | (old & 0x80);
  uint8_t flags = old & value & 0x3f;
  if ((value & 3) != 3) flags &= ~3;
  return flags;
}

void RtcRegisters::write(uint8_t address, uint8_t value) {
  sync();
  address = canonicalAddress(address);
  const uint8_t old = data_[address];
  if ((isDs323x() && address == 0x0f) ||
      ((chip_ == Chip::kRv3028 || chip_ == Chip::kRv8803) && address == 0x0e)) {
    data_[address] = statusWrite(old, value);
    return;
  }
  if (chip_ == Chip::kPcf8523 && address == 2) {
    data_[address] = (value & 0xe3) | (old & value & 8) | (old & 4);
    return;
  }
  if (isDs323x() && (address == 0x11 || address == 0x12)) return;
  if (chip_ == Chip::kPcf8523 && address == 0 && value == 0x58) {
    reset();
    return;
  }
  if (chip_ == Chip::kPcf8523 && address == 3)
    value = (value & 0x7f) | (old & value & 0x80);
  if (chip_ == Chip::kPcf8523 && address == 0) value &= 0xaf;
  if (chip_ == Chip::kRv3028 && address == 0x10) {
    Calendar before;
    const bool valid = decode(before);
    data_[address] = value & 0xfe;
    if (valid && ((old ^ value) & 2) != 0) encode(before);
    if ((value & 1) != 0) fraction_us_ = 0;
    return;
  }
  if (chip_ == Chip::kRv8803 && address == 0x0f) value &= 0x3d;
  data_[address] = value;
  if (address == timeBase() || stopped()) fraction_us_ = 0;
}

void RtcRegisters::simulateTimeLoss() {
  if (isDs323x()) {
    data_[0x0f] |= 0x80;
  } else if (chip_ == Chip::kDs1307) {
    data_[0] |= 0x80;
  } else if (chip_ == Chip::kPcf8523) {
    data_[3] |= 0x80;
  } else {
    data_[0x0e] |= chip_ == Chip::kRv3028 ? 1 : 3;
  }
}

void RtcRegisters::injectRegister(uint8_t address, uint8_t value) {
  sync();
  data_[canonicalAddress(address)] = value;
}

void RtcRegisters::setTemperature(double celsius) {
  if (celsius < -128) celsius = -128;
  if (celsius > 127.75) celsius = 127.75;
  const int quarters = static_cast<int>(celsius * 4);
  data_[0x11] = static_cast<uint8_t>(quarters >> 2);
  data_[0x12] = (quarters & 3) << 6;
}
}  // namespace roo_testing_rtc
