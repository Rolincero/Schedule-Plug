#include <Wire.h>               // Arduino Core ESP32 https://github.com/espressif/arduino-esp32
#include <RTClib.h>             // Adafruit RTC lib https://github.com/adafruit/RTClib
#include <OneWire.h>            // https://github.com/PaulStoffregen/OneWire
#include <DallasTemperature.h>  // DS18B20 driver https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <WiFi.h>               // Arduino Core ESP32 https://github.com/espressif/arduino-esp32
#include <WebServer.h>          // Arduino Core ESP32 https://github.com/espressif/arduino-esp32
#include <Preferences.h>        // Arduino Core ESP32 https://github.com/espressif/arduino-esp32
#include <WiFiUdp.h>            // Arduino Core ESP32 https://github.com/espressif/arduino-esp32
#include <NTPClient.h>          // Still need description? https://github.com/arduino-libraries/NTPClient
#include <SSD1306Wire.h>        // OLED display SSD1306 driver https://github.com/ThingPulse/esp8266-oled-ssd1306
#include <ESP32Encoder.h>       // https://github.com/madhephaestus/ESP32Encoder
#include <TM1637Display.h>      // TM1637 Driver https://github.com/avishorp/TM1637
#include "fontsRus.h"           // Still don't remember where i got it. Just added Russian lang support to SSD1306.

//  ▗▄▄▖ ▗▄▄▄▖▗▖  ▗▖ ▗▄▖ ▗▖ ▗▖▗▄▄▄▖
//  ▐▌ ▐▌  █  ▐▛▚▖▐▌▐▌ ▐▌▐▌ ▐▌  █  
//  ▐▛▀▘   █  ▐▌ ▝▜▌▐▌ ▐▌▐▌ ▐▌  █  
//  ▐▌   ▗▄█▄▖▐▌  ▐▌▝▚▄▞▘▝▚▄▞▘  █  

#define TM1637_CLK 18             // CLK для TM1637
#define TM1637_DIO 19             // DIO для TM1637
#define TEMP_PIN 4                // DS18B20
#define GPIO_CONTROL 5            // Сигнал управления/реле
#define ENCODER_CLK 23            // Энкодер CLK
#define ENCODER_DT 25             // Энкодер DT
#define ENCODER_SW 26             // Энкодер SW
#define I2C_SDA 21                // OLED SDA
#define I2C_SCL 22                // OLED SCL

//   ▗▄▄▖ ▗▄▖ ▗▖  ▗▖▗▄▄▄▖ ▗▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖▗▄▄▄▖ ▗▄▄▖
//  ▐▌   ▐▌ ▐▌▐▛▚▖▐▌  █  ▐▌     █  ▐▌ ▐▌▐▛▚▖▐▌  █  ▐▌   
//  ▐▌   ▐▌ ▐▌▐▌ ▝▜▌  █   ▝▀▚▖  █  ▐▛▀▜▌▐▌ ▝▜▌  █   ▝▀▚▖
//  ▝▚▄▄▖▝▚▄▞▘▐▌  ▐▌  █  ▗▄▄▞▘  █  ▐▌ ▐▌▐▌  ▐▌  █  ▗▄▄▞▘

const int LINE_HEIGHT = 12;                             // Высота строки
const int TOP_PADDING = 5;                              // Отступ сверху
const int LEFT_PADDING = 5;                             // Отступ слева
const int menuItemsCount = 5;                           // Количество пунктов меню настроек
const float TEMP_HIGH_THRESHOLD = 75.0;                 // Порог высокой температуры (75°C)
const float TEMP_LOW_THRESHOLD = 50.0;                  // Порог низкой температуры (50°C)
const int maxSpeed = 20;                                // Максимальный шаг изменения времени (20 минут)
const char* CALIBRATION_KEY = "temp_cal";               // Ключ для хранения в EEPROM
const unsigned long OLED_DRAW_INTERVAL = 100;           // 10 FPS OLED
const unsigned long SENSOR_UPDATE_INTERVAL = 1000;      // 1 Hz
const unsigned long DISPLAY_UPDATE_INTERVAL = 2000;     // 2 секунды

//  ▗▖  ▗▖ ▗▄▖ ▗▄▄▖ ▗▄▄▄▖ ▗▄▖ ▗▄▄▖ ▗▖   ▗▄▄▄▖ ▗▄▄▖
//  ▐▌  ▐▌▐▌ ▐▌▐▌ ▐▌  █  ▐▌ ▐▌▐▌ ▐▌▐▌   ▐▌   ▐▌   
//  ▐▌  ▐▌▐▛▀▜▌▐▛▀▚▖  █  ▐▛▀▜▌▐▛▀▚▖▐▌   ▐▛▀▀▘ ▝▀▚▖
//   ▝▚▞▘ ▐▌ ▐▌▐▌ ▐▌▗▄█▄▖▐▌ ▐▌▐▙▄▞▘▐▙▄▄▖▐▙▄▄▖▗▄▄▞▘

