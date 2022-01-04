// CRC lookup tables, generator and use implementation by Arjen Lentz <arjen
// (at) lentz (dot) com (dot) au>

// Copyright (C) 1992-2017 Arjen Lentz

// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer. Redistributions in binary
// form must reproduce the above copyright notice, this list of conditions and
// the following disclaimer in the documentation and/or other materials provided
// with the distribution. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
// NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "fake_onewire.h"

#include <assert.h>

// Dow-CRC using polynomial X^8 + X^5 + X^4 + X^0
// Tiny 2x16 entry CRC table created by Arjen Lentz
// See
// http://lentz.com.au/blog/calculating-crc-with-a-tiny-32-entry-lookup-table
static const uint8_t dscrc2x16_table[] = {
    0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83, 0xC2, 0x9C, 0x7E,
    0x20, 0xA3, 0xFD, 0x1F, 0x41, 0x00, 0x9D, 0x23, 0xBE, 0x46, 0xDB,
    0x65, 0xF8, 0x8C, 0x11, 0xAF, 0x32, 0xCA, 0x57, 0xE9, 0x74};

uint8_t onewire_crc(const uint8_t *buf, int len) {
  uint8_t crc = 0;
  for (int i = 0; i < len; ++i) {
    crc = buf[i] ^ crc;
    crc =
        dscrc2x16_table[crc & 0x0f] ^ dscrc2x16_table[16 + ((crc >> 4) & 0x0f)];
  }
  return crc;
}

FakeOneWireDevice::Rom::Rom(uint8_t model, uint64_t serial) {
  rom_[0] = model;
  for (int i = 0; i < 6; ++i) {
    rom_[i + 1] = (serial >> (i * 8)) & 0xFF;
  }
  rom_[7] = onewire_crc(rom_, 7);
}

inline static int8_t ParseHexNibble(const char **in) {
  char c;
  do {
    c = *(*in)++;
  } while (c == ' ');
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  } else if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  } else {
    return -1;
  }
}

inline static int16_t ParseHexByte(const char **in) {
  int8_t hi = ParseHexNibble(in);
  if (hi < 0) return -1;
  int8_t lo = ParseHexNibble(in);
  if (lo < 0) return -1;
  return hi << 4 | lo;
}

FakeOneWireDevice::Rom::Rom(const char* address) {
  const char *current = address;
  int i = 0;
  while (i < 8) {
    int16_t v = ParseHexByte(&current);
    if (v < 0) break;
    rom_[i++] = v & 0xFF;
  }
  assert (i >= 7);
  if (i == 7) {
    rom_[7] = onewire_crc(rom_, 7);
  } else {
    assert (rom_[7] == onewire_crc(rom_, 7));
  }
}

uint64_t FakeOneWireDevice::Rom::serial() const {
  return ((uint64_t)rom_[1] << 0) | ((uint64_t)rom_[2] << 8) |
         ((uint64_t)rom_[3] << 16) | ((uint64_t)rom_[4] << 24) |
         ((uint64_t)rom_[5] << 32) | ((uint64_t)rom_[6] << 40);
}

void FakeOneWireBaseDevice::reset() {
  state_ = ROM_COMMAND;
  bits_received_ = 0;
  bits_sent_ = 0;
  rom_command_ = 0;
  function_command_ = 0;
}

void FakeOneWireBaseDevice::write_bit(bool bit) {
  switch (state_) {
    case INACTIVE: {
      return;
    }
    case ROM_COMMAND: {
      // Reading the command.
      rom_command_ >>= 1;
      if (bit) rom_command_ |= 0x80;
      ++bits_received_;
      if (bits_received_ >= 8) {
        if (rom_command_ == 0xCC) {
          // Skip ROM. Broadcast.
          state_ = FUNCTION_COMMAND;
        } else {
          state_ = ROM_COMMAND_DATA_EXCHANGE;
        }
        bits_received_ = 0;
      }
      return;
    }
    case ROM_COMMAND_DATA_EXCHANGE: {
      switch (rom_command_) {
        case 0x55:    // Match ROM
        case 0xF0: {  // Search ROM
          bool address_bit = rom().get_bit(bits_received_);
          ++bits_received_;
          if (address_bit != bit) {
            // Found mismatch.
            state_ = INACTIVE;
            return;
          }
          if (bits_received_ >= 64) {
            state_ = FUNCTION_COMMAND;
            bits_received_ = 0;
          }
          return;
        }
        default: { assert(false); }
      }
      break;
    }
    case FUNCTION_COMMAND: {
      // Reading the command.
      function_command_ >>= 1;
      if (bit) function_command_ |= 0x80;
      ++bits_received_;
      if (bits_received_ >= 8) {
        state_ = FUNCTION_COMMAND_DATA_EXCHANGE;
        bits_received_ = 0;
        start_function_command();
      }
      return;
    }
    case FUNCTION_COMMAND_DATA_EXCHANGE: {
      write_function_bit(bit);
      ++bits_received_;
      return;
    }
  }
}

bool FakeOneWireBaseDevice::read_bit() {
  switch (state_) {
    case INACTIVE: {
      return true;  // the master performs AND of bits from all devices on the
                    // line.
    }
    case ROM_COMMAND_DATA_EXCHANGE: {
      switch (rom_command_) {
        case 0xF0: {  // Search ROM
          bool address_bit = rom().get_bit(bits_received_);
          bool flip = (bits_sent_ % 2);
          if (++bits_sent_ >= 128) {
            state_ = INACTIVE;
          }
          return address_bit ^ flip;
        }
        case 0x33: {  // Read ROM
          bool address_bit = rom().get_bit(bits_sent_);
          if (++bits_sent_ >= 64) {
            state_ = FUNCTION_COMMAND;
          }
          return address_bit;
        }
        default: { assert(false); }
      }
    }
    case FUNCTION_COMMAND_DATA_EXCHANGE: {
      bool result = read_function_bit();
      ++bits_sent_;
      return result;
    }
    default: { assert(false); }
  }
}

FakeOneWireInterface::FakeOneWireInterface() {}

FakeOneWireInterface::FakeOneWireInterface(
    std::initializer_list<FakeOneWireDevice *> devices)
    : FakeOneWireInterface() {
  for (auto device : devices) {
    addDevice(device);
  }
}

void FakeOneWireInterface::addDevice(
    std::unique_ptr<FakeOneWireDevice> device) {
  devices_.insert(std::make_pair(device->rom().serial(), std::move(device)));
}

FakeOneWireDevice *FakeOneWireInterface::getDevice(
    FakeOneWireDevice::Rom rom) const {
  auto i = devices_.find(rom.serial());
  return (i == devices_.end()) ? nullptr : i->second.get();
}

bool FakeOneWireInterface::reset() {
  for (auto &i : devices_) {
    i.second->reset();
  }
  return !devices_.empty();
}

void FakeOneWireInterface::write_bit(bool bit) {
  for (auto &i : devices_) {
    i.second->write_bit(bit);
  }
}

bool FakeOneWireInterface::read_bit() {
  bool result = true;
  for (auto &i : devices_) {
    result &= i.second->read_bit();
  }
  return result;
}
