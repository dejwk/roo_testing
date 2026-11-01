#include "roo_testing/devices/clock/ds3234/ds3234.h"

#include <algorithm>

FakeDs3234::FakeDs3234(roo_testing_transducers::Clock* clock)
    : SimpleFakeSpiDevice("DS3234"),
      registers_(roo_testing_rtc::Chip::kDs3234, clock) {}

void FakeDs3234::onChipSelect(bool selected) {
  if (selected) {
    registers_.beginRead();  // Snapshot on the falling CS edge, before command.
  } else {
    registers_.endRead();
  }
  need_command_ = true;
  frame_valid_ = true;
}

void FakeDs3234::transfer(const FakeSpiInterface& spi, uint8_t* data,
                          uint16_t bits) {
  if (data == nullptr || bits == 0) return;
  const size_t size = (bits + 7) / 8;
  if (!isSelected()) {
    std::fill(data, data + size, 0xff);
    return;
  }
  if (bits % 8 != 0 || spi.bitOrder() != kSpiMsbFirst ||
      (spi.dataMode() != kSpiMode1 && spi.dataMode() != kSpiMode3) ||
      spi.clkHz() == 0 || spi.clkHz() > 4000000) {
    protocol_error_ = true;
    frame_valid_ = false;
  }
  for (size_t i = 0; i < size; ++i) {
    const uint8_t incoming = data[i];
    if (!frame_valid_) {
      data[i] = 0xff;
      continue;
    }
    if (need_command_) {
      writing_ = (incoming & 0x80) != 0;
      address_ = incoming & 0x7f;
      need_command_ = false;
      if (address_ >= registers_.size()) {
        protocol_error_ = true;
        frame_valid_ = false;
        data[i] = 0xff;
      } else {
        data[i] = 0;
      }
    } else {
      if (writing_) {
        registers_.write(address_, incoming);
        data[i] = 0;
      } else {
        data[i] = registers_.read(address_);
      }
      address_ = registers_.nextAddress(address_);
      if (address_ == 0 && !writing_) registers_.beginRead();
    }
  }
}

void FakeDs3234::simulateTimeLoss() { registers_.simulateTimeLoss(); }

void FakeDs3234::injectRegister(uint8_t address, uint8_t value) {
  registers_.injectRegister(address, value);
}

void FakeDs3234::reset() {
  registers_.reset();
  need_command_ = true;
  frame_valid_ = true;
  protocol_error_ = false;
}
