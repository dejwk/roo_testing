// Include Pico first to exercise its macros against subsequent host headers.
// clang-format off
#include "pico.h"
#include <pthread.h>
// clang-format on

// Verifies host pthread declarations and static initializers remain usable
// after Pico defines its compiler attributes.
int main(void) {
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  if (pthread_mutex_lock(&mutex) != 0) return 1;
  if (pthread_mutex_unlock(&mutex) != 0) return 2;
  return pthread_mutex_destroy(&mutex) == 0 ? 0 : 3;
}
