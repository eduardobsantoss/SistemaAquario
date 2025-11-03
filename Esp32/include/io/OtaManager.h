#pragma once
#include <Arduino.h>
class OtaManager {
public:
  void begin(const char* hostname, uint16_t port);
  void handle();
};
