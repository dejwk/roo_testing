#include "roo_testing/frameworks/esp32_shims/sntp_netif.h"

#include <memory>

#include "esp_event.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "freertos/semphr.h"
#include "glog/logging.h"
#include "roo_testing/frameworks/esp32_shims/sntp_host.h"

ESP_EVENT_DEFINE_BASE(NETIF_SNTP_EVENT);

namespace {
/// Keeps a wait semaphore alive until deinitialized callers and callbacks
/// leave.
struct Session {
  explicit Session(const esp_sntp_config_t& config) : callback(config.sync_cb) {
    if (config.wait_for_sync) {
      semaphore = xSemaphoreCreateBinaryStatic(&storage);
      CHECK(semaphore != nullptr);
    }
  }
  ~Session() {
    if (semaphore != nullptr) vSemaphoreDelete(semaphore);
  }

  StaticSemaphore_t storage;
  SemaphoreHandle_t semaphore = nullptr;
  esp_sntp_time_cb_t callback = nullptr;
  uint64_t registration = 0;
  unsigned waiters = 0;
  bool active = true;
};
StaticSemaphore_t mutex_storage;
SemaphoreHandle_t mutex = nullptr;
std::shared_ptr<Session> current;

/// Serializes adapter state; user callbacks and waits happen after unlocking.
class NetifLock {
 public:
  NetifLock() {
    CHECK(mutex != nullptr)
        << "Initialize the host runner before esp-netif SNTP";
    CHECK_EQ(pdTRUE, xSemaphoreTake(mutex, portMAX_DELAY));
  }
  ~NetifLock() { CHECK_EQ(pdTRUE, xSemaphoreGive(mutex)); }
};

/// Delivers only the registration associated with this adapter session.
void Notify(struct timeval* tv) {
  std::shared_ptr<Session> session;
  {
    NetifLock lock;
    session = current;
    if (session == nullptr || !session->active ||
        session->registration !=
            roo_testing::esp32_shims::CurrentSntpNotificationForHost())
      return;
    if (session->semaphore != nullptr) xSemaphoreGive(session->semaphore);
  }
  if (session->callback != nullptr) session->callback(tv);
  const esp_netif_sntp_time_sync_t event = {*tv};
  const esp_err_t result = esp_event_post(
      NETIF_SNTP_EVENT, NETIF_SNTP_TIME_SYNC, &event, sizeof(event), 0);
  if (result != ESP_OK)
    LOG(WARNING) << "SNTP sync event was not queued: " << result;
}

esp_err_t Validate(const esp_sntp_config_t* config) {
  if (config == nullptr || config->num_of_servers == 0 ||
      config->num_of_servers > CONFIG_LWIP_SNTP_MAX_SERVERS)
    return ESP_ERR_INVALID_ARG;
  if (config->smooth_sync || config->server_from_dhcp ||
      config->renew_servers_after_new_IP) {
    return ESP_ERR_NOT_SUPPORTED;
  }
  for (size_t i = 0; i < config->num_of_servers; ++i) {
    if (config->servers[i] == nullptr || config->servers[i][0] == '\0')
      return ESP_ERR_INVALID_ARG;
  }
  return ESP_OK;
}
}  // namespace

namespace roo_testing::esp32_shims {
void InitializeNetifSntpForHost() {
  if (mutex == nullptr) {
    mutex = xSemaphoreCreateMutexStatic(&mutex_storage);
    CHECK(mutex != nullptr);
  }
}
}  // namespace roo_testing::esp32_shims

extern "C" {
esp_err_t esp_netif_sntp_init(const esp_sntp_config_t* config) {
  const esp_err_t result = Validate(config);
  if (result != ESP_OK) return result;
  const std::shared_ptr<Session> candidate = std::make_shared<Session>(*config);
  NetifLock lock;
  if (current != nullptr || esp_sntp_enabled()) return ESP_ERR_INVALID_STATE;
  for (unsigned i = 0; i < CONFIG_LWIP_SNTP_MAX_SERVERS; ++i) {
    esp_sntp_setservername(
        i, i < config->num_of_servers ? config->servers[i] : nullptr);
  }
  esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
  esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
  candidate->registration =
      roo_testing::esp32_shims::InstallSntpNotificationForHost(Notify);
  current = candidate;
  if (config->start) esp_sntp_init();
  return ESP_OK;
}

esp_err_t esp_netif_sntp_start() {
  NetifLock lock;
  if (current == nullptr) return ESP_ERR_INVALID_STATE;
  esp_sntp_stop();
  esp_sntp_init();
  return ESP_OK;
}

void esp_netif_sntp_deinit() {
  std::shared_ptr<Session> retired;
  {
    NetifLock lock;
    if (current == nullptr) return;
    retired = current;
    retired->active = false;
    esp_sntp_stop();
    esp_sntp_set_time_sync_notification_cb(nullptr);
    current.reset();
    // Wake one waiter; each retiring waiter passes the wake to the next.
    if (retired->semaphore != nullptr) xSemaphoreGive(retired->semaphore);
  }
  // Session storage is retained by callbacks/waiters without waiting on
  // oneself.
}

esp_err_t esp_netif_sntp_sync_wait(TickType_t timeout) {
  std::shared_ptr<Session> session;
  {
    NetifLock lock;
    session = current;
    if (session == nullptr || session->semaphore == nullptr)
      return ESP_ERR_INVALID_STATE;
    ++session->waiters;
  }
  const BaseType_t taken = xSemaphoreTake(session->semaphore, timeout);
  NetifLock lock;
  --session->waiters;
  if (!session->active) {
    if (session->waiters != 0) xSemaphoreGive(session->semaphore);
    return ESP_ERR_INVALID_STATE;
  }
  return taken == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t esp_netif_sntp_reachability(unsigned index, unsigned* reachability) {
  if (index >= CONFIG_LWIP_SNTP_MAX_SERVERS || reachability == nullptr)
    return ESP_ERR_INVALID_ARG;
  NetifLock lock;
  if (current == nullptr || !esp_sntp_enabled()) return ESP_ERR_INVALID_STATE;
  *reachability = esp_sntp_getreachability(index);
  return ESP_OK;
}
}  // extern "C"
