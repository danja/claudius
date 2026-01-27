#pragma once

#include <cstdint>

// Voice selection
enum class VoiceType : uint8_t {
    CASCADE = 0,
    ORBIT_FM,
    PITCH_VERB,
    NUM_VOICES
};

// Parameter message for inter-core communication
struct ParamMessage {
    // Normalized values 0.0 - 1.0
    float attack;
    float decay;
    float wavefold;
    float chaos;
    float fmFeedback;
    float fmFold;
    float verbMix;
    float verbExcite;
    uint8_t voice;

    // CV and pot inputs (normalized)
    float cv0;      // Unused (reserved for future)
    float cv1;      // Unused (reserved for future)
    float cv2;      // Pitch CV
    float pot0;     // Harmonic spread knob
    float pot1;     // Cascade rate knob
    float pot2;     // Pitch knob

    // CV pitch calibration
    float cvPitchOffset;  // -1.0 to 1.0, added to CV
    float cvPitchScale;   // 0.0 to 2.0, multiplier for CV

    // Gate state
    bool gateIn;
};

// Status message from DSP to UI
struct StatusMessage {
    float outputLevel;
    bool isPlaying;
    float currentFreq;
};
