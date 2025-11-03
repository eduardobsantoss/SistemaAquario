#include "control/HeaterController.h"

void HeaterController::begin(uint8_t p, bool al){ pin=p; activeLow=al; pinMode(pin,OUTPUT); write(false); }
void HeaterController::write(bool on){ state=on; digitalWrite(pin, activeLow ? !on : on); }
void HeaterController::forceOff(){ write(false); }

bool HeaterController::update(float tC, float sp, float hyst, float minSafe, float maxSafe, unsigned long now, unsigned long minSwitchMs, bool& changed){
  changed=false;
  if (!isfinite(tC) || tC < minSafe || tC > maxSafe){ if (state){ write(false); changed=true; } return changed; }
  float onThr  = sp - hyst;
  float offThr = sp + hyst;
  bool canSwitch = (now - lastSwitch) >= minSwitchMs;
  if (!state && tC < onThr && canSwitch){ write(true);  changed=true; lastSwitch=now; }
  else if (state && tC > offThr && canSwitch){ write(false); changed=true; lastSwitch=now; }
  return changed;
}
