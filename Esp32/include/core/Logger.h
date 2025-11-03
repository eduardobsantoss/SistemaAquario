#pragma once
#include <Arduino.h>
#include <stdarg.h>

struct Logger {
  static void begin() { Serial.begin(115200); delay(200); }

  static void info(const String& m) {
#if LOG_HEARTBEAT
    Serial.println(m);
#endif
  }

  static void printf(const char* fmt, ...) {
#if LOG_HEARTBEAT
    va_list args; va_start(args, fmt);
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.print(buf);
#endif
  }
};
