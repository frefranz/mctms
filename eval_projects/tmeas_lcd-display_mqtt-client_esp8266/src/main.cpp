/*
  Temperature measurement prototype based on ESP8266 and DS18B20 sensor featuring
  - temperature monitoring using an 20x4 LCD-display
  - MQTT data transmission protocol via WiFi connection to a local MQTT broker

  Hardware:
  - ESP3266 microcontroller board (NodeMCU)
  - 20x4 LCD-Matrix display with I2C interface
  - DS18B20, one wire temperature sensor

  Firmware:
  - coded in C/C++ using the Arduino/Platformio framework and the following libraries:
    - LiquidCrystal_I2C by Frank de Brabander (for LCD display)
    - OneWire by Paul Stoffregen (for one-wire communication)
    - DallasTemperature by Miles Burton (for DS18B20 sensor control)
    - ESP8266WiFi and PubSubClient (for WiFi and MQTT communication)

  - Firmware flow:
    - initialize firmware
    - based on the state of an identification-mode jumper, enter
      - identification mode: runs until reset, allows to identify the ROM codes of connected sensors and copy them into the knownSensors[] array for later use in normal operation mode
        or
      - normal operation mode: in an endless loop do the following:
        - start a temperature measurement
        - display the result on the LCD-Matrix display
*/

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>       // make sure to include the LCD I2C library from Frank de Brabander (others may not work)
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Credentials and sensitive data handling:
//   copy secrets_template.h to secrets.h and fill in your WiFi and MQTT credentials  -or-
//   outcomment include.h, remove comment at secrets_template.h to be able to compile on the spot
//   Note: secrets.h not seen publicly as it contains sensitive data (protected by .gitignore)
//
//#include "../include/secrets_template.h"
#include "../include/secrets.h"

// Define I2C pins for ESP8266  (Default: I2C_SDA_PIN = GPIO4, I2C_SCL_PIN = GPIO5)
#define I2C_SDA_PIN 12 // GPIO 12, Pin D6
#define I2C_SCL_PIN 14 // GPIO 14, Pin D5

// Identification jumper pin: pull to GND (LOW) to enable ROM-identification mode.
// Use an INPUT_PULLUP so the normal state is HIGH when jumper is open.
#define ID_PIN D2 // GPIO4 - safe to use, non-boot pin

typedef float TemparatureValue[8];
TemparatureValue TempValue;

LiquidCrystal_I2C lcd(0x27, 16, 4);  // set the LCD address to 0x27 for the 16 chars and 4 line display

// oneWire instance pin (not limited to Maxim/Dallas temperature ICs)
OneWire oneWire(D1);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
IPAddress mqtt_ip;

// ------------------------------------------------------------------
// Configure known/expected sensors by their 8-byte ROM codes (one-wire ID)
// Replace the 0x00 entries with the actual ROM bytes shown by identificationMode.
// Example format: {0x28, 0xFF, 0x4C, 0x3C, 0x92, 0x16, 0x03, 0x4F}
// Fill the corresponding name in `knownNames` so a slot can get consistently referred to by index.
DeviceAddress knownSensors[] = {
  {0x28,0xD0,0x08,0x9F,0x00,0x00,0x00,0x9F}, // slot 0 - Indoor Sensor 0  (Sensor directly connected)
  {0x28,0xEC,0x67,0x9F,0x00,0x00,0x00,0x71}, // slot 1 - Indoor Sensor 1  (Sensor on pin header)
  {0x28,0x2C,0x44,0x6E,0x00,0x00,0x00,0xA6}, // slot 2 - Outdoor Sensor 0 (Sensor with cable)
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // slot 3 - replace with ROM for "sensor 3"
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // slot 4 - replace with ROM for "sensor 4"
  {0x28,0xD0,0x08,0x9F,0x00,0x00,0x00,0x9F}, // slot 5 - replace with ROM for "sensor 5"
  {0x28,0xEC,0x67,0x9F,0x00,0x00,0x00,0x71}, // slot 6 - replace with ROM for "sensor 6"
  {0x28,0x2C,0x44,0x6E,0x00,0x00,0x00,0xA6}  // slot 7 - replace with ROM for "sensor 7"
};

