#include "control/FeederController.h"
#include "core/Clock.h"

void FeederController::begin(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4) {
    _in1 = in1;
    _in2 = in2;
    _in3 = in3;
    _in4 = in4;
    
    pinMode(_in1, OUTPUT);
    pinMode(_in2, OUTPUT);
    pinMode(_in3, OUTPUT);
    pinMode(_in4, OUTPUT);
    
    digitalWrite(_in1, LOW);
    digitalWrite(_in2, LOW);
    digitalWrite(_in3, LOW);
    digitalWrite(_in4, LOW);
}

bool FeederController::request(int portions, int stepsPerPortion, bool waterOk) {
    if (_busy) return false;
    if (!waterOk) {
        Serial.println("[FEEDER] BLOQUEADO: nivel baixo de agua");
        return false;
    }
    
    _targetSteps = portions * stepsPerPortion;
    _stepsDone = 0;
    _stepIndex = 0;
    _busy = true;
    _lastStepTime = 0;
    
    Serial.printf("[FEEDER] Iniciando: %d porcao(oes), %ld passos\n", portions, _targetSteps);
    return true;
}

void FeederController::tick() {
    if (!_busy) return;
    
    unsigned long now = millis();
    if (now - _lastStepTime < 2) return;
    
    switch(_stepIndex & 0x07) {
        case 0: digitalWrite(_in1, HIGH); digitalWrite(_in2, LOW);  digitalWrite(_in3, LOW);  digitalWrite(_in4, LOW);  break;
        case 1: digitalWrite(_in1, HIGH); digitalWrite(_in2, HIGH); digitalWrite(_in3, LOW);  digitalWrite(_in4, LOW);  break;
        case 2: digitalWrite(_in1, LOW);  digitalWrite(_in2, HIGH); digitalWrite(_in3, LOW);  digitalWrite(_in4, LOW);  break;
        case 3: digitalWrite(_in1, LOW);  digitalWrite(_in2, HIGH); digitalWrite(_in3, HIGH); digitalWrite(_in4, LOW);  break;
        case 4: digitalWrite(_in1, LOW);  digitalWrite(_in2, LOW);  digitalWrite(_in3, HIGH); digitalWrite(_in4, LOW);  break;
        case 5: digitalWrite(_in1, LOW);  digitalWrite(_in2, LOW);  digitalWrite(_in3, HIGH); digitalWrite(_in4, HIGH); break;
        case 6: digitalWrite(_in1, LOW);  digitalWrite(_in2, LOW);  digitalWrite(_in3, LOW);  digitalWrite(_in4, HIGH); break;
        case 7: digitalWrite(_in1, HIGH); digitalWrite(_in2, LOW);  digitalWrite(_in3, LOW);  digitalWrite(_in4, HIGH); break;
    }
    
    _stepIndex++;
    _stepsDone++;
    _lastStepTime = now;
    
    if (_stepsDone >= _targetSteps) {
        _busy = false;
        _lastTs = Clock::epochMillis();
        digitalWrite(_in1, LOW);
        digitalWrite(_in2, LOW);
        digitalWrite(_in3, LOW);
        digitalWrite(_in4, LOW);
    }
}
