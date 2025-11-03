#pragma once

// ===== Identificação / OTA =====
#define HOSTNAME_DEFAULT "aquario-esp32-devkitc"
#define OTA_PORT_DEFAULT 3232

// ===== I2C LCD =====
#define I2C_SDA 16
#define I2C_SCL 17

// ===== Botão LCD =====
#define PIN_BTN 21
#define BTN_DEBOUNCE_MS 50

// ===== Sensores =====
#define ONE_WIRE_BUS 4
#define PH_ADC_PIN   34

// ===== Relés =====
#define PIN_RELAY_HEATER    23
#define RELAY_ACTIVE_LOW    1   // 1 = LOW liga

#define PIN_FLOAT_SWITCH    18
#define PIN_RELAY_WATERFALL 22
#define ACTIVE_HIGH         1

// ===== Buzzer =====
#define PIN_BUZZER 19
#define BUZZER_ENABLE 1
#define BUZZER_ACTIVE_LEVEL HIGH
#define BUZZER_ON_MS 120
#define BUZZER_OFF_MS 380
#define BUZZER_GROUP_PAUSE_MS 2000

// ===== Alimentador (28BYJ-48 + ULN2003) =====
#define FEED_IN1 25
#define FEED_IN2 26
#define FEED_IN3 27
#define FEED_IN4 33
