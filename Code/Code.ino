#include <Wire.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <SSD1306Wire.h>
#include <ESP32Encoder.h>
#include <TM1637Display.h>
#include "fontsRus.h"

// Конфигурация пинов
#define TM1637_CLK 18
#define TM1637_DIO 19
#define TEMP_PIN 4
#define GPIO_CONTROL 5
#define ENCODER_CLK 23
#define ENCODER_DT 25
#define ENCODER_SW 26
#define I2C_SDA 21
#define I2C_SCL 22

// scantime WiFi
unsigned long lastScanTime = 0;
const unsigned long SCAN_INTERVAL = 2000;

// Настройки шрифта
const int LINE_HEIGHT = 12;
const int TOP_PADDING = 5;
const int LEFT_PADDING = 5;

// Переменные контроля температуры
const float TEMP_HIGH_THRESHOLD = 75.0;
const float TEMP_LOW_THRESHOLD = 50.0;
bool tempProtectionActive = false;
bool overheatStatus = false;

// Дни недели
const char* daysOfWeek[] = {"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота", "Воскресенье"};

// Меню расписания
int currentDay = 0;
uint32_t tempStartTime = 0;
uint32_t tempEndTime = 0;

// Энкодер
unsigned long lastEncoderChange = 0;
int encoderSpeed = 1;
const int maxSpeed = 20;