int tempYear = 0;                       // Переменные для настройки времени RTC
int tempMonth = 0;                      // Переменные для настройки времени RTC
int tempDay = 0;                        // Переменные для настройки времени RTC
int tempHour = 0;                       // Переменные для настройки времени RTC
int tempMinute = 0;                     // Переменные для настройки времени RTC
int tempSecond = 0;                     // Переменные для настройки времени RTC
bool tempProtectionActive = false;      // Флаг, указывающий, что защита от перегрева активна
bool overheatStatus = false;            // Флаг перегрева
int currentDay = 0;                     // Текущий выбранный день недели
uint32_t tempStartTime = 0;             // Временное значение для времени старта
uint32_t tempEndTime = 0;               // Временное значение для времени окончания
unsigned long lastEncoderChange = 0;    // Время последнего изменения позиции энкодера
int encoderSpeed = 1;                   // Текущий шаг изменения времени (в минутах)
int menuIndex = 0;                      // Текущий выбранный пункт в меню.
int menuScroll = 0;                     // Смещение для прокрутки длинных списков меню.
bool gpioState = false;                 // Текущее состояние реле (GPIO5).
bool timeSynced = false;                // Флаг успешной синхронизации времени через NTP.
bool buttonPressed = false;             // Флаг нажатия кнопки энкодера.
unsigned long lastButtonPress = 0;      // Время последнего нажатия кнопки (в миллисекундах).
long oldEncoderPos = 0;                 // Предыдущее значение счётчика энкодера.
float calibrationOffset = 0.0;          // Величина калибровки
unsigned long lastOLEDDraw = 0;
static String lastDisplayData = "";  
static bool displayToggle = false;   
bool displayActiveMode = false;
unsigned long lastDisplayUpdate = 0;


//   ▗▄▖ ▗▄▄▖    ▗▖▗▄▄▄▖ ▗▄▄▖▗▄▄▄▖     ▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖▗▄▄▄▖
//  ▐▌ ▐▌▐▌ ▐▌   ▐▌▐▌   ▐▌     █         █  ▐▛▚▖▐▌  █    █  
//  ▐▌ ▐▌▐▛▀▚▖   ▐▌▐▛▀▀▘▐▌     █         █  ▐▌ ▝▜▌  █    █  
//  ▝▚▄▞▘▐▙▄▞▘▗▄▄▞▘▐▙▄▄▖▝▚▄▄▖  █       ▗▄█▄▖▐▌  ▐▌▗▄█▄▖  █  

WiFiUDP ntpUDP;                                       // Создание UDP-клиента для работы с NTP-протоколом.
NTPClient timeClient(ntpUDP, "pool.ntp.org", 36000);  // Инициализация NTP-клиента.
TM1637Display display(TM1637_CLK, TM1637_DIO);        // Создание объекта для управления 7-сегментным дисплеем TM1637.
RTC_DS3231 rtc;                                       // Инициализация модуля реального времени DS3231.
SSD1306Wire oled(0x3C, I2C_SDA, I2C_SCL);             // Настройка OLED-дисплея с разрешением 128x64.
OneWire oneWire(TEMP_PIN);                            // Создание шины 1-Wire для датчика температуры.
DallasTemperature sensors(&oneWire);                  // Инициализация драйвера для DS18B20.
ESP32Encoder encoder;                                 // Создание объекта для работы с энкодером.
WebServer server(80);                                 // Запуск веб-сервера на порту 80.
Preferences preferences;                              // Работа с энергонезависимой памятью (EEPROM).

//   ▗▄▄▖▗▄▄▄▖▗▄▄▖ ▗▖ ▗▖ ▗▄▄▖▗▄▄▄▖▗▖ ▗▖▗▄▄▖ ▗▄▄▄▖ ▗▄▄▖
//  ▐▌     █  ▐▌ ▐▌▐▌ ▐▌▐▌     █  ▐▌ ▐▌▐▌ ▐▌▐▌   ▐▌   
//   ▝▀▚▖  █  ▐▛▀▚▖▐▌ ▐▌▐▌     █  ▐▌ ▐▌▐▛▀▚▖▐▛▀▀▘ ▝▀▚▖
//  ▗▄▄▞▘  █  ▐▌ ▐▌▝▚▄▞▘▝▚▄▄▖  █  ▝▚▄▞▘▐▌ ▐▌▐▙▄▄▖▗▄▄▞▘

struct Schedule {   // Структура данных для записи времени старта и стопа управляющего сигнала
  uint32_t start;
  uint32_t end;
};

//  ▗▄▄▄▖▗▖  ▗▖▗▖ ▗▖▗▖  ▗▖      ▗▄▖ ▗▄▄▖ ▗▄▄▖  ▗▄▖ ▗▖  ▗▖ ▗▄▄▖     ▗▄▄▄▖▗▄▄▄▖ ▗▄▄▖
//  ▐▌   ▐▛▚▖▐▌▐▌ ▐▌▐▛▚▞▜▌     ▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌ ▝▚▞▘ ▐▌        ▐▌     █  ▐▌   
//  ▐▛▀▀▘▐▌ ▝▜▌▐▌ ▐▌▐▌  ▐▌     ▐▛▀▜▌▐▛▀▚▖▐▛▀▚▖▐▛▀▜▌  ▐▌   ▝▀▚▖     ▐▛▀▀▘  █  ▐▌   
//  ▐▙▄▄▖▐▌  ▐▌▝▚▄▞▘▐▌  ▐▌     ▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌  ▐▌  ▗▄▄▞▘     ▐▙▄▄▖  █  ▝▚▄▄▖

Schedule weeklySchedule[7];       // Массив структур для хранения расписания на неделю.

enum MenuState {                  // Список функциональных состояний меню
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
  SAVE_EXIT
};

const char* mainMenuItems[] = {   // Массив названий меню настроек
  "Настройки времени",
  "Расписание",
  "Температура",
  "Wi-Fi",
  "Сохранить настройки"
};

const char* daysOfWeek[] = {      // Массив дней недели для отображения на главном экране
  "ПН", 
  "ВТ", 
  "СР", 
  "ЧТ", 
  "ПТ", 
  "СБ", 
  "ВС"
};

