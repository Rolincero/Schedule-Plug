#pragma once
#include "Pins.h"

class RelayController {
public:
  RelayController() {
    pinMode(GPIO_CONTROL, OUTPUT);
    setState(false);
  }

  void setState(bool newState) {
    if(newState != currentState && !blocked) {
      digitalWrite(GPIO_CONTROL, newState);
      currentState = newState;
      lastStateChange = millis();
    }
  }

  void emergencyShutdown() {
    if(!blocked) {
      digitalWrite(GPIO_CONTROL, LOW);
      currentState = false;
      blocked = true;
      emergencyTime = millis();
    }
  }

  void tryReset() {
    if(blocked && (millis() - emergencyTime > 5000)) {
      blocked = false;
    }
  }

  bool isBlocked() const {
    return blocked;
  }

  bool getState() const {
    return currentState;
  }

private:
  bool currentState = false;
  bool blocked = false;
  unsigned long lastStateChange = 0;
  unsigned long emergencyTime = 0;
};