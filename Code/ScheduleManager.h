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
			uint32_t start = prefs.getUInt(getKey(i, true).c_str(), 0);
			uint32_t end = prefs.getUInt(getKey(i, false).c_str(), 0);
			// Валидация данных
			weeklySchedule[i].start = (start <= 86400) ? start : 0;
			weeklySchedule[i].end = (end <= 86400) ? end : 0;
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

  void reset() {
    prefs.begin("schedule", false);
    prefs.clear();
    prefs.end();
    load(); // Загрузить пустые значения
  }

	void forceScheduleCheck(const DateTime& now) {
		bool newState = checkSchedule(now);
		updateRelayState(newState);
	}

	void updateRelayState(bool newState) const {
		if(newState != relay.getState() && !relay.isBlocked()) {
			relay.setState(newState);
		}
	}

	bool checkSchedule(const DateTime& now) const {
		if(relay.isBlocked()) return false;
		
		// Корректировка дня недели согласно DS3231 (0=воскресенье -> 0=понедельник)
		uint8_t currentDay = (now.dayOfTheWeek() + 6) % 7; // Преобразование к 0=ПН, 6=ВС
		uint32_t currentTime = now.hour() * 3600 + now.minute() * 60 + now.second();
		Schedule daySchedule = weeklySchedule[currentDay];
		
		bool newState = false;
		
		if(daySchedule.start == daySchedule.end) {
			newState = false; // Расписание отключено
		}
		else if(daySchedule.start < daySchedule.end) {
			newState = (currentTime >= daySchedule.start && currentTime < daySchedule.end);
		} 
		else {
			newState = (currentTime >= daySchedule.start || currentTime < daySchedule.end);
		}
		
		updateRelayState(newState);
		return newState;
	}

	DateTime getNextStartTime(const DateTime& now) {
		uint8_t currentDay = (now.dayOfTheWeek() + 6) % 7; // Корректировка дня
		uint32_t currentTime = now.hour() * 3600 + now.minute() * 60 + now.second();
		
		for(int i = 0; i < 7; i++) {
			uint8_t day = (currentDay + i) % 7;
			uint32_t targetTime = weeklySchedule[day].start;
			
			if((i == 0 && targetTime > currentTime) || i > 0) {
				return now + TimeSpan((i * 86400) + (targetTime - currentTime));
			}
		}
		return now + TimeSpan(86400); // Fallback: следующее расписание через 24ч
	}

	DateTime getNextShutdownTime(const DateTime& now) {
		uint8_t currentDay = (now.dayOfTheWeek() + 6) % 7; // Исправлено приведение дня недели
		uint32_t currentTime = now.hour() * 3600 + now.minute() * 60 + now.second();
	
		for(int i = 0; i < 7; i++) {
			uint8_t day = (currentDay + i) % 7;
			uint32_t endTime = weeklySchedule[day].end;
	
			// Если время окончания сегодня и еще не наступило
			if(i == 0 && endTime > currentTime) {
				return DateTime(now.year(), now.month(), now.day()) + TimeSpan(endTime);
			}
			// Если сегодняшнее время окончания прошло, ищем следующий день
			else if(i > 0 && weeklySchedule[day].end != weeklySchedule[day].start) {
				DateTime nextDay = now + TimeSpan(i, 0, 0, 0);
				return DateTime(nextDay.year(), nextDay.month(), nextDay.day()) + TimeSpan(weeklySchedule[day].end);
			}
		}
		return now; // Fallback
	}

	bool isActiveNow(const DateTime& now) const {
		return checkSchedule(now);
	}

  Schedule weeklySchedule[7];

private:
  RelayController& relay;
  Preferences prefs;

  String getKey(uint8_t day, bool isStart) {
    return String("d") + day + (isStart ? "s" : "e");
  }
};