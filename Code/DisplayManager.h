#pragma once

#include <SSD1306Wire.h>
#include <TM1637Display.h>
#include "fontsRus.h"
#include "RTCTimeManager.h"
#include "RelayController.h"
#include "Pins.h"
#include "fontsRus.h"

class DisplayManager {
public:
  DisplayManager(RelayController& relay) : 
    oled(0x3C, I2C_SDA, I2C_SCL),
    tmDisplay(TM1637_CLK, TM1637_DIO),
    relay(relay) 
  {}

  void init() {
    if(!oled.init()) {
      Serial.println("OLED Init Error!");
      while(true);
    }
    oled.flipScreenVertically();
    oled.setFont(ArialRus_Plain_10);
    oled.setFontTableLookupFunction(FontUtf8Rus);
    
    tmDisplay.setBrightness(7);
  }

  void updateMainScreen(const DateTime& now, float temp, bool overheatStatus) {
    static String lastRenderedData;
    String newData = String(now.dayOfTheWeek()) + 
                    String(temp) + 
                    String(relay.getState()) + 
                    String(overheatStatus);
    
    if(newData != lastRenderedData) {
      drawMainScreen(now, temp, overheatStatus);
      lastRenderedData = newData;
    }
    updateTM1637(now, temp);
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
  RelayController& relay;
  bool displayToggle = false;
  unsigned long lastDisplayUpdate = 0;
  
  static constexpr int LINE_HEIGHT = 12;
  static constexpr int TOP_PADDING = 5;
  static constexpr int LEFT_PADDING = 5;
  static constexpr unsigned long DISPLAY_UPDATE_INTERVAL = 2000;
  static const char* daysOfWeek[7];

  void drawMainScreen(const DateTime& now, float temp, bool overheatStatus) {
    oled.clear();
    
    // Строка 1: Дата и время с индикацией блокировки
    char datetime[30];
    snprintf(datetime, sizeof(datetime), "%02d:%02d %s %s", 
             now.hour(), now.minute(), 
             daysOfWeek[now.dayOfTheWeek()],
             relay.isBlocked() ? "!" : " ");
    oled.drawString(LEFT_PADDING, TOP_PADDING, datetime);

    // Строка 2: Температура с иконкой
    char tempStr[20];
    snprintf(tempStr, sizeof(tempStr), "%s %+.1fC", 
             overheatStatus ? "🔥" : "🌡", temp);
    oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, tempStr);

    // Строка 3: Статус реле
    String status = String(relay.getState() ? "ON " : "OFF ");
    if(relay.isBlocked()) {
      status += "(BLOCKED)";
    }
    oled.drawString(LEFT_PADDING, TOP_PADDING + 2*LINE_HEIGHT, status);

    oled.display();
  }

  void updateTM1637(const DateTime& now, float temp) {
    static bool showTemp = true;
    
    if(millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
      lastDisplayUpdate = millis();
      
      if(relay.getState()) {
        if(showTemp) {
          displayTemperature(temp);
        } else {
          displayTime(now);
        }
        showTemp = !showTemp;
      } else {
        displayNextScheduleTime(now);
        showTemp = true;
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

  void displayNextScheduleTime(const DateTime& now) {
    // Заглушка для отображения следующего времени включения
    tmDisplay.showNumberDecEx(8888, 0b01000000, true);
  }

};

// Инициализация статических членов
const char* DisplayManager::daysOfWeek[] = {"ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "ВС"};