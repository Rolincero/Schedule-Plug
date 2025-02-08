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
DisplayManager display(relay, scheduler, timeManager);
TemperatureControl tempControl(relay);
WiFiManager wifi(timeManager, scheduler);
EncoderHandler encoder;
MenuSystem menu(display, encoder, timeManager, scheduler, wifi, tempControl);

// Инициализация периферии и загрузка сохраненных настроек
void setup() {
	Serial.begin(115200);
	timeManager.init();
  timeManager.syncTime();
	relay = RelayController();
	tempControl.init();
	display.init();
	wifi.init();
	encoder.init();
	digitalWrite(GPIO_CONTROL, LOW);
	scheduler.load();
}

void loop() {
  encoder.update();
  DateTime now = timeManager.getNow();
  tempControl.update();
  scheduler.checkSchedule(now);
  wifi.handleClient();
  menu.update();
  
  delay(100);
}
