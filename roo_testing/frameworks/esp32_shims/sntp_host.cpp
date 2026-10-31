#define ESP_LWIP_COMPONENT_BUILD 1
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#undef SNTP_OPMODE_POLL
#include <sys/time.h>

#include <array>
#include <string>

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "glog/logging.h"
#include "lwip/apps/sntp.h"
#include "roo_testing/frameworks/esp32_shims/sntp_host.h"
#include "roo_testing/frameworks/esp32_shims/sntp_netif.h"
#include "roo_testing/frameworks/posix_time/host_time.h"
#include "roo_testing/system/timer.h"
#include "roo_testing/system/wall_clock.h"

namespace {
using roo_testing::esp32::sntp::ServerBehavior;
using roo_testing::esp32::sntp::TimeSource;
constexpr uint32_t kDefaultIntervalMs = 3600000;

struct Server {
  std::string name;
  ip_addr_t address = {};
  uint8_t reachability = 0;
};

struct Client {
  StaticSemaphore_t mutex_storage;
  SemaphoreHandle_t mutex = nullptr;
  esp_timer_handle_t timer = nullptr;
  std::array<Server, CONFIG_LWIP_SNTP_MAX_SERVERS> servers;
  ServerBehavior behavior;
  int64_t reference_uptime = 0;
  int64_t deadline = 0;
  uint32_t interval_ms = kDefaultIntervalMs;
  sntp_sync_time_cb_t callback = nullptr;
  uint64_t registration = 0;
  sntp_sync_status_t status = SNTP_SYNC_STATUS_RESET;
  bool enabled = false;
  bool resetting = false;
  unsigned deliveries = 0;
};
Client client;
thread_local unsigned notification_depth = 0;
thread_local uint64_t notification_registration = 0;

/// Serializes short task-side state operations; never held across user code.
class ClientLock {
 public:
  ClientLock() {
    CHECK(client.mutex != nullptr) << "Initialize the host runner before SNTP";
    CHECK_EQ(xSemaphoreTake(client.mutex, portMAX_DELAY), pdTRUE);
  }
  ~ClientLock() { CHECK_EQ(xSemaphoreGive(client.mutex), pdTRUE); }
};

/// Retires a delivery already claimed while holding ClientLock.
class ClaimedDelivery {
 public:
  ClaimedDelivery() { ++notification_depth; }
  ~ClaimedDelivery() {
    --notification_depth;
    ClientLock lock;
    --client.deliveries;
  }
};

void TimerWake(void*);

/// Rearms the reusable timer at a checked future emulated deadline.
void ScheduleLocked(int64_t delay_us) {
  const int64_t now = system_time_get_micros();
  CHECK(!__builtin_add_overflow(now, delay_us, &client.deadline));
  if (client.timer == nullptr) {
    const esp_timer_create_args_t args = {.callback = TimerWake,
                                          .arg = nullptr,
                                          .dispatch_method = ESP_TIMER_TASK,
                                          .name = "sntp",
                                          .skip_unhandled_events = true};
    CHECK_EQ(ESP_OK, esp_timer_create(&args, &client.timer));
  }
  if (esp_timer_is_active(client.timer))
    CHECK_EQ(ESP_OK, esp_timer_stop(client.timer));
  CHECK_EQ(ESP_OK, esp_timer_start_once(client.timer, delay_us));
}

void StopLocked() {
  client.enabled = false;
  if (client.timer != nullptr && esp_timer_is_active(client.timer)) {
    CHECK_EQ(ESP_OK, esp_timer_stop(client.timer));
  }
}

/// Samples the selected server timeline without consulting device UTC.
bool ServerTimeLocked(int64_t& value) {
  if (client.behavior.time_source == TimeSource::kHost) {
    return roo_testing::system::GetHostUnixTimeMicros(value);
  }
  const int64_t elapsed = system_time_get_micros() - client.reference_uptime;
  return !__builtin_add_overflow(client.behavior.reference_unix_us, elapsed,
                                 &value);
}

/// Publishes time and completion before calling the current notification hook.
void PublishTime(struct timeval* tv) {
  CHECK(tv != nullptr);
  CHECK_GE(tv->tv_usec, 0);
  CHECK_LT(tv->tv_usec, 1000000);
  const __int128 micros =
      static_cast<__int128>(tv->tv_sec) * 1000000 + tv->tv_usec;
  CHECK(micros >= INT64_MIN && micros <= INT64_MAX);
  sntp_sync_time_cb_t callback;
  uint64_t registration;
  {
    ClientLock lock;
    CHECK(roo_testing::system::SetUnixTimeMicros(static_cast<int64_t>(micros)));
    client.status = SNTP_SYNC_STATUS_COMPLETED;
    callback = client.callback;
    registration = client.registration;
  }
  if (callback != nullptr) {
    const uint64_t previous = notification_registration;
    notification_registration = registration;
    callback(tv);
    notification_registration = previous;
  }
}

/// Treats each timer wake as a poll of authoritative current state, not old
/// data.
void TimerWake(void*) {
  int64_t value = 0;
  bool deliver = false;
  {
    ClientLock lock;
    if (!client.enabled || system_time_get_micros() < client.deadline) return;
    for (Server& server : client.servers) {
      if (server.name.empty() && ip_addr_isany(&server.address)) continue;
      deliver = client.behavior.responds && ServerTimeLocked(value);
      server.reachability =
          static_cast<uint8_t>((server.reachability << 1) | (deliver ? 1 : 0));
      break;
    }
    ScheduleLocked(static_cast<int64_t>(client.interval_ms) * 1000);
    if (deliver) ++client.deliveries;
  }
  if (!deliver) return;
  ClaimedDelivery delivery;
  struct timeval tv;
  tv.tv_sec = value / 1000000;
  tv.tv_usec = value % 1000000;
  if (tv.tv_usec < 0) {
    --tv.tv_sec;
    tv.tv_usec += 1000000;
  }
  // Respect an application's strong override; the engine never pre-publishes.
  sntp_sync_time(&tv);
}

Server& ServerAt(unsigned index) {
  CHECK_LT(index, client.servers.size());
  return client.servers[index];
}
}  // namespace

