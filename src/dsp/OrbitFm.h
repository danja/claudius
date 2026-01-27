#pragma once

#include <cmath>
#include "Config.h"
#include "Utils.h"
#include "Calibration.h"

// Orbit FM: 2-operator FM with feedback and folding
// INDEX: Modulation depth
// RATIO: Modulator frequency ratio
// FEEDBACK: Modulator feedback amount
// FOLD: Post-FM wave folding

class OrbitFm {
public:
    explicit OrbitFm(float sampleRate = SAMPLE_RATE)
        : sampleRate_(sampleRate)
        , baseFreq_(220.0f)
        , carrierPhase_(0.0f)
        , modPhase_(0.0f)
        , lastMod_(0.0f)
    {
    }

    void reset() {
        carrierPhase_ = 0.0f;
        modPhase_ = 0.0f;
        lastMod_ = 0.0f;
    }

    void setFrequency(float freq) {
        baseFreq_ = clamp(freq, MIN_FREQ, MAX_FREQ);
    }

    void trigger() {
        reset();
    }

    float process(float index, float ratio, float feedback, float fold, float envelope) {
        float ratioVal = 0.25f + ratio * 5.75f;  // 0.25x to 6x
        float indexVal = expMap(index, 0.15f, 8.0f);
        float feedbackVal = clamp(feedback, 0.0f, 1.0f) * 0.9f;

        float modPhaseInc = (baseFreq_ * ratioVal) / sampleRate_;
        modPhase_ += modPhaseInc;
        if (modPhase_ >= 1.0f) modPhase_ -= 1.0f;

        float modInput = modPhase_ + lastMod_ * feedbackVal;
        float modSignal = sinf(modInput * 2.0f * M_PI);
        lastMod_ = modSignal;

        float carrierPhaseInc = baseFreq_ / sampleRate_;
        carrierPhase_ += carrierPhaseInc;
        if (carrierPhase_ >= 1.0f) carrierPhase_ -= 1.0f;

        float phase = carrierPhase_ + modSignal * indexVal * 0.2f;
        float output = sinf(phase * 2.0f * M_PI);

        if (fold > 0.01f) {
            float drive = 1.0f + fold * 4.0f;
            float folded = sinf(output * drive * M_PI);
            output = output * (1.0f - fold) + folded * fold;
        }

        output *= envelope;
        return fastTanh(output);
    }

private:
    float sampleRate_;
    float baseFreq_;
    float carrierPhase_;
    float modPhase_;
    float lastMod_;
};
