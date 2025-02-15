#include "RTCTimeManager.h"
#include "WiFiManager.h"
#include "DisplayManager.h"
#include "RelayController.h"
#include "TemperatureControl.h"
#include "ScheduleManager.h"

RTCTimeManager timeManager;
RelayController relay;
TemperatureControl tempControl(relay);
ScheduleManager scheduler(relay);
DisplayManager display(relay);
WiFiManager wifi;

void setup() {
  Serial.begin(115200);
  
  // Инициализация модулей
  timeManager.init();
  relay.setState(false);
  tempControl.init();
  scheduler.load();
  display.init();
  wifi.init(&timeManager);
}

void loop() {
  // Обновление модулей
  DateTime now = timeManager.getNow();
  tempControl.update();
  scheduler.checkSchedule(now);
  wifi.handleClient();
  
  // Обновление дисплея
  display.updateMainScreen(
    now,
    tempControl.getTemperature(),
    tempControl.isOverheated()
  );
  
  delay(100);
}