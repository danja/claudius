#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "Parameters.h"
#include "Config.h"
#include "Calibration.h"
#include "Utils.h"
#include "../hal/Adc.h"
#include "../hal/Encoder.h"
#include "../hal/Display.h"
#include "../hal/Gate.h"

extern QueueHandle_t gParamQueue;
extern QueueHandle_t gStatusQueue;

class UiTask {
public:
    void init() {
        adc_.init();
        encoder_.init();
        if (!display_.init()) {
            Serial.println("Display init failed!");
        }
        gate_.init();

        // Initialize parameter values
        params_.attack = 0.1f;
        params_.decay = 0.5f;
        params_.wavefold = 0.0f;
        params_.chaos = 0.0f;

        selectedParam_ = ParamIndex::ATTACK;
    }

    void run() {
        unsigned long lastAdcRead = 0;
        unsigned long lastDisplayUpdate = 0;

        // Smoothed ADC values
        float smoothCv0 = 0.5f, smoothCv1 = 0.5f, smoothCv2 = 0.5f;
        float smoothPot0 = 0.5f, smoothPot1 = 0.5f, smoothPot2 = 0.5f;

        StatusMessage status = {0.0f, false, 220.0f};

        while (true) {
            unsigned long now = millis();

            // Read encoder
            if (encoder_.readButtonPress()) {
                // Cycle through parameters
                int idx = static_cast<int>(selectedParam_) + 1;
                if (idx >= static_cast<int>(ParamIndex::NUM_PARAMS)) {
                    idx = 0;
                }
                selectedParam_ = static_cast<ParamIndex>(idx);
            }

            int8_t rotation = encoder_.readRotation();
            if (rotation != 0) {
                adjustParameter(selectedParam_, rotation);
            }

            // Read ADCs at interval
            if (now - lastAdcRead >= ADC_READ_INTERVAL_MS) {
                // Read and normalize CVs
                float cv0 = normalizeAdc(adc_.readCv0(), CAL_CV0);
                float cv1 = normalizeAdc(adc_.readCv1(), CAL_CV1);
                float cv2 = normalizeAdc(adc_.readCv2(), CAL_CV2);

                // Read and normalize pots
                float pot0 = normalizeAdc(adc_.readPot0(), CAL_POT0);
                float pot1 = normalizeAdc(adc_.readPot1(), CAL_POT1);
                float pot2 = normalizeAdc(adc_.readPot2(), CAL_POT2);

                // Smooth values
                constexpr float alpha = 0.2f;
                smoothCv0 = smoothCv0 * (1.0f - alpha) + cv0 * alpha;
                smoothCv1 = smoothCv1 * (1.0f - alpha) + cv1 * alpha;
                smoothCv2 = smoothCv2 * (1.0f - alpha) + cv2 * alpha;
                smoothPot0 = smoothPot0 * (1.0f - alpha) + pot0 * alpha;
                smoothPot1 = smoothPot1 * (1.0f - alpha) + pot1 * alpha;
                smoothPot2 = smoothPot2 * (1.0f - alpha) + pot2 * alpha;

                // Update params
                params_.cv0 = smoothCv0;
                params_.cv1 = smoothCv1;
                params_.cv2 = smoothCv2;
                params_.pot0 = smoothPot0;
                params_.pot1 = smoothPot1;
                params_.pot2 = smoothPot2;

                // Read gate
                params_.gateIn = gate_.readGateIn();

                // Send to DSP
                xQueueOverwrite(gParamQueue, &params_);

                lastAdcRead = now;
            }

            // Read status from DSP
            xQueueReceive(gStatusQueue, &status, 0);

            // Update display at interval
            if (now - lastDisplayUpdate >= DISPLAY_UPDATE_MS) {
                display_.clear();
                display_.showTitle("CLAUDIUS");

                // Show all parameters
                for (int i = 0; i < static_cast<int>(ParamIndex::NUM_PARAMS); ++i) {
                    float value = getParamValue(static_cast<ParamIndex>(i));
                    bool selected = (static_cast<ParamIndex>(i) == selectedParam_);
                    display_.showParameter(static_cast<ParamIndex>(i), value, selected);
                }

                // Show status
                display_.showStatus(status.currentFreq, status.outputLevel, status.isPlaying);

                display_.update();
                lastDisplayUpdate = now;
            }

            // Small delay to prevent tight loop
            vTaskDelay(1);
        }
    }

private:
    void adjustParameter(ParamIndex param, int8_t delta) {
        float step = 0.05f;  // 5% per click for faster adjustment
        float* value = nullptr;

        switch (param) {
            case ParamIndex::ATTACK:   value = &params_.attack; break;
            case ParamIndex::DECAY:    value = &params_.decay; break;
            case ParamIndex::WAVEFOLD: value = &params_.wavefold; break;
            case ParamIndex::CHAOS:    value = &params_.chaos; break;
            default: return;
        }

        if (value) {
            *value = clamp(*value + delta * step, 0.0f, 1.0f);
        }
    }

    float getParamValue(ParamIndex param) const {
        switch (param) {
            case ParamIndex::ATTACK:   return params_.attack;
            case ParamIndex::DECAY:    return params_.decay;
            case ParamIndex::WAVEFOLD: return params_.wavefold;
            case ParamIndex::CHAOS:    return params_.chaos;
            default: return 0.0f;
        }
    }

    Adc adc_;
    Encoder encoder_;
    Display display_;
    Gate gate_;

    ParamMessage params_;
    ParamIndex selectedParam_;
};
