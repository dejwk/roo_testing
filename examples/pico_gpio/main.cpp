#include <cstdio>

#include "examples/pico_gpio/blink.h"

int main() {
  RunPicoBlink();
  std::puts("Pico blink completed: 3 cycles");
  return 0;
}
