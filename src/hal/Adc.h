#pragma once

#include <Arduino.h>
#include "PinConfig.h"

class Adc {
public:
    void init() {
        // Configure ADC resolution
        analogReadResolution(12);
        analogSetAttenuation(ADC_11db);

        // Set pins as inputs
        pinMode(PIN_CV0, INPUT);
        pinMode(PIN_CV1, INPUT);
        pinMode(PIN_CV2, INPUT);
        pinMode(PIN_POT0, INPUT);
        pinMode(PIN_POT1, INPUT);
        pinMode(PIN_POT2, INPUT);
    }

    uint16_t readCv0()  { return analogRead(PIN_CV0); }
    uint16_t readCv1()  { return analogRead(PIN_CV1); }
    uint16_t readCv2()  { return analogRead(PIN_CV2); }
    uint16_t readPot0() { return analogRead(PIN_POT0); }
    uint16_t readPot1() { return analogRead(PIN_POT1); }
    uint16_t readPot2() { return analogRead(PIN_POT2); }
};
