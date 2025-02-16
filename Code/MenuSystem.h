#pragma once
#include "TimeEditField.h"
#include "DisplayManager.h"
#include "EncoderHandler.h"
#include "RTCTimeManager.h"
#include "ScheduleManager.h"
#include "WiFiManager.h"
#include "TemperatureControl.h"

class DisplayManager;

class MenuSystem {
public:
  enum State {
    MAIN_SCREEN,
    MAIN_MENU,
    TIME_SETUP,
    SCHEDULE_SETUP,
    WIFI_RESET,
    TEMP_CALIBRATION,
    RESET_ANIMATION
  };

MenuSystem(DisplayManager& display, EncoderHandler& encoder, 
		   RTCTimeManager& rtc, ScheduleManager& schedule,
		   WiFiManager& wifi, TemperatureControl& temp)
      : display(display), encoder(encoder), rtc(rtc),
      schedule(schedule), wifi(wifi), temp(temp) {}

  void update() {
    handleEncoder();
    updateDisplay();
  }

private:
  DateTime editingTime; // Временная переменная для редактирования времени
  TimeEditField currentEditField = EDIT_YEAR; 
  
  int visibleStartIndex = 0; // Начальный индекс видимого диапазона
  const int visibleItemsCount = 4; // Количество видимых пунктов меню
  DisplayManager& display;
  EncoderHandler& encoder;
  RTCTimeManager& rtc;
  ScheduleManager& schedule;
  WiFiManager& wifi;
  TemperatureControl& temp;
  
  State currentState = MAIN_SCREEN;
  int menuIndex = 0;
  unsigned long resetStartTime = 0;
  const char* mainMenuItems[5] = {
    "Настройка времени",
    "Установка расписания",
    "Сброс Wi-Fi",
    "Калиб-ка температуры",
    "Выход"
  };

  void handleEncoder() {
    EncoderHandler::ButtonAction action = encoder.getButtonAction();
    int delta = encoder.getDelta();

    switch(currentState) {
		  case MAIN_SCREEN:
			if(action == EncoderHandler::SHORT_PRESS) {
			  currentState = MAIN_MENU;
			  menuIndex = 0;
			  visibleStartIndex = 0;
			}
			break;
			
		  case MAIN_MENU:
			if(delta != 0) {
			  menuIndex = constrain(menuIndex + delta, 0, 4);
			  if(menuIndex > visibleStartIndex + visibleItemsCount - 1) {
				visibleStartIndex++;
			  }
			  if(menuIndex < visibleStartIndex) {
				visibleStartIndex--;
			  }
			  visibleStartIndex = constrain(visibleStartIndex, 0, 5 - visibleItemsCount);
			  display.drawMenu(mainMenuItems + visibleStartIndex, visibleItemsCount, menuIndex - visibleStartIndex);
			}
			if(action == EncoderHandler::SHORT_PRESS) {
			  if (menuIndex == 0) { // Настройка времени
				currentState = TIME_SETUP;
				editingTime = rtc.getNow(); // Начинаем редактирование с текущего времени
				currentEditField = EDIT_YEAR; // Начинаем с года
			  } else {
				handleMenuSelection();
			  }
			}
			if(action == EncoderHandler::LONG_PRESS) {
			  currentState = MAIN_SCREEN;
			  DateTime now = rtc.getNow();
			  display.drawMainScreen(now, temp.getTemperature(), 
									temp.isOverheated(), wifi.getState());
			}
			break;
			
		  case TIME_SETUP:
			if (action == EncoderHandler::SHORT_PRESS) {
			  // Переход к следующему полю
			  currentEditField = static_cast<TimeEditField>((currentEditField + 1) % (EDIT_CONFIRM + 1));
			}
			if (action == EncoderHandler::LONG_PRESS) {
			  // Сохраняем время и выходим из режима настройки
			  rtc.setManualTime(editingTime);
			  currentState = MAIN_SCREEN;
			  DateTime now = rtc.getNow();
			  display.drawMainScreen(now, temp.getTemperature(), 
									temp.isOverheated(), wifi.getState());
			}
			if (delta != 0) {
			  // Редактирование текущего поля
			  switch (currentEditField) {
				case EDIT_YEAR:
				  editingTime = DateTime(editingTime.year() + delta, editingTime.month(), editingTime.day(),
										editingTime.hour(), editingTime.minute(), editingTime.second());
				  break;
				case EDIT_MONTH:
				  editingTime = DateTime(editingTime.year(), constrain(editingTime.month() + delta, 1, 12), editingTime.day(),
										editingTime.hour(), editingTime.minute(), editingTime.second());
				  break;
				case EDIT_DAY:
				  editingTime = DateTime(editingTime.year(), editingTime.month(), constrain(editingTime.day() + delta, 1, 31),
										editingTime.hour(), editingTime.minute(), editingTime.second());
				  break;
				case EDIT_HOUR:
				  editingTime = DateTime(editingTime.year(), editingTime.month(), editingTime.day(),
										constrain(editingTime.hour() + delta, 0, 23), editingTime.minute(), editingTime.second());
				  break;
				case EDIT_MINUTE:
				  editingTime = DateTime(editingTime.year(), editingTime.month(), editingTime.day(),
										editingTime.hour(), constrain(editingTime.minute() + delta, 0, 59), editingTime.second());
				  break;
				case EDIT_SECOND:
				  editingTime = DateTime(editingTime.year(), editingTime.month(), editingTime.day(),
										editingTime.hour(), editingTime.minute(), constrain(editingTime.second() + delta, 0, 59));
				  break;
				case EDIT_CONFIRM:
				  // Ничего не делаем, ждем LONG_PRESS для сохранения
				  break;
			  }
			}
			break;

      // Обработка других состояний...
    }

    if(action == EncoderHandler::VERY_LONG_PRESS) {
      resetStartTime = millis();
      currentState = RESET_ANIMATION;
    }
  }

  void updateDisplay() {
    switch(currentState) {
		  case MAIN_SCREEN: {
			DateTime now = rtc.getNow();
			display.drawMainScreen(now, temp.getTemperature(), 
								  temp.isOverheated(), wifi.getState());
			break;
		  }
		  case MAIN_MENU:
			display.drawMenu(mainMenuItems + visibleStartIndex, visibleItemsCount, menuIndex - visibleStartIndex);
			break;
			
		  case TIME_SETUP:
			display.drawTimeSetupScreen(editingTime, currentEditField);
			break;
			
		  case RESET_ANIMATION:
			drawResetAnimation();
			break;
      // Отрисовка других состояний...
    }
  }

  void drawResetAnimation() {
    float progress = (millis() - resetStartTime) / 20000.0;
    display.showResetAnimation(progress);
  }

  void handleMenuSelection() {
    switch(menuIndex) {
      case 0: currentState = TIME_SETUP; break;
      case 1: currentState = SCHEDULE_SETUP; break;
      case 2: resetWiFi(); break;
      case 3: currentState = TEMP_CALIBRATION; break;
      case 4: currentState = MAIN_SCREEN; break;
    }
  }

  void resetWiFi() {
    wifi.resetCredentials();
    display.showDialog("WiFi reset!", 2000);
    currentState = MAIN_SCREEN;
  }

  void performFactoryReset() {
    schedule.reset();
    wifi.resetCredentials();
    temp.resetCalibration();
    display.showDialog("Factory reset!", 3000);
  }
};