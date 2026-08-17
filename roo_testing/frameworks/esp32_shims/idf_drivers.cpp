#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_rom_gpio.h"

#include <array>
#include <cstring>
#include <memory>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32_spi_struct.h"

namespace {

constexpr bool ValidGpio(gpio_num_t gpio) {
  return static_cast<int>(gpio) >= 0 && static_cast<int>(gpio) < 40;
}

struct UartState {
  bool installed = false;
  uint32_t baud = 115200;
  uart_word_length_t word_length = UART_DATA_8_BITS;
  uart_stop_bits_t stop_bits = UART_STOP_BITS_1;
  uart_parity_t parity = UART_PARITY_DISABLE;
};

std::array<UartState, UART_NUM_MAX> g_uart;

bool ValidUart(uart_port_t port) {
  return static_cast<unsigned>(port) < g_uart.size();
}

}  // namespace

struct spi_device_t {
  spi_host_device_t host;
  spi_device_interface_config_t config;
  spi_transaction_t* queued = nullptr;
};

extern "C" {

void esp_rom_gpio_connect_in_signal(uint32_t gpio, uint32_t signal, bool invert) {
  FakeEsp32().in_matrix.assign(gpio, signal, invert);
}

void esp_rom_gpio_connect_out_signal(uint32_t gpio, uint32_t signal,
                                     bool out_invert, bool oen_invert) {
  FakeEsp32().out_matrix.assign(gpio, signal, out_invert, oen_invert);
}

void gpio_matrix_in(uint32_t gpio, uint32_t signal, bool invert) {
  esp_rom_gpio_connect_in_signal(gpio, signal, invert);
}

void gpio_matrix_out(uint32_t gpio, uint32_t signal, bool out_invert,
                     bool oen_invert) {
  esp_rom_gpio_connect_out_signal(gpio, signal, out_invert, oen_invert);
}

esp_err_t gpio_config(const gpio_config_t* config) {
  return config == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}
esp_err_t gpio_reset_pin(gpio_num_t gpio) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_set_intr_type(gpio_num_t gpio, gpio_int_type_t) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_intr_enable(gpio_num_t gpio) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_intr_disable(gpio_num_t gpio) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_set_level(gpio_num_t gpio, uint32_t level) {
  if (!ValidGpio(gpio)) return ESP_ERR_INVALID_ARG;
  FakeEsp32().gpio.get(gpio).digitalWrite(
      level == 0 ? roo_testing_transducers::kDigitalLow
                 : roo_testing_transducers::kDigitalHigh);
  return ESP_OK;
}
int gpio_get_level(gpio_num_t gpio) {
  if (!ValidGpio(gpio)) return 0;
  return FakeEsp32().gpio.get(gpio).read() > 1.7F;
}
esp_err_t gpio_set_direction(gpio_num_t gpio, gpio_mode_t) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_input_enable(gpio_num_t gpio) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_set_pull_mode(gpio_num_t gpio, gpio_pull_mode_t) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_wakeup_enable(gpio_num_t gpio, gpio_int_type_t) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_wakeup_disable(gpio_num_t gpio) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_isr_register(void (*)(void*), void*, int,
                            gpio_isr_handle_t* handle) {
  if (handle != nullptr) *handle = nullptr;
  return ESP_OK;
}
esp_err_t gpio_pullup_en(gpio_num_t gpio) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_pullup_dis(gpio_num_t gpio) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_pulldown_en(gpio_num_t gpio) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_pulldown_dis(gpio_num_t gpio) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_install_isr_service(int) { return ESP_OK; }
void gpio_uninstall_isr_service(void) {}
esp_err_t gpio_isr_handler_add(gpio_num_t gpio, gpio_isr_t, void*) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_isr_handler_remove(gpio_num_t gpio) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_set_drive_capability(gpio_num_t gpio, gpio_drive_cap_t) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_get_drive_capability(gpio_num_t gpio,
                                    gpio_drive_cap_t* strength) {
  if (!ValidGpio(gpio) || strength == nullptr) return ESP_ERR_INVALID_ARG;
  *strength = GPIO_DRIVE_CAP_2;
  return ESP_OK;
}
esp_err_t gpio_hold_en(gpio_num_t gpio) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_hold_dis(gpio_num_t gpio) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
void gpio_deep_sleep_hold_en(void) {}
void gpio_deep_sleep_hold_dis(void) {}
esp_err_t gpio_force_hold_all(void) { return ESP_OK; }
esp_err_t gpio_force_unhold_all(void) { return ESP_OK; }
esp_err_t gpio_sleep_sel_en(gpio_num_t gpio) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_sleep_sel_dis(gpio_num_t gpio) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_sleep_set_direction(gpio_num_t gpio, gpio_mode_t) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_sleep_set_pull_mode(gpio_num_t gpio, gpio_pull_mode_t) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_deep_sleep_wakeup_enable(gpio_num_t gpio, gpio_int_type_t) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t gpio_deep_sleep_wakeup_disable(gpio_num_t gpio) {
  return ValidGpio(gpio) ? ESP_OK : ESP_ERR_INVALID_ARG;
}

esp_err_t uart_driver_install(uart_port_t port, int, int, int,
                              QueueHandle_t* queue, int) {
  if (!ValidUart(port)) return ESP_ERR_INVALID_ARG;
  g_uart[port].installed = true;
  if (queue != nullptr) *queue = nullptr;
  return ESP_OK;
}
esp_err_t uart_driver_delete(uart_port_t port) {
  if (!ValidUart(port)) return ESP_ERR_INVALID_ARG;
  g_uart[port].installed = false;
  return ESP_OK;
}
bool uart_is_driver_installed(uart_port_t port) {
  return ValidUart(port) && g_uart[port].installed;
}
esp_err_t uart_param_config(uart_port_t port, const uart_config_t* config) {
  if (!ValidUart(port) || config == nullptr) return ESP_ERR_INVALID_ARG;
  g_uart[port].baud = config->baud_rate;
  g_uart[port].word_length = config->data_bits;
  g_uart[port].stop_bits = config->stop_bits;
  g_uart[port].parity = config->parity;
  return ESP_OK;
}
esp_err_t uart_set_pin(uart_port_t port, int, int, int, int) {
  return ValidUart(port) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t uart_set_baudrate(uart_port_t port, uint32_t baud) {
  if (!ValidUart(port)) return ESP_ERR_INVALID_ARG;
  g_uart[port].baud = baud;
  return ESP_OK;
}
esp_err_t uart_get_baudrate(uart_port_t port, uint32_t* baud) {
  if (!ValidUart(port) || baud == nullptr) return ESP_ERR_INVALID_ARG;
  *baud = g_uart[port].baud;
  return ESP_OK;
}
esp_err_t uart_set_word_length(uart_port_t port, uart_word_length_t length) {
  if (!ValidUart(port)) return ESP_ERR_INVALID_ARG;
  g_uart[port].word_length = length;
  return ESP_OK;
}
esp_err_t uart_get_word_length(uart_port_t port, uart_word_length_t* length) {
  if (!ValidUart(port) || length == nullptr) return ESP_ERR_INVALID_ARG;
  *length = g_uart[port].word_length;
  return ESP_OK;
}
esp_err_t uart_set_stop_bits(uart_port_t port, uart_stop_bits_t bits) {
  if (!ValidUart(port)) return ESP_ERR_INVALID_ARG;
  g_uart[port].stop_bits = bits;
  return ESP_OK;
}
esp_err_t uart_get_stop_bits(uart_port_t port, uart_stop_bits_t* bits) {
  if (!ValidUart(port) || bits == nullptr) return ESP_ERR_INVALID_ARG;
  *bits = g_uart[port].stop_bits;
  return ESP_OK;
}
esp_err_t uart_set_parity(uart_port_t port, uart_parity_t parity) {
  if (!ValidUart(port)) return ESP_ERR_INVALID_ARG;
  g_uart[port].parity = parity;
  return ESP_OK;
}
esp_err_t uart_get_parity(uart_port_t port, uart_parity_t* parity) {
  if (!ValidUart(port) || parity == nullptr) return ESP_ERR_INVALID_ARG;
  *parity = g_uart[port].parity;
  return ESP_OK;
}
esp_err_t uart_get_sclk_freq(uart_sclk_t, uint32_t* frequency) {
  if (frequency == nullptr) return ESP_ERR_INVALID_ARG;
  *frequency = 80000000;
  return ESP_OK;
}
esp_err_t uart_set_line_inverse(uart_port_t port, uint32_t) {
  return ValidUart(port) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t uart_set_hw_flow_ctrl(uart_port_t port, uart_hw_flowcontrol_t,
                                uint8_t) {
  return ValidUart(port) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t uart_set_sw_flow_ctrl(uart_port_t port, bool, uint8_t, uint8_t) {
  return ValidUart(port) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t uart_wait_tx_done(uart_port_t port, TickType_t) {
  return ValidUart(port) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
int uart_write_bytes(uart_port_t port, const void* source, size_t size) {
  if (!ValidUart(port) || (source == nullptr && size != 0)) return -1;
  return static_cast<int>(FakeEsp32().uart(port).write(
      static_cast<const uint8_t*>(source), size));
}
int uart_write_bytes_with_break(uart_port_t port, const void* source,
                                size_t size, int) {
  return uart_write_bytes(port, source, size);
}
int uart_read_bytes(uart_port_t port, void* destination, uint32_t length,
                    TickType_t) {
  if (!ValidUart(port) || (destination == nullptr && length != 0)) return -1;
  return static_cast<int>(FakeEsp32().uart(port).read(
      static_cast<uint8_t*>(destination), length));
}
esp_err_t uart_flush(uart_port_t port) {
  return ValidUart(port) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t uart_flush_input(uart_port_t port) {
  if (!ValidUart(port)) return ESP_ERR_INVALID_ARG;
  uint8_t buffer[64];
  while (FakeEsp32().uart(port).read(buffer, sizeof(buffer)) != 0) {
  }
  return ESP_OK;
}
esp_err_t uart_get_buffered_data_len(uart_port_t port, size_t* size) {
  if (!ValidUart(port) || size == nullptr) return ESP_ERR_INVALID_ARG;
  *size = FakeEsp32().uart(port).availableForRead();
  return ESP_OK;
}

esp_err_t i2c_driver_install(i2c_port_t port, i2c_mode_t, size_t, size_t,
                             int) {
  return static_cast<unsigned>(port) < 2 ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t i2c_driver_delete(i2c_port_t port) {
  return static_cast<unsigned>(port) < 2 ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t i2c_param_config(i2c_port_t port, const i2c_config_t* config) {
  return static_cast<unsigned>(port) < 2 && config != nullptr
             ? ESP_OK
             : ESP_ERR_INVALID_ARG;
}
esp_err_t i2c_reset_tx_fifo(i2c_port_t port) {
  if (static_cast<unsigned>(port) >= 2) return ESP_ERR_INVALID_ARG;
  FakeEsp32().i2c(port).resetTxFifo();
  return ESP_OK;
}
esp_err_t i2c_reset_rx_fifo(i2c_port_t port) {
  return static_cast<unsigned>(port) < 2 ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t i2c_set_pin(i2c_port_t port, int, int, bool, bool, i2c_mode_t) {
  return static_cast<unsigned>(port) < 2 ? ESP_OK : ESP_ERR_INVALID_ARG;
}
esp_err_t i2c_master_write_to_device(i2c_port_t port, uint8_t address,
                                     const uint8_t* data, size_t size,
                                     TickType_t timeout) {
  if (static_cast<unsigned>(port) >= 2) return ESP_ERR_INVALID_ARG;
  return FakeEsp32().i2c(port).write(address, data, size, timeout);
}
esp_err_t i2c_master_read_from_device(i2c_port_t port, uint8_t address,
                                      uint8_t* data, size_t size,
                                      TickType_t timeout) {
  if (static_cast<unsigned>(port) >= 2) return ESP_ERR_INVALID_ARG;
  return FakeEsp32().i2c(port).read(address, data, size, timeout);
}
esp_err_t i2c_master_write_read_device(i2c_port_t port, uint8_t address,
                                       const uint8_t* write_buffer,
                                       size_t write_size, uint8_t* read_buffer,
                                       size_t read_size, TickType_t timeout) {
  esp_err_t result = i2c_master_write_to_device(
      port, address, write_buffer, write_size, timeout);
  return result == ESP_OK
             ? i2c_master_read_from_device(port, address, read_buffer,
                                           read_size, timeout)
             : result;
}

esp_err_t spi_bus_initialize(spi_host_device_t, const spi_bus_config_t* config,
                             spi_dma_chan_t) {
  return config == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}
esp_err_t spi_bus_free(spi_host_device_t) { return ESP_OK; }
esp_err_t spi_bus_add_device(spi_host_device_t host,
                             const spi_device_interface_config_t* config,
                             spi_device_handle_t* handle) {
  if (config == nullptr || handle == nullptr) return ESP_ERR_INVALID_ARG;
  *handle = new spi_device_t{host, *config, nullptr};
  return ESP_OK;
}
esp_err_t spi_bus_remove_device(spi_device_handle_t handle) {
  delete handle;
  return ESP_OK;
}
esp_err_t spi_device_polling_transmit(spi_device_handle_t handle,
                                      spi_transaction_t* transaction) {
  if (handle == nullptr || transaction == nullptr) return ESP_ERR_INVALID_ARG;
  const size_t bytes = (transaction->length + 7) / 8;
  if (transaction->rx_buffer != nullptr && transaction->tx_buffer != nullptr) {
    memcpy(transaction->rx_buffer, transaction->tx_buffer, bytes);
  } else if (transaction->rx_buffer != nullptr) {
    memset(transaction->rx_buffer, 0xFF, bytes);
  }
  return ESP_OK;
}
esp_err_t spi_device_transmit(spi_device_handle_t handle,
                              spi_transaction_t* transaction) {
  return spi_device_polling_transmit(handle, transaction);
}
esp_err_t spi_device_queue_trans(spi_device_handle_t handle,
                                 spi_transaction_t* transaction, TickType_t) {
  if (handle == nullptr || transaction == nullptr) return ESP_ERR_INVALID_ARG;
  const esp_err_t result = spi_device_polling_transmit(handle, transaction);
  handle->queued = transaction;
  return result;
}
esp_err_t spi_device_get_trans_result(spi_device_handle_t handle,
                                      spi_transaction_t** transaction,
                                      TickType_t) {
  if (handle == nullptr || transaction == nullptr) return ESP_ERR_INVALID_ARG;
  *transaction = handle->queued;
  handle->queued = nullptr;
  return *transaction == nullptr ? ESP_ERR_TIMEOUT : ESP_OK;
}
esp_err_t spi_device_polling_start(spi_device_handle_t handle,
                                   spi_transaction_t* transaction, TickType_t) {
  return spi_device_queue_trans(handle, transaction, 0);
}
esp_err_t spi_device_polling_end(spi_device_handle_t handle, TickType_t) {
  if (handle == nullptr) return ESP_ERR_INVALID_ARG;
  handle->queued = nullptr;
  return ESP_OK;
}
esp_err_t spi_device_acquire_bus(spi_device_handle_t device, TickType_t) {
  return device == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}
void spi_device_release_bus(spi_device_handle_t) {}
esp_err_t spi_device_get_actual_freq(spi_device_handle_t handle,
                                     int* frequency_khz) {
  if (handle == nullptr || frequency_khz == nullptr) return ESP_ERR_INVALID_ARG;
  *frequency_khz = handle->config.clock_speed_hz / 1000;
  return ESP_OK;
}
esp_err_t spi_bus_get_max_transaction_len(spi_host_device_t, size_t* bytes) {
  if (bytes == nullptr) return ESP_ERR_INVALID_ARG;
  *bytes = 4092;
  return ESP_OK;
}

}  // extern "C"
