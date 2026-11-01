#include "Arduino.h"

// Verifies the stable Arduino library links String's clock/logging references
// for ordinary native main(), without a startup runner supplying dependencies.
int main() {
  String value(1.25, 2);
  value.replace("1", "2");
  return value == "2.25" ? 0 : 1;
}
