#include "esp32-hal.h"
#include "esp32-hal-bt.h"
#include "esp32-hal-cpu.h"
#include "esp32-hal-ledc.h"
#include "esp32-hal-psram.h"

#include <stdlib.h>

#include <algorithm>
#include <mutex>
#include <utility>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "roo_testing/system/timer.h"

namespace {
arduino_panic_handler_t panic_handler = nullptr;
void *panic_arg = nullptr;
uint32_t cpu_frequency_mhz = 240;
bool bluetooth_started = false;
uint32_t analog_frequency[SOC_GPIO_PIN_COUNT] = {};
uint8_t analog_resolution[SOC_GPIO_PIN_COUNT] = {};

struct ApbCallback {
  void *arg;
  apb_change_cb_t callback;
};
std::mutex callback_mutex;
std::vector<ApbCallback> apb_callbacks;
}  // namespace

extern "C" {

void yield(void) { taskYIELD(); }

unsigned long micros() { return static_cast<unsigned long>(system_time_get_micros()); }
unsigned long millis() { return micros() / 1000UL; }
void delay(uint32_t milliseconds) { system_time_delay_micros(static_cast<uint64_t>(milliseconds) * 1000); }
void delayMicroseconds(uint32_t microseconds) { system_time_delay_micros(microseconds); }

void analogWrite(uint8_t pin, int value) {
  if (pin >= SOC_GPIO_PIN_COUNT) return;
  if (!ledcReadFreq(pin)) {
    ledcAttach(pin, analog_frequency[pin] ? analog_frequency[pin] : 1000,
               analog_resolution[pin] ? analog_resolution[pin] : 8);
  }
  ledcWrite(pin, std::max(0, value));
}
void analogWriteFrequency(uint8_t pin, uint32_t frequency) {
  if (pin >= SOC_GPIO_PIN_COUNT || frequency == 0) return;
  analog_frequency[pin] = frequency;
  if (ledcReadFreq(pin)) ledcChangeFrequency(pin, frequency, analog_resolution[pin] ? analog_resolution[pin] : 8);
}
void analogWriteResolution(uint8_t pin, uint8_t bits) {
  if (pin >= SOC_GPIO_PIN_COUNT || bits == 0 || bits > 20) return;
  analog_resolution[pin] = bits;
  if (ledcReadFreq(pin)) ledcChangeFrequency(pin, analog_frequency[pin] ? analog_frequency[pin] : 1000, bits);
}

float temperatureRead() { return 25.0f; }
bool testSPIRAM(void) { return true; }
void enableLoopWDT() {}
void disableLoopWDT() {}
void feedLoopWDT() {}
void enableCore0WDT() {}
bool disableCore0WDT() { return true; }
void enableCore1WDT() {}
bool disableCore1WDT() { return true; }

BaseType_t xTaskCreateUniversal(TaskFunction_t task, const char *const name,
                                const uint32_t stack_depth, void *const parameter,
                                UBaseType_t priority, TaskHandle_t *const handle,
                                const BaseType_t) {
  return xTaskCreate(task, name, stack_depth, parameter, priority, handle);
}

void arduino_phy_init() {}
void initArduino() {}

void set_arduino_panic_handler(arduino_panic_handler_t handler, void *arg) {
  panic_handler = handler;
  panic_arg = arg;
}
arduino_panic_handler_t get_arduino_panic_handler(void) { return panic_handler; }
void *get_arduino_panic_handler_arg(void) { return panic_arg; }

bool addApbChangeCallback(void *arg, apb_change_cb_t callback) {
  if (!callback) return false;
  std::lock_guard<std::mutex> lock(callback_mutex);
  apb_callbacks.push_back({arg, callback});
  return true;
}
bool removeApbChangeCallback(void *arg, apb_change_cb_t callback) {
  std::lock_guard<std::mutex> lock(callback_mutex);
  auto it = std::find_if(apb_callbacks.begin(), apb_callbacks.end(),
                         [&](const ApbCallback &entry) {
                           return entry.arg == arg && entry.callback == callback;
                         });
  if (it == apb_callbacks.end()) return false;
  apb_callbacks.erase(it);
  return true;
}
bool setCpuFrequencyMhz(uint32_t frequency) {
  if (frequency == 0) return false;
  const uint32_t old_apb = getApbFrequency();
  std::lock_guard<std::mutex> lock(callback_mutex);
  for (const auto &entry : apb_callbacks) entry.callback(entry.arg, APB_BEFORE_CHANGE, old_apb, old_apb);
  cpu_frequency_mhz = frequency;
  for (const auto &entry : apb_callbacks) entry.callback(entry.arg, APB_AFTER_CHANGE, old_apb, getApbFrequency());
  return true;
}
const char *getSupportedCpuFrequencyMhz(uint8_t) { return "240,160,80,40,20,10"; }
const char *getClockSourceName(uint8_t) { return "host"; }
uint32_t getCpuFrequencyMhz() { return cpu_frequency_mhz; }
uint32_t getXtalFrequencyMhz() { return 40; }
uint32_t getApbFrequency() { return std::min<uint32_t>(cpu_frequency_mhz, 80) * 1000000U; }

bool psramInit() { return false; }
bool psramAddToHeap() { return false; }
bool psramFound() { return false; }
void *ps_malloc(size_t size) { return malloc(size); }
void *ps_calloc(size_t count, size_t size) { return calloc(count, size); }
void *ps_realloc(void *ptr, size_t size) { return realloc(ptr, size); }

bool btInUse(void) { return bluetooth_started; }
bool btStarted() { return bluetooth_started; }
bool btStart() { bluetooth_started = true; return true; }
bool btStartMode(bt_mode) { bluetooth_started = true; return true; }
bool btStop() { bluetooth_started = false; return true; }

}  // extern "C"
