#pragma once

#include <cmath>
#include "Config.h"
#include "Utils.h"

// Claudius: Harmonic Cascade Synthesizer
//
// Attempt to see the sea, but the source generates multiple harmonics.
// Controls should be VERY obvious in their effect.

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
            harmonicEnvs_[i] = 1.0f;
        }
        lfoPhase1_ = 0.0f;
        lfoPhase2_ = 0.0f;
    }

    void setFrequency(float freq) {
        freq = clamp(freq, MIN_FREQ, MAX_FREQ);
        phaseInc_ = freq / sampleRate_;
    }

    void trigger() {
        for (int i = 0; i < MAX_HARMONICS; ++i) {
            harmonicEnvs_[i] = 1.0f;
        }
    }

    float process(float harmonicSpread, float cascadeRate,
                  float wavefold, float chaos, float masterEnv) {

        // LFOs for chaos - faster rates so effect is obvious
        // LFO1: ~5Hz vibrato
        lfoPhase1_ += 5.0f / sampleRate_;
        if (lfoPhase1_ >= 1.0f) lfoPhase1_ -= 1.0f;
        float lfo1 = sinf(lfoPhase1_ * 2.0f * M_PI);

        // LFO2: ~3Hz tremolo
        lfoPhase2_ += 3.0f / sampleRate_;
        if (lfoPhase2_ >= 1.0f) lfoPhase2_ -= 1.0f;
        float lfo2 = sinf(lfoPhase2_ * 2.0f * M_PI);

        float output = 0.0f;

        // SPREAD: Controls how many harmonics play
        // spread=0: only fundamental
        // spread=0.5: 4 harmonics
        // spread=1: all 8 harmonics at equal volume
        int numActive = 1 + static_cast<int>(harmonicSpread * (MAX_HARMONICS - 1));

        for (int i = 0; i < numActive; ++i) {
            int harmonicNum = i + 1;

            // Base amplitude - spread makes higher harmonics LOUDER
            // At spread=0, only fundamental plays
            // At spread=1, all harmonics are equal volume
            float harmonicAmp;
            if (i == 0) {
                harmonicAmp = 1.0f;
            } else {
                // Higher spread = louder upper harmonics
                harmonicAmp = harmonicSpread;
            }

            // CASCADE: Higher harmonics decay faster
            // cascade=0: no decay difference
            // cascade=1: harmonic 8 decays 8x faster than fundamental
            float decayRate = 0.9997f;  // Base decay ~7ms at 44.1kHz
            if (cascadeRate > 0.01f && i > 0) {
                // Faster decay for higher harmonics
                float speedup = 1.0f + cascadeRate * static_cast<float>(i) * 1.5f;
                decayRate = powf(decayRate, speedup);
            }
            harmonicEnvs_[i] *= decayRate;

            // CHAOS: Vibrato (pitch wobble) - very obvious
            // chaos=1 gives ±10% pitch modulation (wide vibrato)
            float vibrato = 1.0f + chaos * lfo1 * 0.10f;

            // CHAOS: Tremolo (volume wobble)
            // chaos=1 gives ±50% amplitude modulation
            float tremolo = 1.0f + chaos * lfo2 * 0.5f;
            if (tremolo < 0.2f) tremolo = 0.2f;

            // Update phase
            float harmonicPhaseInc = phaseInc_ * harmonicNum * vibrato;
            phases_[i] += harmonicPhaseInc;
            if (phases_[i] >= 1.0f) phases_[i] -= 1.0f;

            // Generate sine wave
            float wave = sinf(phases_[i] * 2.0f * M_PI);

            // Combine all amplitude factors
            float amp = harmonicAmp * harmonicEnvs_[i] * tremolo;
            output += wave * amp;
        }

        // Normalize to prevent clipping
        float normFactor = 1.0f + (numActive - 1) * harmonicSpread * 0.5f;
        output /= normFactor;

        // WAVEFOLD: Adds grit and extra harmonics
        if (wavefold > 0.01f) {
            float drive = 1.0f + wavefold * 5.0f;
            float driven = output * drive;
            // Triangle fold
            while (driven > 1.0f || driven < -1.0f) {
                if (driven > 1.0f) driven = 2.0f - driven;
                if (driven < -1.0f) driven = -2.0f - driven;
            }
            output = output * (1.0f - wavefold) + driven * wavefold;
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
    float lfoPhase1_;
    float lfoPhase2_;
};
