#pragma once

// Claudius - Hardware pin assignments
// Based on ESP32 DevKit V1 DOIT module

// DAC outputs
constexpr int PIN_DAC1 = 25;
constexpr int PIN_DAC2 = 26;

// Display I2C
constexpr int PIN_SDA = 21;
constexpr int PIN_SCL = 22;

// Rotary encoder
constexpr int PIN_ENC_SW = 16;
constexpr int PIN_ENC_CLK = 34;
constexpr int PIN_ENC_DT = 35;

// CV inputs (directly on ADC1)
constexpr int PIN_CV0 = 36;  // Harmonic Spread CV
constexpr int PIN_CV1 = 39;  // Cascade Rate CV
constexpr int PIN_CV2 = 32;  // Pitch CV

// Potentiometer inputs
constexpr int PIN_POT0 = 33; // Harmonic Spread knob
constexpr int PIN_POT1 = 27; // Cascade Rate knob
constexpr int PIN_POT2 = 4;  // Pitch knob

// Gate I/O
constexpr int PIN_GATE_IN = 18;
constexpr int PIN_GATE_OUT = 19;
