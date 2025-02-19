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
#include "TempEditField.h"

class RelayController;
class ScheduleManager;
class RTCTimeManager;

class MenuSystem;

class DisplayManager {
public:
  ScheduleManager* scheduler = nullptr;
  
	DisplayManager(
		RelayController& relay, 
		ScheduleManager& sched,
		RTCTimeManager& tm  // Добавить параметр
	) : relay(relay), scheduler(&sched), timeManager(tm) {}
  
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
  int tzOffset = this->timeManager.getTimezoneOffset();
	snprintf(datetime, sizeof(datetime), "%02d:%02d:%02d %s (UTC%+d)", 
			now.hour(), now.minute(), now.second(),
			daysOfWeek[(now.dayOfTheWeek() + 6) % 7],
			tzOffset);
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

	void drawTemperatureCalibrationScreen(
	  float currentTemp, 
	  float calibratedTemp, 
	  float calibrationOffset,
	  TempEditField currentField
	  ) {

    oled.clear();
	  
	  // Заголовок
	  oled.drawString(LEFT_PADDING, TOP_PADDING, "=Кал-ка температуры=");
	  
	  // Исходное значение
	  char currentTempStr[40];
	  snprintf(currentTempStr, sizeof(currentTempStr), "Ист значение: %.1f", currentTemp);
	  if(currentField == EDIT_SOURCE) {
		drawUnderlinedText(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, currentTempStr);
	  } else {
		oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, currentTempStr);
	  }
	  
	  // Совмещенное значение
	  char calibratedTempStr[40];
	  snprintf(calibratedTempStr, sizeof(calibratedTempStr), "Совм значение: %.1f", calibratedTemp);
	  oled.drawString(LEFT_PADDING, TOP_PADDING + 2*LINE_HEIGHT, calibratedTempStr);
	  
	  // Смещение
	  char offsetStr[40];
	  snprintf(offsetStr, sizeof(offsetStr), "Знач смещения: %.1f", calibrationOffset);
	  if(currentField == EDIT_OFFSET) {
		drawUnderlinedText(LEFT_PADDING, TOP_PADDING + 3*LINE_HEIGHT, offsetStr);
	  } else {
		oled.drawString(LEFT_PADDING, TOP_PADDING + 3*LINE_HEIGHT, offsetStr);
	  }
	  
	  // Обязательное обновление дисплея
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
	  String hint = (currentField == TIME_EDIT_CONFIRM) ? "ЗАЖАТЬ - Сохранить" : "Настройка...";
	  
	  // Отрисовка даты с подчеркиванием выбранного поля
	  int dateX = LEFT_PADDING;
	  int dateY = TOP_PADDING + LINE_HEIGHT;
	  
	  // День
	  if (currentField == TIME_EDIT_DAY) {
		drawUnderlinedText(dateX, dateY, dateString.substring(0, 2));
	  } else {
		oled.drawString(dateX, dateY, dateString.substring(0, 2));
	  }
	  
	  // Точка
	  oled.drawString(dateX + 2 * 6, dateY, ".");
	  
	  // Месяц
	  if (currentField == TIME_EDIT_MONTH) {
		drawUnderlinedText(dateX + 3 * 6, dateY, dateString.substring(3, 5));
	  } else {
		oled.drawString(dateX + 3 * 6, dateY, dateString.substring(3, 5));
	  }
	  
	  // Точка
	  oled.drawString(dateX + 5 * 6, dateY, ".");
	  
	  // Год
	  if (currentField == TIME_EDIT_YEAR) {
		drawUnderlinedText(dateX + 6 * 6, dateY, dateString.substring(6, 10));
	  } else {
		oled.drawString(dateX + 6 * 6, dateY, dateString.substring(6, 10));
	  }
	  
	  // Отрисовка времени с подчеркиванием выбранного поля
	  int timeX = LEFT_PADDING;
	  int timeY = TOP_PADDING + 2 * LINE_HEIGHT;
	  
	  // Часы
	  if (currentField == TIME_EDIT_HOUR) {
		drawUnderlinedText(timeX, timeY, timeString.substring(0, 2));
	  } else {
		oled.drawString(timeX, timeY, timeString.substring(0, 2));
	  }
	  
	  // Двоеточие
	  oled.drawString(timeX + 2 * 6, timeY, ":");
	  
