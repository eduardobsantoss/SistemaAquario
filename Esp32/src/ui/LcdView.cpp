#include "ui/LcdView.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

static LiquidCrystal_I2C* lcd=nullptr;

static uint8_t scanI2C(){
  uint8_t found=0;
  for (uint8_t a=1;a<127;a++){ Wire.beginTransmission(a); if (Wire.endTransmission()==0){ found=a; break; } }
  return found;
}

void LcdView::begin(uint8_t sda, uint8_t scl){
  Wire.begin(sda,scl);
  Wire.setClock(100000);
    
  delay(50);
    
  uint8_t found = scanI2C();
  if (found == 0) {
    Serial.println("[LCD] Endereço não encontrado!");
    return;
  }
  
  lcdAddr=found;
  lcd = new LiquidCrystal_I2C(lcdAddr,16,2);
  lcd->init();
  lcd->backlight();
  lcd->setContrast(200);
    
  lcd->clear();
  lcd->setCursor(0,0);
  lcd->print("LCD OK @0x");
  char buf[5];
  sprintf(buf, "%02X", lcdAddr);
  lcd->print(buf);
  delay(1000);
  lcd->clear();
}

void LcdView::update(float tC, float pH, bool heaterOn, bool waterfallOn, bool waterOk){
  char l1[17], l2[17];
  if (current==Screen::RESUMO){
    if (isfinite(tC) && isfinite(pH)) snprintf(l1,sizeof(l1),"T:%4.1fC pH:%4.2f",tC,pH);
    else if (isfinite(tC))            snprintf(l1,sizeof(l1),"T:%4.1fC pH:--.--",tC);
    else if (isfinite(pH))            snprintf(l1,sizeof(l1),"T:--.-C pH:%4.2f",pH);
    else                               snprintf(l1,sizeof(l1),"T:--.-C pH:--.--");
    snprintf(l2,sizeof(l2),"HTR:%s", heaterOn? "ON ":"OFF");
  } else {
    snprintf(l1,sizeof(l1),"CASC:%s", waterfallOn? "ON ":"OFF");
    snprintf(l2,sizeof(l2),"NIV:%s",  waterOk? "OK ":"BAIX");
  }
  lcd->setCursor(0,0); lcd->print("                ");
  lcd->setCursor(0,1); lcd->print("                ");
  lcd->setCursor(0,0); lcd->print(l1);
  lcd->setCursor(0,1); lcd->print(l2);
}

void LcdView::show(float tC, float pH, bool heaterOn, bool waterfallOn, bool waterOk){
  update(tC,pH,heaterOn,waterfallOn,waterOk);
}

void LcdView::handleButton(uint8_t pin, unsigned long debounceMs){
  bool reading = digitalRead(pin);
  unsigned long now = millis();
  static bool stable=HIGH;
  if (reading != lastBtn) tBtnChanged = now;
  if ((now - tBtnChanged) > debounceMs){
    if (reading != stable){
      stable = reading;
      if (stable==LOW){ current = (current==Screen::RESUMO)? Screen::DETALHE : Screen::RESUMO; }
    }
  }
  lastBtn = reading;
}