//  ▗▄▄▄▖▗▖ ▗▖▗▖  ▗▖ ▗▄▄▖▗▄▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖ ▗▄▄▖     ▗▄▄▖ ▗▄▄▖  ▗▄▖ ▗▄▄▄▖ ▗▄▖ ▗▄▄▄▖▗▖  ▗▖▗▄▄▖ ▗▄▄▄▖ ▗▄▄▖
//  ▐▌   ▐▌ ▐▌▐▛▚▖▐▌▐▌     █    █  ▐▌ ▐▌▐▛▚▖▐▌▐▌        ▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌  █  ▐▌ ▐▌  █   ▝▚▞▘ ▐▌ ▐▌▐▌   ▐▌   
//  ▐▛▀▀▘▐▌ ▐▌▐▌ ▝▜▌▐▌     █    █  ▐▌ ▐▌▐▌ ▝▜▌ ▝▀▚▖     ▐▛▀▘ ▐▛▀▚▖▐▌ ▐▌  █  ▐▌ ▐▌  █    ▐▌  ▐▛▀▘ ▐▛▀▀▘ ▝▀▚▖
//  ▐▌   ▝▚▄▞▘▐▌  ▐▌▝▚▄▄▖  █  ▗▄█▄▖▝▚▄▞▘▐▌  ▐▌▗▄▄▞▘     ▐▌   ▐▌ ▐▌▝▚▄▞▘  █  ▝▚▄▞▘  █    ▐▌  ▐▌   ▐▙▄▄▖▗▄▄▞▘

void loadSchedule();                                        // Загружает расписание из энергонезависимой памяти (EEPROM).
void connectToWiFi(const char* ssid, const char* password); // Подключает ESP32 к Wi-Fi.
void handleRoot();                                          // Обработчик главной страницы веб-сервера.
void handleSetSchedule();                                   // Обрабатывает данные из веб-формы.
void checkSchedule(DateTime now);                           // Проверяет, должно ли реле быть включено по расписанию.
String formatTime(uint32_t seconds);                        // Конвертирует время в секундах в строку ЧЧ:ММ.
uint32_t parseTime(String timeStr);                         // Парсит строку времени ЧЧ:ММ в количество секунд.
void saveAndExit();                                         // Сохраняет настройки в EEPROM и возвращает в главное меню.
void handleLongPress();                                     // Обрабатывает длинное нажатие кнопки (>1 сек).
void handleShortPress();                                    // Обрабатывает короткое нажатие кнопки (<1 сек).
void updateMenu();                                          // Обновляет интерфейс на OLED в зависимости от состояния меню.
void drawMainMenu();                                        // Рисует главный экран устройства
void drawMenuNavigation();                                  // Отрисовывает список пунктов меню с подсветкой выбранного.
void drawTimeSetup();                                       // Интерфейс настройки времени RTC.
void drawScheduleSetup();                                   // Интерфейс настройки расписания.
void drawTempSetup();                                       // Экран калибровки датчика температуры.
void drawSaveExit();                                        // Сохраняет настройки и показывает сообщение об успешном сохранении настроек.
void handleEncoder();                                       // Обрабатывает вращение энкодера.
void handleButton();                                        // Обрабатывает нажатия кнопки энкодера.
void displayTime(uint32_t timeInSeconds);                   // Выводит время на 7-сегментный дисплей TM1637.
void displayTemperature(float temp);                        // Выводит температуру на TM1637.

MenuState currentMenu = MAIN_MENU;                          // Устанавливаем главное меню первым экраном

//  ▗▖  ▗▖ ▗▄▖ ▗▄▄▄▖▗▄▄▄        ▗▄▄▖▗▄▄▄▖▗▄▄▄▖▗▖ ▗▖▗▄▄▖ 
//  ▐▌  ▐▌▐▌ ▐▌  █  ▐▌  █      ▐▌   ▐▌     █  ▐▌ ▐▌▐▌ ▐▌
//  ▐▌  ▐▌▐▌ ▐▌  █  ▐▌  █       ▝▀▚▖▐▛▀▀▘  █  ▐▌ ▐▌▐▛▀▘ 
//   ▝▚▞▘ ▝▚▄▞▘▗▄█▄▖▐▙▄▄▀      ▗▄▄▞▘▐▙▄▄▖  █  ▝▚▄▞▘▐▌   

// Инициализация периферии и загрузка сохраненных настроек
void setup() {
  Serial.begin(115200);
  pinMode(ENCODER_SW, INPUT_PULLUP);

  // Инициализация энкодера с фильтром дребезга
  encoder.attachSingleEdge(ENCODER_CLK, ENCODER_DT);
  encoder.setFilter(1023); // Установка фильтра (значение от 0 до 1023)

  // Инициализация I2C
  Wire.begin(I2C_SDA, I2C_SCL);

  // Инициализация OLED
  if(!oled.init()) {
    Serial.println("Ошибка OLED!");
    while(1);
  }
  oled.flipScreenVertically();
  oled.setFont(ArialRus_Plain_10);
  oled.setFontTableLookupFunction(FontUtf8Rus);

  // Инициализация RTC
  if (!rtc.begin()) {
    showDisplayError("Ошибка модуля RTC!");
    while(1);
  }

  // Инициализация датчика температуры
  sensors.begin();
  if (sensors.getDeviceCount() == 0) { // Проверка наличия датчика
    showDisplayError("Ошибка датчика температуры!");
    while(1);
  }

  loadCalibrationOffset();

  // Загрузка расписания
  preferences.begin("schedule", false);
  loadSchedule();

  // Синхронизация времени
  if(WiFi.status() == WL_CONNECTED) {
    timeClient.begin();
    if(timeClient.update()) {
      rtc.adjust(DateTime(timeClient.getEpochTime()));
      timeSynced = true;
    }
  }

  // Инициализация TM1637
  display.setBrightness(7); // Яркость дисплея (0-7)

  // Загрузка калибровки после инициализации preferences
  calibrationOffset = preferences.getFloat(CALIBRATION_KEY, 0.0);

}

