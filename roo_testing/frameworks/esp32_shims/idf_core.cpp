#include <errno.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <random>
#include <vector>

#include "esp_bt.h"
#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "esp_rom_gpio.h"
#include "esp_rom_sys.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "roo_testing/system/timer.h"
#include "shim_internal.h"

namespace {

std::array<uint8_t, 6> g_base_mac = {
    roo_testing::esp32_shims::kDefaultMac[0],
    roo_testing::esp32_shims::kDefaultMac[1],
    roo_testing::esp32_shims::kDefaultMac[2],
    roo_testing::esp32_shims::kDefaultMac[3],
    roo_testing::esp32_shims::kDefaultMac[4],
    roo_testing::esp32_shims::kDefaultMac[5],
};

std::vector<shutdown_handler_t>& ShutdownHandlers() {
  static auto* handlers = new std::vector<shutdown_handler_t>();
  return *handlers;
}

std::atomic<esp_log_level_t> g_log_level{ESP_LOG_INFO};
vprintf_like_t g_vprintf = &vprintf;

constexpr size_t kReportedHeapSize = 64U * 1024U * 1024U;

}  // namespace

extern "C" {

uint32_t esp_random(void) {
  thread_local std::mt19937 generator(std::random_device{}());
  return generator();
}

void esp_fill_random(void* buffer, size_t length) {
  auto* out = static_cast<uint8_t*>(buffer);
  while (length != 0) {
    const uint32_t value = esp_random();
    const size_t count = std::min(length, sizeof(value));
    memcpy(out, &value, count);
    out += count;
    length -= count;
  }
}

int64_t esp_timer_get_time(void) { return system_time_get_micros(); }

esp_err_t esp_timer_early_init(void) { return ESP_OK; }
esp_err_t esp_timer_init(void) { return ESP_OK; }
esp_err_t esp_timer_deinit(void) { return ESP_OK; }
int64_t esp_timer_get_next_alarm(void) { return INT64_MAX; }
int64_t esp_timer_get_next_alarm_for_wake_up(void) { return INT64_MAX; }
void esp_timer_isr_dispatch_need_yield(void) {}

void esp_rom_delay_us(uint32_t us) { system_time_delay_micros(us); }
void ets_delay_us(uint32_t us) { esp_rom_delay_us(us); }

int esp_rom_vprintf(const char* format, va_list args) {
  return vprintf(format, args);
}

int esp_rom_printf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  const int result = esp_rom_vprintf(format, args);
  va_end(args);
  return result;
}

int ets_printf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  const int result = esp_rom_vprintf(format, args);
  va_end(args);
  return result;
}

void esp_rom_install_channel_putc(int, void (*)(char)) {}
void esp_rom_install_uart_printf(void) {}
void esp_rom_output_to_channels(char c) { fputc(c, stdout); }
void ets_install_putc1(void (*)(char)) {}
void ets_install_putc2(void (*)(char)) {}
void ets_install_uart_printf(void) {}
void ets_write_char_uart(char c) { fputc(c, stdout); }
void esp_rom_route_intr_matrix(int, uint32_t, uint32_t) {}
void intr_matrix_set(int, uint32_t, uint32_t) {}
uint32_t esp_rom_get_cpu_ticks_per_us(void) { return 240; }
void esp_rom_set_cpu_ticks_per_us(uint32_t) {}
void ets_update_cpu_frequency(uint32_t) {}
uint32_t ets_get_cpu_frequency(void) { return 240; }

void esp_rom_gpio_pad_select_gpio(uint32_t) {}
void esp_rom_gpio_pad_pullup_only(uint32_t) {}
void esp_rom_gpio_pad_unhold(uint32_t) {}
void esp_rom_gpio_pad_set_drv(uint32_t, uint32_t) {}

const char* esp_get_idf_version(void) { return IDF_VER; }

void esp_chip_info(esp_chip_info_t* info) {
  if (info == nullptr) return;
  *info = {
      .model = CHIP_ESP32,
      .features = CHIP_FEATURE_WIFI_BGN | CHIP_FEATURE_BT | CHIP_FEATURE_BLE,
      .revision = 300,
      .cores = 2,
  };
}

esp_err_t esp_base_mac_addr_set(const uint8_t* mac) {
  if (mac == nullptr || (mac[0] & 1U) != 0) return ESP_ERR_INVALID_ARG;
  std::copy_n(mac, g_base_mac.size(), g_base_mac.begin());
  return ESP_OK;
}

esp_err_t esp_base_mac_addr_get(uint8_t* mac) {
  if (mac == nullptr) return ESP_ERR_INVALID_ARG;
  std::copy(g_base_mac.begin(), g_base_mac.end(), mac);
  return ESP_OK;
}