	  // Минуты
	  if (currentField == TIME_EDIT_MINUTE) {
		drawUnderlinedText(timeX + 3 * 6, timeY, timeString.substring(3, 5));
	  } else {
		oled.drawString(timeX + 3 * 6, timeY, timeString.substring(3, 5));
	  }
	  
	  // Двоеточие
	  oled.drawString(timeX + 5 * 6, timeY, ":");
	  
	  // Секунды
	  if (currentField == TIME_EDIT_SECOND) {
		drawUnderlinedText(timeX + 6 * 6, timeY, timeString.substring(6, 8));
	  } else {
		oled.drawString(timeX + 6 * 6, timeY, timeString.substring(6, 8));
	  }
	  
	  // Подсказка
	  if (currentField == TIME_EDIT_CONFIRM) {
		drawUnderlinedText(LEFT_PADDING, TOP_PADDING + 3 * LINE_HEIGHT, hint);
	  } else {
		oled.drawString(LEFT_PADDING, TOP_PADDING + 3 * LINE_HEIGHT, hint);
	  }
	  
	  oled.display();
  }

	void drawAPInfoScreen(const String& ssid, const String& pass, const String& ip) {
		oled.clear();
		oled.drawString(0, 0, "SSID: " + ssid);
		oled.drawString(0, 12, "Pass: " + pass);
		oled.drawString(0, 24, "IP: " + ip);
		oled.display();
	}

  void drawTimezoneSetupScreen(int currentOffset, bool editing) {
    oled.clear();
    
    // Заголовок
    oled.drawString(LEFT_PADDING, TOP_PADDING, "== Часовой пояс ==");
  
    // Текущее смещение
    char tzStr[30];
    snprintf(tzStr, sizeof(tzStr), "Смещение: UTC%+d", currentOffset);
    
    if(editing) {
      drawUnderlinedText(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, tzStr);
    } else {
      oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, tzStr);
    }
  
    // Подсказка
    String hint = editing ? "Вращайте энкодер" : "Нажмите для редакт.";
    oled.drawString(LEFT_PADDING, TOP_PADDING + 3*LINE_HEIGHT, hint);
  
    oled.display();
  }

  void drawScheduleSetupScreen(
	  uint8_t day,
	  uint32_t start,
	  uint32_t stop,
	  int acceleration,
	  ScheduleEditField field ) {
	  oled.clear();
  
	  // Заголовок
	  oled.drawString(LEFT_PADDING, TOP_PADDING, "Настройка расписания");
  
	  // День недели
	  String dayStr = "День: " + String(daysOfWeek[day]);
	  if(field == SCHEDULE_EDIT_DAY) {
	  	drawUnderlinedText(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, dayStr);
	  } else {
	  	oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, dayStr);
	  }
  
	  // Время старта
	  char startStr[30];
	  TimeSpan tsStart(start);
	  snprintf(startStr, sizeof(startStr), "%sСтарт: %02d:%02d", 
	  		 field == SCHEDULE_EDIT_START ? "> " : "  ",
	  		 tsStart.hours(), 
	  		 tsStart.minutes());
	  oled.drawString(LEFT_PADDING, TOP_PADDING + 2*LINE_HEIGHT, startStr);
  
	  // Время стопа
	  char stopStr[30];
	  TimeSpan tsStop(stop);
	  snprintf(stopStr, sizeof(stopStr), "%sСтоп: %02d:%02d", 
	  		 field == SCHEDULE_EDIT_STOP ? "> " : "  ",
	  		 tsStop.hours(), 
	  		 tsStop.minutes());
	  oled.drawString(LEFT_PADDING, TOP_PADDING + 3*LINE_HEIGHT, stopStr);
  
	  // Ускорение
	  char accelStr[30];
	  snprintf(accelStr, sizeof(accelStr), "Ускорение: %d мин", 
	  		 map(acceleration, 1, 6, 1, 30));
	  oled.drawString(LEFT_PADDING, TOP_PADDING + 4*LINE_HEIGHT, accelStr);
  
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
	  oled.drawString(x, y, text);
	  int width = oled.getStringWidth(text);
	  oled.drawLine(x, y + 10, x + width, y + 10);
	}

private:
  SSD1306Wire oled{0x3c, I2C_SDA, I2C_SCL};
  TM1637Display tmDisplay{TM1637_CLK, TM1637_DIO};
  RelayController& relay;
  RTCTimeManager& timeManager;
  
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