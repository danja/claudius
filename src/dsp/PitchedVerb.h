#pragma once

#include <cmath>
#include <cstdint>
#include "Config.h"
#include "Utils.h"

// Pitched verb resonator: comb + allpass tuned by base frequency
// FEEDBACK: controls self-oscillation amount
// DAMP: high-frequency damping in the feedback loop
// MIX: wet/dry mix
// EXCITE: transient burst level on trigger

class PitchedVerb {
public:
    explicit PitchedVerb(float sampleRate = SAMPLE_RATE)
        : sampleRate_(sampleRate)
        , baseFreq_(220.0f)
        , excite_(0.0f)
        , exciteLevel_(0.6f)
        , noiseSeed_(0x12345678u)
        , impulsePending_(false)
        , excitePhase_(0.0f)
        , excitePhaseInc_(0.0f)
        , dcBlocker_(0.0f)
        , dcBlockerPrev_(0.0f)
    {
        reset();
        updateDelays();
    }

    void reset() {
        for (int i = 0; i < kCombCount; ++i) {
            for (int j = 0; j < kMaxCombDelay; ++j) {
                combBuffers_[i][j] = 0.0f;
            }
            combIndex_[i] = 0;
            combFilter_[i] = 0.0f;
        }
        for (int i = 0; i < kAllpassCount; ++i) {
            for (int j = 0; j < kMaxAllpassDelay; ++j) {
                allpassBuffers_[i][j] = 0.0f;
            }
            allpassIndex_[i] = 0;
        }
        dcBlocker_ = 0.0f;
        dcBlockerPrev_ = 0.0f;
    }

    void setFrequency(float freq) {
        float newFreq = clamp(freq, MIN_FREQ, MAX_FREQ);
        // Only update delays if frequency changed significantly (avoid clicks from ADC noise)
        if (fabsf(newFreq - baseFreq_) > 2.0f) {
            baseFreq_ = newFreq;
            excitePhaseInc_ = baseFreq_ / sampleRate_;
            updateDelays();
            // Reset indices and filter states to prevent discontinuities
            for (int i = 0; i < kCombCount; ++i) {
                combIndex_[i] = 0;
                combFilter_[i] = 0.0f;
            }
            for (int i = 0; i < kAllpassCount; ++i) {
                allpassIndex_[i] = 0;
            }
            // Reset DC blocker state
            dcBlocker_ = 0.0f;
            dcBlockerPrev_ = 0.0f;
        }
    }

    void trigger() {
        excite_ = exciteLevel_;
        impulsePending_ = true;
    }

    void setExcite(float normalized) {
        exciteLevel_ = clamp(normalized, 0.0f, 1.0f);
    }

    float process(float feedback, float damp, float mix, float envelope) {
        // Feedback: 0.5 at min (fast decay), up to 0.92 at max (long sustain)
        const float fb = 0.5f + feedback * 0.42f;
        const float dampCoef = clamp(damp, 0.0f, 1.0f);

        // Excitation: impulse + decaying burst (no continuous oscillator)
        float input = 0.0f;

        if (impulsePending_) {
            input += 1.0f + exciteLevel_ * 0.5f;
            impulsePending_ = false;
        }

        if (excite_ > 0.0001f) {
            input += excite_ * (0.8f + exciteLevel_ * 0.4f);
            excite_ *= 0.93f;
        }

        float combSum = 0.0f;
        for (int i = 0; i < kCombCount; ++i) {
            const int delay = combDelay_[i];
            // Ensure index is always in bounds before reading
            int idx = combIndex_[i] % delay;
            combIndex_[i] = idx;
            float delayed = combBuffers_[i][idx];

            // Simple lowpass in feedback path
            float lpCoef = 0.3f + (1.0f - dampCoef) * 0.65f;
            combFilter_[i] += (delayed - combFilter_[i]) * lpCoef;
            float filtered = combFilter_[i];

            float feedbackSignal = filtered * fb;
            float write = input + feedbackSignal;
            write = fastTanh(write);

            combBuffers_[i][idx] = write;
            combIndex_[i] = (idx + 1) % delay;
            combSum += delayed;
        }

        float combOut = combSum * (1.0f / static_cast<float>(kCombCount));

        // Allpass diffusion section
        float diffused = combOut;
        for (int i = 0; i < kAllpassCount; ++i) {
            const int delay = allpassDelay_[i];
            // Ensure index is always in bounds before reading
            int idx = allpassIndex_[i] % delay;
            allpassIndex_[i] = idx;
            float delayed = allpassBuffers_[i][idx];
            const float g = 0.5f;
            float next = -diffused * g + delayed;
            allpassBuffers_[i][idx] = diffused + delayed * g;
            allpassIndex_[i] = (idx + 1) % delay;
            diffused = next;
        }

        // Mix: 0 = pure comb (metallic), 1 = full diffusion (reverb-like)
        float output = combOut * (1.0f - mix) + diffused * mix;

        // Apply envelope and output gain
        output *= envelope * 10.0f;
        return fastTanh(output);
    }

    void getDelayStats(int &comb0, int &comb1, int &comb2, int &comb3, int &ap0, int &ap1) const {
        comb0 = combDelay_[0];
        comb1 = combDelay_[1];
        comb2 = combDelay_[2];
        comb3 = combDelay_[3];
        ap0 = allpassDelay_[0];
        ap1 = allpassDelay_[1];
    }

    float getBaseFreq() const {
        return baseFreq_;
    }

private:
    static constexpr int kCombCount = 4;
    static constexpr int kAllpassCount = 2;
    static constexpr int kMaxCombDelay = 4096;
    static constexpr int kMaxAllpassDelay = 2048;

    void updateDelays() {
        float base = sampleRate_ / baseFreq_;
        int baseDelay = clamp(static_cast<int>(base + 0.5f), 16, kMaxCombDelay - 1);

        const float ratios[kCombCount] = {1.0f, 1.3333f, 1.5f, 2.0f};
        for (int i = 0; i < kCombCount; ++i) {
            int delay = static_cast<int>(baseDelay * ratios[i]);
            combDelay_[i] = clamp(delay, 8, kMaxCombDelay - 1);
        }

        const float apRatios[kAllpassCount] = {0.5f, 0.75f};
        for (int i = 0; i < kAllpassCount; ++i) {
            int delay = static_cast<int>(baseDelay * apRatios[i]);
            allpassDelay_[i] = clamp(delay, 4, kMaxAllpassDelay - 1);
        }
    }

    float sampleRate_;
    float baseFreq_;
    float excite_;
    float exciteLevel_;
    uint32_t noiseSeed_;
    bool impulsePending_;
    float excitePhase_;
    float excitePhaseInc_;
    float dcBlocker_;
    float dcBlockerPrev_;

    float combBuffers_[kCombCount][kMaxCombDelay];
    float combFilter_[kCombCount];
    int combIndex_[kCombCount];
    int combDelay_[kCombCount];

    float allpassBuffers_[kAllpassCount][kMaxAllpassDelay];
    int allpassIndex_[kAllpassCount];
    int allpassDelay_[kAllpassCount];
};
