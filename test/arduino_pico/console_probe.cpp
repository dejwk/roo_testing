#include <cstring>

#include "Arduino.h"

// Writes through actual Print/Serial before waiting for later communication.
int main(int argc, char** argv) {
  if (argc != 2 || std::strcmp(argv[1], "flush") != 0) return 1;
  init();
  Serial.begin(115200);
  Serial.print("ready\n");
  Serial.setTimeout(10000);
  char byte;
  return Serial.readBytes(&byte, 1) == 1 && byte == '!' ? 0 : 2;
}
