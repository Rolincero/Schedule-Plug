#pragma once

// GPIO Definitions
#define TM1637_CLK      18
#define TM1637_DIO      19
#define TEMP_PIN        4
#define GPIO_CONTROL    5
#define ENCODER_CLK     23
#define ENCODER_DT      25
#define ENCODER_SW      26
#define I2C_SDA         21
#define I2C_SCL         22

// Константы
const float TEMP_HIGH_THRESHOLD = 75.0;
const float TEMP_LOW_THRESHOLD = 50.0;
const unsigned long SENSOR_UPDATE_INTERVAL = 1000;