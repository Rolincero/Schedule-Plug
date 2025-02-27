# Умная розетка с температурным контролем и расписанием

Управляемая розетка на базе ESP32 с функциями:
- Работа по расписанию
- Защита от перегрева
- Визуализация данных на OLED и 7-сегментном дисплее
- Ручная настройка через энкодер
- Веб-интерфейс управления

## Особенности
✅ **Таймер работы** с индивидуальными настройками для каждого дня недели  
🌡️ **Температурная защита** (отключение при 75°C, включение при 50°C)  
📅 **Встроенные часы** с синхронизацией по NTP и модулем RTC DS3231  
📶 **Веб-интерфейс** для удалённой настройки  
🔧 **Ручная настройка** через энкодер с OLED-меню  
⏲️ Индикация времени и температуры на TM1637  
🔌 Управление нагрузкой до 10А через реле

## Технические характеристики
- Микроконтроллер: ESP32
- Модуль времени: DS3231
- Датчик температуры: DS18B20
- Дисплей: OLED 128x64 (SSD1306) + TM1637
- Элементы управления: Энкодер с кнопкой
- Защита: Автоматическое отключение при перегреве

## Подключение компонентов
| Компонент       | Контакт ESP32 |
|-----------------|---------------|
| TM1637 CLK      | GPIO18        |
| TM1637 DIO      | GPIO19        |
| DS18B20         | GPIO4         |
| Реле управления | GPIO5         |
| Энкодер CLK     | GPIO23        |
| Энкодер DT      | GPIO25        |
| Энкодер SW      | GPIO26        |
| OLED SDA        | GPIO21        |
| OLED SCL        | GPIO22        |

## Настройка и использование
1. **Главное меню**:

=======
   - Статус работы
   - Текущее время и дата
   - Температура нагрузки
   - Состояние WiFi

2. **Меню настроек** (активируется нажатием энкодера):
   - Настройка времени RTC
   - Программирование расписания
   - Калибровка температуры
   - Настройка WiFi
   - Сохранение параметров

3. **Веб-интерфейс**:
   - Доступен по адресу `http://[IP-адрес]/`
   - Настройка расписания в формате ЧЧ:ММ
   - Просмотр текущего состояния

## Установка и сборка
1. Установите необходимые библиотеки:
   - RTClib
   - OneWire
   - DallasTemperature
   - WebServer
   - NTPClient
   - SSD1306Wire
   - ESP32Encoder
   - TM1637

2. Соберите схему согласно распиновке

3. Загрузите прошивку через Arduino IDE или PlatformIO

4. Настройте параметры через:
   - Встроенное меню (энкодер + OLED)
   - Веб-интерфейс
   - Прямое редактирование кода

## Безопасность
⚠️ Устройство автоматически отключает нагрузку при:
- Превышении температуры 75°C
- Обрыве датчика температуры
- Истечении времени работы по расписанию

## Лицензия
MIT License. Полный текст доступен в файле LICENSE.
