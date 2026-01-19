#pragma once

#include <Arduino.h>
#include "PinConfig.h"

class Gate {
public:
    void init() {
        pinMode(PIN_GATE_IN, INPUT);
        pinMode(PIN_GATE_OUT, OUTPUT);
        digitalWrite(PIN_GATE_OUT, LOW);
    }

    bool readGateIn() {
        return digitalRead(PIN_GATE_IN) == HIGH;
    }

    void setGateOut(bool active) {
        // Inverted: LOW when playing (typical for eurorack)
        digitalWrite(PIN_GATE_OUT, active ? LOW : HIGH);
    }
};
