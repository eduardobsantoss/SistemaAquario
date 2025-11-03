#pragma once
#include <Arduino.h>
class Buzzer {
public:
  void begin(uint8_t pin, uint8_t activeLevel);
  void handleWaterLow(bool enabled);
private:
  uint8_t _pin=255,_active=HIGH; uint8_t step=0; unsigned long t0=0;
  void on(){ digitalWrite(_pin,_active); }
  void off(){ digitalWrite(_pin, !_active); }
};
