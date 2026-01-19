#pragma once

#include <cmath>
#include <algorithm>
#include "Config.h"
#include "Calibration.h"

// Attack-Decay envelope for the voice module

class Envelope {
public:
    enum class Stage {
        IDLE,
        ATTACK,
        DECAY,
        SUSTAIN
    };

    explicit Envelope(float sampleRate = SAMPLE_RATE)
        : sampleRate_(sampleRate)
        , stage_(Stage::IDLE)
        , level_(0.0f)
        , attackRate_(0.01f)
        , decayRate_(0.001f)
    {
    }

    void setAttack(float normalizedAttack) {
        // Map normalized 0-1 to attack time in seconds
        float attackTime = expMap(normalizedAttack, MIN_ATTACK, MAX_ATTACK);
        attackRate_ = 1.0f / (attackTime * sampleRate_);
    }

    void setDecay(float normalizedDecay) {
        // Map normalized 0-1 to decay time in seconds
        float decayTime = expMap(normalizedDecay, MIN_DECAY, MAX_DECAY);
        // Use exponential decay coefficient
        // We want to reach ~0.001 (-60dB) in decayTime seconds
        decayCoeff_ = powf(0.001f, 1.0f / (decayTime * sampleRate_));
    }

    void trigger() {
        stage_ = Stage::ATTACK;
    }

    void release() {
        if (stage_ != Stage::IDLE) {
            stage_ = Stage::DECAY;
        }
    }

    void gate(bool on) {
        if (on) {
            trigger();
        } else {
            release();
        }
    }

    float process() {
        switch (stage_) {
            case Stage::IDLE:
                level_ = 0.0f;
                break;

            case Stage::ATTACK:
                level_ += attackRate_;
                if (level_ >= 1.0f) {
                    level_ = 1.0f;
                    stage_ = Stage::SUSTAIN;
                }
                break;

            case Stage::SUSTAIN:
                // Hold at maximum while gate is held
                level_ = 1.0f;
                break;

            case Stage::DECAY:
                level_ *= decayCoeff_;
                if (level_ < 0.001f) {
                    level_ = 0.0f;
                    stage_ = Stage::IDLE;
                }
                break;
        }

        return level_;
    }

    bool isActive() const {
        return stage_ != Stage::IDLE;
    }

    Stage getStage() const {
        return stage_;
    }

    float getLevel() const {
        return level_;
    }

private:
    float sampleRate_;
    Stage stage_;
    float level_;
    float attackRate_;
    float decayRate_;
    float decayCoeff_;
};
