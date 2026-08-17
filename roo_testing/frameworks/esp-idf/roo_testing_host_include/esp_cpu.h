#pragma once

// Host implementation of ESP-IDF's CPU-facing inline API. The Linux emulator
// publishes an ESP32 target identity, but it must not expose Xtensa built-ins
// or silently fall through to IDF's RISC-V implementation.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"

#if !defined(ROO_TESTING_SOC_ESP32)
#error "Add a CPU host shim before enabling another roo_testing SoC profile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t esp_cpu_cycle_count_t;

typedef enum {
  ESP_CPU_INTR_TYPE_LEVEL = 0,
  ESP_CPU_INTR_TYPE_EDGE,
  ESP_CPU_INTR_TYPE_NA,
} esp_cpu_intr_type_t;

typedef struct {
  int priority;
  esp_cpu_intr_type_t type;
  uint32_t flags;
} esp_cpu_intr_desc_t;

#define ESP_CPU_INTR_DESC_FLAG_SPECIAL 0x01
#define ESP_CPU_INTR_DESC_FLAG_RESVD 0x02

typedef void (*esp_cpu_intr_handler_t)(void* arg);

typedef enum {
  ESP_CPU_WATCHPOINT_LOAD,
  ESP_CPU_WATCHPOINT_STORE,
  ESP_CPU_WATCHPOINT_ACCESS,
} esp_cpu_watchpoint_trigger_t;

static inline void esp_cpu_stall(int core_id) { (void)core_id; }
static inline void esp_cpu_unstall(int core_id) { (void)core_id; }
static inline void esp_cpu_reset(int core_id) { (void)core_id; }
static inline void esp_cpu_wait_for_intr(void) {}

static inline int esp_cpu_get_core_id(void) { return 0; }
static inline int esp_cpu_get_curr_privilege_level(void) { return -1; }
static inline void* esp_cpu_get_sp(void) { return NULL; }
static inline esp_cpu_cycle_count_t esp_cpu_get_cycle_count(void) { return 0; }
static inline void esp_cpu_set_cycle_count(esp_cpu_cycle_count_t value) {
  (void)value;
}
static inline void* esp_cpu_pc_to_addr(uint32_t pc) {
  return (void*)(uintptr_t)pc;
}
static inline void esp_cpu_set_threadptr(void* threadptr) { (void)threadptr; }
static inline void* esp_cpu_get_threadptr(void) { return NULL; }

static inline void esp_cpu_intr_get_desc(int core_id, int intr_num,
                                         esp_cpu_intr_desc_t* result) {
  (void)core_id;
  (void)intr_num;
  if (result != NULL) {
    result->priority = -1;
    result->type = ESP_CPU_INTR_TYPE_NA;
    result->flags = 0;
  }
}
static inline void esp_cpu_intr_set_ivt_addr(const void* address) {
  (void)address;
}
static inline void esp_cpu_intr_set_mtvt_addr(const void* address) {
  (void)address;
}
static inline void esp_cpu_intr_set_xtvt_addr(const void* address) {
  (void)address;
}
static inline void esp_cpu_disable_wfe_mode(void) {}
static inline void esp_cpu_intr_set_type(int intr_num,
                                         esp_cpu_intr_type_t type) {
  (void)intr_num;
  (void)type;
}
static inline esp_cpu_intr_type_t esp_cpu_intr_get_type(int intr_num) {
  (void)intr_num;
  return ESP_CPU_INTR_TYPE_NA;
}
static inline void esp_cpu_intr_set_priority(int intr_num, int priority) {
  (void)intr_num;
  (void)priority;
}
static inline int esp_cpu_intr_get_priority(int intr_num) {
  (void)intr_num;
  return -1;
}
static inline bool esp_cpu_intr_has_handler(int intr_num) {
  (void)intr_num;
  return false;
}
static inline void esp_cpu_intr_set_handler(int intr_num,
                                             esp_cpu_intr_handler_t handler,
                                             void* handler_arg) {
  (void)intr_num;
  (void)handler;
  (void)handler_arg;
}
static inline void* esp_cpu_intr_get_handler_arg(int intr_num) {
  (void)intr_num;
  return NULL;
}
static inline void esp_cpu_intr_enable(uint32_t intr_mask) { (void)intr_mask; }
static inline void esp_cpu_intr_disable(uint32_t intr_mask) {
  (void)intr_mask;
}
static inline uint32_t esp_cpu_intr_get_enabled_mask(void) { return 0; }
static inline void esp_cpu_intr_edge_ack(int intr_num) { (void)intr_num; }

static inline void esp_cpu_configure_region_protection(void) {}
static inline esp_err_t esp_cpu_set_breakpoint(int number,
                                               const void* address) {
  (void)number;
  (void)address;
  return ESP_OK;
}
static inline esp_err_t esp_cpu_clear_breakpoint(int number) {
  (void)number;
  return ESP_OK;
}
static inline esp_err_t esp_cpu_set_watchpoint(
    int number, const void* address, size_t size,
    esp_cpu_watchpoint_trigger_t trigger) {
  (void)number;
  (void)address;
  (void)size;
  (void)trigger;
  return ESP_OK;
}
static inline esp_err_t esp_cpu_clear_watchpoint(int number) {
  (void)number;
  return ESP_OK;
}
static inline bool esp_cpu_dbgr_is_attached(void) { return false; }
static inline void esp_cpu_dbgr_break(void) {}
static inline intptr_t esp_cpu_get_call_addr(intptr_t return_address) {
  return return_address - 3;
}
static inline bool esp_cpu_compare_and_set(volatile uint32_t* address,
                                            uint32_t compare_value,
                                            uint32_t new_value) {
  if (address == NULL) return false;
  return __atomic_compare_exchange_n(address, &compare_value, new_value, false,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}
static inline void esp_cpu_branch_prediction_enable(void) {}
static inline void esp_cpu_branch_prediction_disable(void) {}

#ifdef __cplusplus
}
#endif