// Friendly names for the sensors, limited to 8 characters for MQTT protocol efficiency.
// Note: For the LCD display sensor names get truncated in the display section to 7 characters to fit the display
//
// Use fixed-size char arrays so any initializer longer than NAME_MAX triggers a compile-time error.
constexpr size_t NAME_MAX = 8;
// Each entry holds up to NAME_MAX characters plus terminating '\0'.
const char knownNames[][NAME_MAX + 1] = {
  "Indoor0",        // friendly name for slot 0
  "Indoor1",        // friendly name for slot 1
  "Outdoor",        // friendly name for slot 2    
  "",               // friendly name for slot 3
  "",               // friendly name for slot 4
  "Indr 0",         // friendly name for slot 5
  "Indr 1",         // friendly name for slot 6
  "Outdr0"          // friendly name for slot 7
};
static_assert(sizeof(knownNames) / sizeof(knownNames[0]) == 8, "knownNames must contain exactly 8 entries");
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

// Display the ROM address on the two lower LCD lines as 4 bytes per line
void displayAddressLines(const DeviceAddress addr) {
  char line[21];
  snprintf(line, sizeof(line), "0x%02X,0x%02X,0x%02X,0x%02X", addr[0], addr[1], addr[2], addr[3]); // MSB..mid
  lcd.setCursor(0, 2); lcd.print(line);
  snprintf(line, sizeof(line), "0x%02X,0x%02X,0x%02X,0x%02X", addr[4], addr[5], addr[6], addr[7]); // mid..LSB
  lcd.setCursor(0, 3); lcd.print(line);
}

void setup_wifi() {
  // Connect to a WiFi network
  delay(10);
  lcd.clear();
  Serial.println();
  lcd.print("Connecting to WiFi");
  lcd.setCursor(0, 1);
  Serial.print("Connecting to ");
  lcd.print(ssid);
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  lcd.setCursor(0, 2);
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    lcd.print(".");
    Serial.print(".");
    attempts++;
  }
  delay(2000);    // wait to allow reading before switching display

  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.print("Connected to WiFi");
    lcd.setCursor(0, 1);
    lcd.print(ssid);
    lcd.setCursor(0, 2);
    lcd.print("IP address: ");
    lcd.setCursor(0, 3);
    lcd.print(WiFi.localIP());
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {

    lcd.setCursor(0, 2);
    lcd.print("Failed to connect to");
    lcd.print("WiFi ");
    lcd.print(ssid);
    Serial.println("Failed to connect to WiFi");
  }
  delay(3000);    // wait to allow reading before switching display

  // Activate the following code if you want to check the MQTT broker connectivity at this point
  // Serial.print("Pinging MQTT broker at ");
  // Serial.println(mqtt_server);
  // if (WiFi.hostByName(mqtt_server, mqtt_ip)) {
  //   Serial.print("Broker found at IP: ");
  //   Serial.println(mqtt_ip);
  // } else {
  //   Serial.println("Cannot resolve broker address");
  // }
}

void reconnect() {
  // Loop until we're connected or reconnected
  while (!client.connected()) {
    lcd.clear();
    lcd.print("Calling MQTT Broker:");
    Serial.print("Attempting MQTT connection...");
    // Attempt to (re)connect
    if (client.connect("temperature_measurement_client")) {
      lcd.setCursor(0, 1);
      lcd.print("Broker connected.");
      Serial.println("connected.");
      delay(3000);    // wait to allow reading before switching display
    } 
    else {
      lcd.setCursor(0, 1);
      lcd.print("Connection failed");
      lcd.setCursor(0, 2);
      int rc = client.state();
      lcd.print("rc=");
      lcd.print(rc);
      Serial.print("failed, rc=");
      Serial.print(rc);
      lcd.setCursor(0, 3);
      lcd.print("Retrying in 5 sec.");
      Serial.println(" trying again in 5 seconds");
      delay(5000);      // Wait before retrying
    }
  }
}

