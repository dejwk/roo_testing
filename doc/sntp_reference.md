# SNTP emulation reference

Status: Implemented for Arduino-ESP32 and ESP-IDF on ESP32, ESP32-S3, and ESP32-C3.

## Start with the normal firmware APIs

Arduino `configTime()`/`configTzTime()`, immediate-mode `esp_sntp_*` and raw lwIP
APIs, and esp-netif's default initialization/wait/event path share one client.
Try the commented [Arduino and IDF examples](../examples/sntp/README.md).

A fresh process starts with Unix epoch plus elapsed emulated uptime. Reading
`time()` or `gettimeofday()` succeeds before synchronization; it does not imply
a correct date or fire a callback. Starting SNTP schedules a simulated response.
After the default 100 ms response delay, host UTC is published to the device clock
before its notification. No DNS or UDP exchange takes place.

The UTC clock then advances with emulated uptime, in either manual or auto mode.
Stopping, restarting, or deinitializing SNTP does not reset it. Explicit clock
sets affect only the emulator process. Monotonic native scheduler waits and the
machine's own clock remain unchanged. Timezone configuration still uses `TZ`.

## Control the virtual server in tests

Include `roo_testing/frameworks/esp32_shims/sntp_host.h` from an emulator fixture:

```cpp
namespace sntp = roo_testing::esp32::sntp;
sntp::ServerBehavior server;
server.time_source = sntp::TimeSource::kReference;
server.reference_unix_us = 1700000000000000LL;
server.response_delay_us = 250000;
ESP_ERROR_CHECK(sntp::ConfigureServer(server));
```

Configure a stopped, quiescent client before normal firmware starts it. The
reference timestamp is anchored at configuration and advances with uptime;
changing it never implicitly initializes device UTC. Set `server.responds=false`
to model a silent server. At least one server name/address must be configured.
Availability is explicit and does not infer Wi-Fi, routing, or DNS state.

Link manual tests with `//roo_testing/system:manual_time_mode`. Advance time,
then wait for task publication using a semaphore or bounded task handshake.
Passive reads do not run callbacks. Lag/pump jumps coalesce overdue polls;
dispatching delays can visit intervening deadlines normally.

`Reset()` drains delivery and restores client defaults but retains device UTC.
Call it with exclusive test ownership, from a task outside a callback. The
wall-clock controls in `roo_testing/system/wall_clock.h` independently set UTC
or restore epoch-plus-current-uptime with `ResetToPowerOnMapping()`.

## Compatibility boundaries

- Callbacks run on the ESP timer task, never an ISR or the native alarm worker.
  Keep them brief and nonblocking. Stop/restart and esp-netif deinit are safe
  from a callback. Low-level stop permits already-claimed delivery to finish.
- Sync status COMPLETED is consumed by a status read. The esp-netif waiter has
  separate completion state, and deinit wakes waiters with INVALID_STATE.
- Update intervals default to one hour and clamp to at least 15 seconds.
  Polling models one virtual server, not lwIP retry backoff or server failover.
- Smooth synchronization, listen-only mode, DHCP/automatic IP server renewal,
  RTC reset/sleep retention, packet exchange, and Pico networking are unsupported.
  Unsupported void settings fail explicitly; esp-netif flags return NOT_SUPPORTED.
- Application `gettimeofday`, `settimeofday`, `time`, and realtime
  `clock_gettime`/`clock_settime`/`clock_getres` use the process clock through
  GNU linker wrapping. Realtime resolution is one microsecond. Non-realtime
  writes and `adjtime` fail with ENOTSUP. Internal C clock calls in prebuilt
  shared libraries, including some `std::chrono::system_clock` implementations,
  are outside that link-wrapper guarantee.
- Choose one configuration/notification owner: direct SNTP or esp-netif. They
  do not create independent clients. Server-name getters borrow storage until
  its next mutation; synchronize concurrent access before retaining a pointer.

The [design and implementation plan](sntp_emulation.md) describes lifecycle,
clock isolation, unsupported behavior, and the validation stages.
