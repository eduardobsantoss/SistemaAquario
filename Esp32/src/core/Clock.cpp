#include "core/Clock.h"
#include <time.h>
uint64_t epoch_ms(){
  time_t s = time(nullptr);
  if (s > 100000) return (uint64_t)s * 1000ULL;
  return (uint64_t)millis();
}
