#pragma once
#include <Arduino.h>
#include <time.h>

namespace Clock {
  inline uint64_t epochMillis() {
    time_t s = time(nullptr);
    if (s > 100000) return (uint64_t)s * 1000ULL;
    return (uint64_t)millis();
  }
}
