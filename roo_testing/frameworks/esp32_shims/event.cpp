#include "esp_event.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

namespace {

struct Registration {
  esp_event_base_t base;
  int32_t id;
  esp_event_handler_t handler;
  void* arg;
};

struct HostEventLoop {
  std::mutex mutex;
  std::vector<std::shared_ptr<Registration>> registrations;
};

HostEventLoop*& DefaultLoop() {
  static HostEventLoop* loop = nullptr;
  return loop;
}

HostEventLoop* AsLoop(esp_event_loop_handle_t handle) {
  return reinterpret_cast<HostEventLoop*>(handle);
}

bool Matches(const Registration& registration, esp_event_base_t base,
             int32_t id) {
  const bool base_matches = registration.base == ESP_EVENT_ANY_BASE ||
                            registration.base == base;
  const bool id_matches = registration.id == ESP_EVENT_ANY_ID ||
                          registration.id == id;
  return base_matches && id_matches;
}

esp_err_t Register(HostEventLoop* loop, esp_event_base_t base, int32_t id,
                   esp_event_handler_t handler, void* arg,
                   esp_event_handler_instance_t* instance, bool unique) {
  if (loop == nullptr || handler == nullptr ||
      (base == ESP_EVENT_ANY_BASE && id != ESP_EVENT_ANY_ID)) {
    return ESP_ERR_INVALID_ARG;
  }
  std::lock_guard<std::mutex> lock(loop->mutex);
  if (unique) {
    for (auto& current : loop->registrations) {
      if (current->base == base && current->id == id &&
          current->handler == handler) {
        current->arg = arg;
        if (instance != nullptr) {
          *instance = reinterpret_cast<esp_event_handler_instance_t>(
              current.get());
        }
        return ESP_OK;
      }
    }
  }
  auto registration =
      std::make_shared<Registration>(Registration{base, id, handler, arg});
  if (instance != nullptr) {
    *instance = reinterpret_cast<esp_event_handler_instance_t>(
        registration.get());
  }
  loop->registrations.push_back(std::move(registration));
  return ESP_OK;
}

esp_err_t UnregisterHandler(HostEventLoop* loop, esp_event_base_t base,
                            int32_t id, esp_event_handler_t handler) {
  if (loop == nullptr || handler == nullptr) return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(loop->mutex);
  loop->registrations.erase(
      std::remove_if(loop->registrations.begin(), loop->registrations.end(),
                     [&](const auto& registration) {
                       return registration->base == base &&
                              registration->id == id &&
                              registration->handler == handler;
                     }),
      loop->registrations.end());
  return ESP_OK;
}

esp_err_t UnregisterInstance(HostEventLoop* loop, esp_event_base_t base,
                             int32_t id,
                             esp_event_handler_instance_t instance) {
  if (loop == nullptr || instance == nullptr) return ESP_ERR_INVALID_ARG;
  const auto* expected = reinterpret_cast<const Registration*>(instance);
  std::lock_guard<std::mutex> lock(loop->mutex);
  loop->registrations.erase(
      std::remove_if(loop->registrations.begin(), loop->registrations.end(),
                     [&](const auto& registration) {
                       return registration.get() == expected &&
                              registration->base == base &&
                              registration->id == id;
                     }),
      loop->registrations.end());
  return ESP_OK;
}

esp_err_t Post(HostEventLoop* loop, esp_event_base_t base, int32_t id,
               const void* data) {
  if (loop == nullptr || base == ESP_EVENT_ANY_BASE || id == ESP_EVENT_ANY_ID) {
    return ESP_ERR_INVALID_ARG;
  }
  std::vector<std::shared_ptr<Registration>> callbacks;
  {
    std::lock_guard<std::mutex> lock(loop->mutex);
    for (const auto& registration : loop->registrations) {
      if (Matches(*registration, base, id)) callbacks.push_back(registration);
    }
  }
  for (const auto& callback : callbacks) {
    callback->handler(callback->arg, base, id, const_cast<void*>(data));
  }
  return ESP_OK;
}

}  // namespace

