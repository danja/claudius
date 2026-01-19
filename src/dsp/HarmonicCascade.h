#pragma once

#include <cmath>
#include "Config.h"
#include "Utils.h"

// Claudius: Additive Synthesizer with Harmonic Cascade
//
// SPREAD: Controls how many harmonics are active (1-8)
// CASCADE: Controls relative amplitude of higher harmonics
// WAVEFOLD: Adds distortion/harmonics
// CHAOS: Lorenz attractor modulation of harmonic amplitudes

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
        lorenzX_ = 0.1f;
        lorenzY_ = 0.0f;
        lorenzZ_ = 0.0f;
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
        // Lorenz attractor for chaotic modulation
        constexpr float kSigma = 10.0f;
        constexpr float kRho = 28.0f;
        constexpr float kBeta = 8.0f / 3.0f;
        float dt = 1.0f / sampleRate_;
        float dx = kSigma * (lorenzY_ - lorenzX_);
        float dy = lorenzX_ * (kRho - lorenzZ_) - lorenzY_;
        float dz = lorenzX_ * lorenzY_ - kBeta * lorenzZ_;
        lorenzX_ += dx * dt;
        lorenzY_ += dy * dt;
        lorenzZ_ += dz * dt;

        // Map chaotic state to a smooth 0-1 modulator
        float chaosNorm = 0.5f + 0.5f * fastTanh(lorenzX_ * 0.08f + lorenzY_ * 0.03f);

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

            float chaosWeight = (numHarmonics > 1)
                ? static_cast<float>(i) / static_cast<float>(numHarmonics - 1)
                : 1.0f;
            float chaosMod = 1.0f + chaos * chaosWeight * (chaosNorm - 0.5f) * 1.8f;
            if (chaosMod < 0.15f) chaosMod = 0.15f;
            ampRolloff *= chaosMod;

            // Calculate frequency
            float freq = baseFreq_ * static_cast<float>(harmonic);

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
    float lorenzX_;
    float lorenzY_;
    float lorenzZ_;
};
