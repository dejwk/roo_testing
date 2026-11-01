#include "pico/multicore.h"
int main() {
  multicore_launch_core1([] {});
}
