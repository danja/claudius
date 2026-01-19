#pragma once

#include "HarmonicCascade.h"
#include "Envelope.h"
#include "Config.h"
#include "Parameters.h"
#include <algorithm>

// Main synthesis engine for Claudius
// Combines HarmonicCascade oscillator with Envelope

class ClaudiusEngine {
public:
    explicit ClaudiusEngine(float sampleRate = SAMPLE_RATE)
        : oscillator_(sampleRate)
        , envelope_(sampleRate)
        , frequency_(220.0f)
        , harmonicSpread_(0.5f)
        , cascadeRate_(0.5f)
        , wavefold_(0.0f)
        , chaos_(0.0f)
        , gateState_(false)
        , smoothedLevel_(0.0f)
    {
    }

    void setFrequency(float freq) {
        frequency_ = std::clamp(freq, MIN_FREQ, MAX_FREQ);
        oscillator_.setFrequency(frequency_);
    }

    void setAttack(float normalized) {
        envelope_.setAttack(normalized);
    }

    void setDecay(float normalized) {
        envelope_.setDecay(normalized);
    }

    void setHarmonicSpread(float normalized) {
        harmonicSpread_ = std::clamp(normalized, 0.0f, 1.0f);
    }

    void setCascadeRate(float normalized) {
        cascadeRate_ = std::clamp(normalized, 0.0f, 1.0f);
    }

    void setWavefold(float normalized) {
        wavefold_ = std::clamp(normalized, 0.0f, 1.0f);
    }

    void setChaos(float normalized) {
        chaos_ = std::clamp(normalized, 0.0f, 1.0f);
    }

    void gate(bool on) {
        if (on && !gateState_) {
            // Rising edge - trigger oscillator and envelope
            oscillator_.trigger();
            envelope_.trigger();
        } else if (!on && gateState_) {
            // Falling edge - release envelope
            envelope_.release();
        }
        gateState_ = on;
    }

    void noteOn(float freq) {
        setFrequency(freq);
        oscillator_.reset();
        oscillator_.trigger();
        envelope_.trigger();
        gateState_ = true;
    }

    void noteOff() {
        envelope_.release();
        gateState_ = false;
    }

    // Process one sample, returns stereo pair (for now, same output)
    float process() {
        // Get envelope level
        float envLevel = envelope_.process();

        // Generate audio
        float sample = oscillator_.process(
            harmonicSpread_,
            cascadeRate_,
            wavefold_,
            chaos_,
            envLevel
        );

        // Apply master gain
        sample *= MASTER_GAIN;

        // Guard against bad samples
        if (!std::isfinite(sample)) {
            sample = 0.0f;
        }
        sample = std::clamp(sample, -SAMPLE_GUARD, SAMPLE_GUARD);

        // Update smoothed level for metering
        float absSample = fabsf(sample);
        smoothedLevel_ = smoothedLevel_ * 0.999f + absSample * 0.001f;

        return sample;
    }

    bool isPlaying() const {
        return envelope_.isActive();
    }

    float getFrequency() const {
        return frequency_;
    }

    float getOutputLevel() const {
        return std::min(1.0f, smoothedLevel_ * 2.0f);
    }

private:
    HarmonicCascade oscillator_;
    Envelope envelope_;

    float frequency_;
    float harmonicSpread_;
    float cascadeRate_;
    float wavefold_;
    float chaos_;
    bool gateState_;
    float smoothedLevel_;
};
