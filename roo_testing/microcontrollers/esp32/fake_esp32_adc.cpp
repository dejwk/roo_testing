#include "fake_esp32_adc.h"

#include "fake_esp32.h"
#include "glog/logging.h"

Esp32Adc::Esp32Adc(FakeEsp32Board& board, std::vector<int8_t> channel_to_pin)
    : board_(board), channel_to_pin_(std::move(channel_to_pin)), width_(3) {
  std::fill_n(attenuation_, 10, 0);
}

void Esp32Adc::setWidth(uint8_t width) {
  DCHECK_GE(width, 0);
  DCHECK_LE(width, 3);
  width_ = width;
}

void Esp32Adc::setAttenuation(uint8_t channel, uint8_t attenuation) {
  DCHECK_GE(attenuation, 0);
  DCHECK_LE(attenuation, 3);
  attenuation_[channel] = attenuation;
}

uint8_t Esp32Adc::getAttenuation(uint8_t channel) {
  return attenuation_[channel];
}

int16_t Esp32Adc::convert(int channel) {
  int8_t pin = channel_to_pin_[channel];
  float voltage = board_.gpio.get(pin).read();
  int16_t value = 0;
  switch (attenuation_[channel]) {
    // https://people.eecs.berkeley.edu/~boser/courses/49_sp_2019/N_gpio.html
    case 0: {
      // 0 db.
      if (voltage < 0.065) {
        value = 0;
      } else if (voltage < 1.03) {
        value = (uint16_t)((voltage - 0.065) * 4243.5);
      } else {
        value = 4095;
      }
      break;
    }
    case 1: {
      // 2.5 db.
      if (voltage < 0.068) {
        value = 0;
      } else if (voltage < 1.36) {
        value = (uint16_t)((voltage - 0.068) * 3169.5);
      } else {
        value = 4095;
      }
      break;
    }
    case 2: {
      // 6 db.
      if (voltage < 0.093) {
        value = 0;
      } else if (voltage < 1.88) {
        value = (uint16_t)((voltage - 0.093) * 2291.5);
      } else {
        value = 4095;
      }
      break;
    }
    case 3: {
      // 11 db.
      if (voltage < 0.13) {
        value = 0;
      } else if (voltage < 3.12) {
        value = (uint16_t)((voltage - 0.13) * 1233.4);
        if (voltage > 2.5) {
          // ADC behaves non-linearly in this range.
          float d = voltage - 2.5;
          value += (d * d) * 1059;
        }
      } else {
        value = 4095;
      }
      break;
    }
    default: {
      value = 0;
      break;
    }
  }
  // Add noise.
  int16_t offset = rand() / (RAND_MAX / 16) - (RAND_MAX / 32);
  value += offset;
  if (value < 0) value = 0;
  if (value > 4095) value = 4095;

  // Shift result if necessary
  return value >> (3 - width_);
}
