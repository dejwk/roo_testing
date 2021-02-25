extern void setup();
extern void loop();

extern "C" int main() {
  setup();
  for (;;) {
    loop();
  }
}