//  ▗▖  ▗▖ ▗▄▖ ▗▄▄▄▖▗▄▄▄       ▗▖    ▗▄▖  ▗▄▖ ▗▄▄▖ 
//  ▐▌  ▐▌▐▌ ▐▌  █  ▐▌  █      ▐▌   ▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌
//  ▐▌  ▐▌▐▌ ▐▌  █  ▐▌  █      ▐▌   ▐▌ ▐▌▐▌ ▐▌▐▛▀▘ 
//   ▝▚▞▘ ▝▚▄▞▘▗▄█▄▖▐▙▄▄▀      ▐▙▄▄▖▝▚▄▞▘▝▚▄▞▘▐▌   

void loop() {
  server.handleClient();
  static unsigned long lastSensorUpdate = 0;
  static unsigned long lastOLEDUpdate = 0;
  DateTime now = rtc.now();

  // Обновление данных датчиков
  if (millis() - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
    lastSensorUpdate = millis();
    
    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0) + calibrationOffset;
    checkSchedule(now);

    // Обновление TM1637
    updateTM1637Display();
  }
  // Обновление OLED
  if (millis() - lastOLEDUpdate >= OLED_DRAW_INTERVAL) {
    lastOLEDUpdate = millis();
    
    if(currentMenu == MAIN_MENU) {
      drawMainMenu();
    } else {
      updateMenu();
    }
  }

  handleEncoder();
  handleButton();
}

//   ▗▄▖ ▗▖   ▗▄▄▄▖▗▄▄▄       ▗▄▄▄▖▗▖ ▗▖▗▖  ▗▖ ▗▄▄▖▗▄▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖ ▗▄▄▖
//  ▐▌ ▐▌▐▌   ▐▌   ▐▌  █      ▐▌   ▐▌ ▐▌▐▛▚▖▐▌▐▌     █    █  ▐▌ ▐▌▐▛▚▖▐▌▐▌   
//  ▐▌ ▐▌▐▌   ▐▛▀▀▘▐▌  █      ▐▛▀▀▘▐▌ ▐▌▐▌ ▝▜▌▐▌     █    █  ▐▌ ▐▌▐▌ ▝▜▌ ▝▀▚▖
//  ▝▚▄▞▘▐▙▄▄▖▐▙▄▄▖▐▙▄▄▀      ▐▌   ▝▚▄▞▘▐▌  ▐▌▝▚▄▄▖  █  ▗▄█▄▖▝▚▄▞▘▐▌  ▐▌▗▄▄▞▘

void showDisplayError(const char* msg) {
  oled.clear();
  oled.setFont(ArialRus_Plain_10);
  oled.drawString(0, 0, msg);
  oled.display();
}

void drawMainMenu() {
  DateTime now = rtc.now();
  char timeStr[30];
  sprintf(timeStr, "%s  %02d:%02d:%02d",
          daysOfWeek[(now.dayOfTheWeek() + 6) % 7],
          now.hour(),
          now.minute(),
          now.second());

  float temp = sensors.getTempCByIndex(0) + calibrationOffset;
  String currentData = String(timeStr) + String(temp, 1) + 
                      String(gpioState) + String(overheatStatus);

  if(currentData != lastDisplayData) {
    oled.clear();
    oled.setFont(ArialRus_Plain_10);

    // Время
    oled.drawString(LEFT_PADDING, TOP_PADDING, timeStr);

    // Температура
    char tempStr[30];
    sprintf(tempStr, "%+.1fC (%+.1f)", temp, calibrationOffset);
    oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, tempStr);

    // Состояние
    String stateLine = String(gpioState ? "В РАБОТЕ" : "ОЖИДАНИЕ") +
                      String(overheatStatus ? " / ПЕРЕГРЕВ!" : " / Штатное");
    oled.drawString(LEFT_PADDING, TOP_PADDING + 2*LINE_HEIGHT, stateLine);

    oled.display();
    lastDisplayData = currentData;
  }
}
void drawMenuNavigation() {
  oled.clear();
  oled.setFont(ArialRus_Plain_10);
  oled.drawString(LEFT_PADDING, TOP_PADDING, "== Меню ==");

  for (int i = 0; i < 6; i++) {
    int itemIndex = menuScroll + i;
    if (itemIndex >= menuItemsCount) break;

    String itemText = mainMenuItems[itemIndex];
    if (itemIndex == menuIndex) {
      itemText = "> " + itemText;
    }

    oled.drawString(LEFT_PADDING, TOP_PADDING + (i + 1) * LINE_HEIGHT, itemText);
  }

  // Курсор
  int scrollbarHeight = 50;
  int scrollbarPos = map(menuIndex, 0, menuItemsCount - 1, 0, scrollbarHeight);
  oled.drawVerticalLine(122, TOP_PADDING, scrollbarHeight);
  oled.fillRect(120, TOP_PADDING + scrollbarPos, 4, 4);

  oled.display();
}

void drawTimeSetup() {
  oled.clear();
  oled.setFont(ArialRus_Plain_10);

  // Заголовок
  oled.drawString(LEFT_PADDING, TOP_PADDING, "Настройка времени:");

  // Текущее время
  DateTime now = rtc.now();
  char dateStr[30];
  sprintf(dateStr, "%04d-%02d-%02d", now.year(), now.month(), now.day());
  oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, dateStr);

  char timeStr[30];
  sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  oled.drawString(LEFT_PADDING, TOP_PADDING + 2 * LINE_HEIGHT, timeStr);

  // Подсказка
  oled.drawString(LEFT_PADDING, TOP_PADDING + 4 * LINE_HEIGHT, "Нажмите для настройки");

  oled.display();
}

