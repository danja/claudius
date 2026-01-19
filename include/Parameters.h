#pragma once

#include <cstdint>

// Parameter indices for menu navigation
enum class ParamIndex : uint8_t {
    ATTACK = 0,
    DECAY,
    WAVEFOLD,
    CHAOS,
    NUM_PARAMS
};

// Parameter message for inter-core communication
struct ParamMessage {
    // Normalized values 0.0 - 1.0
    float attack;
    float decay;
    float wavefold;
    float chaos;

    // CV and pot inputs (normalized)
    float cv0;      // Harmonic spread CV
    float cv1;      // Cascade rate CV
    float cv2;      // Pitch CV
    float pot0;     // Harmonic spread knob
    float pot1;     // Cascade rate knob
    float pot2;     // Pitch knob

    // Gate state
    bool gateIn;
};

// Status message from DSP to UI
struct StatusMessage {
    float outputLevel;
    bool isPlaying;
    float currentFreq;
};

// Parameter metadata for display
struct ParamInfo {
    const char* name;
    const char* unit;
    float minDisplay;
    float maxDisplay;
    bool isTime;  // Use exponential display scaling
};

constexpr ParamInfo PARAM_INFO[] = {
    {"Attack", "ms", 1.0f, 2000.0f, true},
    {"Decay", "ms", 10.0f, 8000.0f, true},
    {"Wavefold", "%", 0.0f, 100.0f, false},
    {"Chaos", "%", 0.0f, 100.0f, false}
};