// Identification mode: once entered, runs until reset.
void identificationMode() {
  Serial.println("Entering Sensor ID Mode until powerdown/reset");
  Serial.println("Copy the ROM codes for each sensor into knownSensors[] and re-flash");
  while (true) {
    sensors.begin();
    delay(300);
    uint8_t devCount = sensors.getDeviceCount();
    if (devCount == 0) {
      // No sensor
      lcd.setCursor(0, 0); lcd.print("-- Sensor ID Mode --");
      lcd.setCursor(0, 1); lcd.print("Error:              ");
      lcd.setCursor(0, 2); lcd.print("No Sensor connected ");
      lcd.setCursor(0, 3); lcd.print("                    ");
      Serial.println("Error: No Sensor connected");
    }
    else if (devCount > 1) {
      // Too many sensors
      lcd.setCursor(0, 0); lcd.print("-- Sensor ID Mode --");
      lcd.setCursor(0, 1); lcd.print("Error:              ");
      lcd.setCursor(0, 2); lcd.print("More than one       ");
      lcd.setCursor(0, 3); lcd.print("sensor connected    ");
      Serial.println("Error: More than one sensor connected");
    }
    else {
      // Exactly one sensor connected
      DeviceAddress addr;
      if (sensors.getAddress(addr, 0)) {
        lcd.setCursor(0, 0); lcd.print("-- Sensor ID Mode --");
        lcd.setCursor(0, 1); lcd.print("ROM addr MSB to LSB:");
        displayAddressLines(addr);

        Serial.println("Sensor connected, ROM addr MSB to LSB:");
        char line[64];
        snprintf(line, sizeof(line), "0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X", \
          addr[7], addr[6], addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]); Serial.println(line);
      } else {
        lcd.setCursor(0, 0); lcd.print("-- Sensor ID Mode --");
        lcd.setCursor(0, 1); lcd.print("Error:              ");
        lcd.setCursor(0, 1); lcd.print("Addr. reading failed");
        lcd.setCursor(0, 3); lcd.print("                    ");
        Serial.println("Error: Address reading failed");
      }
    }

    delay(5000); // delay to allow sensor hot-plugging, allow some time to read the display/serial output.
  }
}

