#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>
#include "RelayController.h"
#include "Pins.h"

class TemperatureControl {
public:
  TemperatureControl(RelayController& relay) : 
    relay(relay), 
    oneWire(TEMP_PIN),
    sensors(&oneWire) 
  {}

  void init() {
    sensors.begin();
    if(sensors.getDeviceCount() == 0) {
      Serial.println("No temperature sensors!");
      relay.emergencyShutdown();
    }
    loadCalibration();
  }

  void update() {
    static unsigned long lastUpdate = 0;
    if(millis() - lastUpdate >= SENSOR_UPDATE_INTERVAL) {
      sensors.requestTemperatures();
      currentTemp = sensors.getTempCByIndex(0) + calibrationOffset;
      checkProtection();
      lastUpdate = millis();
    }
  }

  float getTemperature() const { return currentTemp; }
  bool isOverheated() const { return overheatStatus; }
  void setCalibration(float offset) { calibrationOffset = offset; saveCalibration(); }
  float getCalibration() const { return calibrationOffset; }

private:
  RelayController& relay;
  OneWire oneWire;
  DallasTemperature sensors;
  Preferences prefs;
  float calibrationOffset = 0.0;
  float currentTemp = 0.0;
  bool overheatStatus = false;

  void checkProtection() {
    if(relay.isBlocked() && currentTemp <= TEMP_LOW_THRESHOLD) {
      relay.tryReset();
    }
    
    if(currentTemp >= TEMP_HIGH_THRESHOLD && !overheatStatus) {
      relay.emergencyShutdown();
      overheatStatus = true;
    }
    else if(currentTemp <= TEMP_LOW_THRESHOLD && overheatStatus) {
      overheatStatus = false;
    }
  }

  void loadCalibration() {
    prefs.begin("temp", true);
    calibrationOffset = prefs.getFloat("calib", 0.0);
    prefs.end();
  }

  void saveCalibration() {
    prefs.begin("temp", false);
    prefs.putFloat("calib", calibrationOffset);
    prefs.end();
  }
};