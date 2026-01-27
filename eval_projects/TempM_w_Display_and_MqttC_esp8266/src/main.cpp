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
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Credentials and sensitive data handling:
//   copy secrets_template.h to secrets.h and fill in your WiFi and MQTT credentials  -or-
//   outcomment include.h, remove comment at secrets_template.h to be able to compile on the spot
//   Note: secrets.h not seen publicly as it contains sensitive data (protected by .gitignore)
//#include "../include/secrets_template.h"
#include "../include/secrets.h"

// Set LCD I2C address and number of display columns and rows
LiquidCrystal_I2C lcd(0x27, 16, 4);  // set the LCD address to 0x27 for the 16 chars and 4 line display

// oneWire instance pin (not limited to Maxim/Dallas temperature ICs)
OneWire oneWire(D1);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
float temp = 0;
int inPin = 5;
IPAddress mqtt_ip;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // UTC offset 0, update every 60s

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect to WiFi");
  }

  Serial.print("Pinging MQTT broker at ");
  // Serial.println(mqtt_server);
  // if (WiFi.hostByName(mqtt_server, mqtt_ip)) {
  //   Serial.print("Broker found at IP: ");
  //   Serial.println(mqtt_ip);
  // } else {
  //   Serial.println("Cannot resolve broker address");
  // }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient_temperature_sensor")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  // Start Serial for terminal communication
  Serial.begin(9600);
  setup_wifi(); 
  client.setServer(mqtt_server, 1883);
  timeClient.begin();  // Initialize NTP
  timeClient.update(); // Get initial time

  // // Start LCD
  // lcd.init();
  // lcd.clear();
  // lcd.backlight();                  // Make sure backlight is on
  // lcd.begin(20, 4);                 // Initialize LCD (20 columns by 4 rows)
  // lcd.print("Temperature Readout"); // Print a message to the LCD, row 0
  // lcd.setCursor(0, 1);              // Set cursor to column 0, row 1
  // lcd.print("Demovers. 2026-01-12");
  // lcd.setCursor(0, 2);              // Set cursor to column 0, row 2
  // lcd.print("--------------------");

  // Start up the sensor library
  sensors.begin();
}


// void loop()
// {
//   // Request temperature reading from all sensors
//   sensors.requestTemperatures();

//   // Print sensor number, read sensor, check data and print temperature value (or error message)
//   lcd.setCursor(0, 3); // start at column 0, row 3
//   lcd.print("Sensor 00: ");
//   float tempC = sensors.getTempCByIndex(0);
//   if (tempC != DEVICE_DISCONNECTED_C) {
//     lcd.print(tempC);
//     lcd.print(" \xDF" "C");
//   }
//   else {
//     lcd.print("Read-Err");
//   }
// }

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();    // maintain the MQTT connection and process incoming messages

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    // sensors.setResolution(12);
    sensors.requestTemperatures(); // Send the command to get temperatures
    temp = sensors.getTempCByIndex(0);
    // temp = 33;
    Serial.println(temp);
    if((temp > -20) && (temp <60))
      {
      timeClient.update(); // Ensure time is current
      String timestamp = timeClient.getFormattedTime(); // HH:MM:SS format
      String payload = timestamp + " " + String(temp);
      client.publish("ha/_temperature1", payload.c_str(), true);
      }
  }
}