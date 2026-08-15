#include "roo_testing/host_event/host_event_endpoint.h"

#include <atomic>
#include <cstddef>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

namespace roo_testing {
namespace {

constexpr size_t kEndpointCapacity = 32;
constexpr uint32_t kGatewayStackBytes = 4096;
constexpr uint32_t kGatewayStackDepth =
    kGatewayStackBytes / sizeof(StackType_t);

static_assert(std::atomic<bool>::is_always_lock_free,
              "HostEventEndpoint requires a lock-free atomic<bool>");
static_assert(kGatewayStackBytes % sizeof(StackType_t) == 0,
              "gateway stack must be expressed in StackType_t words");

HostEventEndpoint *endpoints[kEndpointCapacity] = {};
StaticSemaphore_t delivery_mutex_storage;
SemaphoreHandle_t delivery_mutex = nullptr;
StaticTask_t gateway_task_storage;
StackType_t gateway_stack[kGatewayStackDepth];
TaskHandle_t gateway_task = nullptr;

bool IsFreeRtosTaskContext() {
  return xTaskGetSchedulerState() == taskSCHEDULER_RUNNING &&
         xPortIsFreeRtosTask() == pdTRUE;
}

} // namespace

namespace internal {

class HostEventGateway {
public:
  static void deliver(void *) {
    for (;;) {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      xSemaphoreTake(delivery_mutex, portMAX_DELAY);
      for (HostEventEndpoint *endpoint : endpoints) {
        if (endpoint == nullptr)
          continue;
        if (endpoint->pending_.exchange(false, std::memory_order_acquire)) {
          endpoint->handler_(endpoint->context_);
        }
      }
      xSemaphoreGive(delivery_mutex);
    }
  }

  static bool startIfNeeded() {
    if (gateway_task != nullptr)
      return true;

    delivery_mutex = xSemaphoreCreateMutexStatic(&delivery_mutex_storage);
    if (delivery_mutex == nullptr)
      return false;
    gateway_task = xTaskCreateStatic(deliver, "host_events", kGatewayStackDepth,
                                     nullptr, tskIDLE_PRIORITY + 2,
                                     gateway_stack, &gateway_task_storage);
    return gateway_task != nullptr;
  }

  static void onTick() noexcept {
    if (gateway_task == nullptr)
      return;
    for (HostEventEndpoint *endpoint : endpoints) {
      if (endpoint == nullptr ||
          !endpoint->pending_.load(std::memory_order_relaxed)) {
        continue;
      }
      BaseType_t higher_priority_task_woken = pdFALSE;
      xTaskNotifyFromISR(gateway_task, 0, eIncrement,
                         &higher_priority_task_woken);
      return;
    }
  }
};

void hostEventTickHook() noexcept { HostEventGateway::onTick(); }

} // namespace internal

HostEventConnectResult HostEventEndpoint::connect(Handler handler,
                                                  void *context) {
  if (!IsFreeRtosTaskContext() || handler == nullptr) {
    return HostEventConnectResult::kWrongContext;
  }
  configASSERT(xTaskGetCurrentTaskHandle() != gateway_task);

  portENTER_CRITICAL(nullptr);
  if (slot_ >= 0) {
    portEXIT_CRITICAL(nullptr);
    return HostEventConnectResult::kAlreadyConnected;
  }
  if (!internal::HostEventGateway::startIfNeeded()) {
    portEXIT_CRITICAL(nullptr);
    return HostEventConnectResult::kNoCapacity;
  }
  for (size_t i = 0; i < kEndpointCapacity; ++i) {
    if (endpoints[i] != nullptr)
      continue;
    pending_.store(false, std::memory_order_relaxed);
    handler_ = handler;
    context_ = context;
    slot_ = static_cast<int8_t>(i);
    endpoints[i] = this;
    portEXIT_CRITICAL(nullptr);
    return HostEventConnectResult::kConnected;
  }
  portEXIT_CRITICAL(nullptr);
  return HostEventConnectResult::kNoCapacity;
}

void HostEventEndpoint::disconnect() {
  if (!IsFreeRtosTaskContext() || slot_ < 0)
    return;
  configASSERT(xTaskGetCurrentTaskHandle() != gateway_task);

  xSemaphoreTake(delivery_mutex, portMAX_DELAY);
  portENTER_CRITICAL(nullptr);
  if (slot_ >= 0) {
    endpoints[slot_] = nullptr;
    slot_ = -1;
    handler_ = nullptr;
    context_ = nullptr;
    pending_.store(false, std::memory_order_relaxed);
  }
  portEXIT_CRITICAL(nullptr);
  xSemaphoreGive(delivery_mutex);
}

bool HostEventEndpoint::isConnected() const {
  return IsFreeRtosTaskContext() && slot_ >= 0;
}

void HostEventEndpoint::notifyFromHost() noexcept {
  pending_.store(true, std::memory_order_release);
}

} // namespace roo_testing
