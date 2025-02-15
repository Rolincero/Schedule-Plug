#pragma once
#include <Preferences.h>
#include <RTClib.h>
#include "RelayController.h"

class ScheduleManager {
public:
  struct Schedule {
    uint32_t start;
    uint32_t end;
  };

  ScheduleManager(RelayController& relay) : relay(relay) {}

  void load() {
    prefs.begin("schedule", true);
    for(int i = 0; i < 7; i++) {
      weeklySchedule[i].start = prefs.getUInt(getKey(i, true).c_str(), 0);
      weeklySchedule[i].end = prefs.getUInt(getKey(i, false).c_str(), 0);
    }
    prefs.end();
  }

  void save() {
    prefs.begin("schedule", false);
    for(int i = 0; i < 7; i++) {
      prefs.putUInt(getKey(i, true).c_str(), weeklySchedule[i].start);
      prefs.putUInt(getKey(i, false).c_str(), weeklySchedule[i].end);
    }
    prefs.end();
  }

  bool checkSchedule(const DateTime& now) {
    if(relay.isBlocked()) return false;

    uint8_t currentDay = now.dayOfTheWeek() % 7;
    uint32_t currentTime = now.hour() * 3600 + now.minute() * 60 + now.second();
    
    bool newState = false;
    Schedule daySchedule = weeklySchedule[currentDay];

    if(daySchedule.start < daySchedule.end) {
      newState = (currentTime >= daySchedule.start && currentTime <= daySchedule.end);
    } else {
      newState = (currentTime >= daySchedule.start || currentTime <= daySchedule.end);
    }
    
    if(newState != relay.getState()) {
      relay.setState(newState);
    }
    return newState;
  }

  Schedule weeklySchedule[7];

private:
  RelayController& relay;
  Preferences prefs;

  String getKey(uint8_t day, bool isStart) {
    return String("d") + day + (isStart ? "s" : "e");
  }
};