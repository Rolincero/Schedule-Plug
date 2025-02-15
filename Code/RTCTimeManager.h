#pragma once

#include <RTClib.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Preferences.h>
#include "Pins.h"

class RTCTimeManager {
public:
  RTCTimeManager() : 
    timeClient(ntpUDP, "pool.ntp.org", 3600 * 3, 60000) // 3 часа смещения, обновление раз в минуту
  {}

  void init() {
    if (!rtc.begin()) {
      Serial.println("Couldn't find RTC");
      while (1);
    }
    
    if (rtc.lostPower()) {
      Serial.println("RTC lost power, need sync!");
      needsSync = true;
    }
  }

  void syncTime() {
    if (WiFi.status() == WL_CONNECTED) {
      timeClient.begin();
      if (timeClient.forceUpdate()) {
        rtc.adjust(DateTime(timeClient.getEpochTime()));
        needsSync = false;
        Serial.println("NTP sync successful");
      }
    }
  }

  DateTime getNow() {
    return rtc.now();
  }

  bool needsTimeSync() const {
    return needsSync;
  }

  void setManualTime(const DateTime& dt) {
    rtc.adjust(dt);
    needsSync = false;
  }

  String formatDateTime(const DateTime& dt) {
    char buf[25];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             dt.year(), dt.month(), dt.day(),
             dt.hour(), dt.minute(), dt.second());
    return String(buf);
  }

private:
  RTC_DS3231 rtc;
  WiFiUDP ntpUDP;
  NTPClient timeClient;
  bool needsSync = true;
};