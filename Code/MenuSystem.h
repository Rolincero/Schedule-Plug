#pragma once
#include "DisplayManager.h"
#include "EncoderHandler.h"
#include "RTCTimeManager.h"
#include "ScheduleManager.h"
#include "WiFiManager.h"
#include "TemperatureControl.h"

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
  DisplayManager& display;
  Encoder ```cpp
Handler& encoder;
  RTCTimeManager& rtc;
  ScheduleManager& schedule;
  WiFiManager& wifi;
  TemperatureControl& temp;
  
  State currentState = MAIN_SCREEN;
  int menuIndex = 0;
  unsigned long resetStartTime = 0;
  const char* mainMenuItems[5] = {
    "Set Time",
    "Set Schedule",
    "Reset WiFi",
    "Calibrate Temp",
    "Exit"
  };

  void handleEncoder() {
    EncoderHandler::ButtonAction action = encoder.getButtonAction();
    int delta = encoder.getDelta();

    switch(currentState) {
      case MAIN_SCREEN:
        if(action == EncoderHandler::SHORT_PRESS) {
          currentState = MAIN_MENU;
          menuIndex = 0;
        }
        break;

      case MAIN_MENU:
        if(delta != 0) {
          menuIndex = constrain(menuIndex + delta, 0, 4);
        }
        if(action == EncoderHandler::SHORT_PRESS) {
          handleMenuSelection();
        }
        if(action == EncoderHandler::LONG_PRESS) {
          currentState = MAIN_SCREEN;
        }
        break;

      case RESET_ANIMATION:
        if(millis() - resetStartTime > 20000) {
          performFactoryReset();
          currentState = MAIN_SCREEN;
        }
        if(action == EncoderHandler::LONG_PRESS) {
          currentState = MAIN_SCREEN;
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
        display.drawMenu(mainMenuItems, 5, menuIndex);
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