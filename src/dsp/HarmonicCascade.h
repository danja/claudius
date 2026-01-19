#pragma once

#include <cmath>
#include <algorithm>
#include "Config.h"

// Claudius: Harmonic Cascade Synthesizer
//
// Generates multiple harmonics that decay at different rates.
// Higher harmonics decay faster, creating evolving timbres.
// Features wave folding and chaos modulation for organic sound.

class HarmonicCascade {
public:
    explicit HarmonicCascade(float sampleRate = SAMPLE_RATE)
        : sampleRate_(sampleRate)
        , phaseInc_(0.0f)
    {
        reset();
    }

    void reset() {
        for (int i = 0; i < MAX_HARMONICS; ++i) {
            phases_[i] = 0.0f;
            harmonicEnvs_[i] = 0.0f;
        }
        chaosX_ = 0.1f;
        chaosY_ = 0.1f;
        chaosZ_ = 0.1f;
    }

    void setFrequency(float freq) {
        freq = std::clamp(freq, MIN_FREQ, MAX_FREQ);
        phaseInc_ = freq / sampleRate_;
    }

    void trigger() {
        // Reset all harmonic envelopes to maximum
        for (int i = 0; i < MAX_HARMONICS; ++i) {
            harmonicEnvs_[i] = 1.0f;
        }
    }

    // Process one sample
    // harmonicSpread: 0-1, controls how many harmonics and their amplitudes
    // cascadeRate: 0-1, how much faster higher harmonics decay
    // wavefold: 0-1, amount of wave folding
    // chaos: 0-1, amount of chaotic modulation
    // masterEnv: 0-1, main envelope level
    float process(float harmonicSpread, float cascadeRate,
                  float wavefold, float chaos, float masterEnv) {

        // Update chaos oscillator (Lorenz-like, simplified)
        updateChaos();

        // Calculate how many harmonics are active based on spread
        float numHarmonics = 1.0f + harmonicSpread * (MAX_HARMONICS - 1);

        // Calculate cascade decay multipliers
        // Higher cascade = higher harmonics decay much faster
        float baseDecay = 0.99995f; // Very slow base decay

        float output = 0.0f;
        float ampSum = 0.0f;

        for (int i = 0; i < MAX_HARMONICS; ++i) {
            int harmonicNum = i + 1;

            // Calculate harmonic amplitude based on spread
            float harmonicAmp = 1.0f;
            if (i > 0) {
                float falloff = static_cast<float>(i) / numHarmonics;
                harmonicAmp = std::max(0.0f, 1.0f - falloff * falloff);
            }

            if (harmonicAmp <= 0.0f) continue;

            // Update this harmonic's decay envelope
            // Higher harmonics decay faster based on cascade rate
            float decayMult = 1.0f + cascadeRate * static_cast<float>(i) * 0.5f;
            float decay = powf(baseDecay, decayMult);
            harmonicEnvs_[i] *= decay;

            // Add chaos modulation to frequency
            float freqMod = 1.0f + chaos * chaosX_ * 0.01f * harmonicNum;

            // Update phase for this harmonic
            float harmonicPhaseInc = phaseInc_ * harmonicNum * freqMod;
            phases_[i] += harmonicPhaseInc;
            if (phases_[i] >= 1.0f) phases_[i] -= 1.0f;

            // Generate waveform - use a mix of sine and saw for richness
            float phase2pi = phases_[i] * 2.0f * M_PI;
            float sine = sinf(phase2pi);
            float saw = 2.0f * phases_[i] - 1.0f;

            // Mix sine and saw based on harmonic number
            // Lower harmonics = more sine, higher = more saw
            float sawMix = std::min(1.0f, static_cast<float>(i) * 0.2f);
            float wave = sine * (1.0f - sawMix) + saw * sawMix;

            // Apply harmonic's individual envelope
            float envAmp = harmonicEnvs_[i] * harmonicAmp;

            // Add amplitude chaos
            float ampMod = 1.0f + chaos * chaosY_ * 0.1f;
            envAmp *= ampMod;

            output += wave * envAmp;
            ampSum += envAmp;
        }

        // Normalize output
        if (ampSum > 0.0f) {
            output /= std::max(1.0f, ampSum * 0.5f);
        }

        // Apply wave folding for additional harmonics
        if (wavefold > 0.01f) {
            output = applyWavefold(output, wavefold);
        }

        // Apply master envelope
        output *= masterEnv;

        // Soft clip
        output = tanhf(output);

        return output;
    }

private:
    float applyWavefold(float input, float amount) {
        // Wave folder: folds the signal back when it exceeds threshold
        float drive = 1.0f + amount * 4.0f;
        float signal = input * drive;

        // Multiple fold stages
        for (int i = 0; i < 3; ++i) {
            if (signal > 1.0f) {
                signal = 2.0f - signal;
            } else if (signal < -1.0f) {
                signal = -2.0f - signal;
            }
        }

        // Mix wet/dry
        return input * (1.0f - amount) + signal * amount;
    }

    void updateChaos() {
        // Simplified Lorenz-like chaotic oscillator
        // Runs at a slow rate for modulation purposes
        constexpr float dt = 0.0001f;
        constexpr float sigma = 10.0f;
        constexpr float rho = 28.0f;
        constexpr float beta = 8.0f / 3.0f;

        float dx = sigma * (chaosY_ - chaosX_);
        float dy = chaosX_ * (rho - chaosZ_) - chaosY_;
        float dz = chaosX_ * chaosY_ - beta * chaosZ_;

        chaosX_ += dx * dt;
        chaosY_ += dy * dt;
        chaosZ_ += dz * dt;

        // Keep bounded
        chaosX_ = std::clamp(chaosX_, -50.0f, 50.0f);
        chaosY_ = std::clamp(chaosY_, -50.0f, 50.0f);
        chaosZ_ = std::clamp(chaosZ_, 0.0f, 60.0f);

        // Normalize to -1..1 range for modulation
        chaosX_ = chaosX_ / 50.0f;
        chaosY_ = chaosY_ / 50.0f;
    }

    float sampleRate_;
    float phaseInc_;

    float phases_[MAX_HARMONICS];
    float harmonicEnvs_[MAX_HARMONICS];

    // Chaos state
    float chaosX_, chaosY_, chaosZ_;
};
