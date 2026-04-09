// ************************************************************
// *                                                          *
// * Laesst auf einem ESP32 die an GPIO 2 angeschlossene      *
// * blaue on-board LED blinken                    2026-04-10 *
// *                                                          *
// ************************************************************

// Note: The ESP32 boards on Windows require drivers for the
//    CP210x or CH340 serial converters, see
//    https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers
//

#include <Arduino.h>

#define LED_BUILTIN 2

void ledON() {
  digitalWrite(LED_BUILTIN, LOW);
}

void ledOFF() {
  digitalWrite(LED_BUILTIN, HIGH);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  ledON();
  delay(750);

  ledOFF();
  delay(750);
}
