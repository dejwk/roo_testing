#include "glog/logging.h"
#include "pico/mutex.h"

#if defined(__FREERTOS)
namespace {
SemaphoreHandle_t Handle(mutex_t* mutex) {
  CHECK(mutex != nullptr);
  portENTER_CRITICAL();
  if (mutex->handle == nullptr) {
    mutex->handle = xSemaphoreCreateRecursiveMutexStatic(&mutex->storage);
  }
  portEXIT_CRITICAL();
  return mutex->handle;
}
}  // namespace
extern "C" {
void mutex_init(mutex_t* mutex) {
  CHECK(mutex != nullptr);
  mutex->handle = xSemaphoreCreateRecursiveMutexStatic(&mutex->storage);
}
bool mutex_try_enter(mutex_t* mutex, uint32_t* owner_out) {
  if (owner_out != nullptr) *owner_out = 0;
  return xSemaphoreTake(Handle(mutex), 0) == pdTRUE;
}
void mutex_enter_blocking(mutex_t* mutex) {
  CHECK_EQ(xSemaphoreTake(Handle(mutex), portMAX_DELAY), pdTRUE);
}
void mutex_exit(mutex_t* mutex) {
  CHECK_EQ(xSemaphoreGive(Handle(mutex)), pdTRUE);
}
SemaphoreHandle_t __get_freertos_mutex_for_ptr(mutex_t* mutex, bool) {
  return Handle(mutex);
}
void __initFreeRTOSMutexes() {}
}
#else

extern "C" {
void mutex_init(mutex_t* mutex) {
  CHECK(mutex != nullptr);
  mutex->locked = false;
}
bool mutex_try_enter(mutex_t* mutex, uint32_t* owner_out) {
  CHECK(mutex != nullptr);
  if (owner_out != nullptr) *owner_out = 0;
  if (mutex->locked) return false;
  mutex->locked = true;
  return true;
}
void mutex_enter_blocking(mutex_t* mutex) {
  CHECK(mutex_try_enter(mutex, nullptr)) << "Pico core-0 mutex contention";
}
void mutex_exit(mutex_t* mutex) {
  CHECK(mutex != nullptr && mutex->locked);
  mutex->locked = false;
}
}

#endif

#include "hardware/clocks.h"
extern "C" uint32_t clock_get_hz(clock_handle_t clock) {
  CHECK_EQ(clock, clk_sys);
  return 125000000;
}
