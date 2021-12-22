#include "esp_system.h"
#include <cstdlib>
#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

void system_init(void) {}

void system_restore(void) {}

esp_err_t esp_register_shutdown_handler(shutdown_handler_t handle) { return 0; }

void esp_restart(void) {
  exit(0);
}

void system_restart(void) { esp_restart(); }

// esp_reset_reason_t esp_reset_reason(void) {
//   return 0;
// }

uint32_t system_get_time(void) {
  return 0;
}

uint32_t esp_get_free_heap_size(void) {
  return 200000;
}

uint32_t system_get_free_heap_size(void) { return 1000; }

uint32_t esp_get_minimum_free_heap_size( void ) { 
  return mallinfo().uordblks; 
}

uint32_t esp_random(void) { return 1; }

void esp_fill_random(void *buf, size_t len) {}

esp_err_t esp_base_mac_addr_set(uint8_t *mac) { return 0; }

esp_err_t esp_base_mac_addr_get(uint8_t *mac) { return 0; }

esp_err_t esp_efuse_mac_get_custom(uint8_t *mac) { return 0; }

esp_err_t esp_efuse_mac_get_default(uint8_t *mac) { return 0; }

esp_err_t esp_efuse_read_mac(uint8_t *mac) { return 0; }

esp_err_t system_efuse_read_mac(uint8_t *mac) {
  return 0;    
}

esp_err_t esp_read_mac(uint8_t* mac, esp_mac_type_t type) {
  return 0;
}

esp_err_t esp_derive_local_mac(uint8_t* local_mac, const uint8_t* universal_mac) {
  return 0;  
}

const char* system_get_sdk_version(void) {
    return "100";
}

const char* esp_get_idf_version(void) { return "EMULATED"; }


#ifdef __cplusplus
}
#endif