esp_err_t esp_efuse_mac_get_default(uint8_t* mac) {
  return esp_base_mac_addr_get(mac);
}

esp_err_t esp_efuse_mac_get_custom(uint8_t* mac) {
  return esp_base_mac_addr_get(mac);
}

esp_err_t esp_read_mac(uint8_t* mac, esp_mac_type_t type) {
  if (mac == nullptr) return ESP_ERR_INVALID_ARG;
  std::copy(g_base_mac.begin(), g_base_mac.end(), mac);
  if (type != ESP_MAC_WIFI_STA && type != ESP_MAC_BASE &&
      type != ESP_MAC_EFUSE_FACTORY) {
    mac[5] = static_cast<uint8_t>(mac[5] + static_cast<unsigned>(type));
  }
  return ESP_OK;
}

esp_err_t esp_derive_local_mac(uint8_t* local_mac,
                               const uint8_t* universal_mac) {
  if (local_mac == nullptr || universal_mac == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  std::copy_n(universal_mac, 6, local_mac);
  local_mac[0] ^= (local_mac[0] & 0x02U) != 0 ? 0x04U : 0x02U;
  return ESP_OK;
}

esp_err_t esp_iface_mac_addr_set(const uint8_t*, esp_mac_type_t) {
  return ESP_OK;
}

size_t esp_mac_addr_len_get(esp_mac_type_t) { return 6; }

esp_err_t esp_register_shutdown_handler(shutdown_handler_t handler) {
  if (handler == nullptr) return ESP_ERR_INVALID_ARG;
  auto& handlers = ShutdownHandlers();
  if (std::find(handlers.begin(), handlers.end(), handler) != handlers.end()) {
    return ESP_ERR_INVALID_STATE;
  }
  handlers.push_back(handler);
  return ESP_OK;
}

esp_err_t esp_unregister_shutdown_handler(shutdown_handler_t handler) {
  auto& handlers = ShutdownHandlers();
  const auto it = std::find(handlers.begin(), handlers.end(), handler);
  if (it == handlers.end()) return ESP_ERR_INVALID_STATE;
  handlers.erase(it);
  return ESP_OK;
}

void esp_restart(void) {
  for (auto it = ShutdownHandlers().rbegin(); it != ShutdownHandlers().rend();
       ++it) {
    (*it)();
  }
  fflush(nullptr);
  _Exit(0);
}

esp_reset_reason_t esp_reset_reason(void) { return ESP_RST_POWERON; }
uint32_t esp_get_free_heap_size(void) { return kReportedHeapSize; }
uint32_t esp_get_free_internal_heap_size(void) { return kReportedHeapSize; }
uint32_t esp_get_minimum_free_heap_size(void) { return kReportedHeapSize; }

void esp_system_abort(const char* details) {
  fprintf(stderr, "ESP system abort: %s\n", details == nullptr ? "" : details);
  abort();
}

void* heap_caps_malloc(size_t size, uint32_t) { return malloc(size); }
void* heap_caps_calloc(size_t count, size_t size, uint32_t) {
  return calloc(count, size);
}
void* heap_caps_realloc(void* ptr, size_t size, uint32_t) {
  return realloc(ptr, size);
}
void heap_caps_free(void* ptr) { free(ptr); }

void* heap_caps_aligned_alloc(size_t alignment, size_t size, uint32_t) {
  void* result = nullptr;
  if (posix_memalign(&result, alignment, size) != 0) return nullptr;
  return result;
}

void heap_caps_aligned_free(void* ptr) { free(ptr); }

void* heap_caps_aligned_calloc(size_t alignment, size_t count, size_t size,
                               uint32_t caps) {
  const size_t total = count * size;
  void* result = heap_caps_aligned_alloc(alignment, total, caps);
  if (result != nullptr) memset(result, 0, total);
  return result;
}

size_t heap_caps_get_total_size(uint32_t caps) {
  return (caps & MALLOC_CAP_SPIRAM) != 0 ? 0 : kReportedHeapSize;
}
size_t heap_caps_get_free_size(uint32_t caps) {
  return heap_caps_get_total_size(caps);
}
size_t heap_caps_get_minimum_free_size(uint32_t caps) {
  return heap_caps_get_total_size(caps);
}
size_t heap_caps_get_largest_free_block(uint32_t caps) {
  return heap_caps_get_total_size(caps);
}
size_t heap_caps_get_allocated_size(void* ptr) {
#if defined(__GLIBC__)
  return ptr == nullptr ? 0 : malloc_usable_size(ptr);
#else
  (void)ptr;
  return 0;
#endif
}
size_t heap_caps_get_containing_block_size(void* ptr) {
  return heap_caps_get_allocated_size(ptr);
}
bool esp_ptr_dma_capable(const void* p) { return p != nullptr; }
bool esp_ptr_executable(const void* p) { return p != nullptr; }
bool esp_ptr_byte_accessible(const void* p) { return p != nullptr; }
bool esp_ptr_internal(const void* p) { return p != nullptr; }
bool esp_ptr_external_ram(const void*) { return false; }

vprintf_like_t esp_log_set_vprintf(vprintf_like_t function) {
  vprintf_like_t old = g_vprintf;
  g_vprintf = function == nullptr ? &vprintf : function;
  return old;
}

void esp_log_writev(esp_log_level_t level, const char*, const char* format,
                    va_list args) {
  if (level <= g_log_level.load()) g_vprintf(format, args);
}

void esp_log_write(esp_log_level_t level, const char* tag, const char* format,
                   ...) {
  va_list args;
  va_start(args, format);
  esp_log_writev(level, tag, format, args);
  va_end(args);
}

void esp_log_level_set(const char*, esp_log_level_t level) {
  g_log_level.store(level);
}
esp_log_level_t esp_log_level_get(const char*) { return g_log_level.load(); }
void esp_log_set_default_level(esp_log_level_t level) {
  g_log_level.store(level);
}
uint32_t esp_log_timestamp(void) {
  return static_cast<uint32_t>(esp_timer_get_time() / 1000);
}
uint32_t esp_log_early_timestamp(void) { return esp_log_timestamp(); }

void _esp_error_check_failed(esp_err_t rc, const char* file, int line,
                             const char* function, const char* expression) {
  fprintf(stderr, "ESP_ERROR_CHECK failed: %s returned 0x%x at %s:%d (%s)\n",
          expression, static_cast<unsigned>(rc), file, line, function);
  abort();
}

void _esp_error_check_failed_without_abort(esp_err_t rc, const char* file,
                                           int line, const char* function,
                                           const char* expression) {
  fprintf(stderr,
          "ESP_ERROR_CHECK_WITHOUT_ABORT: %s returned 0x%x at %s:%d (%s)\n",
          expression, static_cast<unsigned>(rc), file, line, function);
}

esp_err_t esp_task_wdt_init(const esp_task_wdt_config_t*) { return ESP_OK; }
esp_err_t esp_task_wdt_reconfigure(const esp_task_wdt_config_t*) {
  return ESP_OK;
}
esp_err_t esp_task_wdt_deinit(void) { return ESP_OK; }
esp_err_t esp_task_wdt_add(TaskHandle_t) { return ESP_OK; }
esp_err_t esp_task_wdt_reset(void) { return ESP_OK; }
esp_err_t esp_task_wdt_delete(TaskHandle_t) { return ESP_OK; }
esp_err_t esp_task_wdt_status(TaskHandle_t) { return ESP_OK; }

esp_err_t esp_bt_controller_mem_release(esp_bt_mode_t) { return ESP_OK; }
esp_err_t esp_bt_mem_release(esp_bt_mode_t) { return ESP_OK; }

esp_err_t esp_sleep_enable_timer_wakeup(uint64_t) { return ESP_OK; }
esp_err_t esp_sleep_disable_wakeup_source(esp_sleep_source_t) { return ESP_OK; }
esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause(void) {
  return ESP_SLEEP_WAKEUP_UNDEFINED;
}
esp_err_t esp_light_sleep_start(void) { return ESP_OK; }
void esp_deep_sleep_start(void) { _Exit(0); }
void esp_deep_sleep(uint64_t time_us) {
  system_time_delay_micros(time_us);
  _Exit(0);
}

uint8_t temprature_sens_read(void) { return 75; }

esp_err_t esp_intr_alloc(int, int, intr_handler_t, void*,
                         intr_handle_t* return_handle) {
  if (return_handle != nullptr) *return_handle = nullptr;
  return ESP_OK;
}
esp_err_t esp_intr_alloc_intrstatus(int source, int flags, uint32_t, uint32_t,
                                    intr_handler_t handler, void* arg,
                                    intr_handle_t* return_handle) {
  return esp_intr_alloc(source, flags, handler, arg, return_handle);
}
esp_err_t esp_intr_free(intr_handle_t) { return ESP_OK; }
esp_err_t esp_intr_enable(intr_handle_t) { return ESP_OK; }
esp_err_t esp_intr_disable(intr_handle_t) { return ESP_OK; }

}  // extern "C"
