#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "ClaudiusEngine.h"
#include "Parameters.h"
#include "Config.h"
#include "Calibration.h"
#include "../hal/AudioOutput.h"
#include "../hal/Gate.h"

extern QueueHandle_t gParamQueue;
extern QueueHandle_t gStatusQueue;

class DspTask {
public:
    void init() {
        audioOut_.init();
        gate_.init();
    }

    void run() {
        ParamMessage params;
        bool hasParams = false;

        // Smoothed values
        float smoothPitch = 0.5f;
        float smoothSpread = 0.5f;
        float smoothCascade = 0.5f;

        unsigned long lastStatusTime = 0;

        while (true) {
            // Read latest parameters (non-blocking, drain queue)
            while (xQueueReceive(gParamQueue, &params, 0) == pdTRUE) {
                hasParams = true;
            }

            if (hasParams) {
                // Update envelope parameters
                engine_.setAttack(params.attack);
                engine_.setDecay(params.decay);
                engine_.setWavefold(params.wavefold);
                engine_.setChaos(params.chaos);

                // Calculate effective pitch from CV2 + Pot2
                float pitchCv = (params.cv2 - 0.5f) * CV_MOD_AMOUNT * 2.0f;
                float pitchPot = params.pot2;
                float rawPitch = std::clamp(pitchPot + pitchCv, 0.0f, 1.0f);

                // Smooth pitch changes
                smoothPitch = smoothPitch * (1.0f - PITCH_SMOOTH_ALPHA)
                            + rawPitch * PITCH_SMOOTH_ALPHA;

                // Map to frequency (exponential for musical pitch)
                float freq = MIN_FREQ * powf(MAX_FREQ / MIN_FREQ, smoothPitch);
                engine_.setFrequency(freq);

                // Calculate effective harmonic spread from CV0 + Pot0
                float spreadRaw = params.pot0 + (params.cv0 - 0.5f) * CV_MOD_AMOUNT * 2.0f;
                smoothSpread = smoothSpread * (1.0f - PARAM_SMOOTH_ALPHA)
                             + std::clamp(spreadRaw, 0.0f, 1.0f) * PARAM_SMOOTH_ALPHA;
                engine_.setHarmonicSpread(smoothSpread);

                // Calculate effective cascade rate from CV1 + Pot1
                float cascadeRaw = params.pot1 + (params.cv1 - 0.5f) * CV_MOD_AMOUNT * 2.0f;
                smoothCascade = smoothCascade * (1.0f - PARAM_SMOOTH_ALPHA)
                              + std::clamp(cascadeRaw, 0.0f, 1.0f) * PARAM_SMOOTH_ALPHA;
                engine_.setCascadeRate(smoothCascade);

                // Handle gate
                engine_.gate(params.gateIn);
            }

            // Generate audio block
            for (int i = 0; i < AUDIO_BLOCK_SIZE; ++i) {
                float sample = engine_.process();
                audioOut_.writeMono(sample);
            }

            // Update gate output
            gate_.setGateOut(engine_.isPlaying());

            // Send status update periodically
            unsigned long now = millis();
            if (now - lastStatusTime > 100) {
                StatusMessage status;
                status.outputLevel = engine_.getOutputLevel();
                status.isPlaying = engine_.isPlaying();
                status.currentFreq = engine_.getFrequency();
                xQueueOverwrite(gStatusQueue, &status);
                lastStatusTime = now;
            }

            // Small yield to prevent watchdog issues
            taskYIELD();
        }
    }

private:
    ClaudiusEngine engine_;
    AudioOutput audioOut_;
    Gate gate_;
};
