/*
  LCD Display Demo with an ESP8266 NodeMCU and a LCD-Display with an I2C Backpack (only 4 wires required)
  
  Copied from https://lastminuteengineers.com/esp8266-i2c-lcd-tutorial/ (2-line display), modified for a 4 line display
  Text ouput taken from mctms\eval_projects\lcd_display_16x4

*/

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>       // make sure to include the LCD I2C library from Frank de Brabander (others may not work)

LiquidCrystal_I2C lcd(0x27, 16, 4);  // set the LCD address to 0x27 for the 16 chars and 4 line display

void setup() {
  lcd.init();
  lcd.clear();
  lcd.backlight();  // Make sure backlight is on

  lcd.print(" Hello world!"); // Print a message to the LCD
  lcd.setCursor(0, 1); // Set cursor to column 0, row 1
  lcd.print(" LCD 20x4 Demo");
  lcd.setCursor(0, 2); // Set cursor to column 0, row 2
  lcd.print(" Powered by ESP8266");
}

void loop() {
  lcd.setCursor(0, 3); // Move cursor to the second row
  lcd.print(" Uptime: ");
  lcd.print(millis() / 1000); // Print elapsed seconds since reset
  lcd.print("s");
}
