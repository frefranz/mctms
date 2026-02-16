/*
  LCD Display Demo
  
  Copied from https://docs.cirkitdesigner.com (2-line display), modified for a 4 line display

  The LiquidCrystal library works with all LCD displays that are compatible with the
  Hitachi HD44780 driver.

  Circuit Configuration:
  - LCD RS pin to Arduino digital pin 12
  - LCD Enable pin to Arduino digital pin 11
  - LCD D4 pin to Arduino digital pin 5
  - LCD D5 pin to Arduino digital pin 4
  - LCD D6 pin to Arduino digital pin 3
  - LCD D7 pin to Arduino digital pin 2
  - LCD R/W pin to ground (write mode)
  - LCD VSS pin to ground
  - LCD VDD pin to 5V
  - LCD VO pin (contrast control) connected to the wiper of a 10K potentiometer
    (other ends of the potentiometer connected to +5V and ground)
  - LCD A (anode) pin to 5V through a 220 Ohm resistor for backlight
  - LCD K (cathode) pin to ground
*/

#include <Arduino.h>
#include <LiquidCrystal.h>

// Pin assignments for the LCD interface
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  lcd.begin(20, 4); // Initialize LCD (20 columns by 4 rows)
  lcd.print(" Hello world!"); // Print a message to the LCD
  lcd.setCursor(0, 1); // Set cursor to column 0, row 1
  lcd.print(" LCD 20x4 Demo");
  lcd.setCursor(0, 2); // Set cursor to column 0, row 2
  lcd.print(" Powered by Arduino");
}

void loop() {
  lcd.setCursor(0, 3); // Move cursor to the second row
  lcd.print(" Uptime: ");
  lcd.print(millis() / 1000); // Print elapsed seconds since reset
  lcd.print("s");
}