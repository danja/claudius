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

        // Read initial state
        lastState_ = (digitalRead(PIN_ENC_CLK) << 1) | digitalRead(PIN_ENC_DT);
        lastSw_ = HIGH;
        accumulator_ = 0;
    }

    // Returns rotation delta: -1, 0, or +1
    int8_t readRotation() {
        // Read current state of both pins
        int clk = digitalRead(PIN_ENC_CLK);
        int dt = digitalRead(PIN_ENC_DT);
        int state = (clk << 1) | dt;

        // Gray code state machine
        // Valid transitions: 00->01->11->10->00 (CW) or 00->10->11->01->00 (CCW)
        if (state != lastState_) {
            // Debounce: only accept if enough time has passed
            unsigned long now = millis();
            if (now - lastRotTime_ >= 2) {  // 2ms minimum between transitions
                // Determine direction from state transition (swapped for correct direction)
                int transition = (lastState_ << 2) | state;
                switch (transition) {
                    case 0b0001: case 0b0111: case 0b1110: case 0b1000:
                        accumulator_--;  // Was ++, swapped
                        break;
                    case 0b0010: case 0b1011: case 0b1101: case 0b0100:
                        accumulator_++;  // Was --, swapped
                        break;
                }
                lastRotTime_ = now;
            }
            lastState_ = state;
        }

        // Return accumulated clicks (detent = 2 state changes for faster response)
        int8_t delta = 0;
        if (accumulator_ >= 2) {
            delta = 1;
            accumulator_ -= 2;
        } else if (accumulator_ <= -2) {
            delta = -1;
            accumulator_ += 2;
        }
        return delta;
    }

    // Returns true if button was just pressed
    bool readButtonPress() {
        int sw = digitalRead(PIN_ENC_SW);
        bool pressed = false;

        if (sw == LOW && lastSw_ == HIGH) {
            unsigned long now = millis();
            if (now - lastSwTime_ >= 50) {  // 50ms debounce for button
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
    int lastState_ = 0;
    int lastSw_ = HIGH;
    int accumulator_ = 0;
    unsigned long lastRotTime_ = 0;
    unsigned long lastSwTime_ = 0;
};
