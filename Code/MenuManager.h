// Новый класс MenuManager.h
#pragma once
#include "EncoderHandler.h"
#include "DisplayManager.h"

class MenuManager {
public:
  enum MenuState {
    MAIN_SCREEN,
    TIME_SETUP,
    SCHEDULE_SETUP,
    TEMP_CALIBRATION,
    WIFI_SETUP
  };

  MenuManager(DisplayManager& display, EncoderHandler& encoder) 
    : display(display), encoder(encoder) {}

  void update() {
    EncoderHandler::ButtonAction action = encoder.getButtonAction();
    int delta = encoder.getDelta();
    
    switch(currentState) {
      case MAIN_SCREEN:
        if(action == EncoderHandler::LONG_PRESS) {
          currentState = MENU_ROOT;
          currentItem = 0;
          display.drawMenu(menuItems, 5, currentItem);
        }
        break;
        
      case MENU_ROOT:
        if(delta != 0) {
          currentItem = constrain(currentItem + delta, 0, 4);
          display.drawMenu(menuItems, 5, currentItem);
        }
        if(action == EncoderHandler::SHORT_PRESS) {
          enterSubMenu(currentItem);
        }
        break;
        
      // Обработка подменю...
    }
  }

private:
  DisplayManager& display;
  EncoderHandler& encoder;
  MenuState currentState = MAIN_SCREEN;
  int currentItem = 0;
  const char* menuItems[5] = {
    "Set Time", 
    "Schedule",
    "Calibrate",
    "WiFi",
    "Exit"
  };

  void enterSubMenu(int index) {
    switch(index) {
      case 0: currentState = TIME_SETUP; break;
      case 1: currentState = SCHEDULE_SETUP; break;
      // ... другие пункты
    }
    display.drawSubMenu(menuItems[index]);
  }
};