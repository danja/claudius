#pragma once

// Claudius - Harmonic Cascade Synthesizer
// Configuration constants

// Audio settings
constexpr float SAMPLE_RATE = 44100.0f;
constexpr int AUDIO_BLOCK_SIZE = 64;

// Harmonic cascade settings
constexpr int MAX_HARMONICS = 8;
constexpr float MIN_FREQ = 27.5f;   // A0
constexpr float MAX_FREQ = 880.0f;  // A5

// Envelope time ranges (seconds)
constexpr float MIN_ATTACK = 0.001f;
constexpr float MAX_ATTACK = 2.0f;
constexpr float MIN_DECAY = 0.01f;
constexpr float MAX_DECAY = 8.0f;

// Modulation amounts
constexpr float CV_MOD_AMOUNT = 0.5f;
constexpr float PITCH_SMOOTH_ALPHA = 0.3f;
constexpr float PARAM_SMOOTH_ALPHA = 0.1f;

// Output settings
constexpr float MASTER_GAIN = 0.8f;
constexpr float SAMPLE_GUARD = 2.0f;

// UI settings
constexpr int ENCODER_DEBOUNCE_MS = 5;
constexpr int DISPLAY_UPDATE_MS = 50;
constexpr int ADC_READ_INTERVAL_MS = 2;
