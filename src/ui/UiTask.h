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
        params_.fmFeedback = 0.2f;
        params_.fmFold = 0.0f;
        params_.verbMix = 0.6f;
        params_.verbExcite = 0.5f;
        params_.voice = static_cast<uint8_t>(VoiceType::CASCADE);
        params_.cvPitchOffset = 0.0f;
        params_.cvPitchScale = 1.0f;

        currentPage_ = MenuPage::VOICE;
        selectedItem_ = 0;
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
                handleButtonPress();
            }

            int8_t rotation = encoder_.readRotation();
            if (rotation != 0) {
                handleRotation(rotation);
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

                char line[32];
                formatTitleLine(line, sizeof(line));
                display_.showMenuLine(line, 0, selectedItem_ == 0);

                int itemCount = getPageItemCount(currentPage_);
                for (int i = 0; i < itemCount; ++i) {
                    formatMenuItem(currentPage_, i, line, sizeof(line));
                    display_.showMenuLine(line, i + 1, selectedItem_ == (i + 1));
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
    enum class MenuPage : uint8_t {
        VOICE = 0,
        SHAPE,
        ENV,
        PITCH,
        NUM_PAGES
    };

    void handleButtonPress() {
        int itemCount = getPageItemCount(currentPage_);
        selectedItem_++;
        if (selectedItem_ > itemCount) {
            selectedItem_ = 0;
        }
    }

    void handleRotation(int8_t delta) {
        if (selectedItem_ == 0) {
            int pageCount = static_cast<int>(MenuPage::NUM_PAGES);
            int next = static_cast<int>(currentPage_) + (delta > 0 ? 1 : -1);
            if (next < 0) next = pageCount - 1;
            if (next >= pageCount) next = 0;
            currentPage_ = static_cast<MenuPage>(next);
            return;
        }

        adjustMenuItem(currentPage_, selectedItem_ - 1, delta);
    }

    int getPageItemCount(MenuPage page) const {
        switch (page) {
            case MenuPage::VOICE: return 1;
            case MenuPage::SHAPE: return 2;
            case MenuPage::ENV: return 2;
            case MenuPage::PITCH: return 2;
            default: return 0;
        }
    }

    void adjustMenuItem(MenuPage page, int itemIndex, int8_t delta) {
        constexpr float kStep = 0.04f;
        float step = kStep * static_cast<float>(delta);
        VoiceType voice = static_cast<VoiceType>(params_.voice);

        switch (page) {
            case MenuPage::VOICE:
                if (itemIndex == 0) {
                    int voices = static_cast<int>(VoiceType::NUM_VOICES);
                    int next = static_cast<int>(params_.voice) + (delta > 0 ? 1 : -1);
                    if (next < 0) next = voices - 1;
                    if (next >= voices) next = 0;
                    params_.voice = static_cast<uint8_t>(next);
                }
                break;
            case MenuPage::SHAPE:
                if (voice == VoiceType::CASCADE) {
                    if (itemIndex == 0) {
                        params_.wavefold = clamp(params_.wavefold + step, 0.0f, 1.0f);
                    } else if (itemIndex == 1) {
                        params_.chaos = clamp(params_.chaos + step, 0.0f, 1.0f);
                    }
                } else if (voice == VoiceType::ORBIT_FM) {
                    if (itemIndex == 0) {
                        params_.fmFeedback = clamp(params_.fmFeedback + step, 0.0f, 1.0f);
                    } else if (itemIndex == 1) {
                        params_.fmFold = clamp(params_.fmFold + step, 0.0f, 1.0f);
                    }
                } else {
                    if (itemIndex == 0) {
                        params_.verbMix = clamp(params_.verbMix + step, 0.0f, 1.0f);
                    } else if (itemIndex == 1) {
                        params_.verbExcite = clamp(params_.verbExcite + step, 0.0f, 1.0f);
                    }
                }
                break;
            case MenuPage::ENV:
                if (itemIndex == 0) {
                    params_.attack = clamp(params_.attack + step, 0.0f, 1.0f);
                } else if (itemIndex == 1) {
                    params_.decay = clamp(params_.decay + step, 0.0f, 1.0f);
                }
                break;
            case MenuPage::PITCH:
                if (itemIndex == 0) {
                    params_.cvPitchOffset = clamp(params_.cvPitchOffset + step, -1.0f, 1.0f);
                } else if (itemIndex == 1) {
                    params_.cvPitchScale = clamp(params_.cvPitchScale + step, 0.0f, 2.0f);
                }
                break;
            default:
                break;
        }
    }

    void formatTitleLine(char* out, size_t size) const {
        const char* title = "MENU";
        switch (currentPage_) {
            case MenuPage::VOICE: title = "VOICE"; break;
            case MenuPage::SHAPE: title = "SHAPE"; break;
            case MenuPage::ENV: title = "ENV"; break;
            case MenuPage::PITCH: title = "PITCH CV"; break;
            default: break;
        }
        snprintf(out, size, "%s < >", title);
    }

    void formatMenuItem(MenuPage page, int itemIndex, char* out, size_t size) const {
        VoiceType voice = static_cast<VoiceType>(params_.voice);
        switch (page) {
            case MenuPage::VOICE:
                if (itemIndex == 0) {
                    const char* voiceName = (voice == VoiceType::CASCADE)
                        ? "Cascade"
                        : (voice == VoiceType::ORBIT_FM ? "Orbit FM" : "PitchVerb");
                    snprintf(out, size, "Voice: %s", voiceName);
                }
                break;
            case MenuPage::SHAPE:
                if (voice == VoiceType::CASCADE) {
                    if (itemIndex == 0) {
                        formatPercentLine("Wavefold", params_.wavefold, out, size);
                    } else if (itemIndex == 1) {
                        formatPercentLine("Chaos", params_.chaos, out, size);
                    }
                } else if (voice == VoiceType::ORBIT_FM) {
                    if (itemIndex == 0) {
                        formatPercentLine("Feedback", params_.fmFeedback, out, size);
                    } else if (itemIndex == 1) {
                        formatPercentLine("Fold", params_.fmFold, out, size);
                    }
                } else {
                    if (itemIndex == 0) {
                        formatPercentLine("Mix", params_.verbMix, out, size);
                    } else if (itemIndex == 1) {
                        formatPercentLine("Excite", params_.verbExcite, out, size);
                    }
                }
                break;
            case MenuPage::ENV:
                if (itemIndex == 0) {
                    formatTimeLine("Attack", params_.attack, 1.0f, 2000.0f, out, size);
                } else if (itemIndex == 1) {
                    formatTimeLine("Decay", params_.decay, 10.0f, 8000.0f, out, size);
                }
                break;
            case MenuPage::PITCH:
                if (itemIndex == 0) {
                    float offsetPercent = params_.cvPitchOffset * 100.0f;
                    snprintf(out, size, "Offset: %+.0f%%", offsetPercent);
                } else if (itemIndex == 1) {
                    float scalePercent = params_.cvPitchScale * 100.0f;
                    snprintf(out, size, "Scale: %.0f%%", scalePercent);
                }
                break;
            default:
                snprintf(out, size, "");
                break;
        }
    }

    void formatPercentLine(const char* name, float normalized, char* out, size_t size) const {
        float value = linMap(normalized, 0.0f, 100.0f);
        snprintf(out, size, "%s: %.0f%%", name, value);
    }

    void formatTimeLine(const char* name, float normalized, float minMs, float maxMs, char* out, size_t size) const {
        float value = expMap(normalized, minMs, maxMs);
        if (value >= 1000.0f) {
            snprintf(out, size, "%s: %.1fs", name, value / 1000.0f);
        } else {
            snprintf(out, size, "%s: %.0fms", name, value);
        }
    }

    Adc adc_;
    Encoder encoder_;
    Display display_;
    Gate gate_;

    ParamMessage params_;
    MenuPage currentPage_;
    int selectedItem_;
};
