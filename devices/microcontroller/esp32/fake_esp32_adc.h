#pragma once

#include <inttypes.h>

#include <vector>

class FakeEsp32Board;

class Esp32Adc {
 public:
  Esp32Adc(FakeEsp32Board& board, std::vector<int8_t> channel_to_pin);

  void setAttenuation(uint8_t channel, uint8_t attenuation);

  uint8_t getAttenuation(uint8_t channel);

  void setWidth(uint8_t width);

  int16_t convert(int channel);

 private:
  FakeEsp32Board& board_;
  std::vector<int8_t> channel_to_pin_;
  uint8_t attenuation_[10];
  uint8_t width_;
};
