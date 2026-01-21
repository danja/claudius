#pragma once

#include <Arduino.h>
#include "PinConfig.h"

class Adc {
public:
    void init() {
        // Configure ADC resolution
        analogReadResolution(12);
        analogSetWidth(12);
        analogSetAttenuation(ADC_11db);

        // Set pins as inputs
        pinMode(PIN_CV0, INPUT);
        pinMode(PIN_CV1, INPUT);
        pinMode(PIN_CV2, INPUT);
        pinMode(PIN_POT0, INPUT);
        pinMode(PIN_POT1, INPUT);
        pinMode(PIN_POT2, INPUT);

        // Ensure pins are attached to ADC and have expected attenuation.
        adcAttachPin(PIN_CV0);
        adcAttachPin(PIN_CV1);
        adcAttachPin(PIN_CV2);
        adcAttachPin(PIN_POT0);
        adcAttachPin(PIN_POT1);
        adcAttachPin(PIN_POT2);
        analogSetPinAttenuation(PIN_CV0, ADC_11db);
        analogSetPinAttenuation(PIN_CV1, ADC_11db);
        analogSetPinAttenuation(PIN_CV2, ADC_11db);
        analogSetPinAttenuation(PIN_POT0, ADC_11db);
        analogSetPinAttenuation(PIN_POT1, ADC_11db);
        analogSetPinAttenuation(PIN_POT2, ADC_11db);
    }

    uint16_t readCv0()  { return analogRead(PIN_CV0); }
    uint16_t readCv1()  { return analogRead(PIN_CV1); }
    uint16_t readCv2()  { return analogRead(PIN_CV2); }
    uint16_t readPot0() { return analogRead(PIN_POT0); }
    uint16_t readPot1() { return analogRead(PIN_POT1); }
    uint16_t readPot2() { return analogRead(PIN_POT2); }
};