void drawTimeSetupStep() {
  oled.clear();
  oled.setFont(ArialRus_Plain_10);

  // Заголовок
  oled.drawString(LEFT_PADDING, TOP_PADDING, "Настройка времени:");

  // Отображение текущего этапа
  switch (currentMenu) {
    case TIME_SETUP_YEAR:
      oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, "> Год: " + String(tempYear));
      oled.drawString(LEFT_PADDING, TOP_PADDING + 2 * LINE_HEIGHT, "Месяц: " + String(tempMonth));
      oled.drawString(LEFT_PADDING, TOP_PADDING + 3 * LINE_HEIGHT, "День: " + String(tempDay));
      break;
    case TIME_SETUP_MONTH:
      oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, "Год: " + String(tempYear));
      oled.drawString(LEFT_PADDING, TOP_PADDING + 2 * LINE_HEIGHT, "> Месяц: " + String(tempMonth));
      oled.drawString(LEFT_PADDING, TOP_PADDING + 3 * LINE_HEIGHT, "День: " + String(tempDay));
      break;
    case TIME_SETUP_DAY:
      oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, "Год: " + String(tempYear));
      oled.drawString(LEFT_PADDING, TOP_PADDING + 2 * LINE_HEIGHT, "Месяц: " + String(tempMonth));
      oled.drawString(LEFT_PADDING, TOP_PADDING + 3 * LINE_HEIGHT, "> День: " + String(tempDay));
      break;
    case TIME_SETUP_HOUR:
      oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, "> Часы: " + String(tempHour));
      oled.drawString(LEFT_PADDING, TOP_PADDING + 2 * LINE_HEIGHT, "Минуты: " + String(tempMinute));
      oled.drawString(LEFT_PADDING, TOP_PADDING + 3 * LINE_HEIGHT, "Секунды: " + String(tempSecond));
      break;
    case TIME_SETUP_MINUTE:
      oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, "Часы: " + String(tempHour));
      oled.drawString(LEFT_PADDING, TOP_PADDING + 2 * LINE_HEIGHT, "> Минуты: " + String(tempMinute));
      oled.drawString(LEFT_PADDING, TOP_PADDING + 3 * LINE_HEIGHT, "Секунды: " + String(tempSecond));
      break;
    case TIME_SETUP_SECOND:
      oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, "Часы: " + String(tempHour));
      oled.drawString(LEFT_PADDING, TOP_PADDING + 2 * LINE_HEIGHT, "Минуты: " + String(tempMinute));
      oled.drawString(LEFT_PADDING, TOP_PADDING + 3 * LINE_HEIGHT, "> Секунды: " + String(tempSecond));
      break;
  }

  // Подсказка
  oled.drawString(LEFT_PADDING, TOP_PADDING + 5 * LINE_HEIGHT, "Крути = изменить");
  oled.drawString(LEFT_PADDING, TOP_PADDING + 6 * LINE_HEIGHT, "Зажми = сохранить");

  oled.display();
}

void drawScheduleSetup() {
  oled.clear();
  oled.setFont(ArialRus_Plain_10);

  // Заголовок
  oled.drawString(LEFT_PADDING, TOP_PADDING, "Настройка расписания");

  // Текущий день
  const char* days[] = {"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота", "Воскресенье"};
  oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, "День: " + String(days[currentDay]));

  // Время включения
  String startText = "Старт: " + formatTime(tempStartTime);
  if (currentMenu == SCHEDULE_START_SELECT) {
    startText = "> " + startText; // Подсветка строки Start
  }
  oled.drawString(LEFT_PADDING, TOP_PADDING + 2 * LINE_HEIGHT, startText);

  // Время выключения
  String endText = "Стоп: " + formatTime(tempEndTime);
  if (currentMenu == SCHEDULE_END_SELECT) {
    endText = "> " + endText; // Подсветка строки End
  }
  oled.drawString(LEFT_PADDING, TOP_PADDING + 3 * LINE_HEIGHT, endText);

  // Отображение текущего шага перемотки
  String speedText = "Шаг: " + String(encoderSpeed) + " мин";
  oled.drawString(LEFT_PADDING, TOP_PADDING + 4 * LINE_HEIGHT, speedText);

  // Подсказка
  oled.drawString(LEFT_PADDING, TOP_PADDING + 5 * LINE_HEIGHT, "OK       Отмена");

  oled.display();
}

void drawTempSetup() {
  oled.clear();
  oled.setFont(ArialRus_Plain_10);

  // Заголовок
  oled.drawString(LEFT_PADDING, TOP_PADDING, "Калибровка температуры:");

  // Текущая температура с калибровкой
  float rawTemp = sensors.getTempCByIndex(0);
  float calibratedTemp = rawTemp + calibrationOffset;
  
  char tempStr[40];
  sprintf(tempStr, "Сырая: %+.1fC", rawTemp);
  oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, tempStr);

  sprintf(tempStr, "Калибр: %+.1fC", calibratedTemp);
  oled.drawString(LEFT_PADDING, TOP_PADDING + 2*LINE_HEIGHT, tempStr);

  // Подстроечное значение
  sprintf(tempStr, "Смещение: %+.1fC", calibrationOffset);
  oled.drawString(LEFT_PADDING, TOP_PADDING + 4*LINE_HEIGHT, tempStr);

  // Подсказки управления
  oled.drawString(LEFT_PADDING, TOP_PADDING + 6*LINE_HEIGHT, "Вращай - изменить");
  oled.drawString(80, TOP_PADDING + 6*LINE_HEIGHT, "Удерж - сохранить");

  oled.display();
}

