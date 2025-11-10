
// ***********************************************************
// *                                                         *
// * Laesst auf einem Arduino Uno eine an Pin 13 und GND an- *
// * geschlossene LED blinken                     2025-11-06 *
// *                                                         *
// ***************************************************+*******

// My Arduino Uno Board: eine gelbe LED auf dem Board blinkt mit,
// offensichtlich ebenfalls an Pin 13 angeschlossen

#include <Arduino.h>

int ledPin1 = 13; // LED an digitalen Pin 13 angeschlossen

void setup()
{
  pinMode(ledPin1, OUTPUT); // setze digitalen Pin als Output
}

void loop()
{
  digitalWrite(ledPin1, HIGH);
  delay(500);
  digitalWrite(ledPin1, LOW);
  delay(500);
}
