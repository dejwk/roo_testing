#include <array>
#include <thread>

#include "Arduino.h"
#include "glog/logging.h"
#include "pico/time.h"
#include "roo_testing/system/timer.h"

void __clearADCPin(pin_size_t pin);

namespace {
std::array<PinMode, 30> modes{};

/// Makes unsupported interrupt context explicit instead of inventing masking.
void UnsupportedInterrupt() {
  LOG(FATAL) << "Unimplemented: Arduino-Pico interrupts";
}
}  // namespace

extern "C" {
void init() { system_time_is_auto_sync_enabled(); }
void __attribute__((weak)) initVariant() {}
void yield() { std::this_thread::yield(); }
unsigned long micros() { return static_cast<uint32_t>(time_us_64()); }
unsigned long millis() { return static_cast<uint32_t>(time_us_64() / 1000); }
void delay(unsigned long ms) {
  CHECK_LE(ms, UINT64_MAX / 1000);
  sleep_us(uint64_t{ms} * 1000);
}

void delayMicroseconds(unsigned int us) { sleep_us(us); }
void interrupts() { UnsupportedInterrupt(); }
void noInterrupts() { UnsupportedInterrupt(); }
void pinMode(pin_size_t pin, PinMode mode) {
  CHECK_LT(pin, 30) << "Invalid Arduino-Pico pin";
  CHECK(mode == INPUT || mode == INPUT_PULLUP || mode == INPUT_PULLDOWN ||
        mode == OUTPUT || mode == OUTPUT_4MA)
      << "Unimplemented: Arduino-Pico drive-strength/open-drain mode";
  __clearADCPin(pin);
  gpio_set_function(pin, GPIO_FUNC_SIO);
  gpio_set_dir(pin, false);
  gpio_set_pulls(pin, mode == INPUT_PULLUP, mode == INPUT_PULLDOWN);
  if (mode == OUTPUT || mode == OUTPUT_4MA) gpio_set_dir(pin, true);
  modes[pin] = mode;
}

void digitalWrite(pin_size_t pin, PinStatus value) {
  CHECK_LT(pin, 30) << "Invalid Arduino-Pico pin";
  CHECK(value == LOW || value == HIGH) << "Invalid Arduino-Pico digital level";
  gpio_set_function(pin, GPIO_FUNC_SIO);
  if (modes[pin] == INPUT_PULLUP) {
    gpio_put(pin, false);
    gpio_set_dir(pin, value == LOW);
  } else if (modes[pin] == INPUT_PULLDOWN) {
    gpio_put(pin, true);
    gpio_set_dir(pin, value == HIGH);
  } else {
    gpio_put(pin, value == HIGH);
  }
}

PinStatus digitalRead(pin_size_t pin) { return gpio_get(pin) ? HIGH : LOW; }
void analogReference(uint8_t) {
  LOG(FATAL) << "Unimplemented: Arduino-Pico analog reference";
}

void attachInterrupt(pin_size_t, voidFuncPtr, PinStatus) {
  UnsupportedInterrupt();
}

void attachInterruptParam(pin_size_t, voidFuncPtrParam, PinStatus, void*) {
  UnsupportedInterrupt();
}

void detachInterrupt(pin_size_t) { UnsupportedInterrupt(); }
unsigned long pulseIn(pin_size_t, uint8_t, unsigned long) {
  LOG(FATAL) << "Unimplemented: Arduino-Pico pulseIn";
  return 0;
}

unsigned long pulseInLong(pin_size_t, uint8_t, unsigned long) {
  LOG(FATAL) << "Unimplemented: Arduino-Pico pulseInLong";
  return 0;
}
}

const String emptyString;