void drawSaveExit() {
  oled.clear();
  oled.setFont(ArialRus_Plain_10);

  // Заголовок
  oled.drawString(LEFT_PADDING, TOP_PADDING, "Сохранение настроек");

  // Подсказка
  oled.drawString(LEFT_PADDING, TOP_PADDING + LINE_HEIGHT, "Настройки сохранены!");
  oled.drawString(LEFT_PADDING, TOP_PADDING + 2 * LINE_HEIGHT, "Возвращаемся в главное меню");

  oled.display();
}

void showSaveMessage() {
  oled.clear();
  oled.setFont(ArialRus_Plain_10);
  oled.drawString(LEFT_PADDING, TOP_PADDING, "Настройки сохранены!");
  oled.display();
  delay(1000); // Показываем сообщение 1 секунду
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


//  ▗▄▄▄▖▗▖  ▗▖█ ▄▄▄▄ ▄▄▄▄ ▗▄▄▄▖     ▗▄▄▄▖▗▖ ▗▖▗▖  ▗▖ ▗▄▄▖▗▄▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖ ▗▄▄▖
//    █  ▐▛▚▞▜▌█ █       █    ▐▌     ▐▌   ▐▌ ▐▌▐▛▚▖▐▌▐▌     █    █  ▐▌ ▐▌▐▛▚▖▐▌▐▌   
//    █  ▐▌  ▐▌█ █▀▀█ ▀▀▀█    ▐▌     ▐▛▀▀▘▐▌ ▐▌▐▌ ▝▜▌▐▌     █    █  ▐▌ ▐▌▐▌ ▝▜▌ ▝▀▚▖
//    █  ▐▌  ▐▌█ █▄▄█ ▄▄▄█    ▐▌     ▐▌   ▝▚▄▞▘▐▌  ▐▌▝▚▄▄▖  █  ▗▄█▄▖▝▚▄▞▘▐▌  ▐▌▗▄▄▞▘

void displayTime(uint32_t timeInSeconds) {
    if(timeInSeconds == 0) {
        // Отображаем "--:--"
        uint8_t segments[] = {0x40, 0x40, 0x40, 0x40};
        display.setSegments(segments);
        return;
    }
    
    uint8_t hours = timeInSeconds / 3600;
    uint8_t minutes = (timeInSeconds % 3600) / 60;
    display.showNumberDecEx(hours * 100 + minutes, 0b01000000, true);
}

// Добавим функцию для преобразования цифры в код сегментов
uint8_t encodeDigit(int digit) {
  return display.encodeDigit(digit);
}

void displayTemperature(float temp) {
    if(temp == DEVICE_DISCONNECTED_C) {
        // Отображаем "Err"
        uint8_t segments[] = {0x79, 0x50, 0x50, 0x00};
        display.setSegments(segments);
        return;
    }
    
    int16_t tempInt = round(temp * 10);
    bool negative = tempInt < 0;
    tempInt = abs(tempInt);
    
    uint8_t data[4] = {
        negative ? 0x40 : 0x00,
        display.encodeDigit(tempInt / 100),
        display.encodeDigit((tempInt / 10) % 10),
        0x63 | (tempInt % 10 << 4)
    };
    
    if(tempInt < 100) data[1] = 0x00;
    display.setSegments(data);
}

void updateTM1637Display() {
    static bool showTemp = true;
    
    if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        lastDisplayUpdate = millis();
        
        if (gpioState) {
            if(showTemp) {
                displayTemperature(sensors.getTempCByIndex(0) + calibrationOffset);
            } else {
                displayTime(getCurrentDayEndTime(rtc.now()));
            }
            showTemp = !showTemp;
        } else {
            displayTime(getNextStartTime(rtc.now()));
            showTemp = true;
        }
    }
}

//  ▗▖  ▗▖▗▄▄▄▖▗▖  ▗▖▗▖ ▗▖
//  ▐▛▚▞▜▌▐▌   ▐▛▚▖▐▌▐▌ ▐▌
//  ▐▌  ▐▌▐▛▀▀▘▐▌ ▝▜▌▐▌ ▐▌
//  ▐▌  ▐▌▐▙▄▄▖▐▌  ▐▌▝▚▄▞▘

void updateMenu() {
  
  switch (currentMenu) {
    case MAIN_MENU:
      drawMainMenu();
      break;

    case MENU_NAVIGATION:
      drawMenuNavigation();
      break;

    case TIME_SETUP:
      drawTimeSetup();
      break;

    case TIME_SETUP_YEAR:
    case TIME_SETUP_MONTH:
    case TIME_SETUP_DAY:
    case TIME_SETUP_HOUR:
    case TIME_SETUP_MINUTE:
    case TIME_SETUP_SECOND:
      drawTimeSetupStep();
      break;

    case SCHEDULE_SETUP:
    case SCHEDULE_DAY_SELECT:
    case SCHEDULE_START_SELECT:
    case SCHEDULE_END_SELECT:
      drawScheduleSetup();
      break;

    case TEMP_SETUP:
      drawTempSetup();
      break;

    case SAVE_EXIT:
      drawSaveExit();
      break;

    default:
      // Если состояние меню неизвестно, возвращаемся в главное меню
      currentMenu = MAIN_MENU;
      drawMainMenu();
      break;
  }
}

