#include "actuators/Buzzer.h"
#include "config/pins.h"
void Buzzer::begin(uint8_t pin, uint8_t active){ _pin=pin; _active=active; pinMode(_pin,OUTPUT); off(); }
void Buzzer::handleWaterLow(bool enabled){
#if BUZZER_ENABLE
  if (!enabled){ off(); step=0; return; }
  unsigned long now = millis();
  switch (step){
    case 0: on(); t0=now; step=1; break;
    case 1: if (now-t0>=BUZZER_ON_MS){ off(); t0=now; step=2; } break;
    case 2: if (now-t0>=BUZZER_OFF_MS){ on();  t0=now; step=3; } break;
    case 3: if (now-t0>=BUZZER_ON_MS){ off(); t0=now; step=4; } break;
    case 4: if (now-t0>=BUZZER_OFF_MS){ on();  t0=now; step=5; } break;
    case 5: if (now-t0>=BUZZER_ON_MS){ off(); t0=now; step=6; } break;
    case 6: if (now-t0>=BUZZER_GROUP_PAUSE_MS){ step=0; } break;
  }
#else
  off();
#endif
}
