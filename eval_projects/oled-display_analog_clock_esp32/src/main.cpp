#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <U8g2lib.h>
#include <time.h>

const char* ssid = "-----------";
const char* password = "----------";
const char* ntpServer = "pool.ntp.org";
const int NTP_PACKET_SIZE = 48;
const unsigned long SYNC_INTERVAL_MS = 10UL * 60UL * 1000UL;
const long CET_OFFSET_SEC = 3600UL;

WiFiUDP udp;
byte ntpPacketBuffer[NTP_PACKET_SIZE];
unsigned long currentTimeSeconds = 0;
unsigned long lastMillis = 0;
unsigned long lastSyncMillis = 0;
bool timeSynced = false;

// I2C OLED display for SSD1309 / SSD1306 128x64 modules
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

static const int DISPLAY_WIDTH = 128;
static const int DISPLAY_HEIGHT = 64;
static const int CLOCK_CENTER_X = DISPLAY_WIDTH / 2;
static const int CLOCK_CENTER_Y = DISPLAY_HEIGHT / 2;
static const int CLOCK_RADIUS = 29; // keep one pixel distance from the frame

void drawClockFace() {
  u8g2.drawFrame(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  u8g2.drawCircle(CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_RADIUS);

  for (int tick = 0; tick < 12; tick++) {
    float angle = radians(tick * 30.0f - 90.0f);
    int x0 = CLOCK_CENTER_X + int((CLOCK_RADIUS - 2) * cos(angle));
    int y0 = CLOCK_CENTER_Y + int((CLOCK_RADIUS - 2) * sin(angle));
    int x1 = CLOCK_CENTER_X + int((CLOCK_RADIUS - 6) * cos(angle));
    int y1 = CLOCK_CENTER_Y + int((CLOCK_RADIUS - 6) * sin(angle));
    u8g2.drawLine(x0, y0, x1, y1);
  }
}

void drawHand(float angleDegrees, int length) {
  float angle = radians(angleDegrees - 90.0f);
  int x = CLOCK_CENTER_X + int(length * cos(angle));
  int y = CLOCK_CENTER_Y + int(length * sin(angle));
  u8g2.drawLine(CLOCK_CENTER_X, CLOCK_CENTER_Y, x, y);
}

void drawAnalogClock(int hours, int minutes, int seconds) {
  float secondAngle = seconds * 6.0f;
  float minuteAngle = (minutes + seconds / 60.0f) * 6.0f;
  float hourAngle = (hours + minutes / 60.0f) * 30.0f;

  drawHand(hourAngle, 18);
  drawHand(minuteAngle, 26);
  drawHand(secondAngle, 28);
  u8g2.drawDisc(CLOCK_CENTER_X, CLOCK_CENTER_Y, 1);
}

static int dayOfWeek(int year, int month, int day) {
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (month < 3) {
    year -= 1;
  }
  return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
}

static int lastSundayOfMonth(int year, int month) {
  int daysInMonth;
  if (month == 2) {
    bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    daysInMonth = leap ? 29 : 28;
  } else if (month == 4 || month == 6 || month == 9 || month == 11) {
    daysInMonth = 30;
  } else {
    daysInMonth = 31;
  }

  int dowLast = dayOfWeek(year, month, daysInMonth);
  return daysInMonth - dowLast;
}

static bool isBerlinDST(unsigned long utcEpochSeconds) {
  time_t t = utcEpochSeconds;
  struct tm tm;
  gmtime_r(&t, &tm);

  int year = tm.tm_year + 1900;
  int month = tm.tm_mon + 1;
  int day = tm.tm_mday;
  int hour = tm.tm_hour;

  if (month < 3 || month > 10) {
    return false;
  }
  if (month > 3 && month < 10) {
    return true;
  }

  int lastSunday = lastSundayOfMonth(year, month);
  if (month == 3) {
    if (day < lastSunday) {
      return false;
    }
    if (day > lastSunday) {
      return true;
    }
    return hour >= 1;
  }

  // October
  if (day < lastSunday) {
    return true;
  }
  if (day > lastSunday) {
    return false;
  }
  return hour < 1;
}

void sendNTPpacket(const char* address) {
  memset(ntpPacketBuffer, 0, NTP_PACKET_SIZE);
  ntpPacketBuffer[0] = 0b11100011;
  ntpPacketBuffer[1] = 0;
  ntpPacketBuffer[2] = 6;
  ntpPacketBuffer[3] = 0xEC;
  ntpPacketBuffer[12] = 49;
  ntpPacketBuffer[13] = 0x4E;
  ntpPacketBuffer[14] = 49;
  ntpPacketBuffer[15] = 52;

  udp.beginPacket(address, 123);
  udp.write(ntpPacketBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

bool syncTime() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
      delay(200);
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  udp.begin(2390);
  sendNTPpacket(ntpServer);
  unsigned long start = millis();
  while (millis() - start < 2000) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      udp.read(ntpPacketBuffer, NTP_PACKET_SIZE);
      unsigned long highWord = word(ntpPacketBuffer[40], ntpPacketBuffer[41]);
      unsigned long lowWord = word(ntpPacketBuffer[42], ntpPacketBuffer[43]);
      unsigned long secsSince1900 = (highWord << 16) | lowWord;
      const unsigned long seventyYears = 2208988800UL;
      unsigned long epoch = secsSince1900 - seventyYears;
      unsigned long localEpoch = epoch + CET_OFFSET_SEC + (isBerlinDST(epoch) ? CET_OFFSET_SEC : 0);
      currentTimeSeconds = localEpoch;
      lastMillis = millis();
      timeSynced = true;
      udp.stop();
      return true;
    }
    delay(10);
  }

  udp.stop();
  return false;
}

void setup() {
  u8g2.begin();
  u8g2.setDrawColor(1);
  u8g2.clearBuffer();
  u8g2.sendBuffer();

  WiFi.mode(WIFI_STA);
  syncTime();
  lastSyncMillis = millis();
  lastMillis = millis();
}

void loop() {
  unsigned long now = millis();

  if (now - lastMillis >= 1000) {
    currentTimeSeconds += (now - lastMillis) / 1000;
    lastMillis += ((now - lastMillis) / 1000) * 1000;
  }

  if (now - lastSyncMillis >= SYNC_INTERVAL_MS) {
    if (syncTime()) {
      lastSyncMillis = now;
    } else {
      lastSyncMillis = now; // try again after another interval
    }
  }

  unsigned long secondsOfDay = currentTimeSeconds % 86400UL;
  int hours = secondsOfDay / 3600;
  int minutes = (secondsOfDay % 3600) / 60;
  int seconds = secondsOfDay % 60;

  u8g2.clearBuffer();
  drawClockFace();
  drawAnalogClock(hours, minutes, seconds);
  u8g2.sendBuffer();

  delay(50);
}
