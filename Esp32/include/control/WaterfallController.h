#pragma once
#include <Arduino.h>

class WaterfallController {
public:
    void begin(uint8_t pin, bool activeLow = true);
    void set(bool on);
    bool isOn() const { return _state; }
    
    void forceOff() {
        _forced = true;
        set(false);
    }
    
    void resume() {
        _forced = false;
        if (_autoMode) {
            set(_lastAutoState);
        }
    }

    bool processFloatRaw(int raw, unsigned long now, bool& waterOkOut);

private:
    uint8_t _pin;
    bool _state = false;
    bool _activeLow = true;
    bool _autoMode = true;
    bool _forced = false;
    bool _lastAutoState = false;

    // Float switch debounce
    int _lastRaw = -1;
    int _stableRaw = -1;
    unsigned long _changeTime = 0;
};