void setup()
{
  // Start serial (for discovery and debugging)
  Serial.begin(115200);
  delay(50);

  // Configure identification-mode jumper pin (INPUT_PULLUP). Pull to GND to enable identification mode
  pinMode(ID_PIN, INPUT_PULLUP);
  
  // Start the LCD
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  lcd.init();
  lcd.clear();
  lcd.backlight();                    // Make sure backlight is on
  lcd.begin(20, 4);                   // Init LCD (20 col. by 4 rows), cursor is at top-left
  
  // Start up the sensor library
  sensors.begin();
  delay(50);

  // Enter identification mode if identification-mode jumper is pulled to ground
  if (digitalRead(ID_PIN) == LOW) {
    identificationMode();
    // never reached: identificationMode loops forever until reset
  }
  lcd.print("MQTT MC-TempM Client");  // Line 0: print a message to the LCD
  lcd.print("Vers. 2026-02-16    ");  // no cursor repositioning as previous line is fully used
  lcd.print("--------------------");
  delay(500);
  lcd.print("Setting up client...");  
  delay(3000);                        // wait to allow reading before switching display

  setup_wifi(); 
  client.setServer(mqtt_server, 1883);
}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();    // maintain the MQTT connection and process incoming messages

  sensors.requestTemperatures();
  lcd.clear();
  uint8_t rowcnt = 0;           // reset row count for a new page

  // Loop through each configured (known) sensor slot
  for (size_t i = 0; i < KNOWN_SENSORS; i++) {
    TempValue[i] = DEVICE_DISCONNECTED_C;
    bool configured = !isAddressZero(knownSensors[i]);

    // If configured and physically present, read by ROM address
    if (configured) {
      if (sensors.isConnected(knownSensors[i])) {
        TempValue[i] = sensors.getTempC(knownSensors[i]);
      }
    }

    // Update LCD with new temperature values:
    // - only sensors configured are shown, 4 sensors on one LCD page, ordered by slot index.
    // - if more than 4 sensores are configured, a second page is shown after some delay
    // Note: all 8 slots are getting logged to the Serial Monitor
    //
    // Example of oene LCD page with 4 configured sensors (slots 0,1,2,7) and 4 unconfigured slots (3,4,5,6):
    // |12345678901234567890|
    // +--------------------+
    // !S0: Sensor_1 23.45°C!
    // !S1: Sensor_2 23.45°C!
    // !S4: Sensor_5 23.45°C!
    // !S7: Sensor_6 23.45°C!
    // +--------------------+
    // 
    // Note: Sensor names are truncated to 7 Characters to fit the display,
    //       if a sensor is configured but not connected, the display shows "--.--"

    if (configured) {
      if (rowcnt == 4) {          // 2nd page for more than 4 sensors: show after a delay to allow reading the first page
        delay(4000);              // wait n seconds before switching to the second page
        lcd.clear();
        rowcnt = 0;               // reset row count for the new page
      }
      lcd.setCursor(0, rowcnt);   // set cursor to current row (rowcnt is reset to 0 on a new page)


      lcd.print("S"); lcd.print(i); lcd.print(": ");

      // Make sure that a sensor name is always 7 characters long to ensure a consistent display format.
      char eq_length_str[9] = "        "; // 8 chars + null terminator as buffer for the formatted name
      strncpy(eq_length_str, knownNames[i], strlen(knownNames[i]));
      eq_length_str[7] = '\0';
      lcd.print(eq_length_str);
      lcd.print(" ");

      if (TempValue[i] != DEVICE_DISCONNECTED_C) {
        // Sensor configured and connected, show sensor name and temperature value
        // Make sure the temp. value is always 5 characters long to ensure a consistent display format.
        dtostrf(TempValue[i], 5, 2, eq_length_str);
        lcd.print(eq_length_str);
      } else {
        // Sensor configured but not connected: show sensor name use "--.--" in place for the temperature value
        lcd.print("--.--");
      }
      lcd.print(" \xDF" "C"); // print degree symbol and C
      rowcnt++;
    }
    
    // Simple logging to the Serial Monitor for all slots
    Serial.print("Slot "); Serial.print(i); Serial.print(" ");
    if (knownNames[i] && knownNames[i][0]) { Serial.print(knownNames[i]); Serial.print(" "); }
    if (!configured) {
      Serial.println("- not configured");
    } else {
      Serial.print("-> "); printAddress(knownSensors[i]); Serial.print(" : ");
      if (TempValue[i] != DEVICE_DISCONNECTED_C) Serial.println(TempValue[i]);
      else Serial.println("disconnected");
    }
  }

// Loop through all temperature valuesand assemble a payload string for MQTT transmission
  String payload = "";
  for (size_t i = 0; i < KNOWN_SENSORS; i++) {
    payload = payload + String(TempValue[i]);             // simple CSV format oredered by slot number
    if (i < KNOWN_SENSORS - 1) payload = payload + ", ";  // comma separated values but not after the last value
  }

  // publish via MQTT to topic "sbc01/sb01" (tmc = temperatur measurement client, sb = sensor bank)
  // retain flag set to false:
  //   - last value is not retained by the broker
  //   - all subscribers get the last value immediately upon subscription

  client.publish("tmc01/sb01", payload.c_str(), false);

  // Wait n seconds before starting next measurement cycle
  delay(4000);
}
