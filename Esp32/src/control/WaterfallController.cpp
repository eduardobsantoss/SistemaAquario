#include "control/WaterfallController.h"

void WaterfallController::begin(uint8_t p, bool al) {
    _pin = p;
    _activeLow = al;
    pinMode(_pin, OUTPUT);
    set(true);
}

void WaterfallController::set(bool on) {
    _state = on;
    digitalWrite(_pin, _activeLow ? !on : on);
    
    if (_autoMode) {
        _lastAutoState = on;
    }
}

bool WaterfallController::processFloatRaw(int raw, unsigned long now, bool& waterOkOut) {
    bool changed = false;

    if (raw != _lastRaw) {
        _changeTime = now;
        _lastRaw = raw;
    }

    if (raw != _stableRaw) {
        unsigned long elapsed = now - _changeTime;
        
        unsigned long requiredStable = (raw == 1) ? 2000 : 1000;
        
        if (elapsed >= requiredStable) {
            _stableRaw = raw;
            changed = true;
        }
    }

    waterOkOut = (_stableRaw == 1);
    return changed;
}
