
#include "driver/uart.h"

#include <glog/logging.h>

#include "esp_err.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "soc/uart_struct.h"

extern "C" {

uart_dev_t UART0;
uart_dev_t UART1;
uart_dev_t UART2;

int uart_write_bytes(uart_port_t uart_num, const void* src, size_t size) {
  FakeEsp32().uart(uart_num).write(reinterpret_cast<const uint8_t*>(src), size);
  return size;
}

int uart_read_bytes(uart_port_t uart_num, void* buf, uint32_t length,
                    TickType_t ticks_to_wait) {
  return FakeEsp32().uart(uart_num).read(reinterpret_cast<uint8_t*>(buf),
                                         length);
}

esp_err_t uart_get_buffered_data_len(uart_port_t uart_num, size_t* size) {
  *size = FakeEsp32().uart(uart_num).availableForRead();
  return ESP_OK;
}

}
