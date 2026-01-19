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

        // Audio buffer (stereo interleaved)
        uint16_t audioBuffer[AUDIO_BLOCK_SIZE * 2];

        unsigned long lastStatusTime = 0;
        unsigned long lastDebugTime = 0;

        while (true) {
            // Read latest parameters
            while (xQueueReceive(gParamQueue, &params, 0) == pdTRUE) {
            }

            // Update envelope parameters
            engine_.setAttack(params.attack);
            engine_.setDecay(params.decay);
            engine_.setWavefold(params.wavefold);
            engine_.setChaos(params.chaos);

            // DIRECT MAPPING - no smoothing, pot is the value
            // Pot0 = Spread (0-1)
            // Pot1 = Cascade (0-1)
            // Pot2 = Pitch (0-1)
            // CV adds/subtracts from pot value

            float spread = params.pot0 + (params.cv0 - 0.5f);
            spread = clamp(spread, 0.0f, 1.0f);
            engine_.setHarmonicSpread(spread);

            float cascade = params.pot1 + (params.cv1 - 0.5f);
            cascade = clamp(cascade, 0.0f, 1.0f);
            engine_.setCascadeRate(cascade);

            float pitch = params.pot2 + (params.cv2 - 0.5f);
            pitch = clamp(pitch, 0.0f, 1.0f);
            float freq = MIN_FREQ * powf(MAX_FREQ / MIN_FREQ, pitch);
            engine_.setFrequency(freq);

            // Drone mode when decay > 98%
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

            // Debug output every 1 second
            unsigned long now = millis();
            if (now - lastDebugTime > 1000) {
                Serial.printf("POT0:%.2f POT1:%.2f POT2:%.2f | Spread:%.2f Cascade:%.2f Freq:%.0f\n",
                    params.pot0, params.pot1, params.pot2, spread, cascade, freq);
                lastDebugTime = now;
            }

            // Send status update
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
