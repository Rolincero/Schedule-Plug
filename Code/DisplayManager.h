#pragma once

#include "WiFiManager.h"
#include <SSD1306Wire.h>
#include <TM1637Display.h>
#include "fontsRus.h"
#include "RTCTimeManager.h"
#include "RelayController.h"
#include "Pins.h"
#include "fontsRus.h"
#include "TimeEditField.h"


class MenuSystem;

class DisplayManager {
public:
  ScheduleManager* scheduler = nullptr;
  
  DisplayManager(RelayController& relay, ScheduleManager& sched) 
  : relay(relay), scheduler(&sched) {}
  
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
  
  void drawMenu(const char** items, int count, int selectedIndex) {
	oled.clear();
	oled.drawString(LEFT_PADDING, TOP_PADDING, "======= Меню =======");
	
	for(int i = 0; i < count; i++) {
	  String text = (i == selectedIndex) ? "> " + String(items[i]) : String(items[i]);
	  oled.drawString(LEFT_PADDING, TOP_PADDING + (i+1)*LINE_HEIGHT, text);
	}
	oled.display();
  }
  
  void drawMainScreen(const DateTime& now, float temp, bool overheatStatus, WiFiManager::WiFiState wifiState) {
	oled.clear();
	
	// Строка 1: Время и день недели
	char datetime[30];
	snprintf(datetime, sizeof(datetime), "%02d:%02d:%02d %s", 
			 now.hour(), now.minute(), now.second(),
			 daysOfWeek[(now.dayOfTheWeek() + 6) % 7]); // Исправление индексации дней
	oled.drawString(LEFT_PADDING, TOP_PADDING, datetime);
	
	// Строка 2: Температура и статус
	char tempStr[20];
	snprintf(tempStr, sizeof(tempStr), "%.0fC %s", 
			 temp, 
		  overheatStatus ? "Перегрев" : "Норма");
	oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, tempStr);
	
	// Строка 3: Состояние реле
	String status = relay.getState() ? "АКТИВНО" : "ОЖИДАНИЕ";
	if(relay.isBlocked()) status += " (БЛОКИРОВКА)";
	oled.drawString(LEFT_PADDING, TOP_PADDING + 2*LINE_HEIGHT, status);
	
	// Строка 4: Статус Wi-Fi
	String wifiStatus;
	switch(wifiState) {
	  case WiFiManager::WiFiState::CONNECTED: wifiStatus = "Подключен"; break;
	  case WiFiManager::WiFiState::AP_MODE: wifiStatus = "Точка доступа"; break;
	  default: wifiStatus = "Отключен";
	}
	oled.drawString(LEFT_PADDING, TOP_PADDING + 3*LINE_HEIGHT, "WiFi: " + wifiStatus);
	
