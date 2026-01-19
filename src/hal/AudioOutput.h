#pragma once

#include <Arduino.h>
#include "PinConfig.h"

class AudioOutput {
public:
    void init() {
        // ESP32 DAC outputs are 8-bit (0-255)
        // Pins 25 and 26 are the built-in DAC channels
    }

    // Write stereo sample pair
    // Input range: -1.0 to 1.0
    void write(float left, float right) {
        dacWrite(PIN_DAC1, floatToDac(left));
        dacWrite(PIN_DAC2, floatToDac(right));
    }

    // Write mono to both channels
    void writeMono(float sample) {
        uint8_t dac = floatToDac(sample);
        dacWrite(PIN_DAC1, dac);
        dacWrite(PIN_DAC2, dac);
    }

private:
    static uint8_t floatToDac(float sample) {
        // Clamp to valid range
        sample = constrain(sample, -1.0f, 1.0f);
        // Convert bipolar to unipolar
        float unipolar = (sample + 1.0f) * 0.5f;
        // Scale to 8-bit
        return static_cast<uint8_t>(unipolar * 255.0f);
    }
};
