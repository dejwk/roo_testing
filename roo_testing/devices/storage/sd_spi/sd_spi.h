#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "roo_testing/buses/spi/fake_spi.h"

/// Models SD SPI presence commands CMD0 and CMD58 across active-low CS frames.
/// Supports mode 0, MSB first, up to 25 MHz, and validates command CRC bytes.
/// Attach cs() to the board's CS output. Filesystem/block transfers, card
/// initialization commands, power-up clock timing and SDIO are outside this
/// model. Configure and inspect the fake under the same serialization as its
/// SPI bus.
class FakeSdSpiCard : public SimpleFakeSpiDevice {
 public:
  /// Creates an inserted card in idle state with a valid OCR register.
  FakeSdSpiCard();

  /// Exchanges selected command/response bytes; an absent card returns 0xff.
  void transfer(const FakeSpiInterface& spi, uint8_t* data,
                uint16_t bit_count) override;

  /// Simulates insertion/removal. Either transition clears readiness and
  /// frames.
  void setPresent(bool present);

  /// Models the readiness established by initialization outside this probe
  /// model.
  void setReady(bool ready) { ready_ = ready; }

  /// Reports readiness; accepting CMD0 resets it to false.
  bool ready() const { return ready_; }

  /// Injects the raw OCR value, including malformed values for failure tests.
  void setOcr(uint32_t ocr) { ocr_ = ocr; }

  /// Inserts @p bytes dummy 0xff bytes before each command response.
  void setResponseDelay(size_t bytes) { response_delay_ = bytes; }

  /// Counts accepted CMD0 commands, including repeated reset probes.
  uint32_t resetCommands() const { return reset_commands_; }

  /// Counts accepted CMD58 commands, which preserve readiness.
  uint32_t statusCommands() const { return status_commands_; }

  /// Reports unsupported SPI settings/commands, invalid arguments or bad CRC.
  bool protocolError() const { return protocol_error_; }

 protected:
  /// Starts or discards a command/response frame on a CS edge.
  void onChipSelect(bool selected) override;

 private:
  // Exchanges one byte and queues a response after a complete six-byte command.
  uint8_t exchange(uint8_t value);

  // Validates CMD0/CMD58 and prepares the R1/optional OCR response.
  void prepareResponse();

  std::array<uint8_t, 6> command_{};
  std::array<uint8_t, 5> response_{};
  size_t command_size_ = 0;
  size_t response_size_ = 0;
  size_t response_position_ = 0;
  size_t response_delay_ = 0;
  size_t delay_remaining_ = 0;
  uint32_t ocr_ = 0xc0ff8000;
  uint32_t reset_commands_ = 0;
  uint32_t status_commands_ = 0;
  bool present_ = true;
  bool ready_ = false;
  bool protocol_error_ = false;
};
