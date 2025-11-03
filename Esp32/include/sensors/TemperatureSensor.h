#pragma once
#include <Arduino.h>
class TemperatureSensor {
public:
  void begin(uint8_t oneWirePin);
  float readCelsius();
  float latest() const { return lastC; }
private:
  float lastC = NAN;
};
