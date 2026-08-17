#include "esp32-hal-periman.h"

#include <array>
#include <mutex>
#include <string>

namespace {

constexpr size_t kPinCount = 40;

struct PinAssignment {
  peripheral_bus_type_t type = ESP32_BUS_TYPE_INIT;
  void* bus = nullptr;
  int8_t bus_number = -1;
  int8_t bus_channel = -1;
  std::string extra_type;
};

std::array<PinAssignment, kPinCount> g_pins;
std::array<peripheral_bus_deinit_cb_t, ESP32_BUS_TYPE_MAX> g_deinit = {};
std::mutex g_mutex;

}  // namespace

extern "C" {

const char* perimanGetTypeName(peripheral_bus_type_t type) {
  switch (type) {
    case ESP32_BUS_TYPE_INIT: return "INIT";
    case ESP32_BUS_TYPE_GPIO: return "GPIO";
    case ESP32_BUS_TYPE_UART_RX: return "UART_RX";
    case ESP32_BUS_TYPE_UART_TX: return "UART_TX";
    case ESP32_BUS_TYPE_UART_CTS: return "UART_CTS";
    case ESP32_BUS_TYPE_UART_RTS: return "UART_RTS";
#if SOC_ADC_SUPPORTED
    case ESP32_BUS_TYPE_ADC_ONESHOT: return "ADC_ONESHOT";
    case ESP32_BUS_TYPE_ADC_CONT: return "ADC_CONT";
#endif
#if SOC_LEDC_SUPPORTED
    case ESP32_BUS_TYPE_LEDC: return "LEDC";
#endif
#if SOC_I2C_SUPPORTED
    case ESP32_BUS_TYPE_I2C_MASTER_SDA: return "I2C_MASTER_SDA";
    case ESP32_BUS_TYPE_I2C_MASTER_SCL: return "I2C_MASTER_SCL";
    case ESP32_BUS_TYPE_I2C_SLAVE_SDA: return "I2C_SLAVE_SDA";
    case ESP32_BUS_TYPE_I2C_SLAVE_SCL: return "I2C_SLAVE_SCL";
#endif
#if SOC_GPSPI_SUPPORTED
    case ESP32_BUS_TYPE_SPI_MASTER_SCK: return "SPI_MASTER_SCK";
    case ESP32_BUS_TYPE_SPI_MASTER_MISO: return "SPI_MASTER_MISO";
    case ESP32_BUS_TYPE_SPI_MASTER_MOSI: return "SPI_MASTER_MOSI";
    case ESP32_BUS_TYPE_SPI_MASTER_SS: return "SPI_MASTER_SS";
#endif
    case ESP32_BUS_TYPE_MAX: return "INVALID";
    default: return "PERIPHERAL";
  }
}

bool perimanPinIsValid(uint8_t pin) { return pin < kPinCount; }

bool perimanSetPinBus(uint8_t pin, peripheral_bus_type_t type, void* bus,
                      int8_t bus_number, int8_t bus_channel) {
  if (!perimanPinIsValid(pin) || type >= ESP32_BUS_TYPE_MAX) return false;
  peripheral_bus_deinit_cb_t callback = nullptr;
  void* previous_bus = nullptr;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    const auto& previous = g_pins[pin];
    if (previous.type != ESP32_BUS_TYPE_INIT && previous.type != type &&
        previous.type < ESP32_BUS_TYPE_MAX) {
      callback = g_deinit[previous.type];
      previous_bus = previous.bus;
    }
  }
  if (callback != nullptr && !callback(previous_bus)) return false;
  std::lock_guard<std::mutex> lock(g_mutex);
  g_pins[pin] = PinAssignment{type, bus, bus_number, bus_channel, {}};
  return true;
}

void* perimanGetPinBus(uint8_t pin, peripheral_bus_type_t type) {
  if (!perimanPinIsValid(pin)) return nullptr;
  std::lock_guard<std::mutex> lock(g_mutex);
  return g_pins[pin].type == type ? g_pins[pin].bus : nullptr;
}

peripheral_bus_type_t perimanGetPinBusType(uint8_t pin) {
  if (!perimanPinIsValid(pin)) return ESP32_BUS_TYPE_MAX;
  std::lock_guard<std::mutex> lock(g_mutex);
  return g_pins[pin].type;
}

int8_t perimanGetPinBusNum(uint8_t pin) {
  if (!perimanPinIsValid(pin)) return -1;
  std::lock_guard<std::mutex> lock(g_mutex);
  return g_pins[pin].bus_number;
}

int8_t perimanGetPinBusChannel(uint8_t pin) {
  if (!perimanPinIsValid(pin)) return -1;
  std::lock_guard<std::mutex> lock(g_mutex);
  return g_pins[pin].bus_channel;
}

bool perimanSetBusDeinit(peripheral_bus_type_t type,
                         peripheral_bus_deinit_cb_t callback) {
  if (type >= ESP32_BUS_TYPE_MAX) return false;
  std::lock_guard<std::mutex> lock(g_mutex);
  g_deinit[type] = callback;
  return true;
}

bool perimanClearBusDeinit(peripheral_bus_type_t type) {
  return perimanSetBusDeinit(type, nullptr);
}

peripheral_bus_deinit_cb_t perimanGetBusDeinit(peripheral_bus_type_t type) {
  if (type >= ESP32_BUS_TYPE_MAX) return nullptr;
  std::lock_guard<std::mutex> lock(g_mutex);
  return g_deinit[type];
}

bool perimanSetPinBusExtraType(uint8_t pin, const char* extra_type) {
  if (!perimanPinIsValid(pin)) return false;
  std::lock_guard<std::mutex> lock(g_mutex);
  g_pins[pin].extra_type = extra_type == nullptr ? "" : extra_type;
  return true;
}

const char* perimanGetPinBusExtraType(uint8_t pin) {
  if (!perimanPinIsValid(pin)) return nullptr;
  std::lock_guard<std::mutex> lock(g_mutex);
  return g_pins[pin].extra_type.empty() ? nullptr
                                        : g_pins[pin].extra_type.c_str();
}

}  // extern "C"
