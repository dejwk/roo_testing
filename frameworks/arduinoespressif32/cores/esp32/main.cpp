#include "fake_esp32.h"

#include "glog/logging.h"

extern void setup();
extern void loop();

extern "C" int main() {
  FLAGS_stderrthreshold = google::INFO;
  setup();
  for (;;) {
    loop();
    FakeEsp32().time().sync();
  }
}
