#pragma once
#include <Arduino.h>

class FeederController {
public:
    void begin(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4);
    bool request(int portions, int stepsPerPortion, bool waterOk);
    void tick();
    bool isBusy() const { return _busy; }
    uint64_t lastTimestamp() const { return _lastTs; }

private:
    uint8_t _in1, _in2, _in3, _in4;
    bool _busy = false;
    long _targetSteps = 0;
    long _stepsDone = 0;
    uint8_t _stepIndex = 0;
    unsigned long _lastStepTime = 0;
    uint64_t _lastTs = 0;
};
