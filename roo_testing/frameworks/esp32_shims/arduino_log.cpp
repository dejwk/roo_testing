#include "esp32-hal-log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

extern "C" {

const char *pathToFileName(const char *path) {
  if (!path) return "";
  const char *slash = strrchr(path, '/');
  const char *backslash = strrchr(path, '\\');
  const char *separator = slash > backslash ? slash : backslash;
  return separator ? separator + 1 : path;
}

int log_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  const int result = vprintf(format, args);
  va_end(args);
  return result;
}

void log_print_buf(const uint8_t *buffer, size_t length) {
  if (!buffer) return;
  for (size_t i = 0; i < length; ++i) {
    printf("%02x%s", buffer[i], (i + 1) % 16 == 0 || i + 1 == length ? "\n" : " ");
  }
}

void __wrap_esp_log_writev(esp_log_level_t level, const char *tag,
                           const char *format, va_list args) {
  esp_log_writev(level, tag, format, args);
}

void __wrap_esp_log_write(esp_log_level_t level, const char *tag,
                          const char *format, ...) {
  va_list args;
  va_start(args, format);
  esp_log_writev(level, tag, format, args);
  va_end(args);
}

}  // extern "C"
