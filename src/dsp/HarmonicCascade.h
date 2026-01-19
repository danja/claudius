#pragma once

#include <cmath>
#include "Config.h"
#include "Utils.h"

// Claudius: Harmonic Cascade Synthesizer
//
// Generates multiple harmonics that decay at different rates.
// Higher harmonics decay faster, creating evolving timbres.

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
            harmonicEnvs_[i] = 1.0f;  // Start with all harmonics active
        }
        chaosPhase_ = 0.0f;
        lfoPhase_ = 0.0f;
    }

    void setFrequency(float freq) {
        freq = clamp(freq, MIN_FREQ, MAX_FREQ);
        phaseInc_ = freq / sampleRate_;
    }

    void trigger() {
        // Reset all harmonic envelopes to maximum
        for (int i = 0; i < MAX_HARMONICS; ++i) {
            harmonicEnvs_[i] = 1.0f;
        }
    }

    // Process one sample
    float process(float harmonicSpread, float cascadeRate,
                  float wavefold, float chaos, float masterEnv) {

        // LFO for chaos modulation (slow sine wave)
        lfoPhase_ += 0.00005f;  // Very slow LFO ~2Hz at 44.1kHz
        if (lfoPhase_ >= 1.0f) lfoPhase_ -= 1.0f;
        float lfo1 = sinf(lfoPhase_ * 2.0f * M_PI);

        // Second LFO at different rate for complexity
        chaosPhase_ += 0.00003f;
        if (chaosPhase_ >= 1.0f) chaosPhase_ -= 1.0f;
        float lfo2 = sinf(chaosPhase_ * 2.0f * M_PI * 1.618f);  // Golden ratio offset

        float output = 0.0f;

        // Calculate number of harmonics based on spread (1 to MAX_HARMONICS)
        int numActive = 1 + static_cast<int>(harmonicSpread * (MAX_HARMONICS - 1));

        for (int i = 0; i < numActive; ++i) {
            int harmonicNum = i + 1;

            // Amplitude falls off with harmonic number (1/n for sawtooth-like spectrum)
            float baseAmp = 1.0f / static_cast<float>(harmonicNum);

            // Apply spread - higher spread means more equal harmonic levels
            float spreadBoost = 1.0f + harmonicSpread * static_cast<float>(i) * 0.5f;
            float harmonicAmp = baseAmp * spreadBoost;

            // Apply cascade decay - higher harmonics decay faster
            // Very slow decay so you can actually hear the effect
            float decayRate = 0.99999f - cascadeRate * 0.00002f * static_cast<float>(i);
            if (decayRate < 0.999f) decayRate = 0.999f;
            harmonicEnvs_[i] *= decayRate;

            // Chaos: pitch modulation (vibrato) - more on higher harmonics
            float vibrato = 1.0f + chaos * lfo1 * 0.02f * (1.0f + i * 0.3f);

            // Chaos: amplitude modulation (tremolo)
            float tremolo = 1.0f + chaos * lfo2 * 0.3f;
            if (tremolo < 0.1f) tremolo = 0.1f;

            // Update phase
            float harmonicPhaseInc = phaseInc_ * harmonicNum * vibrato;
            phases_[i] += harmonicPhaseInc;
            if (phases_[i] >= 1.0f) phases_[i] -= 1.0f;

            // Generate sine wave
            float wave = sinf(phases_[i] * 2.0f * M_PI);

            // Combine amplitude factors
            float amp = harmonicAmp * harmonicEnvs_[i] * tremolo;
            output += wave * amp;
        }

        // Normalize based on number of active harmonics
        output /= (1.0f + numActive * 0.3f);

        // Wave folding
        if (wavefold > 0.01f) {
            float drive = 1.0f + wavefold * 4.0f;
            float folded = sinf(output * drive * M_PI * 0.5f);
            output = output * (1.0f - wavefold) + folded * wavefold;
        }

        // Apply master envelope
        output *= masterEnv;

        // Soft clip
        return fastTanh(output);
    }

private:
    float sampleRate_;
    float phaseInc_;
    float phases_[MAX_HARMONICS];
    float harmonicEnvs_[MAX_HARMONICS];
    float chaosPhase_;
    float lfoPhase_;
};
