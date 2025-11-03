#pragma once
#include <Arduino.h>
class PhSensor {
public:
  void begin(uint8_t adcPin);
  float readPH();
  float latest() const { return lastPH; }
private:
  uint8_t pin = 255;
  bool init = false;
  float emaV = 0.0f;
  float lastPH = NAN;
};
