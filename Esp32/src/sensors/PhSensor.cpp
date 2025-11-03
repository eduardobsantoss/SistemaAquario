#include "sensors/PhSensor.h"
#include "config/thresholds.h"

void PhSensor::begin(uint8_t adcPin){ pin = adcPin; }

float PhSensor::readPH(){
  const int N=12; long acc=0;
  for (int i=0;i<N;i++){ acc += analogRead(pin); delayMicroseconds(100); }
  float adc = acc/(float)N;
  float v = adc * (ADC_VREF/ADC_MAX_COUNTS);
  if (!init){ emaV = v; init = true; } else { emaV = ALPHA_PH*v + (1.0f-ALPHA_PH)*emaV; }
  float pH = M_PH * emaV + B_PH;
  if (pH < PH_MIN) pH = PH_MIN;
  if (pH > PH_MAX) pH = PH_MAX;
  lastPH = pH;
  return pH;
}
