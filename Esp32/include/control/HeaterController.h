#pragma once
#include <Arduino.h>
class HeaterController {
public:
  void begin(uint8_t relayPin, bool activeLow);
  bool update(float tC, float sp, float hyst, float minSafe, float maxSafe, unsigned long now, unsigned long minSwitchMs, bool& outStateChanged);
  bool isOn() const { return state; }
  void forceOff();
private:
  uint8_t pin=255; bool activeLow=true; bool state=false; unsigned long lastSwitch=0;
  void write(bool on);
};
