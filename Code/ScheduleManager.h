// ScheduleManager.h
#pragma once

#include <Preferences.h>
#include <RTClib.h>
#include "Pins.h"

class ScheduleManager {
public:
  struct Schedule {
    uint32_t start;
    uint32_t end;
  };

  ScheduleManager() : prefs() {}

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
    uint8_t currentDay = now.dayOfTheWeek() % 7;
    uint32_t currentTime = now.hour() * 3600 + now.minute() * 60 + now.second();
    
    bool newState = false;
    Schedule daySchedule = weeklySchedule[currentDay];

    if(daySchedule.start < daySchedule.end) {
      newState = (currentTime >= daySchedule.start && currentTime <= daySchedule.end);
    } else {
      newState = (currentTime >= daySchedule.start || currentTime <= daySchedule.end);
    }
    
    return newState;
  }

  String formatTime(uint32_t seconds) {
    if(seconds == 0) return "--:--";
    uint8_t h = seconds / 3600;
    uint8_t m = (seconds % 3600) / 60;
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
    return String(buf);
  }

  uint32_t parseTime(const String& timeStr) {
    if(timeStr.length() != 5 || timeStr[2] != ':') return 0;
    
    uint8_t h = timeStr.substring(0, 2).toInt();
    uint8_t m = timeStr.substring(3, 5).toInt();
    
    if(h > 23 || m > 59) return 0;
    
    return h * 3600 + m * 60;
  }

  Schedule weeklySchedule[7];

private:
  Preferences prefs;

  String getKey(uint8_t day, bool isStart) {
    return String("d") + day + (isStart ? "s" : "e");
  }
};