	oled.display();
  }
  
	void drawTimeSetupScreen(const DateTime& time, TimeEditField currentField) {
	  oled.clear();
	  
	  // Заголовок
	  oled.drawString(LEFT_PADDING, TOP_PADDING, "Время и дата");
	  
	  // Дата: формат "ДД.ММ.ГГГГ"
	  char dateStr[20];
	  snprintf(dateStr, sizeof(dateStr), "%02d.%02d.%04d", 
			  time.day(), time.month(), time.year());
	  String dateString = String(dateStr);
	  
	  // Время: формат "ЧЧ:ММ:СС"
	  char timeStr[20];
	  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", 
			  time.hour(), time.minute(), time.second());
	  String timeString = String(timeStr);
	  
	  // Подсказка
	  String hint = (currentField == EDIT_CONFIRM) ? "ОК - Сохранить" : "Настройка...";
	  
	  // Отрисовка даты с подчеркиванием выбранного поля
	  int dateX = LEFT_PADDING;
	  int dateY = TOP_PADDING + LINE_HEIGHT;
	  
	  // День
	  if (currentField == EDIT_DAY) {
		drawUnderlinedText(dateX, dateY, dateString.substring(0, 2));
	  } else {
		oled.drawString(dateX, dateY, dateString.substring(0, 2));
	  }
	  
	  // Точка
	  oled.drawString(dateX + 2 * 6, dateY, ".");
	  
	  // Месяц
	  if (currentField == EDIT_MONTH) {
		drawUnderlinedText(dateX + 3 * 6, dateY, dateString.substring(3, 5));
	  } else {
		oled.drawString(dateX + 3 * 6, dateY, dateString.substring(3, 5));
	  }
	  
	  // Точка
	  oled.drawString(dateX + 5 * 6, dateY, ".");
	  
	  // Год
	  if (currentField == EDIT_YEAR) {
		drawUnderlinedText(dateX + 6 * 6, dateY, dateString.substring(6, 10));
	  } else {
		oled.drawString(dateX + 6 * 6, dateY, dateString.substring(6, 10));
	  }
	  
	  // Отрисовка времени с подчеркиванием выбранного поля
	  int timeX = LEFT_PADDING;
	  int timeY = TOP_PADDING + 2 * LINE_HEIGHT;
	  
	  // Часы
	  if (currentField == EDIT_HOUR) {
		drawUnderlinedText(timeX, timeY, timeString.substring(0, 2));
	  } else {
		oled.drawString(timeX, timeY, timeString.substring(0, 2));
	  }
	  
	  // Двоеточие
	  oled.drawString(timeX + 2 * 6, timeY, ":");
	  
	  // Минуты
	  if (currentField == EDIT_MINUTE) {
		drawUnderlinedText(timeX + 3 * 6, timeY, timeString.substring(3, 5));
	  } else {
		oled.drawString(timeX + 3 * 6, timeY, timeString.substring(3, 5));
	  }
	  
	  // Двоеточие
	  oled.drawString(timeX + 5 * 6, timeY, ":");
	  
	  // Секунды
	  if (currentField == EDIT_SECOND) {
		drawUnderlinedText(timeX + 6 * 6, timeY, timeString.substring(6, 8));
	  } else {
		oled.drawString(timeX + 6 * 6, timeY, timeString.substring(6, 8));
	  }
	  
	  // Подсказка
	  if (currentField == EDIT_CONFIRM) {
		drawUnderlinedText(LEFT_PADDING, TOP_PADDING + 3 * LINE_HEIGHT, hint);
	  } else {
		oled.drawString(LEFT_PADDING, TOP_PADDING + 3 * LINE_HEIGHT, hint);
	  }
	  
	  oled.display();
  }

  void showResetAnimation(float progress) {
	  oled.clear();
	
	  // Анимация прогресса
	  int barWidth = 128 * progress;
	  oled.fillRect(0, 20, barWidth, 10);
	
	  // Предупреждающий символ
	  oled.setFont(ArialMT_Plain_24);
	  oled.drawString(48, 40, "!");
	
	  oled.display();
  }
  
  void showDialog(const char* message, unsigned long duration) {
	  oled.clear();
	  oled.drawString(0, 20, message);
	  oled.display();
	  delay(duration);
  }

  void showError(const char* message) {
    oled.clear();
    oled.drawString(0, 0, message);
    oled.display();
  }

	void drawUnderlinedText(int x, int y, const String& text) {
	  // Рисуем текст
	  oled.drawString(x, y, text);
	  
	  // Рисуем линию под текстом
	  int textWidth = oled.getStringWidth(text); // Ширина текста
	  int underlineY = y + 10; // Позиция линии под текстом (10 пикселей ниже текста)
	  oled.drawLine(x, underlineY, x + textWidth, underlineY);
	}

private:
  SSD1306Wire oled{0x3c, I2C_SDA, I2C_SCL};
  TM1637Display tmDisplay{TM1637_CLK, TM1637_DIO};
  RelayController& relay;
  bool displayToggle = false;
  unsigned long lastDisplayUpdate = 0;
  
  static constexpr int LINE_HEIGHT = 12;
  static constexpr int TOP_PADDING = 5;
  static constexpr int LEFT_PADDING = 5;
  static constexpr unsigned long DISPLAY_UPDATE_INTERVAL = 2000;
  static const char* daysOfWeek[7];

  void updateTM1637(const DateTime& now, float temp) {
	static bool showTemp = true;
	static unsigned long lastSwitch = 0;
	
	if(millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
	  lastDisplayUpdate = millis();
	  
	  if(relay.getState()) {
		if(millis() - lastSwitch > 2000) {
		  showTemp = !showTemp;
		  lastSwitch = millis();
		}
		showTemp ? displayTemperature(temp) : displayShutdownTime(now);
	  } else {
		displayNextScheduleTime(now);
	  }
	}
  }


  void displayTime(const DateTime& now) {
    tmDisplay.showNumberDecEx(now.hour() * 100 + now.minute(), 0b01000000, true);
  }

  void displayTemperature(float temp) {
    int16_t tempInt = round(temp * 10);
    uint8_t data[4] = {
      static_cast<uint8_t>(tempInt < 0 ? 0x40 : 0x00),
      tmDisplay.encodeDigit(abs(tempInt) / 100),
      tmDisplay.encodeDigit((abs(tempInt) / 10) % 10),
      static_cast<uint8_t>(0x63 | (abs(tempInt) % 10 << 4))
    };
    tmDisplay.setSegments(data);
  }

	void displayShutdownTime(const DateTime& now) {
	  DateTime nextOff = scheduler->getNextShutdownTime(now);
	  tmDisplay.showNumberDecEx(nextOff.hour() * 100 + nextOff.minute(), 0b01000000, true);
	}

	void displayNextScheduleTime(const DateTime& now) {
	  DateTime nextOn = scheduler->getNextStartTime(now);
	  tmDisplay.showNumberDecEx(nextOn.hour() * 100 + nextOn.minute(), 0b01000000, true);
	}

};

// Инициализация статических членов
const char* DisplayManager::daysOfWeek[] = {"ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "ВС"};