// Wi-Fi
struct WiFiNetwork {
  String ssid;
  int32_t rssi;
  bool secured;
};
WiFiNetwork wifiNetworks[15];
int wifiNetworkCount = 0;
int selectedNetwork = 0;
String wifiPassword;
int selectedCharIndex = 0;
const char passwordChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+[]{}|;:,.<>?/`~ ";

// RTC
int tempYear = 0;
int tempMonth = 0;
int tempDay = 0;
int tempHour = 0;
int tempMinute = 0;
int tempSecond = 0;

// Настройки WiFi
const char* ssid = "Rolincero 2.4G";
const char* password = "Password";

// NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 36000);

// Устройства
TM1637Display display(TM1637_CLK, TM1637_DIO);
RTC_DS3231 rtc;
SSD1306Wire oled(0x3C, I2C_SDA, I2C_SCL);
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
ESP32Encoder encoder;
WebServer server(80);
Preferences preferences;

// Структуры данных
struct Schedule {
  uint32_t start;
  uint32_t end;
};

// Состояния меню
enum MenuState {
  MAIN_MENU,
  MENU_NAVIGATION,
  TIME_SETUP,
  TIME_SETUP_YEAR,
  TIME_SETUP_MONTH,
  TIME_SETUP_DAY,
  TIME_SETUP_HOUR,
  TIME_SETUP_MINUTE,
  TIME_SETUP_SECOND,
  SCHEDULE_SETUP,
  SCHEDULE_DAY_SELECT,
  SCHEDULE_START_SELECT,
  SCHEDULE_END_SELECT,
  TEMP_SETUP,
  WIFI_IDLE,
  WIFI_SCANNING,
  WIFI_CONNECTING,
  WIFI_SAVING_CREDENTIALS,
  WIFI_SHOW_LIST,
  WIFI_PASSWORD_ENTRY,
  SAVE_EXIT
};

// Элементы меню
const char* mainMenuItems[] = {
  "Настройки времени",
  "Расписание",
  "Температура",
  "Wi-Fi",
  "Сохранить настройки"
};
const int menuItemsCount = 5;

// Глобальные переменные
Schedule weeklySchedule[7];
MenuState currentMenu = MAIN_MENU;
int menuIndex = 0;
int menuScroll = 0;
bool gpioState = false;
bool timeSynced = false;
bool buttonPressed = false;
unsigned long lastButtonPress = 0;
long oldEncoderPos = 0;

// Прототипы функций
void loadSchedule();
void connectToWiFi(const char* ssid, const char* password);
void handleRoot();
void handleSetSchedule();
void checkSchedule(DateTime now);
String formatTime(uint32_t seconds);
uint32_t parseTime(String timeStr);
void saveAndExit();
void handleLongPress();
void handleShortPress();
void updateMenu();
void drawMainMenu();
void drawMenuNavigation();
void drawTimeSetup();
void drawScheduleSetup();
void drawTempSetup();
void drawWiFiSetup();
void drawSaveExit();
void handleEncoder();
void handleButton();
void displayTime(uint32_t timeInSeconds);
void displayTemperature(float temp);

void setup() {
  Serial.begin(115200);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  pinMode(GPIO_CONTROL, OUTPUT);

  encoder.attachSingleEdge(ENCODER_CLK, ENCODER_DT);
  encoder.setFilter(1023);

  Wire.begin(I2C_SDA, I2C_SCL);

  if(!oled.init()) {
    Serial.println("Ошибка OLED!");
    while(1);
  }
  oled.flipScreenVertically();
  oled.setFont(ArialRus_Plain_10);
  oled.setFontTableLookupFunction(FontUtf8Rus);

  preferences.begin("wifi", true);
  String savedSSID = preferences.getString("wifi_ssid", "");
  String savedPass = preferences.getString("wifi_pass", "");
  preferences.end();

  if (savedSSID != "") {
    connectToWiFi(savedSSID.c_str(), savedPass.c_str());
  } else {
    connectToWiFi(ssid, password);
  }

  if (!rtc.begin()) {
    showDisplayError("Ошибка RTC!");
    while(1);
  }

  sensors.begin();
  if (sensors.getDeviceCount() == 0) {
    showDisplayError("Ошибка датчика!");
    while(1);
  }

  preferences.begin("schedule", false);
  loadSchedule();

  if(WiFi.status() == WL_CONNECTED) {
    timeClient.begin();
    if(timeClient.update()) {
      rtc.adjust(DateTime(timeClient.getEpochTime()));
      timeSynced = true;
    }
  }

  server.on("/", handleRoot);
  server.on("/set", handleSetSchedule);
  server.begin();

  display.setBrightness(7);
  updateDisplay(rtc.now(), sensors.getTempCByIndex(0));
}

void loop() {
  server.handleClient();
  DateTime now = rtc.now();

  static unsigned long lastWifiUpdate =0;
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();

    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);
    checkTemperatureProtection(temp);

    if (currentMenu == MAIN_MENU) {
      checkSchedule(now);
      drawMainMenu();
    }

    if (gpioState) {
      displayTemperature(temp);
    } else {
      displayTime(getNextStartTime(now));
    }
  }

  if(currentMenu == WiFi LIST && millis()-lastScanTime >= SCAN INTERVAL){
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
    currentMenu = WiFi SCAN;
    lastScanTime = millis();
    updateMenu();
   }

  handleEncoder();
  handleButton();
}

// OLED функции
void updateDisplay(DateTime now, float temp) {
  if(currentMenu != MAIN_MENU) return;

  oled.clear();
  oled.setFont(ArialRus_Plain_10);

  uint8_t dayOfWeek = (now.dayOfTheWeek() + 6) % 7;
  char timeStr[40];
  sprintf(timeStr, "%s  %02d:%02d:%02d", daysOfWeek[dayOfWeek], now.hour(), now.minute(), now.second());
  oled.drawString(LEFT_PADDING, TOP_PADDING, timeStr);

  char tempStr[10];
  sprintf(tempStr, "%+.1fC", temp);
  oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, tempStr);

  oled.drawString(LEFT_PADDING, TOP_PADDING + 2 * LINE_HEIGHT, gpioState ? "В РАБОТЕ" : "ОЖИДАНИЕ");

  if(WiFi.status() != WL_CONNECTED) {
    oled.drawString(LEFT_PADDING, TOP_PADDING + 3 * LINE_HEIGHT, "WiFi ВЫКЛЮЧЕН!");
  }

  oled.display();
}

void showDisplayError(const char* msg) {
  oled.clear();
  oled.drawString(0, 0, msg);
  oled.display();
}

// TM1637 функции
void displayTime(uint32_t timeInSeconds) {
  uint8_t hours = timeInSeconds / 3600;
  uint8_t minutes = (timeInSeconds % 3600) / 60;
  display.showNumberDecEx(hours * 100 + minutes, 0b01000000, true);
}

uint8_t encodeDigit(int digit) {
  return display.encodeDigit(digit);
}

void displayTemperature(float temp) {
  int16_t tempInt = round(temp);
  uint8_t segments[4] = {0};
  segments[0] = 0b00111001;

  if (tempInt < 0) {
    segments[1] = 0b01000000;
    tempInt = abs(tempInt);
    segments[2] = encodeDigit((tempInt / 10) % 10);
    segments[3] = encodeDigit(tempInt % 10);
  } else {
    segments[1] = encodeDigit((tempInt / 10) % 10);
    segments[2] = encodeDigit(tempInt % 10);
    segments[3] = 0;
  }

  display.setSegments(segments);
}

// Меню
void updateMenu() {
  switch (currentMenu) {
    case MAIN_MENU: drawMainMenu(); break;
    case MENU_NAVIGATION: drawMenuNavigation(); break;
    case TIME_SETUP: drawTimeSetup(); break;
    case TIME_SETUP_YEAR:
    case TIME_SETUP_MONTH:
    case TIME_SETUP_DAY:
    case TIME_SETUP_HOUR:
    case TIME_SETUP_MINUTE:
    case TIME_SETUP_SECOND: drawTimeSetupStep(); break;
    case SCHEDULE_SETUP:
    case SCHEDULE_DAY_SELECT:
    case SCHEDULE_START_SELECT:
    case SCHEDULE_END_SELECT: drawScheduleSetup(); break;
    case TEMP_SETUP: drawTempSetup(); break;
    case WIFI_SETUP:
    case WIFI_SCAN:
    case WIFI_LIST:
    case WIFI_PASSWORD_INPUT: drawWiFiSetup(); break;
    case SAVE_EXIT: drawSaveExit(); break;
    default: currentMenu = MAIN_MENU; drawMainMenu(); break;
  }
}

// Рисование элементов меню
void drawMainMenu() {
  oled.clear();
  oled.setFont(ArialRus_Plain_10);
  oled.drawString(LEFT_PADDING, TOP_PADDING, "Главное меню");

  DateTime now = rtc.now();
  char timeStr[30];
  sprintf(timeStr, "%s  %02d:%02d:%02d", daysOfWeek[(now.dayOfTheWeek() + 6) % 7], now.hour(), now.minute(), now.second());
  oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, timeStr);

  float temp = sensors.getTempCByIndex(0);
  char tempStr[10];
  sprintf(tempStr, "%+.1fC", temp);
  oled.drawString(LEFT_PADDING, TOP_PADDING + 2 * LINE_HEIGHT, tempStr);

  String stateLine = gpioState ? "В РАБОТЕ" : "ОЖИДАНИЕ";
  stateLine += overheatStatus ? " / ПЕРЕГРЕВ!" : " / Штатное";
  oled.drawString(LEFT_PADDING, TOP_PADDING + 3 * LINE_HEIGHT, stateLine);

  oled.display();
}

void drawWiFiSetup() {
  oled.clear();
  oled.setFont(ArialRus_Plain_10);

  switch(currentMenu) {
    case WIFI_SCAN:
      oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT*0,"Сканирование...");
      oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT*1,"Найдено сетей:");
      oled.drawString(LEFT_PADDING + 80, TOP_PADDING + LINE_HEIGHT*1,String(WiFi.scanComplete()).c_str());
      break;

    case WIFI_LIST: {
      oled.drawString(LEFT_PADDING+30, TOP_PADDING,"Сети WiFi");
      int startIdx = max(0, selectedNetwork - 1);
      int endIdx = min(wifiNetworkCount-1.startIdx +3);

      for(int i=startIdx; i<=endIdx; i++){ 
        String line;
        if(i == selectedNetwork) line += ">";
        line += wifiNetworks[i].ssid.substring(0.min(wifiNetworks[i].ssid.length(),15));
        line += " ";
        line += wifiNetworks[i].secured ? "[🔒]" : "[ ]";
        
        int yPos = TOP_PADDING + LINE_HEIGHT*(i-startIdx+1);
        if(yPos > OLED_HEIGHT-LINE_HEIGHT) break;
        
        oled.drawString(LEFT_PADDING,yPos.line);
       }
       break;
     }

    case WIFI_PASSWORD_INPUT: {
      oled.drawString(LEFT_PADDING, TOP_PADDING, "Пароль для:");
      oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, wifiNetworks[selectedNetwork].ssid);
      oled.drawString(LEFT_PADDING, TOP_PADDING + 2*LINE_HEIGHT, wifiPassword);
      
      String charLine = "[";
      charLine += passwordChars[selectedCharIndex];
      charLine += "]";
      oled.drawString(LEFT_PADDING, TOP_PADDING + 3*LINE_HEIGHT, charLine);
      
      oled.drawString(LEFT_PADDING, TOP_PADDING + 5*LINE_HEIGHT, "Коротко: символ");
      oled.drawString(LEFT_PADDING, TOP_PADDING + 6*LINE_HEIGHT, "Долго: сохранить");
      break;
    }

    default:
      if (WiFi.status() == WL_CONNECTED) {
        oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, "Подключен");
        oled.drawString(LEFT_PADDING, TOP_PADDING + 2*LINE_HEIGHT, WiFi.localIP().toString());
      } else {
        oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, "Отключен");
      }
      oled.drawString(LEFT_PADDING, TOP_PADDING + 4*LINE_HEIGHT, "OK - Сканировать");
      break;
  }

  if(currentMenu != MAIN_MENU){
     oled.drawString(LEFT PADDING.Oled.getHeight()-LINE HEIGHT."Долгое: Назад");
   }
   
   oled.display();
}

// Обработчики ввода
void handleEncoder() {
  static unsigned long lastReadTime = 0;
  const unsigned long debounceDelay = 50;

  if (currentMenu == MAIN_MENU) return;
  if (millis() - lastReadTime < debounceDelay) return;

  long newPos = encoder.getCount();
  static long oldPos = -1;

  if (oldPos == -1) oldPos = newPos;
  if (newPos == oldPos) return;

  int delta = (newPos > oldPos) ? 1 : -1;
  unsigned long currentTime = millis();
  
  if (currentTime - lastEncoderChange < 200) {
    encoderSpeed = constrain(encoderSpeed + 1, 1, maxSpeed);
  } else {
    encoderSpeed = 1;
  }
  lastEncoderChange = currentTime;

  switch (currentMenu) {
    case MENU_NAVIGATION:
      menuIndex = constrain(menuIndex + delta, 0, menuItemsCount - 1);
      if (menuIndex < menuScroll) menuScroll = menuIndex;
      else if (menuIndex >= menuScroll + 4) menuScroll = menuIndex - 3;
      menuScroll = constrain(menuScroll, 0, menuItemsCount - 4);
      break;

    case WIFI_LIST:
      selectedNetwork = constrain(selectedNetwork + delta, 0, wifiNetworkCount-1);
      break;

    case WIFI_PASSWORD_INPUT:
      selectedCharIndex = (selectedCharIndex + delta + strlen(passwordChars)) % strlen(passwordChars);
      break;

    // Другие case...
  }

  oldPos = newPos;
  lastReadTime = millis();
  updateMenu();
}

void handleButton() {
  int btnState = digitalRead(ENCODER_SW);

  if (btnState == LOW && !buttonPressed) {
    buttonPressed = true;
    lastButtonPress = millis();
  }

  if (btnState == HIGH && buttonPressed) {
    buttonPressed = false;
    unsigned long duration = millis() - lastButtonPress;
    if (duration < 50) return;

    if (currentMenu == MAIN_MENU) {
      currentMenu = MENU_NAVIGATION;
      menuIndex = 0;
      menuScroll = 0;
    } else {
      if (duration < 1000) handleShortPress();
      else handleLongPress();
    }
    updateMenu();
  }
}

void handleShortPress() {
  DateTime now = rtc.now(); // Перемещаем объявление переменной за пределы switch

  switch (currentMenu) {
    case MENU_NAVIGATION:
      switch (menuIndex) {
        case 0:
          currentMenu = TIME_SETUP;
          tempYear = now.year();
          tempMonth = now.month();
          tempDay = now.day();
          tempHour = now.hour();
          tempMinute = now.minute();
          tempSecond = now.second();
          break;
        case 1:
          currentMenu = SCHEDULE_DAY_SELECT;
          break; // Переход к выбору дня
        case 2:
          currentMenu = TEMP_SETUP;
          break;
        case 3:
          currentMenu = WIFI_SETUP;
          break;
        case 4:
          saveAndExit();
          break;
      }
      updateMenu(); // Обновляем экран после изменения состояния меню
      break;

    case TIME_SETUP:
      currentMenu = TIME_SETUP_YEAR;
      updateMenu(); // Обновляем экран
      break;

    case TIME_SETUP_YEAR:
      currentMenu = TIME_SETUP_MONTH;
      updateMenu(); // Обновляем экран
      break;

    case TIME_SETUP_MONTH:
      currentMenu = TIME_SETUP_DAY;
      updateMenu(); // Обновляем экран
      break;

    case TIME_SETUP_DAY:
      currentMenu = TIME_SETUP_HOUR;
      updateMenu(); // Обновляем экран
      break;

    case TIME_SETUP_HOUR:
      currentMenu = TIME_SETUP_MINUTE;
      updateMenu(); // Обновляем экран
      break;

    case TIME_SETUP_MINUTE:
      currentMenu = TIME_SETUP_SECOND;
      updateMenu(); // Обновляем экран
      break;

    case TIME_SETUP_SECOND:
      currentMenu = TIME_SETUP_YEAR;
      updateMenu(); // Обновляем экран
      break;

    case SCHEDULE_DAY_SELECT:
      // Переход к настройке времени старта
      currentMenu = SCHEDULE_START_SELECT;
      tempStartTime = weeklySchedule[currentDay].start; // Загружаем текущее время старта
      updateMenu();
      break;

    case SCHEDULE_START_SELECT:
      // Переход к настройке времени окончания
      currentMenu = SCHEDULE_END_SELECT;
      tempEndTime = weeklySchedule[currentDay].end; // Загружаем текущее время окончания
      updateMenu();
      break;

    case SCHEDULE_END_SELECT:
      // Сохранение настроек и выход
      weeklySchedule[currentDay].start = tempStartTime;
      weeklySchedule[currentDay].end = tempEndTime;
      saveSchedule(); // Сохраняем изменения
      showSaveMessage(); // Показываем сообщение о сохранении
      currentMenu = MAIN_MENU; // Возврат в главное меню
      updateMenu();
      break;

    case TEMP_SETUP:
      // Подтверждение температурных настроек
      {
        float targetTemp = sensors.getTempCByIndex(0); // Пример значения
        // preferences.putFloat("target_temp", targetTemp);
        showSaveMessage(); // Показываем сообщение о сохранении
        currentMenu = MAIN_MENU;
      }
      updateMenu();
      break;

    case WIFI_SETUP:
      // Начать сканирование сетей
      WiFi.scanDelete();
      WiFi.scanNetworks(true);
      currentMenu = WIFI_SCAN; // Переход в состояние сканирования
      updateMenu(); // Обновляем экран
      break;

    case WIFI_LIST:
      // Выбрать сеть
      if(wifiNetworks[selectedNetwork].secured) {
        wifiPassword = "";
        selectedCharIndex = 0;
        currentMenu = WIFI_PASSWORD_INPUT;
      } else {
        // Подключиться без пароля
        connectToWiFi(wifiNetworks[selectedNetwork].ssid.c_str(), "");
      }
      updateMenu(); // Обновляем экран
      break;

    case WIFI_PASSWORD_INPUT:
      // Добавить выбранный символ
      wifiPassword += passwordChars[selectedCharIndex];
      updateMenu(); // Обновляем экран
      break;

    case SAVE_EXIT:
      // Дублируем сохранение на случай прямого доступа
      saveAndExit();
      break;

    default:
      // Возврат в главное меню по умолчанию
      currentMenu = MAIN_MENU;
      updateMenu();
      break;
  }
}

void handleLongPress() {
  unsigned long duration=millis()-lastButtonPress;
  
  if (currentMenu >= TIME_SETUP_YEAR && currentMenu <= TIME_SETUP_SECOND) {
    // Сохраняем настройки времени
    rtc.adjust(DateTime(tempYear, tempMonth, tempDay, tempHour, tempMinute, tempSecond));
    showSaveMessage();
    currentMenu = MAIN_MENU;
    drawMainMenu();
  }

 if(currentMenu >=WIFISETUP && currentMenu <=WIFIPASSWORDINPUT){
    currentMenu=MAINMENU;
    updateMenu(); 
    return; 
 }
}

// ====================== Остальные функции ====================== //
void checkTemperatureProtection(float temp) {
  if (temp >= TEMP_HIGH_THRESHOLD) {
    digitalWrite(GPIO_CONTROL, LOW);
    gpioState = false;
    tempProtectionActive = true;
    overheatStatus = true; // Активируем статус перегрева
  } else if (temp <= TEMP_LOW_THRESHOLD && tempProtectionActive) {
    digitalWrite(GPIO_CONTROL, HIGH);
    gpioState = true;
    tempProtectionActive = false;
    overheatStatus = false; // Сбрасываем статус перегрева
  }
}

void checkSchedule(DateTime now) {
  uint8_t currentDay = (now.dayOfTheWeek() + 6) % 7;
  uint32_t currentTime = now.hour()*3600 + now.minute()*60 + now.second();

  if(currentTime >= weeklySchedule[currentDay].start &&
     currentTime <= weeklySchedule[currentDay].end) {
    digitalWrite(GPIO_CONTROL, HIGH);
    gpioState = true;
  } else {
    digitalWrite(GPIO_CONTROL, LOW);
    gpioState = false;
  }
}

void loadSchedule() {
  for(int i=0; i<7; i++) {
    weeklySchedule[i].start = preferences.getUInt(("day"+String(i)+"_start").c_str(),0);
    weeklySchedule[i].end = preferences.getUInt(("day"+String(i)+"_end").c_str(),0);
  }
}

void saveSchedule() {
  for(int i=0; i<7; i++) {
    preferences.putUInt(("day"+String(i)+"_start").c_str(), weeklySchedule[i].start);
    preferences.putUInt(("day"+String(i)+"_end").c_str(), weeklySchedule[i].end);
  }
}

uint32_t getNextStartTime(DateTime now) {
  uint32_t currentTime = now.hour()*3600 + now.minute()*60 + now.second();
  uint8_t currentDay = (now.dayOfTheWeek() + 6) % 7;

  for(int i=0; i<7; i++) {
    uint8_t day = (currentDay + i) % 7;
    if(weeklySchedule[day].start > currentTime || i>0) {
      return weeklySchedule[day].start;
    }
  }
  return weeklySchedule[0].start;
}

String formatTime(uint32_t seconds) {
  uint8_t h = seconds/3600;
  uint8_t m = (seconds%3600)/60;
  return (h<10?"0":"") + String(h) + ":" + (m<10?"0":"") + String(m);
}

uint32_t parseTime(String timeStr) {
  int h = timeStr.substring(0,2).toInt();
  int m = timeStr.substring(3,5).toInt();
  return h*3600 + m*60;
}

void connectToWiFi(const char* ssid.const char* password){
  preferences.begin("wifi".false);
  preferences.putString("wifi_ssid".ssid);
  preferences.putString("wifi_pass".password); 
  preferences.end();
  WiFi.begin(ssid, password);
  
  unsigned long start = millis();
  while(WiFi.status() != WL_CONNECTED && millis()-start < 15000) {
    delay(500);
    oled.clear();
    oled.drawString(0, 0, "Подключаемся к:");
    oled.drawString(0, 12, ssid);
    oled.display();
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    // Синхронизация времени
    timeClient.update();
    rtc.adjust(DateTime(timeClient.getEpochTime()));
  }
}

void saveAndExit() {
  preferences.end();
  oled.clear();
  oled.setFont(ArialRus_Plain_10);
  oled.drawString(0, 0, "Настройки сохранены!");
  oled.display();
  delay(2000); 
  currentMenu = MAIN_MENU;
  // Убираем вызов updateMenu(), чтобы избежать двойной перерисовки
  drawMainMenu(); // Принудительно рисуем главный экран
}
void handleRoot() {
  String html = "<html><body><h1>Управление розеткой</h1>";
  html += "<form action='/set' method='POST'>";

  const char* days[] = {"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота", "Воскресенье"};

  for(int i=0; i<7; i++) {
    html += "<h3>" + String(days[i]) + "</h3>";
    html += "Вкл: <input type='time' name='day" + String(i) + "_start' value='" +
            formatTime(weeklySchedule[i].start) + "'><br>";
    html += "Выкл: <input type='time' name='day" + String(i) + "_end' value='" +
            formatTime(weeklySchedule[i].end) + "'><br>";
  }

  html += "<input type='submit' value='Сохранить'></form>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleSetSchedule() {
  for(int i=0; i<7; i++) {
    String start = server.arg("day" + String(i) + "_start");
    String end = server.arg("day" + String(i) + "_end");

    weeklySchedule[i].start = parseTime(start);
    weeklySchedule[i].end = parseTime(end);
  }

  saveSchedule();
  server.send(200, "text/plain", "Расписание обновлено!");
}

void showSaveMessage() {
  oled.clear();
  oled.setFont(ArialRus_Plain_10);
  oled.drawString(LEFT_PADDING, TOP_PADDING, "Настройки сохранены!");
  oled.display();
  delay(1000); // Показываем сообщение 1 секунду
}

char FontUtf8Rus(const byte ch) {
    static uint8_t LASTCHAR;

    if ((LASTCHAR == 0) && (ch < 0xC0)) {
      return ch;
    }

    if (LASTCHAR == 0) {
        LASTCHAR = ch;
        return 0;
    }

    uint8_t last = LASTCHAR;
    LASTCHAR = 0;

    switch (last) {
        case 0xD0:
            if (ch == 0x81) return 0xA8;
            if (ch >= 0x90 && ch <= 0xBF) return ch + 0x30;
            break;
        case 0xD1:
            if (ch == 0x91) return 0xB8;
            if (ch >= 0x80 && ch <= 0x8F) return ch + 0x70;
            break;
    }

    return (uint8_t) 0;
}
