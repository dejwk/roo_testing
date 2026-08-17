#pragma once

// Compatibility layer for code which still calls IDF's legacy CPU LL API.
// All CPU and interrupt behavior stays explicitly inert on the host.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t cpu_ll_get_core_id(void) {
  return (uint32_t)esp_cpu_get_core_id();
}
static inline uint32_t cpu_ll_get_cycle_count(void) {
  return (uint32_t)esp_cpu_get_cycle_count();
}
static inline void cpu_ll_set_cycle_count(uint32_t value) {
  esp_cpu_set_cycle_count(value);
}
static inline void* cpu_ll_get_sp(void) { return esp_cpu_get_sp(); }
static inline void cpu_ll_init_hwloop(void) {}
static inline void cpu_ll_set_breakpoint(int number, uint32_t pc) {
  (void)esp_cpu_set_breakpoint(number, (const void*)(uintptr_t)pc);
}
static inline void cpu_ll_clear_breakpoint(int number) {
  (void)esp_cpu_clear_breakpoint(number);
}
static inline uint32_t cpu_ll_ptr_to_pc(const void* address) {
  return (uint32_t)(uintptr_t)address;
}
static inline void* cpu_ll_pc_to_ptr(uint32_t pc) {
  return esp_cpu_pc_to_addr(pc);
}
static inline void cpu_ll_set_watchpoint(int number, const void* address,
                                         size_t size, bool on_read,
                                         bool on_write) {
  const esp_cpu_watchpoint_trigger_t trigger =
      on_read && on_write ? ESP_CPU_WATCHPOINT_ACCESS
      : on_read          ? ESP_CPU_WATCHPOINT_LOAD
                         : ESP_CPU_WATCHPOINT_STORE;
  (void)esp_cpu_set_watchpoint(number, address, size, trigger);
}
static inline void cpu_ll_clear_watchpoint(int number) {
  (void)esp_cpu_clear_watchpoint(number);
}
static inline bool cpu_ll_is_debugger_attached(void) {
  return esp_cpu_dbgr_is_attached();
}
static inline void cpu_ll_break(void) { esp_cpu_dbgr_break(); }
static inline void cpu_ll_set_vecbase(const void* address) {
  esp_cpu_intr_set_ivt_addr(address);
}
static inline void cpu_ll_waiti(void) { esp_cpu_wait_for_intr(); }
static inline void cpu_ll_compare_and_set_native(volatile uint32_t* address,
                                                  uint32_t compare,
                                                  uint32_t* set) {
  if (address == NULL || set == NULL) return;
  uint32_t expected = compare;
  const uint32_t desired = *set;
  (void)__atomic_compare_exchange_n(address, &expected, desired, false,
                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  *set = expected;
}

#ifdef __cplusplus
}
#endif
