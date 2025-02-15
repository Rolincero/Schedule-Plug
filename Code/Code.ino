#include "MenuSystem.h"
#include "RTCTimeManager.h"
#include "RelayController.h"
#include "ScheduleManager.h"
#include "DisplayManager.h"
#include "TemperatureControl.h"
#include "WiFiManager.h"
#include "EncoderHandler.h"

// Создаем все объекты
RTCTimeManager timeManager;
RelayController relay;
ScheduleManager scheduler(relay);
DisplayManager display(relay, scheduler);
TemperatureControl tempControl(relay);
WiFiManager wifi(timeManager, scheduler);
EncoderHandler encoder;
MenuSystem menu(display, encoder, timeManager, scheduler, wifi, tempControl);

void setup() {
  Serial.begin(115200);
  timeManager.init();
  relay = RelayController();
  tempControl.init();
  display.init();
  wifi.init();
  encoder.init();
}

void loop() {
  DateTime now = timeManager.getNow();
  tempControl.update();
  scheduler.checkSchedule(now);
  wifi.handleClient();
  menu.update();
  
  display.drawMainScreen(now, tempControl.getTemperature(), 
                       tempControl.isOverheated(), wifi.getState());
  
  delay(100);
}