//  ▗▖ ▗▖ ▗▄▖ ▗▖  ▗▖▗▄▄▄  ▗▖   ▗▄▄▄▖     ▗▄▄▄▖▗▖  ▗▖ ▗▄▄▖ ▗▄▖ ▗▄▄▄  ▗▄▄▄▖▗▄▄▖ 
//  ▐▌ ▐▌▐▌ ▐▌▐▛▚▖▐▌▐▌  █ ▐▌   ▐▌        ▐▌   ▐▛▚▖▐▌▐▌   ▐▌ ▐▌▐▌  █ ▐▌   ▐▌ ▐▌
//  ▐▛▀▜▌▐▛▀▜▌▐▌ ▝▜▌▐▌  █ ▐▌   ▐▛▀▀▘     ▐▛▀▀▘▐▌ ▝▜▌▐▌   ▐▌ ▐▌▐▌  █ ▐▛▀▀▘▐▛▀▚▖
//  ▐▌ ▐▌▐▌ ▐▌▐▌  ▐▌▐▙▄▄▀ ▐▙▄▄▖▐▙▄▄▖     ▐▙▄▄▖▐▌  ▐▌▝▚▄▄▖▝▚▄▞▘▐▙▄▄▀ ▐▙▄▄▖▐▌ ▐▌

void handleEncoder() {
  static unsigned long lastReadTime = 0;
  const unsigned long debounceDelay = 50; // Задержка в миллисекундах

  if (currentMenu != MAIN_MENU) { // Только если НЕ в главном меню
    if (millis() - lastReadTime < debounceDelay) return;

    long newPos = encoder.getCount();
    static long oldPos = -1;

    if (oldPos == -1) {
      oldPos = newPos;
      return;
    }

    if (newPos != oldPos) {
      int delta = (newPos > oldPos) ? 1 : -1;

      // Определяем скорость вращения энкодера
      unsigned long currentTime = millis();
      if (currentTime - lastEncoderChange < 200) { // Если вращение быстрое
        encoderSpeed = constrain(encoderSpeed + 1, 1, maxSpeed); // Увеличиваем шаг
      } else {
        encoderSpeed = 1; // Сбрасываем шаг, если вращение медленное
      }
      lastEncoderChange = currentTime;

      // Обработка изменения позиции энкодера
      switch (currentMenu) {
        case TIME_SETUP_YEAR:
          tempYear = constrain(tempYear + delta * encoderSpeed, 2000, 2099);
          break;

        case TIME_SETUP_MONTH:
          tempMonth = constrain(tempMonth + delta * encoderSpeed, 1, 12);
          break;

        case TIME_SETUP_DAY:
          tempDay = constrain(tempDay + delta * encoderSpeed, 1, 31);
          break;

        case TIME_SETUP_HOUR:
          tempHour = constrain(tempHour + delta * encoderSpeed, 0, 23);
          break;

        case TIME_SETUP_MINUTE:
          tempMinute = constrain(tempMinute + delta * encoderSpeed, 0, 59);
          break;

        case TIME_SETUP_SECOND:
          tempSecond = constrain(tempSecond + delta * encoderSpeed, 0, 59);
          break;

        case MENU_NAVIGATION:
          menuIndex = constrain(menuIndex + delta, 0, menuItemsCount - 1);
          if (menuIndex < menuScroll) {
            menuScroll = menuIndex;
          } else if (menuIndex >= menuScroll + 4) {
            menuScroll = menuIndex - 3;
          }
          menuScroll = constrain(menuScroll, 0, menuItemsCount - 4);
          break;

        case SCHEDULE_DAY_SELECT:
          currentDay = (currentDay + delta + 7) % 7;
          break;

        case SCHEDULE_START_SELECT:
          tempStartTime = (tempStartTime + delta * encoderSpeed * 60) % 86400;
          break;

        case SCHEDULE_END_SELECT:
          tempEndTime = (tempEndTime + delta * encoderSpeed * 60) % 86400;
          break;

        case TEMP_SETUP:
          if (newPos != oldPos) {
            int delta = (newPos > oldPos) ? 1 : -1;
            calibrationOffset += delta * 0.1; // Шаг 0.1°C
            calibrationOffset = constrain(calibrationOffset, -10.0, 10.0); // Ограничение ±10°C
            oldPos = newPos;
            updateMenu();
          }
          break;

        default:
          break;
      }

      oldPos = newPos;
      lastReadTime = millis();
    }
  }
}

//  ▗▖ ▗▖ ▗▄▖ ▗▖  ▗▖▗▄▄▄  ▗▖   ▗▄▄▄▖     ▗▄▄▖ ▗▖ ▗▖▗▄▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖
//  ▐▌ ▐▌▐▌ ▐▌▐▛▚▖▐▌▐▌  █ ▐▌   ▐▌        ▐▌ ▐▌▐▌ ▐▌  █    █  ▐▌ ▐▌▐▛▚▖▐▌
//  ▐▛▀▜▌▐▛▀▜▌▐▌ ▝▜▌▐▌  █ ▐▌   ▐▛▀▀▘     ▐▛▀▚▖▐▌ ▐▌  █    █  ▐▌ ▐▌▐▌ ▝▜▌
//  ▐▌ ▐▌▐▌ ▐▌▐▌  ▐▌▐▙▄▄▀ ▐▙▄▄▖▐▙▄▄▖     ▐▙▄▞▘▝▚▄▞▘  █    █  ▝▚▄▞▘▐▌  ▐▌

void handleButton() {
  int btnState = digitalRead(ENCODER_SW);

  if (btnState == LOW && !buttonPressed) {
    buttonPressed = true;
    lastButtonPress = millis();
  }

  if (btnState == HIGH && buttonPressed) {
    buttonPressed = false;
    unsigned long duration = millis() - lastButtonPress;

    if (duration < 50) return; // Игнорируем дребезг

    if (currentMenu == MAIN_MENU) {
      currentMenu = MENU_NAVIGATION;
      oled.clear(); // Очистка экрана перед переходом
      drawMenuNavigation();
    } else {
      if (duration < 1000) {
        handleShortPress();
      } else {
        handleLongPress();
      }
    }
    lastOLEDDraw = 0;
    lastDisplayData = "";
  }
}

