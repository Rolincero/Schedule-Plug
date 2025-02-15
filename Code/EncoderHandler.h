#pragma once
#include <ESP32Encoder.h>
#include "Pins.h"

class EncoderHandler {
public:
  enum ButtonAction {
    NONE,
    SHORT_PRESS,
    LONG_PRESS,
    VERY_LONG_PRESS
  };

  EncoderHandler() : encoder() {}

  void init() {
    encoder.attachSingleEdge(ENCODER_CLK, ENCODER_DT);
    encoder.setFilter(1023);
    pinMode(ENCODER_SW, INPUT_PULLUP);
    lastEncoderPos = encoder.getCount();
  }

  void update() {
    handleRotation();
    handleButton();
  }

  int getDelta() {
    int delta = rotationDelta;
    rotationDelta = 0;
    return delta;
  }

  ButtonAction getButtonAction() {
    ButtonAction action = currentAction;
    currentAction = NONE;
    return action;
  }

  void setEncoderSpeed(int speed) {
    encoderSpeed = constrain(speed, 1, 20);
  }

private:
  ESP32Encoder encoder;
  int encoderSpeed = 1;
  int rotationDelta = 0;
  ButtonAction currentAction = NONE;
  long lastEncoderPos = 0;
  bool buttonPressed = false;
  unsigned long lastButtonPress = 0;
  unsigned long lastRotationTime = 0;

  void handleRotation() {
    long newPos = encoder.getCount();
    if (newPos != lastEncoderPos) {
      rotationDelta += (newPos > lastEncoderPos) ? 1 : -1;
      lastEncoderPos = newPos;
      
      // Автоматическая регулировка скорости
      unsigned long now = millis();
      if (now - lastRotationTime < 200) {
        encoderSpeed = min(encoderSpeed + 1, 20);
      } else {
        encoderSpeed = 1;
      }
      lastRotationTime = now;
    }
  }

  void handleButton() {
    int btnState = digitalRead(ENCODER_SW);
    
    if (btnState == LOW && !buttonPressed) {
      buttonPressed = true;
      lastButtonPress = millis();
    }
    
    if (btnState == HIGH && buttonPressed) {
      buttonPressed = false;
      unsigned long duration = millis() - lastButtonPress;
      
      if (duration > 50) { // Антидребезг
        if (duration < 1000) {
          currentAction = SHORT_PRESS;
        } else if (duration < 5000) {
          currentAction = LONG_PRESS;
        } else {
          currentAction = VERY_LONG_PRESS;
        }
      }
    }
  }
};