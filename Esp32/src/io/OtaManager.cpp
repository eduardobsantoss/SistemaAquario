#include "io/OtaManager.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

void OtaManager::begin(const char* hostname, uint16_t port){
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPort(port);
  ArduinoOTA.onStart([](){ Serial.println("\nOTA: start"); });
  ArduinoOTA.onEnd([](){ Serial.println("\nOTA: end"); });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t){ Serial.printf("OTA: %u%%\r",(p*100U)/t); });
  ArduinoOTA.onError([](ota_error_t e){ Serial.printf("OTA erro %u\n", e); });
  ArduinoOTA.begin();

  if (MDNS.begin(hostname)) {
    MDNS.addService("arduino", "tcp", port);
    MDNS.addServiceTxt("arduino","tcp","board","esp32");
    Serial.printf("OTA pronto: %s:%u\n", hostname, port);
  } else {
    Serial.println("mDNS falhou");
  }
}
void OtaManager::handle(){ ArduinoOTA.handle(); }
