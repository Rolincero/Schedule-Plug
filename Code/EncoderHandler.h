#pragma once
#include <ESP32Encoder.h>
#include "Pins.h"

class EncoderHandler {
public:
  enum ButtonAction {
    NONE,
    SHORT_PRESS,    // <500ms
    LONG_PRESS,     // >=2000ms
    VERY_LONG_PRESS // >=20000ms
  };

  EncoderHandler() : encoder() {}

  void init() {
    encoder.attachSingleEdge(ENCODER_CLK, ENCODER_DT);
    encoder.setFilter(1023);
    pinMode(ENCODER_SW, INPUT_PULLUP);
    lastEncoderPos = encoder.getCount();
  }

  bool isButtonPressed() const {
	  return buttonPressed;
  }

  unsigned long getPressDuration() const {
	  if (buttonPressed) {
		  return millis() - buttonPressStart;
	  }
	  return 0;
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

private:
  ESP32Encoder encoder;
  int rotationDelta = 0;
  ButtonAction currentAction = NONE;
  long lastEncoderPos = 0;
  bool buttonPressed = false;
  unsigned long buttonPressStart = 0;

  void handleRotation() {
    long newPos = encoder.getCount();
    if (newPos != lastEncoderPos) {
      rotationDelta += (newPos > lastEncoderPos) ? 1 : -1;
      lastEncoderPos = newPos;
    }
  }

  void handleButton() {
    int btnState = digitalRead(ENCODER_SW);
    
    if (btnState == LOW && !buttonPressed) {
      buttonPressed = true;
      buttonPressStart = millis();
    }
    
    if (btnState == HIGH && buttonPressed) {
      buttonPressed = false;
      unsigned long duration = millis() - buttonPressStart;
      
      if (duration > 50) { // Debounce
        if (duration < 500) {
          currentAction = SHORT_PRESS;
        } else if (duration >= 1000 && duration < 20000) {
          currentAction = LONG_PRESS;
        } else if (duration >= 20000) {
          currentAction = VERY_LONG_PRESS;
        }
      }
    }
  }
};