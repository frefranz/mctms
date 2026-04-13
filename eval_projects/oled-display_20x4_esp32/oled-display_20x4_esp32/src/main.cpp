#include <Arduino.h>
#include <U8g2lib.h>

// U8G2 Constructor for SSD1309 128x64 OLED with I2C
// Adjust pins if necessary: SDA=21, SCL=22 for ESP32
U8G2_SSD1309_128X64_NONAME0_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 22, /* data=*/ 21, /* reset=*/ U8X8_PIN_NONE);

class OledLcd : public Print {
public:
  static const uint8_t COLS = 20;
  static const uint8_t ROWS = 4;

  void init() {
    u8g2.begin();
    u8g2.setFont(u8g2_font_6x10_tf);
    clear();
  }

  void clear() {
    for (uint8_t r = 0; r < ROWS; ++r) {
      lines[r] = "";
    }
    cursorCol = 0;
    cursorRow = 0;
    dirty = true;
    sendBuffer();
  }

  void backlight() {
    // No backlight control on the OLED; keep compatibility with LCD code.
  }

  void clearLine(uint8_t row) {
    if (row < ROWS) {
      lines[row] = "";
      dirty = true;
    }
  }

  void setCursor(uint8_t col, uint8_t row) {
    cursorCol = min(col, (uint8_t)(COLS - 1));
    cursorRow = min(row, (uint8_t)(ROWS - 1));
  }

  void home() {
    cursorCol = 0;
    cursorRow = 0;
  }

  void cursor() {
    // No visible text cursor on the OLED, compatibility only.
  }

  void noCursor() {
    // No visible text cursor on the OLED, compatibility only.
  }

  void blink() {
    // No visible blinking cursor on the OLED, compatibility only.
  }

  void noBlink() {
    // No visible blinking cursor on the OLED, compatibility only.
  }

  virtual size_t write(uint8_t c) override {
    if (c == '\n') {
      cursorRow = min<uint8_t>(cursorRow + 1, ROWS - 1);
      cursorCol = 0;
      return 1;
    }

    if (cursorCol >= COLS) {
      cursorCol = 0;
      cursorRow = min<uint8_t>(cursorRow + 1, ROWS - 1);
    }

    if (cursorRow < ROWS) {
      if (cursorCol < lines[cursorRow].length()) {
        lines[cursorRow].setCharAt(cursorCol, c);
      } else {
        while (lines[cursorRow].length() < cursorCol) {
          lines[cursorRow] += ' ';
        }
        lines[cursorRow] += (char)c;
      }
      cursorCol++;
      dirty = true;
    }
    return 1;
  }

  void sendBuffer() {
    if (!dirty) {
      return;
    }

    u8g2.clearBuffer();
    for (uint8_t row = 0; row < ROWS; ++row) {
      if (lines[row].length() > 0) {
        u8g2.drawStr(0, 10 + row * 14, lines[row].c_str());
      }
    }
    u8g2.sendBuffer();
    dirty = false;
  }

private:
  String lines[ROWS];
  uint8_t cursorCol = 0;
  uint8_t cursorRow = 0;
  bool dirty = false;
};

OledLcd lcd;

void setup() {
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Hello world!");
  lcd.setCursor(0, 1);
  lcd.print("OLED 20x4 Demo");
  lcd.setCursor(0, 2);
  lcd.print("Powered by ESP32");
  lcd.sendBuffer();
}

void loop() {
  lcd.setCursor(0, 3);
  lcd.print("Uptime: ");
  lcd.print(millis() / 1000);
  lcd.print("s");
  lcd.sendBuffer();

  delay(500);
}