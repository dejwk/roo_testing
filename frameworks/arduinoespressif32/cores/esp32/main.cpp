#include "fake_esp32.h"

extern void setup();
extern void loop();

extern "C" int main() {
  setup();
  for (;;) {
    loop();
    FakeEsp32().time().sync();
  }
}
