#pragma once

#include "HarmonicCascade.h"
#include "OrbitFm.h"
#include "PitchedVerb.h"
#include "Envelope.h"
#include "Config.h"
#include "Parameters.h"
#include "Utils.h"

// Main synthesis engine for Claudius
// Combines HarmonicCascade oscillator with Envelope

class ClaudiusEngine {
public:
    explicit ClaudiusEngine(float sampleRate = SAMPLE_RATE)
        : oscillator_(sampleRate)
        , fmOsc_(sampleRate)
        , verbOsc_(sampleRate)
        , envelope_(sampleRate)
        , frequency_(220.0f)
        , harmonicSpread_(0.5f)
        , cascadeRate_(0.5f)
        , wavefold_(0.0f)
        , chaos_(0.0f)
        , fmIndex_(0.5f)
        , fmRatio_(0.5f)
        , fmFeedback_(0.2f)
        , fmFold_(0.0f)
        , verbFeedback_(0.4f)
        , verbDamp_(0.3f)
        , verbMix_(0.6f)
        , verbExcite_(0.5f)
        , voice_(VoiceType::CASCADE)
        , lastVoice_(VoiceType::CASCADE)
        , gateState_(false)
        , smoothedLevel_(0.0f)
    {
    }

    void setFrequency(float freq) {
        frequency_ = clamp(freq, MIN_FREQ, MAX_FREQ);
        oscillator_.setFrequency(frequency_);
        fmOsc_.setFrequency(frequency_);
        verbOsc_.setFrequency(frequency_);
    }

    void setAttack(float normalized) {
        envelope_.setAttack(normalized);
    }

    void setDecay(float normalized) {
        envelope_.setDecay(normalized);
    }

    void setHarmonicSpread(float normalized) {
        harmonicSpread_ = clamp(normalized, 0.0f, 1.0f);
    }

    void setCascadeRate(float normalized) {
        cascadeRate_ = clamp(normalized, 0.0f, 1.0f);
    }

    void setWavefold(float normalized) {
        wavefold_ = clamp(normalized, 0.0f, 1.0f);
    }

    void setChaos(float normalized) {
        chaos_ = clamp(normalized, 0.0f, 1.0f);
    }

    void setFmIndex(float normalized) {
        fmIndex_ = clamp(normalized, 0.0f, 1.0f);
    }

    void setFmRatio(float normalized) {
        fmRatio_ = clamp(normalized, 0.0f, 1.0f);
    }

    void setFmFeedback(float normalized) {
        fmFeedback_ = clamp(normalized, 0.0f, 1.0f);
    }

    void setFmFold(float normalized) {
        fmFold_ = clamp(normalized, 0.0f, 1.0f);
    }

    void setVoice(VoiceType voice) {
        if (voice != lastVoice_) {
            voice_ = voice;
            if (voice_ == VoiceType::PITCH_VERB && gateState_) {
                verbOsc_.trigger();
            }
            lastVoice_ = voice_;
        }
    }

    void setVerbFeedback(float normalized) {
        verbFeedback_ = clamp(normalized, 0.0f, 1.0f);
    }

    void setVerbDamp(float normalized) {
        verbDamp_ = clamp(normalized, 0.0f, 1.0f);
    }

    void setVerbMix(float normalized) {
        verbMix_ = clamp(normalized, 0.0f, 1.0f);
    }

    void setVerbExcite(float normalized) {
        verbExcite_ = clamp(normalized, 0.0f, 1.0f);
        verbOsc_.setExcite(verbExcite_);
    }

    void gate(bool on) {
        if (on && !gateState_) {
            // Rising edge - trigger oscillator and envelope
            oscillator_.trigger();
            fmOsc_.trigger();
            verbOsc_.trigger();
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
        fmOsc_.reset();
        verbOsc_.reset();
        oscillator_.trigger();
        fmOsc_.trigger();
        verbOsc_.trigger();
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
        float sample = 0.0f;
        if (voice_ == VoiceType::CASCADE) {
            sample = oscillator_.process(
                harmonicSpread_,
                cascadeRate_,
                wavefold_,
                chaos_,
                envLevel
            );
        } else if (voice_ == VoiceType::ORBIT_FM) {
            sample = fmOsc_.process(
                fmIndex_,
                fmRatio_,
                fmFeedback_,
                fmFold_,
                envLevel
            );
        } else {
            sample = verbOsc_.process(
                verbFeedback_,
                verbDamp_,
                verbMix_,
                envLevel
            );
        }

        // Apply master gain
        sample *= MASTER_GAIN;

        // Guard against bad samples
        if (!std::isfinite(sample)) {
            sample = 0.0f;
        }
        sample = clamp(sample, -SAMPLE_GUARD, SAMPLE_GUARD);

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
        float level = smoothedLevel_ * 2.0f;
        return level < 1.0f ? level : 1.0f;
    }

    float getEnvelopeLevel() const {
        return envelope_.getLevel();
    }

    Envelope::Stage getEnvelopeStage() const {
        return envelope_.getStage();
    }

    void getVerbDelayStats(int &comb0, int &comb1, int &comb2, int &comb3, int &ap0, int &ap1) const {
        verbOsc_.getDelayStats(comb0, comb1, comb2, comb3, ap0, ap1);
    }

    float getVerbBaseFreq() const {
        return verbOsc_.getBaseFreq();
    }

private:
    HarmonicCascade oscillator_;
    OrbitFm fmOsc_;
    PitchedVerb verbOsc_;
    Envelope envelope_;

    float frequency_;
    float harmonicSpread_;
    float cascadeRate_;
    float wavefold_;
    float chaos_;
    float fmIndex_;
    float fmRatio_;
    float fmFeedback_;
    float fmFold_;
    float verbFeedback_;
    float verbDamp_;
    float verbMix_;
    float verbExcite_;
    VoiceType voice_;
    VoiceType lastVoice_;
    bool gateState_;
    float smoothedLevel_;
};