//  ▗▖ ▗▖ ▗▄▖ ▗▖  ▗▖▗▄▄▄  ▗▖   ▗▄▄▄▖      ▗▄▄▖▗▖ ▗▖ ▗▄▖ ▗▄▄▖ ▗▄▄▄▖     ▗▄▄▖ ▗▄▄▖ ▗▄▄▄▖ ▗▄▄▖ ▗▄▄▖
//  ▐▌ ▐▌▐▌ ▐▌▐▛▚▖▐▌▐▌  █ ▐▌   ▐▌        ▐▌   ▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌  █       ▐▌ ▐▌▐▌ ▐▌▐▌   ▐▌   ▐▌   
//  ▐▛▀▜▌▐▛▀▜▌▐▌ ▝▜▌▐▌  █ ▐▌   ▐▛▀▀▘      ▝▀▚▖▐▛▀▜▌▐▌ ▐▌▐▛▀▚▖  █       ▐▛▀▘ ▐▛▀▚▖▐▛▀▀▘ ▝▀▚▖ ▝▀▚▖
//  ▐▌ ▐▌▐▌ ▐▌▐▌  ▐▌▐▙▄▄▀ ▐▙▄▄▖▐▙▄▄▖     ▗▄▄▞▘▐▌ ▐▌▝▚▄▞▘▐▌ ▐▌  █       ▐▌   ▐▌ ▐▌▐▙▄▄▖▗▄▄▞▘▗▄▄▞▘

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

//  ▗▖ ▗▖ ▗▄▖ ▗▖  ▗▖▗▄▄▄  ▗▖   ▗▄▄▄▖     ▗▖    ▗▄▖ ▗▖  ▗▖ ▗▄▄▖     ▗▄▄▖ ▗▄▄▖ ▗▄▄▄▖ ▗▄▄▖ ▗▄▄▖
//  ▐▌ ▐▌▐▌ ▐▌▐▛▚▖▐▌▐▌  █ ▐▌   ▐▌        ▐▌   ▐▌ ▐▌▐▛▚▖▐▌▐▌        ▐▌ ▐▌▐▌ ▐▌▐▌   ▐▌   ▐▌   
//  ▐▛▀▜▌▐▛▀▜▌▐▌ ▝▜▌▐▌  █ ▐▌   ▐▛▀▀▘     ▐▌   ▐▌ ▐▌▐▌ ▝▜▌▐▌▝▜▌     ▐▛▀▘ ▐▛▀▚▖▐▛▀▀▘ ▝▀▚▖ ▝▀▚▖
//  ▐▌ ▐▌▐▌ ▐▌▐▌  ▐▌▐▙▄▄▀ ▐▙▄▄▖▐▙▄▄▖     ▐▙▄▄▖▝▚▄▞▘▐▌  ▐▌▝▚▄▞▘     ▐▌   ▐▌ ▐▌▐▙▄▄▖▗▄▄▞▘▗▄▄▞▘

void handleLongPress() {
  if (currentMenu >= TIME_SETUP_YEAR && currentMenu <= TIME_SETUP_SECOND) {
    // Сохраняем настройки времени
    rtc.adjust(DateTime(tempYear, tempMonth, tempDay, tempHour, tempMinute, tempSecond));
    showSaveMessage();
    currentMenu = MAIN_MENU;
    drawMainMenu();
  }

  if (currentMenu == TEMP_SETUP) {
    preferences.putFloat(CALIBRATION_KEY, calibrationOffset);
    showSaveMessage();
    currentMenu = MAIN_MENU;
    drawMainMenu();
  }
}

//   ▗▄▖ ▗▄▄▄▖▗▖ ▗▖▗▄▄▄▖▗▄▄▖      ▗▖  ▗▖▗▄▄▄▖▗▄▄▄▖▗▖ ▗▖ ▗▄▖ ▗▄▄▄   ▗▄▄▖
//  ▐▌ ▐▌  █  ▐▌ ▐▌▐▌   ▐▌ ▐▌     ▐▛▚▞▜▌▐▌     █  ▐▌ ▐▌▐▌ ▐▌▐▌  █ ▐▌   
//  ▐▌ ▐▌  █  ▐▛▀▜▌▐▛▀▀▘▐▛▀▚▖     ▐▌  ▐▌▐▛▀▀▘  █  ▐▛▀▜▌▐▌ ▐▌▐▌  █  ▝▀▚▖
//  ▝▚▄▞▘  █  ▐▌ ▐▌▐▙▄▄▖▐▌ ▐▌     ▐▌  ▐▌▐▙▄▄▖  █  ▐▌ ▐▌▝▚▄▞▘▐▙▄▄▀ ▗▄▄▞▘

void checkTemperatureProtection(float temp) {
  float calibratedTemp = temp + calibrationOffset;

  if (calibratedTemp >= TEMP_HIGH_THRESHOLD) {
    digitalWrite(GPIO_CONTROL, LOW);
    gpioState = false;
    tempProtectionActive = true;
    overheatStatus = true; // Активируем статус перегрева
  } else if (calibratedTemp <= TEMP_LOW_THRESHOLD && tempProtectionActive) {
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

void connectToWiFi(const char* ssid, const char* password) {
  WiFi.disconnect();
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

void loadCalibrationOffset() {
  calibrationOffset = preferences.getFloat(CALIBRATION_KEY, 0.0);
}


void saveCalibrationOffset() {
  preferences.putFloat(CALIBRATION_KEY, calibrationOffset);
}

uint32_t getCurrentDayEndTime(DateTime now) {
    uint8_t currentDay = (now.dayOfTheWeek() + 6) % 7;
    return weeklySchedule[currentDay].end;
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
