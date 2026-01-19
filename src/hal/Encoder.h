#pragma once

#include <Arduino.h>
#include "PinConfig.h"
#include "Config.h"

class Encoder {
public:
    void init() {
        pinMode(PIN_ENC_CLK, INPUT_PULLUP);
        pinMode(PIN_ENC_DT, INPUT_PULLUP);
        pinMode(PIN_ENC_SW, INPUT_PULLUP);

        lastClk_ = digitalRead(PIN_ENC_CLK);
        lastSw_ = HIGH;
    }

    // Returns rotation delta: -1, 0, or +1
    int8_t readRotation() {
        int8_t delta = 0;
        int clk = digitalRead(PIN_ENC_CLK);

        if (clk != lastClk_) {
            unsigned long now = millis();
            if (now - lastRotTime_ > ENCODER_DEBOUNCE_MS) {
                int dt = digitalRead(PIN_ENC_DT);
                if (dt != clk) {
                    delta = 1;
                } else {
                    delta = -1;
                }
                lastRotTime_ = now;
            }
        }
        lastClk_ = clk;
        return delta;
    }

    // Returns true if button was just pressed
    bool readButtonPress() {
        int sw = digitalRead(PIN_ENC_SW);
        bool pressed = false;

        if (sw == LOW && lastSw_ == HIGH) {
            unsigned long now = millis();
            if (now - lastSwTime_ > ENCODER_DEBOUNCE_MS) {
                pressed = true;
                lastSwTime_ = now;
            }
        }
        lastSw_ = sw;
        return pressed;
    }

    // Returns true if button is currently held
    bool isButtonHeld() {
        return digitalRead(PIN_ENC_SW) == LOW;
    }

private:
    int lastClk_ = HIGH;
    int lastSw_ = HIGH;
    unsigned long lastRotTime_ = 0;
    unsigned long lastSwTime_ = 0;
};
