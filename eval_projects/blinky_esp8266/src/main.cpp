// ************************************************************
// *                                                          *
// * Laesst auf einem ESP8266 die an Pin 10 angeschlossene    *
// * blaue on-board LED blinken                    2026-01-10 *
// *                                                          *
// ************************************************************

/* 
  I am having a NodeMCU Lua Lolin V3 Module ESP8266 ESP-12F board, most
  web-sites are about the ESP-12E, so what is the difference?
  -> https://www.utmel.com/components/esp12e-vs-esp12f-which-one-is-better?id=1441
  Select the board on platformio:
  -> https://loginov-rocks.medium.com/quick-start-with-nodemcu-v3-esp8266-arduino-ecosystem-and-platformio-ide-b8415bf9a038
  HW description from board vendor;
  -> https://www.az-delivery.de/en/products/nodemcu-lua-lolin-v3-modul-mit-esp8266-12e-unverlotet
*/

#include <Arduino.h>

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
  delay(1000);

  ledOFF();
  delay(1000);
}
