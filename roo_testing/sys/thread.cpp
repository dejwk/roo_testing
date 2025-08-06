#include "roo_testing/sys/thread.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "glog/logging.h"

namespace roo_testing {

namespace {

struct ThreadAttr {
  uint16_t stack_size;
  uint16_t priority;
  bool joinable;

  ThreadAttr()
      : stack_size(configMINIMAL_STACK_SIZE * sizeof(portSTACK_TYPE)),
        priority(1),
        joinable(true) {}
};

struct thread_state {
  ThreadAttr attr;

  std::unique_ptr<internal::VirtualCallable> start = nullptr;
  TaskHandle_t task;
  StaticSemaphore_t join_barrier;
  StaticSemaphore_t join_mutex;
  void* result;
};

}  // namespace

thread::~thread() { CHECK(!joinable()); }

thread& thread::operator=(thread&& other) noexcept {
  CHECK(!joinable());
  swap(other);
  return *this;
}

thread_state* thread_self() {
  return (thread_state*)xTaskGetApplicationTaskTag(nullptr);
}

static void run_thread(void* arg) {
  thread_state* p = (thread_state*)arg;
  p->start->call();
  vTaskSuspendAll();
  if (p->attr.joinable) {
    xSemaphoreGive((SemaphoreHandle_t)&p->join_barrier);
    xTaskResumeAll();
    vTaskSuspend(nullptr);
  } else {
    xTaskResumeAll();
    delete p;
    vTaskDelete(nullptr);
  }
}

void thread::start(std::unique_ptr<internal::VirtualCallable> start) {
  thread_state* state = new thread_state;
  CHECK_NOTNULL(state);
  state->attr = ThreadAttr();
  state->start = std::move(start);
  if (state->attr.joinable) {
    xSemaphoreCreateMutexStatic(&state->join_mutex);
    xSemaphoreCreateBinaryStatic(&state->join_barrier);
  }
  vTaskSuspendAll();
  if (xTaskCreate(run_thread, "roo_thread",
                  (uint16_t)(state->attr.stack_size / sizeof(portSTACK_TYPE)),
                  (void*)state, state->attr.priority, &state->task) != pdPASS) {
    delete state;
    LOG(FATAL) << "Failed to create a new thread";
  }
  // Hack: store the pointer to the thread object in the task tag, for
  // thread_self().
  vTaskSetApplicationTaskTag(state->task, (TaskHookFunction_t)state);
  id_.id_ = state;
  xTaskResumeAll();
}

void thread::join() {
  thread_state* state = (thread_state*)id_.id_;
  CHECK(state != nullptr) << "Attempting to join a null thread";
  CHECK(state->attr.joinable) << "Attempting to join a non-joinable thread";
  if (xSemaphoreTake((SemaphoreHandle_t)&state->join_mutex, 0) != pdPASS) {
    LOG(FATAL) << "Another thread has already joined the requested thread";
  }
  if (thread_self() == id_.id_) {
    LOG(FATAL) << "Thread attempting to join itself";
  }

  // Wait for the joined thread to finish.
  xSemaphoreTake((SemaphoreHandle_t)&state->join_barrier, portMAX_DELAY);

  vTaskSuspendAll();
  xSemaphoreGive((SemaphoreHandle_t)&state->join_barrier);
  vSemaphoreDelete((SemaphoreHandle_t)&state->join_barrier);

  xSemaphoreGive((SemaphoreHandle_t)&state->join_mutex);
  vSemaphoreDelete((SemaphoreHandle_t)&state->join_mutex);

  vTaskDelete(state->task);

  id_.id_ = 0;
  delete state;
  xTaskResumeAll();
}

namespace this_thread {

thread::id get_id() noexcept { return thread::id(thread_self()); }

void yield() noexcept { vPortYield(); }

namespace internal {
void sleep_ns(std::chrono::nanoseconds ns) {
  // uint64_t start = (uint64_t)esp_timer_get_time();

  // uint64_t us = (ns.count() + 999) / 1000;
  // const TickType_t delay = (us + 999999) * configTICK_RATE_HZ / 1000000;
  // if (delay > 0) {
  //   vTaskDelay(delay);
  // }
  // uint64_t now = esp_timer_get_time();
  // if (now - start < us) {
  //   delayMicroseconds(us - (now - start));
  // }
}
}  // namespace internal

}  // namespace this_thread

}  // namespace roo_testing