#include "glog/logging.h"
#include "pico/mutex.h"

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

#include "hardware/clocks.h"
extern "C" uint32_t clock_get_hz(clock_handle_t clock) {
  CHECK_EQ(clock, clk_sys);
  return 125000000;
}
