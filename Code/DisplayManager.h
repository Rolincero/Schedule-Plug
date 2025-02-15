#pragma once

#include <SSD1306Wire.h>
#include <TM1637Display.h>
#include "fontsRus.h"         // Ваш кастомный шрифт
#include "RTCTimeManager.h"   // Для работы со временем
#include "Pins.h"             // GPIO и константы

class DisplayManager {
public:
  DisplayManager() : 
    oled(0x3C, I2C_SDA, I2C_SCL),
    tmDisplay(TM1637_CLK, TM1637_DIO) 
  {}

  void init() {
    // Инициализация OLED
    if(!oled.init()) {
      Serial.println("OLED Init Error!");
      while(true);
    }
    oled.flipScreenVertically();
    oled.setFont(ArialRus_Plain_10);
    oled.setFontTableLookupFunction(FontUtf8Rus);

    // Инициализация TM1637
    tmDisplay.setBrightness(7);
  }

  void updateMainScreen(const DateTime& now, float temp, bool gpioState, bool overheatStatus) {
    static String lastRenderedData;
    String newData = String(now.dayOfTheWeek()) + String(temp) + String(gpioState);
    
    if(newData != lastRenderedData) {
      drawMainScreen(now, temp, gpioState, overheatStatus);
      lastRenderedData = newData;
    }
    updateTM1637(now, temp, gpioState);
  }

  void drawMenu(const char** items, int count, int selectedIndex) {
    oled.clear();
    oled.drawString(LEFT_PADDING, TOP_PADDING, "== Меню ==");
    
    for(int i = 0; i < count; i++) {
      String text = (i == selectedIndex) ? "> " + String(items[i]) : String(items[i]);
      oled.drawString(LEFT_PADDING, TOP_PADDING + (i+1)*LINE_HEIGHT, text);
    }
    oled.display();
  }

  void showError(const char* message) {
    oled.clear();
    oled.drawString(0, 0, message);
    oled.display();
  }

private:
  SSD1306Wire oled;
  TM1637Display tmDisplay;
  bool displayToggle = false;
  unsigned long lastDisplayUpdate = 0;

  void drawMainScreen(const DateTime& now, float temp, bool gpioState, bool overheatStatus) {
    oled.clear();
    
    // Строка 1: Дата и время
    char datetime[25];
    snprintf(datetime, sizeof(datetime), "%02d:%02d %s", 
             now.hour(), now.minute(), 
             daysOfWeek[now.dayOfTheWeek()]);
    oled.drawString(LEFT_PADDING, TOP_PADDING, datetime);

    // Строка 2: Температура
    char tempStr[15];
    snprintf(tempStr, sizeof(tempStr), "Temp: %+.1fC", temp);
    oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, tempStr);

    // Строка 3: Статус
    String status = String(gpioState ? "ON " : "OFF") + 
                   (overheatStatus ? " OVERHEAT" : "");
    oled.drawString(LEFT_PADDING, TOP_PADDING + 2*LINE_HEIGHT, status);

    oled.display();
  }

  void updateTM1637(const DateTime& now, float temp, bool gpioState) {
    static bool showTemp = true;
    
    if(millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
      lastDisplayUpdate = millis();
      
      if(gpioState) {
        showTemp ? displayTemperature(temp) : displayTime(now);
        showTemp = !showTemp;
      } else {
        displayTime(now);
      }
    }
  }

  void displayTime(const DateTime& now) {
    tmDisplay.showNumberDecEx(now.hour() * 100 + now.minute(), 0b01000000, true);
  }

  void displayTemperature(float temp) {
    int16_t tempInt = round(temp * 10);
    uint8_t data[4] = {
      tempInt < 0 ? 0x40 : 0x00,
      tmDisplay.encodeDigit(abs(tempInt) / 100),
      tmDisplay.encodeDigit((abs(tempInt) / 10) % 10),
      0x63 | (abs(tempInt) % 10 << 4)
    };
    tmDisplay.setSegments(data);
  }

  // Константы из Pins.h
  static constexpr int LINE_HEIGHT = 12;
  static constexpr int TOP_PADDING = 5;
  static constexpr int LEFT_PADDING = 5;
  static constexpr unsigned long DISPLAY_UPDATE_INTERVAL = 2000;
  static const char* daysOfWeek[7];
};

// Инициализация статических членов
const char* DisplayManager::daysOfWeek[] = {"ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "ВС"};