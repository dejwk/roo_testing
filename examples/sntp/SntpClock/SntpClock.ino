// Observe an unsynchronized device clock, then synchronize using Arduino APIs.
//
// The emulator starts at Unix epoch plus elapsed emulated time. configTime()
// starts a virtual SNTP request; the callback arrives later on a framework
// task. The virtual server uses host UTC by default, without DNS or NTP
// packets. No Wi-Fi fixture is needed for this clock-only emulator
// demonstration. On a board, connect to a network before using these same
// synchronization APIs. Serial output appears in the host terminal (115200 baud
// on a board).
#include <Arduino.h>
#include <esp_sntp.h>
#include <time.h>

#include <atomic>

// SNTP calls back from another task, so the small readiness flag is atomic.
std::atomic<bool> time_ready{false};
void OnTimeSync(struct timeval*) { time_ready = true; }

// Format the clock in UTC; local timezone configuration is a separate choice.
void PrintUtc() {
  const time_t now = time(nullptr);
  struct tm calendar;
  gmtime_r(&now, &calendar);
  char text[32];
  if (strftime(text, sizeof(text), "%Y-%m-%dT%H:%M:%SZ", &calendar) != 0) {
    Serial.printf("Synchronized UTC: %s\n", text);
  }
}

void setup() {
  Serial.begin(115200);
  time_ready = false;
  Serial.printf("Before sync (seconds since epoch): %lld\n",
                static_cast<long long>(time(nullptr)));

  // Register first so a quick response cannot be missed. Starting SNTP alone
  // does not set the clock: wait for its successful response notification.
  esp_sntp_set_time_sync_notification_cb(OnTimeSync);
  configTime(0, 0, "pool.ntp.org");
  const unsigned long started = millis();
  while (!time_ready && millis() - started < 2000) delay(10);
  if (time_ready)
    PrintUtc();
  else
    Serial.println("Synchronization timed out.");
}

void loop() {
  // Continue reading the corrected clock; no new network request is necessary
  // for each read. Its progression follows the emulator's uptime policy.
  if (time_ready) PrintUtc();
  delay(1000);
}
