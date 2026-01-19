#pragma once

#include <cstdint>
#include "Utils.h"

// ADC calibration for CV and potentiometer inputs

struct AdcCalibration {
    uint16_t minValue;
    uint16_t maxValue;
    bool invert;
};

// Calibration constants for each input
// Adjust these based on your specific hardware
constexpr AdcCalibration CAL_CV0  = {0, 4095, true};   // CV typically inverted
constexpr AdcCalibration CAL_CV1  = {0, 4095, true};
constexpr AdcCalibration CAL_CV2  = {0, 4095, true};
constexpr AdcCalibration CAL_POT0 = {0, 4095, false};
constexpr AdcCalibration CAL_POT1 = {0, 4095, false};
constexpr AdcCalibration CAL_POT2 = {0, 4095, false};

// Normalize raw ADC value to 0.0 - 1.0 range
inline float normalizeAdc(uint16_t value, const AdcCalibration& cal) {
    float range = static_cast<float>(cal.maxValue - cal.minValue);
    if (range <= 0.0f) return 0.5f;

    float normalized = static_cast<float>(value - cal.minValue) / range;
    if (cal.invert) normalized = 1.0f - normalized;

    return clamp(normalized, 0.0f, 1.0f);
}

// Exponential mapping for time parameters (attack, decay)
inline float expMap(float normalized, float minVal, float maxVal) {
    return minVal * powf(maxVal / minVal, normalized);
}

// Linear mapping
inline float linMap(float normalized, float minVal, float maxVal) {
    return minVal + normalized * (maxVal - minVal);
}
