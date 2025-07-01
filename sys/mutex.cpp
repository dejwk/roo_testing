#include "roo_testing/sys/mutex.h"

#include "freertos/semphr.h"
#include "glog/logging.h"

namespace roo_testing {

mutex::mutex() noexcept { xSemaphoreCreateMutexStatic(&mutex_); }

void mutex::lock() {
  xSemaphoreTake((SemaphoreHandle_t)&mutex_, portMAX_DELAY);
}

bool mutex::try_lock() { return xSemaphoreTake((SemaphoreHandle_t)&mutex_, 0); }

void mutex::unlock() { xSemaphoreGive((SemaphoreHandle_t)&mutex_); }

namespace internal {

void checkLockUnowned(const void* lock, bool owns) {
  CHECK_NOTNULL(lock);
  CHECK(!owns) << "Deadlock would occur";
}

void checkLockOwned(bool owns) {
  CHECK(owns) << "Lock not owned";
}

}  // namespace internal

}  // namespace roo_testing