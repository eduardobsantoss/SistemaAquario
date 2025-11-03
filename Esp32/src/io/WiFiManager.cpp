#include "io/WiFiManager.h"
#include <WiFi.h>

void WiFiManager::begin(const char* ssid, const char* pass){
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(ssid, pass);
  Serial.print("Conectando Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) { delay(250); Serial.print("."); }
  Serial.printf("\nWi-Fi OK. IP: %s\n", WiFi.localIP().toString().c_str());
}
void WiFiManager::handle(){
  if (WiFi.status() != WL_CONNECTED){
    WiFi.reconnect();
  }
}
bool WiFiManager::connected() const { return WiFi.status() == WL_CONNECTED; }
