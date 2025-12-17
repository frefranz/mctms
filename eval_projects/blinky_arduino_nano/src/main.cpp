
// ************************************************************
// *                                                          *
// * Laesst auf einem Arduino Nano eine an Pin 13 und GND an- *
// * geschlossene LED blinken                      2025-11-29 *
// *                                                          *
// ************************************************************


#include <Arduino.h>

int ledPin1 = 13; // on-board red LED, angeschlossen an Pin 13

void setup()
{
  pinMode(ledPin1, OUTPUT); // setze digitalen Pin als Output
}

void loop()
{
  digitalWrite(ledPin1, HIGH);
  delay(700);
  digitalWrite(ledPin1, LOW);
  delay(700);
}
