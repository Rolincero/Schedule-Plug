#pragma once
#include <WiFi.h>
#include <RTClib.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Preferences.h>
#include "Pins.h"
#include "RelayController.h"
#include "ScheduleManager.h"

class RTCTimeManager {
public:

  RTCTimeManager() : 
  timeClient(ntpUDP, "pool.ntp.org", 0, 60000) {} 

	void init() {
		prefs.begin("time", true);
		timezoneOffset = prefs.getInt("tz", 3); // Значение по умолчанию 3 часа
		prefs.end();
		
		if (!rtc.begin()) {
			Serial.println("Couldn't find RTC");
			while (1);
		}
	}

	void syncTime() {
		if(WiFi.status() == WL_CONNECTED) {
			timeClient.setTimeOffset(timezoneOffset * 3600);
			if(timeClient.forceUpdate()) {
				unsigned long epoch = timeClient.getEpochTime();
				rtc.adjust(DateTime(epoch));
			}
		}
	}
  
  void setTimezoneOffset(int offset) {
		timezoneOffset = offset;
		prefs.begin("time", false);
		prefs.putInt("tz", offset);
		prefs.end();
	}

  int getTimezoneOffset() const {
		return timezoneOffset;
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
		DateTime adjusted = dt + TimeSpan(timezoneOffset * 3600);
		char buf[25];
		snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
				adjusted.year(), adjusted.month(), adjusted.day(),
				adjusted.hour(), adjusted.minute(), adjusted.second());
		return String(buf);
	}

private:
  RTC_DS3231 rtc;
  WiFiUDP ntpUDP;
  NTPClient timeClient;
  bool needsSync = true;
  int timezoneOffset = 3;
	Preferences prefs;
};