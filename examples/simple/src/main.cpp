#include <Arduino.h>

void setup() {
  Serial.begin(9600);
}

void loop() {
  Serial.print("Welcome to the ESP32 emulator! ");
  Serial.printf("The uptime is now %ld ms.\n", millis());
  delay(500);
}