namespace roo_testing::esp32_shims {
uint64_t InstallSntpNotificationForHost(sntp_sync_time_cb_t callback) {
  ClientLock lock;
  CHECK_NE(client.registration, UINT64_MAX);
  client.callback = callback;
  return ++client.registration;
}

uint64_t CurrentSntpNotificationForHost() { return notification_registration; }

void InitializeSntpForHost() {
  if (client.mutex == nullptr) {
    client.mutex = xSemaphoreCreateMutexStatic(&client.mutex_storage);
    CHECK(client.mutex != nullptr);
  }
  InitializeNetifSntpForHost();
}

esp_err_t ShutdownSntpForHost() {
  if (client.mutex == nullptr) return ESP_OK;
  roo_testing::esp32::sntp::Reset();
  esp_timer_handle_t timer;
  {
    ClientLock lock;
    timer = client.timer;
    client.timer = nullptr;
  }
  if (timer == nullptr) return ESP_OK;
  const esp_err_t result = esp_timer_delete(timer);
  if (result != ESP_OK) return result;
  // Deletion is a due event in the ordered ESP timer list. In manual mode a
  // task delay alone does not dispatch its hardware compare. Pump due work,
  // then let the timer task consume it before the runner deinitializes timers.
  // Once the list has no due head, our already-due delete has been processed;
  // never inspect the retired handle itself.
  for (unsigned i = 0; i < 1000; ++i) {
    ProcessSystemTimeAlarms();
    if (esp_timer_get_next_alarm() > esp_timer_get_time()) return ESP_OK;
    vTaskDelay(1);
  }
  return ESP_ERR_TIMEOUT;
}
}  // namespace roo_testing::esp32_shims

namespace roo_testing::esp32::sntp {
esp_err_t ConfigureServer(const ServerBehavior& behavior) {
  int64_t first_response;
  if (behavior.response_delay_us <= 0 ||
      (behavior.time_source != TimeSource::kHost &&
       behavior.time_source != TimeSource::kReference) ||
      __builtin_add_overflow(behavior.reference_unix_us,
                             behavior.response_delay_us, &first_response)) {
    return ESP_ERR_INVALID_ARG;
  }
  ClientLock lock;
  if (client.enabled || client.resetting || client.deliveries != 0)
    return ESP_ERR_INVALID_STATE;
  client.behavior = behavior;
  client.reference_uptime = system_time_get_micros();
  return ESP_OK;
}

void Reset() {
  CHECK_EQ(notification_depth, 0u) << "Cannot reset inside SNTP delivery";
  esp_netif_sntp_deinit();
  {
    ClientLock lock;
    CHECK(!client.resetting);
    client.resetting = true;
    StopLocked();
  }
  for (;;) {
    {
      ClientLock lock;
      if (client.deliveries == 0) break;
    }
    vTaskDelay(1);
  }
  ClientLock lock;
  client.servers = {};
  client.behavior = {};
  client.reference_uptime = 0;
  client.interval_ms = kDefaultIntervalMs;
  client.callback = nullptr;
  client.status = SNTP_SYNC_STATUS_RESET;
  client.resetting = false;
}
}  // namespace roo_testing::esp32::sntp

