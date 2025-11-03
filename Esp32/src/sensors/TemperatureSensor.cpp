#include "sensors/TemperatureSensor.h"
#include <OneWire.h>
#include <DallasTemperature.h>

static OneWire* _ow=nullptr;
static DallasTemperature* _dt=nullptr;

void TemperatureSensor::begin(uint8_t pin){
  _ow = new OneWire(pin);
  _dt = new DallasTemperature(_ow);
  _dt->begin();
}
float TemperatureSensor::readCelsius(){
  _dt->requestTemperatures();
  lastC = _dt->getTempCByIndex(0);
  return lastC;
}
