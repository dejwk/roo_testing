#pragma once

#include <map>
#include <memory>

#include <stdint.h>

uint8_t onewire_crc(const uint8_t* buf, int len);

class FakeOneWireDevice {
 public:
  class Rom {
   public:
    Rom(uint8_t model, uint64_t serial);
    Rom(const char* address);

    uint8_t model() const { return rom_[0]; }
    uint64_t serial() const;
    uint8_t crc() const { return rom_[7]; }
    const uint8_t* raw() const { return rom_; }

    bool get_bit(int idx) const {
      return (rom_[idx >> 3] >> (idx & 0x7)) & 1;
    }

   private:
    uint8_t rom_[8];
  };

  FakeOneWireDevice(Rom rom) : rom_(rom) {}
  virtual ~FakeOneWireDevice() {}

  const Rom& rom() const { return rom_; }

  virtual void reset() = 0;
  virtual void write_bit(bool bit) = 0;
  virtual bool read_bit() = 0;

 private:
  const Rom rom_;
};

class FakeOneWireBaseDevice : public FakeOneWireDevice {
 public:
  enum State {
    INACTIVE                       = 0,
    ROM_COMMAND                    = 1,
    ROM_COMMAND_DATA_EXCHANGE      = 2,
    FUNCTION_COMMAND               = 3,
    FUNCTION_COMMAND_DATA_EXCHANGE = 4,
  };

  FakeOneWireBaseDevice(Rom rom) : FakeOneWireDevice(rom) {}

  void reset() override;
  void write_bit(bool bit) override;
  bool read_bit() override;

 protected:
  uint8_t function_command() const { return function_command_; }
  int bits_received() const { return bits_received_; }
  int bits_sent() const { return bits_sent_; }

  virtual void start_function_command() {}
  virtual void write_function_bit(bool bit) = 0;
  virtual bool read_function_bit() = 0;

 private:
  State state_;
  int bits_received_;
  int bits_sent_;
  uint8_t rom_command_;
  uint8_t function_command_;
};

class FakeOneWireInterface {
 public:
  FakeOneWireInterface();
  FakeOneWireInterface(std::initializer_list<FakeOneWireDevice*> devices);

  void addDevice(FakeOneWireDevice* device) {
    addDevice(std::unique_ptr<FakeOneWireDevice>(device));
  }

  void addDevice(std::unique_ptr<FakeOneWireDevice> device);

  FakeOneWireDevice* getDevice(FakeOneWireDevice::Rom rom) const;

  bool reset();
  void write_bit(bool bit);
  bool read_bit();

 private:
  std::map<uint64_t, std::unique_ptr<FakeOneWireDevice>> devices_;
};
