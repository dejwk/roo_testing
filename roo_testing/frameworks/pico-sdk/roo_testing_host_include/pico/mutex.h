#pragma once
#include "pico.h"
#ifdef __cplusplus
extern "C" {
#endif
/// A checked single-core, non-ISR mutex; no thread/interrupt exclusion is
/// claimed.
typedef struct {
  bool locked;
} mutex_t;
/// Initializes an unlocked native core-0 mutex.
void mutex_init(mutex_t* mutex);
/// Acquires if unlocked, returning false and owner 0 on recursive contention.
bool mutex_try_enter(mutex_t* mutex, uint32_t* owner_out);
/// Acquires an unlocked mutex; contention is invalid in this bare-metal
/// runtime.
void mutex_enter_blocking(mutex_t* mutex);
/// Releases a held mutex.
void mutex_exit(mutex_t* mutex);
#define auto_init_mutex(name) static mutex_t name = {false}

#ifdef __cplusplus
}
#endif
