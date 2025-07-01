#include "roo_testing/sys/condition_variable.h"
#include "glog/logging.h"

namespace roo_testing {

condition_variable::condition_variable() noexcept {
  for (int i = 0; i < kMaxWaitingThreads; ++i) {
    tasks_waiting_[i] = nullptr;
  }
}

void condition_variable::notify_one() noexcept {
  TaskHandle_t* task_to_notify = nullptr;
  taskENTER_CRITICAL();
  for (int i = 0; i < kMaxWaitingThreads; ++i) {
    if (tasks_waiting_[i] != nullptr) {
      if (task_to_notify == nullptr ||
          uxTaskPriorityGet(*task_to_notify) <
              uxTaskPriorityGet(tasks_waiting_[i])) {
        task_to_notify = &tasks_waiting_[i];
      }
    }
  }
  if (task_to_notify != nullptr) {
    xTaskNotify(*task_to_notify, 0, eNoAction);
    *task_to_notify = nullptr;
  }
  taskEXIT_CRITICAL();
}

void condition_variable::notify_all() noexcept {
  taskENTER_CRITICAL();
  for (int i = 0; i < kMaxWaitingThreads; ++i) {
    if (tasks_waiting_[i] != nullptr) {
      xTaskNotify(tasks_waiting_[i], 0, eNoAction);
      tasks_waiting_[i] = nullptr;
    }
  }
  taskEXIT_CRITICAL();
}

void condition_variable::wait(unique_lock<mutex>& lock) noexcept {
  TaskHandle_t me = xTaskGetCurrentTaskHandle();
  bool queued = false;
  taskENTER_CRITICAL();
  for (int i = 0; i < kMaxWaitingThreads; ++i) {
    if (tasks_waiting_[i] == nullptr) {
      tasks_waiting_[i] = me;
      queued = true;
      break;
    }
  }
  lock.unlock();
  taskEXIT_CRITICAL();
  CHECK(queued) << "Maximum number of queued threads reached";

  // Wait on the condition variable.
  bool signaled = xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY) == pdPASS;
  lock.lock();
  taskENTER_CRITICAL();
  for (int i = 0; i < kMaxWaitingThreads; ++i) {
    if (tasks_waiting_[i] == me) {
      tasks_waiting_[i] = nullptr;
      break;
    }
  }
  taskEXIT_CRITICAL();
}

void condition_variable::timed_wait(unique_lock<mutex>& lock, uint64_t ns) noexcept {
  TaskHandle_t me = xTaskGetCurrentTaskHandle();
  uint64_t us = (ns + 999) / 1000;
  TickType_t delay = ((us * configTICK_RATE_HZ) + 999999) / 1000000;
  bool queued = false;
  taskENTER_CRITICAL();
  for (int i = 0; i < kMaxWaitingThreads; ++i) {
    if (tasks_waiting_[i] == nullptr) {
      tasks_waiting_[i] = me;
      queued = true;
      break;
    }
  }
  lock.unlock();
  taskEXIT_CRITICAL();
  CHECK(queued) << "Maximum number of queued threads reached";

  // Wait on the condition variable.
  bool signaled = xTaskNotifyWait(0, 0, nullptr, delay) == pdPASS;
  lock.lock();
  taskENTER_CRITICAL();
  for (int i = 0; i < kMaxWaitingThreads; ++i) {
    if (tasks_waiting_[i] == me) {
      tasks_waiting_[i] = nullptr;
      break;
    }
  }
  taskEXIT_CRITICAL();
}

}  // namespace roo_testing