extern "C" {
void __attribute__((weak)) sntp_sync_time(struct timeval* tv) {
  if (notification_depth != 0) {
    PublishTime(tv);
    return;
  }
  {
    ClientLock lock;
    CHECK(!client.resetting);
    ++client.deliveries;
  }
  ClaimedDelivery delivery;
  PublishTime(tv);
}

void sntp_set_time_sync_notification_cb(sntp_sync_time_cb_t callback) {
  roo_testing::esp32_shims::InstallSntpNotificationForHost(callback);
}

sntp_sync_status_t sntp_get_sync_status() {
  ClientLock lock;
  const sntp_sync_status_t status = client.status;
  if (status == SNTP_SYNC_STATUS_COMPLETED)
    client.status = SNTP_SYNC_STATUS_RESET;
  return status;
}

void sntp_set_sync_status(sntp_sync_status_t status) {
  CHECK(status == SNTP_SYNC_STATUS_RESET ||
        status == SNTP_SYNC_STATUS_COMPLETED);
  ClientLock lock;
  client.status = status;
}

void sntp_set_sync_mode(sntp_sync_mode_t mode) {
  CHECK_EQ(mode, SNTP_SYNC_MODE_IMMED)
      << "SNTP smooth synchronization is unsupported";
}
sntp_sync_mode_t sntp_get_sync_mode() { return SNTP_SYNC_MODE_IMMED; }

void sntp_set_sync_interval(uint32_t interval) {
  ClientLock lock;
  client.interval_ms = interval < 15000 ? 15000 : interval;
}
uint32_t sntp_get_sync_interval() {
  ClientLock lock;
  return client.interval_ms;
}

void esp_sntp_init() {
  ClientLock lock;
  CHECK(!client.resetting);
  if (client.enabled) return;
  client.enabled = true;
  ScheduleLocked(client.behavior.response_delay_us);
}
void esp_sntp_stop() {
  ClientLock lock;
  StopLocked();
}
bool esp_sntp_enabled() {
  ClientLock lock;
  return client.enabled;
}
bool sntp_restart() {
  ClientLock lock;
  CHECK(!client.resetting);
  if (!client.enabled) return false;
  ScheduleLocked(client.behavior.response_delay_us);
  return true;
}

void esp_sntp_setoperatingmode(esp_sntp_operatingmode_t mode) {
  CHECK_EQ(mode, ESP_SNTP_OPMODE_POLL)
      << "SNTP listen-only mode is unsupported";
}
esp_sntp_operatingmode_t esp_sntp_getoperatingmode() {
  return ESP_SNTP_OPMODE_POLL;
}

void esp_sntp_setservername(u8_t index, const char* name) {
  ClientLock lock;
  Server& server = ServerAt(index);
  server.name = name == nullptr ? "" : name;
  server.address = {};
  server.reachability = 0;
}
const char* esp_sntp_getservername(u8_t index) {
  ClientLock lock;
  Server& server = ServerAt(index);
  return server.name.empty() ? nullptr : server.name.c_str();
}
void esp_sntp_setserver(u8_t index, const ip_addr_t* address) {
  ClientLock lock;
  Server& server = ServerAt(index);
  server.name.clear();
  server.address = address == nullptr ? ip_addr_t{} : *address;
  server.reachability = 0;
}
const ip_addr_t* esp_sntp_getserver(u8_t index) {
  ClientLock lock;
  return &ServerAt(index).address;
}
uint8_t esp_sntp_getreachability(u8_t index) {
  ClientLock lock;
  return ServerAt(index).reachability;
}
#if LWIP_DHCP_GET_NTP_SRV
void esp_sntp_servermode_dhcp(bool enabled) {
  CHECK(!enabled) << "SNTP DHCP servers are unsupported";
}
#endif

// Raw lwIP entry points retain their C ABI in addition to the ESP wrappers.
void sntp_init() { esp_sntp_init(); }
void sntp_stop() { esp_sntp_stop(); }
u8_t sntp_enabled() { return esp_sntp_enabled() ? 1 : 0; }
void sntp_setoperatingmode(u8_t mode) {
  esp_sntp_setoperatingmode(static_cast<esp_sntp_operatingmode_t>(mode));
}
u8_t sntp_getoperatingmode() {
  return static_cast<u8_t>(esp_sntp_getoperatingmode());
}
void sntp_setservername(u8_t index, const char* name) {
  esp_sntp_setservername(index, name);
}
const char* sntp_getservername(u8_t index) {
  return esp_sntp_getservername(index);
}
void sntp_setserver(u8_t index, const ip_addr_t* addr) {
  esp_sntp_setserver(index, addr);
}
const ip_addr_t* sntp_getserver(u8_t index) {
  return esp_sntp_getserver(index);
}
#if SNTP_MONITOR_SERVER_REACHABILITY
u8_t sntp_getreachability(u8_t index) {
  return esp_sntp_getreachability(index);
}
#endif
u8_t sntp_getkodreceived(u8_t) { return 0; }
#if SNTP_GET_SERVERS_FROM_DHCP || SNTP_GET_SERVERS_FROM_DHCPV6
void sntp_servermode_dhcp(int enabled) {
  CHECK_EQ(enabled, 0) << "SNTP DHCP servers are unsupported";
}
#endif
void sntp_set_system_time(uint32_t sec, uint32_t us) {
  CHECK_LT(us, 1000000u);
  struct timeval tv = {static_cast<time_t>(sec), static_cast<suseconds_t>(us)};
  sntp_sync_time(&tv);
}
void sntp_get_system_time(uint32_t* sec, uint32_t* us) {
  CHECK(sec != nullptr);
  CHECK(us != nullptr);
  int64_t value;
  CHECK(roo_testing::system::GetUnixTimeMicros(value));
  int64_t seconds = value / 1000000;
  int64_t fraction = value % 1000000;
  if (fraction < 0) {
    --seconds;
    fraction += 1000000;
  }
  *sec = static_cast<uint32_t>(seconds);
  *us = static_cast<uint32_t>(fraction);
  sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
}
}  // extern "C"
