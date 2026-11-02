#pragma once
#include "pico.h"
#if defined(__FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif
/// A non-ISR SDK mutex; the FreeRTOS profile supports task contention.
/// The bare-metal profile retains checked core-0 ownership.
typedef struct {
#if defined(__FREERTOS)
  StaticSemaphore_t storage;
  SemaphoreHandle_t handle;
#else
  bool locked;
#endif
} mutex_t;
/// Initializes an unlocked native core-0 mutex.
void mutex_init(mutex_t* mutex);
/// Acquires if unlocked, returning false and owner 0 on recursive contention.
bool mutex_try_enter(mutex_t* mutex, uint32_t* owner_out);
/// Waits for another task to release the mutex in the FreeRTOS profile.
/// Bare-metal contention is invalid.
void mutex_enter_blocking(mutex_t* mutex);
/// Releases a held mutex.
void mutex_exit(mutex_t* mutex);
#if defined(__FREERTOS)
#define auto_init_mutex(name) static mutex_t name = {}
#else
#define auto_init_mutex(name) static mutex_t name = {false}
#endif

#ifdef __cplusplus
}
#endif