extern "C" {

esp_err_t esp_event_loop_create(const esp_event_loop_args_t* args,
                                esp_event_loop_handle_t* event_loop) {
  if (args == nullptr || event_loop == nullptr) return ESP_ERR_INVALID_ARG;
  auto loop = std::make_unique<HostEventLoop>();
  *event_loop = reinterpret_cast<esp_event_loop_handle_t>(loop.release());
  return ESP_OK;
}

esp_err_t esp_event_loop_delete(esp_event_loop_handle_t event_loop) {
  if (event_loop == nullptr) return ESP_ERR_INVALID_ARG;
  delete AsLoop(event_loop);
  return ESP_OK;
}

esp_err_t esp_event_loop_create_default(void) {
  if (DefaultLoop() != nullptr) return ESP_ERR_INVALID_STATE;
  DefaultLoop() = new HostEventLoop();
  return ESP_OK;
}

esp_err_t esp_event_loop_delete_default(void) {
  delete DefaultLoop();
  DefaultLoop() = nullptr;
  return ESP_OK;
}

esp_err_t esp_event_loop_run(esp_event_loop_handle_t event_loop, TickType_t) {
  return event_loop == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

esp_err_t esp_event_handler_register_with(esp_event_loop_handle_t event_loop,
                                          esp_event_base_t base, int32_t id,
                                          esp_event_handler_t handler,
                                          void* arg) {
  return Register(AsLoop(event_loop), base, id, handler, arg, nullptr, true);
}

esp_err_t esp_event_handler_register(esp_event_base_t base, int32_t id,
                                     esp_event_handler_t handler, void* arg) {
  if (DefaultLoop() == nullptr) return ESP_ERR_INVALID_STATE;
  return Register(DefaultLoop(), base, id, handler, arg, nullptr, true);
}

esp_err_t esp_event_handler_instance_register_with(
    esp_event_loop_handle_t event_loop, esp_event_base_t base, int32_t id,
    esp_event_handler_t handler, void* arg,
    esp_event_handler_instance_t* instance) {
  return Register(AsLoop(event_loop), base, id, handler, arg, instance, false);
}

esp_err_t esp_event_handler_instance_register(
    esp_event_base_t base, int32_t id, esp_event_handler_t handler, void* arg,
    esp_event_handler_instance_t* instance) {
  if (DefaultLoop() == nullptr) return ESP_ERR_INVALID_STATE;
  return Register(DefaultLoop(), base, id, handler, arg, instance, false);
}

esp_err_t esp_event_handler_unregister_with(esp_event_loop_handle_t event_loop,
                                            esp_event_base_t base, int32_t id,
                                            esp_event_handler_t handler) {
  return UnregisterHandler(AsLoop(event_loop), base, id, handler);
}

esp_err_t esp_event_handler_unregister(esp_event_base_t base, int32_t id,
                                       esp_event_handler_t handler) {
  return UnregisterHandler(DefaultLoop(), base, id, handler);
}

esp_err_t esp_event_handler_instance_unregister_with(
    esp_event_loop_handle_t event_loop, esp_event_base_t base, int32_t id,
    esp_event_handler_instance_t instance) {
  return UnregisterInstance(AsLoop(event_loop), base, id, instance);
}

esp_err_t esp_event_handler_instance_unregister(
    esp_event_base_t base, int32_t id, esp_event_handler_instance_t instance) {
  return UnregisterInstance(DefaultLoop(), base, id, instance);
}

esp_err_t esp_event_post_to(esp_event_loop_handle_t event_loop,
                            esp_event_base_t base, int32_t id,
                            const void* data, size_t, TickType_t) {
  return Post(AsLoop(event_loop), base, id, data);
}

esp_err_t esp_event_post(esp_event_base_t base, int32_t id, const void* data,
                         size_t size, TickType_t ticks) {
  if (DefaultLoop() == nullptr) return ESP_ERR_INVALID_STATE;
  return esp_event_post_to(reinterpret_cast<esp_event_loop_handle_t>(DefaultLoop()),
                           base, id, data, size, ticks);
}

esp_err_t esp_event_isr_post_to(esp_event_loop_handle_t event_loop,
                                esp_event_base_t base, int32_t id,
                                const void* data, size_t size,
                                BaseType_t* task_unblocked) {
  if (task_unblocked != nullptr) *task_unblocked = pdFALSE;
  return esp_event_post_to(event_loop, base, id, data, size, 0);
}

esp_err_t esp_event_isr_post(esp_event_base_t base, int32_t id,
                             const void* data, size_t size,
                             BaseType_t* task_unblocked) {
  if (task_unblocked != nullptr) *task_unblocked = pdFALSE;
  return esp_event_post(base, id, data, size, 0);
}

}  // extern "C"
