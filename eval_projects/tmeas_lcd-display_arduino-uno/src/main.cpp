/*
  Temperature measurement prototype based on Arduino UNO

  Hardware:
  - Arduino UNO
  - 20x8 LCD-Matrix display
  - DS18B20, one wire temperature sensor

  Firmware:
  - initialize firmware
  - in an endless loop do the following:
      - start a temperature measurement
      - display the result on the LCD-Matrix display

  See the folder "eval_projects" for details on the respctive building block

*/

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pin assignments for the peripherals:

// List of all pins assigned
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2, d14 = 14;

// LCD pins
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// oneWire instance pin (not limited to Maxim/Dallas temperature ICs
OneWire oneWire(d14);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

void setup()
{
  // Start LCD
  lcd.begin(20, 4);                 // Initialize LCD (20 columns by 4 rows)
  lcd.print("Temperature Readout"); // Print a message to the LCD, row 0
  lcd.setCursor(0, 1);              // Set cursor to column 0, row 1
  lcd.print("Demovers. 2025-12-18");
  lcd.setCursor(0, 2); // Set cursor to column 0, row 2
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