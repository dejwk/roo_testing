// The embedded libc supplies these functions on hardware; glibc does not.
#include <charconv>
#include <system_error>

#include "Arduino.h"

extern "C" char* utoa(unsigned value, char* result, int base) {
  if (base < 2 || base > 36) {
    *result = 0;
    return result;
  }
  const std::to_chars_result converted =
      std::to_chars(result, result + 32, value, base);
  *converted.ptr = 0;
  return result;
}

extern "C" char* itoa(int value, char* result, int base) {
  if (value < 0 && base == 10) {
    *result = '-';
    utoa(0u - static_cast<unsigned>(value), result + 1, base);
    return result;
  }
  return utoa(static_cast<unsigned>(value), result, base);
}
