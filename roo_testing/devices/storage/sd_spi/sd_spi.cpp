#include "roo_testing/devices/storage/sd_spi/sd_spi.h"

#include <algorithm>

FakeSdSpiCard::FakeSdSpiCard() : SimpleFakeSpiDevice("SD SPI card") {}

void FakeSdSpiCard::setPresent(bool present) {
  present_ = present;
  ready_ = false;
  command_size_ = 0;
  response_size_ = 0;
  response_position_ = 0;
}

void FakeSdSpiCard::onChipSelect(bool selected) {
  if (!selected && command_size_ != 0) protocol_error_ = true;
  command_size_ = 0;
  response_size_ = 0;
  response_position_ = 0;
  delay_remaining_ = 0;
}

void FakeSdSpiCard::transfer(const FakeSpiInterface& spi, uint8_t* data,
                             uint16_t bit_count) {
  if (data == nullptr || bit_count == 0) return;
  const size_t size = (bit_count + 7) / 8;
  if (!isSelected() || !present_) {
    std::fill(data, data + size, 0xff);
    return;
  }
  if (bit_count % 8 != 0 || spi.dataMode() != kSpiMode0 ||
      spi.bitOrder() != kSpiMsbFirst || spi.clkHz() == 0 ||
      spi.clkHz() > 25000000) {
    protocol_error_ = true;
    std::fill(data, data + size, 0xff);
    return;
  }
  for (size_t i = 0; i < size; ++i) data[i] = exchange(data[i]);
}

uint8_t FakeSdSpiCard::exchange(uint8_t value) {
  if (response_position_ < response_size_) {
    if (delay_remaining_ != 0) {
      --delay_remaining_;
      return 0xff;
    }
    return response_[response_position_++];
  }
  if (command_size_ == 0 && (value & 0xc0) != 0x40) return 0xff;
  command_[command_size_++] = value;
  if (command_size_ == command_.size()) {
    prepareResponse();
    command_size_ = 0;
  }
  return 0xff;
}

void FakeSdSpiCard::prepareResponse() {
  response_position_ = 0;
  response_size_ = 1;
  delay_remaining_ = response_delay_;
  const bool reset = command_[0] == 0x40;
  const bool status = command_[0] == 0x7a;
  if (!reset && !status) {
    protocol_error_ = true;
    response_[0] = 0x04;  // Illegal command.
    return;
  }
  if (command_[1] != 0 || command_[2] != 0 || command_[3] != 0 ||
      command_[4] != 0 || command_[5] != (reset ? 0x95 : 0xfd)) {
    protocol_error_ = true;
    response_[0] = 0x08;  // Command CRC/argument error.
    return;
  }
  if (reset) {
    ++reset_commands_;
    ready_ = false;
  } else {
    ++status_commands_;
    response_size_ = 5;
    for (size_t i = 0; i < 4; ++i) {
      response_[i + 1] = static_cast<uint8_t>(ocr_ >> (24 - 8 * i));
    }
  }
  response_[0] = ready_ ? 0x00 : 0x01;
}
