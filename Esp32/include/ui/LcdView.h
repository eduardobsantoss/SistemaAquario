#pragma once
#include <Arduino.h>
enum class Screen : uint8_t { RESUMO=0, DETALHE=1 };
class LcdView {
public:
  void begin(uint8_t sda, uint8_t scl);
  void show(float tC, float pH, bool heaterOn, bool waterfallOn, bool waterOk);
  void handleButton(uint8_t pin, unsigned long debounceMs);
private:
  Screen current = Screen::RESUMO;
  uint8_t lcdAddr = 0x27;
  unsigned long tBtnChanged=0; bool lastBtn=HIGH;
  void update(float tC, float pH, bool heaterOn, bool waterfallOn, bool waterOk);
};
