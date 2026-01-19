#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "ClaudiusEngine.h"
#include "Parameters.h"
#include "Config.h"
#include "Calibration.h"
#include "Utils.h"
#include "../hal/AudioOutput.h"
#include "../hal/Gate.h"

extern QueueHandle_t gParamQueue;
extern QueueHandle_t gStatusQueue;

class DspTask {
public:
    void init() {
        if (!audioOut_.init()) {
            Serial.println("Audio init failed!");
        }
        gate_.init();
    }

    void run() {
        ParamMessage params{};
        params.pot0 = 0.5f;
        params.pot1 = 0.5f;
        params.pot2 = 0.5f;
        params.cv0 = 0.5f;
        params.cv1 = 0.5f;
        params.cv2 = 0.5f;

        // Smoothed values
        float smoothPitch = 0.5f;
        float smoothSpread = 0.5f;
        float smoothCascade = 0.5f;

        // Audio buffer (stereo interleaved: L, R, L, R, ...)
        uint16_t audioBuffer[AUDIO_BLOCK_SIZE * 2];

        unsigned long lastStatusTime = 0;
        unsigned long lastDebugTime = 0;

        while (true) {
            // Read latest parameters (non-blocking, drain queue)
            while (xQueueReceive(gParamQueue, &params, 0) == pdTRUE) {
            }

            // Update envelope parameters
            engine_.setAttack(params.attack);
            engine_.setDecay(params.decay);
            engine_.setWavefold(params.wavefold);
            engine_.setChaos(params.chaos);

            // PITCH: CV2 + Pot2
            float pitchVal = params.pot2 + (params.cv2 - 0.5f) * 2.0f;
            pitchVal = clamp(pitchVal, 0.0f, 1.0f);
            smoothPitch = smoothPitch * 0.9f + pitchVal * 0.1f;
            float freq = MIN_FREQ * powf(MAX_FREQ / MIN_FREQ, smoothPitch);
            engine_.setFrequency(freq);

            // HARMONIC SPREAD: CV0 + Pot0
            float spreadVal = params.pot0 + (params.cv0 - 0.5f) * 2.0f;
            spreadVal = clamp(spreadVal, 0.0f, 1.0f);
            smoothSpread = smoothSpread * 0.9f + spreadVal * 0.1f;
            engine_.setHarmonicSpread(smoothSpread);

            // CASCADE RATE: CV1 + Pot1
            float cascadeVal = params.pot1 + (params.cv1 - 0.5f) * 2.0f;
            cascadeVal = clamp(cascadeVal, 0.0f, 1.0f);
            smoothCascade = smoothCascade * 0.9f + cascadeVal * 0.1f;
            engine_.setCascadeRate(smoothCascade);

            // Handle gate - also enable drone mode when decay is at max
            bool droneMode = (params.decay > 0.98f);
            engine_.gate(params.gateIn || droneMode);

            // Generate audio block
            for (int i = 0; i < AUDIO_BLOCK_SIZE; ++i) {
                float sample = engine_.process();
                uint16_t dacSample = AudioOutput::floatToSample(sample);
                audioBuffer[i * 2] = dacSample;
                audioBuffer[i * 2 + 1] = dacSample;
            }

            // Write buffer to I2S
            size_t bytesWritten = 0;
            audioOut_.write(audioBuffer, sizeof(audioBuffer), &bytesWritten);

            // Update gate output
            gate_.setGateOut(engine_.isPlaying());

            // Debug output
            unsigned long now = millis();
            if (now - lastDebugTime > 2000) {
                Serial.printf("Spread:%.2f Cascade:%.2f Chaos:%.2f Freq:%.0f\n",
                    smoothSpread, smoothCascade, params.chaos, freq);
                Serial.printf("  CV0:%.2f CV1:%.2f Pot0:%.2f Pot1:%.2f\n",
                    params.cv0, params.cv1, params.pot0, params.pot1);
                lastDebugTime = now;
            }

            // Send status update periodically
            if (now - lastStatusTime > 100) {
                StatusMessage status;
                status.outputLevel = engine_.getOutputLevel();
                status.isPlaying = engine_.isPlaying();
                status.currentFreq = engine_.getFrequency();
                xQueueOverwrite(gStatusQueue, &status);
                lastStatusTime = now;
            }
        }
    }

private:
    ClaudiusEngine engine_;
    AudioOutput audioOut_;
    Gate gate_;
};
