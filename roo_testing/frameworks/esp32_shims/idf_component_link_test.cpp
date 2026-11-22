#include "esp_event.h"
#include "freertos/ringbuf.h"

// Verifies that a dynamically linked ESP-IDF consumer can reference the event
// loop and ring buffer APIs.
int main() {
  auto* volatile create_event_loop = &esp_event_loop_create_default;
  auto* volatile create_ringbuffer = &xRingbufferCreate;
  return create_event_loop != nullptr && create_ringbuffer != nullptr ? 0 : 1;
}
