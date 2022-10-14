
#include "driver/uart.h"

#include <glog/logging.h>

#include "esp_err.h"
#include "roo_testing/devices/microcontroller/esp32/fake_esp32.h"
#include "soc/uart_struct.h"

extern "C" {

uart_dev_t UART0;
uart_dev_t UART1;
uart_dev_t UART2;

esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size,
                              int tx_buffer_size, int queue_size,
                              QueueHandle_t* uart_queue, int intr_alloc_flags) {
  return ESP_OK;
}

esp_err_t uart_driver_delete(uart_port_t uart_num) { return ESP_OK; }

bool uart_is_driver_installed(uart_port_t uart_num) { return false; }

esp_err_t uart_set_word_length(uart_port_t uart_num,
                               uart_word_length_t data_bit) {
  return ESP_OK;
}

esp_err_t uart_get_word_length(uart_port_t uart_num,
                               uart_word_length_t* data_bit) {
  return ESP_OK;
}

esp_err_t uart_set_stop_bits(uart_port_t uart_num, uart_stop_bits_t stop_bits) {
  return ESP_OK;
}

esp_err_t uart_get_stop_bits(uart_port_t uart_num,
                             uart_stop_bits_t* stop_bits) {
  return ESP_OK;
}

esp_err_t uart_set_parity(uart_port_t uart_num, uart_parity_t parity_mode) {
  return ESP_OK;
}

esp_err_t uart_get_parity(uart_port_t uart_num, uart_parity_t* parity_mode) {
  return ESP_OK;
}

esp_err_t uart_set_baudrate(uart_port_t uart_num, uint32_t baudrate) {
  return ESP_OK;
}

esp_err_t uart_get_baudrate(uart_port_t uart_num, uint32_t* baudrate) {
  return ESP_OK;
}

esp_err_t uart_set_line_inverse(uart_port_t uart_num, uint32_t inverse_mask) {
  return ESP_OK;
}

esp_err_t uart_set_hw_flow_ctrl(uart_port_t uart_num,
                                uart_hw_flowcontrol_t flow_ctrl,
                                uint8_t rx_thresh) {
  return ESP_OK;
}

esp_err_t uart_set_sw_flow_ctrl(uart_port_t uart_num, bool enable,
                                uint8_t rx_thresh_xon, uint8_t rx_thresh_xoff) {
  return ESP_OK;
}

esp_err_t uart_get_hw_flow_ctrl(uart_port_t uart_num,
                                uart_hw_flowcontrol_t* flow_ctrl) {
  return ESP_OK;
}

esp_err_t uart_clear_intr_status(uart_port_t uart_num, uint32_t clr_mask) {
  return ESP_OK;
}

esp_err_t uart_enable_intr_mask(uart_port_t uart_num, uint32_t enable_mask) {
  return ESP_OK;
}

esp_err_t uart_disable_intr_mask(uart_port_t uart_num, uint32_t disable_mask) {
  return ESP_OK;
}

esp_err_t uart_enable_rx_intr(uart_port_t uart_num) { return ESP_OK; }

esp_err_t uart_disable_rx_intr(uart_port_t uart_num) { return ESP_OK; }

esp_err_t uart_disable_tx_intr(uart_port_t uart_num) { return ESP_OK; }

esp_err_t uart_enable_tx_intr(uart_port_t uart_num, int enable, int thresh) {
  return ESP_OK;
}

esp_err_t uart_isr_register(uart_port_t uart_num, void (*fn)(void*), void* arg,
                            int intr_alloc_flags, uart_isr_handle_t* handle) {
  return ESP_OK;
}

esp_err_t uart_isr_free(uart_port_t uart_num) { return ESP_OK; }

esp_err_t uart_set_pin(uart_port_t uart_num, int tx_io_num, int rx_io_num,
                       int rts_io_num, int cts_io_num) {
  return ESP_OK;
}

esp_err_t uart_set_rts(uart_port_t uart_num, int level) { return ESP_OK; }

esp_err_t uart_set_dtr(uart_port_t uart_num, int level) { return ESP_OK; }

esp_err_t uart_set_tx_idle_num(uart_port_t uart_num, uint16_t idle_num) {
  return ESP_OK;
}

esp_err_t uart_param_config(uart_port_t uart_num,
                            const uart_config_t* uart_config) {
  return ESP_OK;
}

esp_err_t uart_intr_config(uart_port_t uart_num,
                           const uart_intr_config_t* intr_conf) {
  return ESP_OK;
}

esp_err_t uart_wait_tx_done(uart_port_t uart_num, TickType_t ticks_to_wait) {
  return ESP_OK;
}

int uart_tx_chars(uart_port_t uart_num, const char* buffer, uint32_t len) {
  return 0;
}

int uart_write_bytes(uart_port_t uart_num, const void* src, size_t size) {
  FakeEsp32().uart(uart_num).write(reinterpret_cast<const uint8_t*>(src), size);
  return size;
}

// int uart_write_bytes_with_break(uart_port_t uart_num, const void* src, size_t
// size, int brk_len) {
//     return 0;
// }

int uart_read_bytes(uart_port_t uart_num, void* buf, uint32_t length,
                    TickType_t ticks_to_wait) {
  return 0;
}

esp_err_t uart_flush(uart_port_t uart_num) { return ESP_OK; }

esp_err_t uart_flush_input(uart_port_t uart_num) { return ESP_OK; }

esp_err_t uart_get_buffered_data_len(uart_port_t uart_num, size_t* size) {
  return ESP_OK;
}

esp_err_t uart_disable_pattern_det_intr(uart_port_t uart_num) { return ESP_OK; }

esp_err_t uart_enable_pattern_det_intr(uart_port_t uart_num, char pattern_chr,
                                       uint8_t chr_num, int chr_tout,
                                       int post_idle, int pre_idle) {
  return ESP_OK;
}

esp_err_t uart_enable_pattern_det_baud_intr(uart_port_t uart_num,
                                            char pattern_chr, uint8_t chr_num,
                                            int chr_tout, int post_idle,
                                            int pre_idle) {
  return ESP_OK;
}

int uart_pattern_pop_pos(uart_port_t uart_num) { return 0; }

int uart_pattern_get_pos(uart_port_t uart_num) { return 0; }

esp_err_t uart_pattern_queue_reset(uart_port_t uart_num, int queue_length) {
  return ESP_OK;
}

esp_err_t uart_set_mode(uart_port_t uart_num, uart_mode_t mode) {
  return ESP_OK;
}

esp_err_t uart_set_rx_full_threshold(uart_port_t uart_num, int threshold) {
  return ESP_OK;
}

esp_err_t uart_set_tx_empty_threshold(uart_port_t uart_num, int threshold) {
  return ESP_OK;
}

esp_err_t uart_set_rx_timeout(uart_port_t uart_num, const uint8_t tout_thresh) {
  return ESP_OK;
}

esp_err_t uart_get_collision_flag(uart_port_t uart_num, bool* collision_flag) {
  return ESP_OK;
}

esp_err_t uart_set_wakeup_threshold(uart_port_t uart_num,
                                    int wakeup_threshold) {
  return ESP_OK;
}

esp_err_t uart_get_wakeup_threshold(uart_port_t uart_num,
                                    int* out_wakeup_threshold) {
  return ESP_OK;
}

esp_err_t uart_wait_tx_idle_polling(uart_port_t uart_num) { return ESP_OK; }

esp_err_t uart_set_loop_back(uart_port_t uart_num, bool loop_back_en) {
  return ESP_OK;
}

void uart_set_always_rx_timeout(uart_port_t uart_num,
                                bool always_rx_timeout_en) {}
}
