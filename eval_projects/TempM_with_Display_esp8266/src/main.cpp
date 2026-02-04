
/*
Fixed mapping of a sensor-index to a physical sensor

Finding:
The current code reads temperatures by index and can't guarantee a fixed mapping to a physical sensor.
I'll update main.cpp to discover ROM addresses at startup and add a fixed mapping (by ROM) with human-readable names,
plus serial discovery output and display logic.

What I changed (quick summary):
Updated main.cpp to discover DS18B20 ROM IDs at startup and map physical sensors to named slots.
Added a knownSensors[] array (fill with ROM bytes) and knownNames[] so a slot always refers to the same physical sensor.

The sketch prints detected ROM addresses to Serial (115200) so you can copy them into the knownSensors[] entries.

How it works (short):
The Dallas/OneWire sensors each have a unique 8‑byte ROM code.
Instead of relying on unstable indices, the code reads by ROM:
Use sensors.getTempC( DeviceAddress ) to get temperature for a specific ROM.
On first run, the sketch prints ROMs to Serial (look for lines like Index 0 -> 28FF...).
Copy each ROM into knownSensors[] and give it a friendly name in knownNames[].
The display shows the first two configured slots; full info is logged to Serial.

How to set up your sensor mapping:
- Flash the updated main.cpp.
- Open Serial Monitor at 115200. You’ll see:
  "Detected devices: N"
- For each: Index X -> <ROM bytes>
  Replace the placeholder {0x00,...} in knownSensors[] with the ROM bytes
  printed (example in code: {0x28,0xFF,0x4C,0x3C,0x92,0x16,0x03,0x4F}).
- Put a human-readable label in the corresponding knownNames[] slot (e.g. "Bowl 5").
- Recompile/flash. From now on, reading slot 0 uses the exact physical sensor whose ROM you put into knownSensors[0].
*/


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

// Define I2C pins for ESP8266  (Default: I2C_SDA_PIN = GPIO4, I2C_SCL_PIN = GPIO5)
#define I2C_SDA_PIN 12 // GPIO 12, Pin D6
#define I2C_SCL_PIN 14 // GPIO 14, Pin D5

LiquidCrystal_I2C lcd(0x27, 16, 4);  // set the LCD address to 0x27 for the 16 chars and 4 line display

// oneWire instance pin (not limited to Maxim/Dallas temperature ICs)
OneWire oneWire(D1);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// ------------------------------------------------------------------
// Configure known/expected sensors by their 8-byte ROM codes (one-wire ID)
// Replace the 0x00 entries with the actual ROM bytes shown by the serial
// discovery below. Example format:
// {0x28, 0xFF, 0x4C, 0x3C, 0x92, 0x16, 0x03, 0x4F}
// Fill the corresponding name in `knownNames` so you can refer to slots
// consistently by index.
DeviceAddress knownSensors[] = {
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // slot 0 - replace with ROM for "sensor 0"
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // slot 1 - replace with ROM for "sensor 1"
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // slot 2 - replace with ROM for "sensor 2"
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // slot 3 - replace with ROM for "sensor 3"
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // slot 4 - replace with ROM for "sensor 4"
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // slot 5 - replace with ROM for "sensor 5"
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // slot 6 - replace with ROM for "sensor 6"
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}  // slot 7 - replace with ROM for "sensor 7"
};

const char* knownNames[] = {
  "Indoor",     // friendly name for slot 0
  "Outdoor",    // friendly name for slot 1
  "",           // friendly name for slot 2    
  "",           // friendly name for slot 3
  "",           // friendly name for slot 4
  "",           // friendly name for slot 5
  "",           // friendly name for slot 6
  ""            // friendly name for slot 7
};

const size_t KNOWN_SENSORS = sizeof(knownSensors) / sizeof(knownSensors[0]);

// Helpers
bool isAddressZero(const DeviceAddress addr) {
  for (uint8_t i = 0; i < 8; i++) if (addr[i] != 0) return false;
  return true;
}

void printAddress(const DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print('0');
    Serial.print(deviceAddress[i], HEX);
  }
}

void setup()
{
  // Start serial (for discovery and debugging)
  Serial.begin(115200);
  delay(50);

  // Start LCD
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  lcd.init();
  lcd.clear();
  lcd.backlight();                    // Make sure backlight is on
  lcd.begin(20, 4);                   // Initialize LCD (20 columns by 4 rows)
  lcd.print("ReadTemp. 2026-01-27");  // Print a message to the LCD, row 0
  lcd.setCursor(0, 1);                // Set cursor to column 0, row 1
  lcd.print("--------------------");

  // Start up the sensor library
  sensors.begin();

  // Discover connected sensors and print their ROMs so you can copy
  // them into the `knownSensors[]` array above to lock sensors to slots.
  Serial.println();
  Serial.println("--- DS18B20 sensor discovery ---");
  uint8_t devCount = sensors.getDeviceCount();
  Serial.print("Detected devices: "); Serial.println(devCount);
  for (uint8_t i = 0; i < devCount; i++) {
    DeviceAddress addr;
    if (sensors.getAddress(addr, i)) {
      Serial.print("Index "); Serial.print(i);
      Serial.print(" -> ");
      printAddress(addr);
      Serial.println();
    }
  }
  Serial.println("Copy the ROM codes above into knownSensors[] and re-flash to lock names to physical sensors.");
}

void loop()
{
  // Continuous measurement loop
  while (true) {
    // Request temperature reading from all sensors
    sensors.requestTemperatures();

    // Loop through each configured (known) sensor slot
    for (size_t i = 0; i < KNOWN_SENSORS; i++) {
      float tempC = DEVICE_DISCONNECTED_C;
      bool configured = !isAddressZero(knownSensors[i]);

      // If configured and physically present, read by ROM address
      if (configured) {
        if (sensors.isConnected(knownSensors[i])) {
          tempC = sensors.getTempC(knownSensors[i]);
        } else {
          tempC = DEVICE_DISCONNECTED_C;
        }
      }

      // Update LCD: only show first two configured slots on the 2 display lines
      if (i < 2) {
        lcd.setCursor(0, 2 + i);
        lcd.print("                    "); // clear 20-char line
        lcd.setCursor(0, 2 + i);
        lcd.print("S"); lcd.print(i); lcd.print(": ");
        if (knownNames[i] && knownNames[i][0]) {
          lcd.print(knownNames[i]); lcd.print(" ");
        } else if (configured) {
          lcd.print("Addr"); lcd.print(i); lcd.print(" ");
        } else {
          lcd.print("not cfg");
        }

        if (!configured) {
          // nothing to show
        } else if (tempC != DEVICE_DISCONNECTED_C) {
          lcd.setCursor(14, 2 + i);
          lcd.print(tempC);
          lcd.print(" \xDF" "C");
        } else {
          lcd.setCursor(14, 2 + i);
          lcd.print("---- \xDF" "C");
        }
      }

      // Serial log for all slots
      Serial.print("Slot "); Serial.print(i); Serial.print(" ");
      if (knownNames[i] && knownNames[i][0]) { Serial.print(knownNames[i]); Serial.print(" "); }
      if (!configured) {
        Serial.println("- not configured");
      } else {
        Serial.print("-> "); printAddress(knownSensors[i]); Serial.print(" : ");
        if (tempC != DEVICE_DISCONNECTED_C) Serial.println(tempC);
        else Serial.println("disconnected");
      }
    }

    // Wait 2 seconds before next measurement
    delay(2000);
  }
}