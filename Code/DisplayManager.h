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
	String status;
	if (relay.isBlocked()) {
		status = "БЛОКИРОВКА";
	} else {
		status = scheduler->isActiveNow(timeManager.getNow()) ? "АКТИВНО" : "ОЖИДАНИЕ"; // Добавить метод isActiveNow
	}
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
		String currentPrefix = (currentField == EDIT_SOURCE) ? "> " : "  ";
		snprintf(currentTempStr, sizeof(currentTempStr), "%sИст значение: %.1f", 
				currentPrefix.c_str(), currentTemp);
		oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, currentTempStr);
		
		// Совмещенное значение
		char calibratedTempStr[40];
		snprintf(calibratedTempStr, sizeof(calibratedTempStr), "  Совм значение: %.1f", calibratedTemp);
		oled.drawString(LEFT_PADDING, TOP_PADDING + 2*LINE_HEIGHT, calibratedTempStr);
		
		// Смещение
		char offsetStr[40];
		String offsetPrefix = (currentField == EDIT_OFFSET) ? "> " : "  ";
		snprintf(offsetStr, sizeof(offsetStr), "%sЗнач смещения: %.1f", 
				offsetPrefix.c_str(), calibrationOffset);
		oled.drawString(LEFT_PADDING, TOP_PADDING + 3*LINE_HEIGHT, offsetStr);
		
		oled.display();
	}
  
	void drawTimeSetupScreen(const DateTime& time, TimeEditField currentField) {
		oled.clear();
		
		// Заголовок
		oled.drawString(LEFT_PADDING, TOP_PADDING, "Время и дата");
		
		// Форматирование даты и времени
		char dateStr[30];
		snprintf(dateStr, sizeof(dateStr), "%s%02d.%s%02d.%s%04d",
				(currentField == TIME_EDIT_DAY) ? ">" : " ",
				time.day(),
				(currentField == TIME_EDIT_MONTH) ? "> " : " ",
				time.month(),
				(currentField == TIME_EDIT_YEAR) ? "> " : " ",
				time.year());
		
		char timeStr[30];
		snprintf(timeStr, sizeof(timeStr), "%s%02d:%s%02d:%s%02d",
				(currentField == TIME_EDIT_HOUR) ? ">" : " ",
				time.hour(),
				(currentField == TIME_EDIT_MINUTE) ? ">" : " ",
				time.minute(),
				(currentField == TIME_EDIT_SECOND) ? ">" : " ",
				time.second());
		
		// Подсказка
		String hint = (currentField == TIME_EDIT_CONFIRM) ? 
		"> ЗАЖАТЬ - Сохранить" : 
		"  Настройка...";
		
		// Отрисовка
		oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, dateStr);
		oled.drawString(LEFT_PADDING, TOP_PADDING + 2*LINE_HEIGHT, timeStr);
		oled.drawString(LEFT_PADDING, TOP_PADDING + 3*LINE_HEIGHT, hint);
		
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
		String prefix = editing ? "> " : "  ";
		snprintf(tzStr, sizeof(tzStr), "%sСмещение: UTC%+d", 
				prefix.c_str(), currentOffset);
		oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, tzStr);
		
		// Подсказка
		String hint = editing ? "  Вращайте энкодер" : "> Нажмите для редакт.";
		oled.drawString(LEFT_PADDING, TOP_PADDING + 3*LINE_HEIGHT, hint);
		
		oled.display();
	}

	void drawScheduleSetupScreen(
		uint8_t day,
		uint32_t start,
		uint32_t stop,
		int acceleration,
		ScheduleEditField field) 
	{
		oled.clear();
		
		// Заголовок
		oled.drawString(LEFT_PADDING, TOP_PADDING, "Настройка расписания");
		
		// День недели
		String dayPrefix = (field == SCHEDULE_EDIT_DAY) ? "> " : "  ";
		String dayStr = dayPrefix + "День: " + String(daysOfWeek[day]);
		oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, dayStr);
		
		// Время старта
		char startStr[30];
		TimeSpan tsStart(start);
		String startPrefix = (field == SCHEDULE_EDIT_START) ? "> " : "  ";
		snprintf(startStr, sizeof(startStr), "%sСтарт: %02d:%02d", 
				startPrefix.c_str(),
				tsStart.hours(), 
				tsStart.minutes());
		oled.drawString(LEFT_PADDING, TOP_PADDING + 2*LINE_HEIGHT, startStr);
		
		// Время стопа
		char stopStr[30];
		TimeSpan tsStop(stop);
		String stopPrefix = (field == SCHEDULE_EDIT_STOP) ? "> " : "  ";
		snprintf(stopStr, sizeof(stopStr), "%sСтоп: %02d:%02d", 
				stopPrefix.c_str(),
				tsStop.hours(), 
				tsStop.minutes());
		oled.drawString(LEFT_PADDING, TOP_PADDING + 3*LINE_HEIGHT, stopStr);
		
		// Ускорение
		char accelStr[30];
		snprintf(accelStr, sizeof(accelStr), "  Ускорение: %d мин", 
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