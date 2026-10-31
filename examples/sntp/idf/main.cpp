// Demonstrate ESP-IDF's SNTP initialization and bounded synchronization wait.
//
// The device clock begins near the Unix epoch. The emulator's virtual server
// supplies host UTC after a simulated response delay; it needs no Internet.
// This clock-only example omits network setup. Physical firmware must establish
// connectivity before starting SNTP. Output is sent to the host terminal.
#include <stdio.h>
#include <time.h>

#include "esp_event.h"
#include "esp_netif_sntp.h"

extern "C" void app_main() {
  printf("Before sync (seconds since epoch): %lld\n",
         static_cast<long long>(time(nullptr)));

  // The default event loop receives the usual synchronization event. The
  // default SNTP config also enables the completion semaphore used below.
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  ESP_ERROR_CHECK(esp_netif_sntp_init(&config));

  // Waiting uses RTOS ticks, independently of the UTC correction. A successful
  // return means a response was delivered, not merely that time() was readable.
  if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(2000)) == ESP_OK) {
    printf("Synchronized UTC epoch seconds: %lld\n",
           static_cast<long long>(time(nullptr)));
  } else {
    printf("Synchronization timed out.\n");
  }

  // Releasing the client stops future polls but preserves the device's time.
  esp_netif_sntp_deinit();
  ESP_ERROR_CHECK(esp_event_loop_delete_default());

  // The host runner keeps idling after app_main returns; flush piped output.
  fflush(stdout);
}
