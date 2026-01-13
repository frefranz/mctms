/*
  Temperature measurement prototype based on ESP3266 and DS18B20 sensor with LCD-Matrix display

  Hardware:
  - ESP3266 microcontroller board (NodeMCU)
  - 20x4 LCD-Matrix display with I2C interface (2nd PCB located under the display)
  - DS18B20, one wire temperature sensor

  Firmware:
  - initialize firmware
  - in an endless loop do the following:
      - start a temperature measurement
      - display the result on the LCD-Matrix display

  See the folder "eval_projects" for details on the respctive building block

*/

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>       // make sure to include the LCD I2C library from Frank de Brabander (others may not work)
#include <OneWire.h>
#include <DallasTemperature.h>

LiquidCrystal_I2C lcd(0x27, 16, 4);  // set the LCD address to 0x27 for the 16 chars and 4 line display


// oneWire instance pin (not limited to Maxim/Dallas temperature ICs
OneWire oneWire(D0);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

void setup()
{
  // Start LCD
  lcd.init();
  lcd.clear();
  lcd.backlight();                  // Make sure backlight is on
  lcd.begin(20, 4);                 // Initialize LCD (20 columns by 4 rows)
  lcd.print("Temperature Readout"); // Print a message to the LCD, row 0
  lcd.setCursor(0, 1);              // Set cursor to column 0, row 1
  lcd.print("Demovers. 2026-01-12");
  lcd.setCursor(0, 2);              // Set cursor to column 0, row 2
  lcd.print("--------------------");

  // Start up the sensor library
  sensors.begin();
}

void loop()
{
  // Request temperature reading from all sensors
  sensors.requestTemperatures();

  // Print sensor number, read sensor, check data and print temperature value (or error message)
  lcd.setCursor(0, 3); // start at column 0, row 3
  lcd.print("Sensor 00: ");
  float tempC = sensors.getTempCByIndex(0);
  if (tempC != DEVICE_DISCONNECTED_C) {
    lcd.print(tempC);
    lcd.print(" \xDF" "C");
  }
  else {
    lcd.print("Read-Err");
  }
}