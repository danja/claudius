#pragma once

#include <cmath>
#include "Config.h"
#include "Utils.h"

// Claudius: Additive Synthesizer with Harmonic Cascade
//
// SPREAD: Controls how many harmonics are active (1-8)
// CASCADE: Controls relative amplitude of higher harmonics
// WAVEFOLD: Adds distortion/harmonics
// CHAOS: Vibrato and tremolo

class HarmonicCascade {
public:
    explicit HarmonicCascade(float sampleRate = SAMPLE_RATE)
        : sampleRate_(sampleRate)
        , baseFreq_(220.0f)
    {
        reset();
    }

    void reset() {
        for (int i = 0; i < MAX_HARMONICS; ++i) {
            phases_[i] = 0.0f;
        }
        lfoPhase_ = 0.0f;
    }

    void setFrequency(float freq) {
        baseFreq_ = clamp(freq, MIN_FREQ, MAX_FREQ);
    }

    void trigger() {
        // Reset phases for clean attack
        for (int i = 0; i < MAX_HARMONICS; ++i) {
            phases_[i] = 0.0f;
        }
    }

    float process(float spread, float cascade, float wavefold, float chaos, float envelope) {
        // LFO for chaos effects (~4 Hz)
        lfoPhase_ += 4.0f / sampleRate_;
        if (lfoPhase_ >= 1.0f) lfoPhase_ -= 1.0f;
        float lfo = sinf(lfoPhase_ * 2.0f * M_PI);

        // Vibrato: chaos controls depth (0 to ±5%)
        float vibratoMult = 1.0f + chaos * lfo * 0.05f;

        // Tremolo: chaos controls depth (0 to ±40%)
        float tremoloMult = 1.0f + chaos * lfo * 0.4f;
        if (tremoloMult < 0.2f) tremoloMult = 0.2f;

        float output = 0.0f;
        float totalAmp = 0.0f;

        // SPREAD determines how many harmonics (1 to 8)
        // At spread=0, only fundamental
        // At spread=1, all 8 harmonics
        int numHarmonics = 1 + static_cast<int>(spread * 7.0f);

        for (int i = 0; i < numHarmonics; ++i) {
            int harmonic = i + 1;  // 1, 2, 3, 4, 5, 6, 7, 8

            // CASCADE determines amplitude rolloff
            // cascade=0: all harmonics equal amplitude
            // cascade=1: 1/n rolloff (like sawtooth)
            float ampRolloff;
            if (cascade < 0.01f) {
                // No rolloff - all harmonics equal
                ampRolloff = 1.0f;
            } else {
                // Interpolate between equal (1.0) and 1/n
                float equalAmp = 1.0f;
                float sawAmp = 1.0f / static_cast<float>(harmonic);
                ampRolloff = equalAmp * (1.0f - cascade) + sawAmp * cascade;
            }

            // Calculate frequency with vibrato
            float freq = baseFreq_ * static_cast<float>(harmonic) * vibratoMult;

            // Anti-aliasing: skip harmonics above Nyquist
            if (freq > sampleRate_ * 0.45f) continue;

            // Phase increment
            float phaseInc = freq / sampleRate_;
            phases_[i] += phaseInc;
            if (phases_[i] >= 1.0f) phases_[i] -= 1.0f;

            // Generate sine
            float sample = sinf(phases_[i] * 2.0f * M_PI);

            // Accumulate
            output += sample * ampRolloff;
            totalAmp += ampRolloff;
        }

        // Normalize to prevent clipping
        if (totalAmp > 1.0f) {
            output /= totalAmp;
        }

        // Apply tremolo
        output *= tremoloMult;

        // Wavefold for extra harmonics/distortion
        if (wavefold > 0.01f) {
            float drive = 1.0f + wavefold * 4.0f;
            float folded = output * drive;
            // Fold back
            while (folded > 1.0f || folded < -1.0f) {
                if (folded > 1.0f) folded = 2.0f - folded;
                if (folded < -1.0f) folded = -2.0f - folded;
            }
            output = output * (1.0f - wavefold) + folded * wavefold;
        }

        // Apply envelope
        output *= envelope;

        // Soft clip
        return fastTanh(output);
    }

private:
    float sampleRate_;
    float baseFreq_;
    float phases_[MAX_HARMONICS];
    float lfoPhase_;
};
