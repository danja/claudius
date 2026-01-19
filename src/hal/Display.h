#pragma once

#include <Wire.h>
#include <SH1106Wire.h>
#include "PinConfig.h"
#include "Parameters.h"

class Display {
public:
    void init() {
        display_.init();
        display_.flipScreenVertically();
        display_.setFont(ArialMT_Plain_10);
        display_.setTextAlignment(TEXT_ALIGN_LEFT);
        clear();
    }

    void clear() {
        display_.clear();
    }

    void showTitle(const char* title) {
        display_.setFont(ArialMT_Plain_16);
        display_.setTextAlignment(TEXT_ALIGN_CENTER);
        display_.drawString(64, 0, title);
    }

    void showParameter(ParamIndex param, float normalizedValue, bool selected) {
        const ParamInfo& info = PARAM_INFO[static_cast<int>(param)];

        // Calculate display value
        float displayVal;
        if (info.isTime) {
            displayVal = info.minDisplay * powf(info.maxDisplay / info.minDisplay, normalizedValue);
        } else {
            displayVal = info.minDisplay + normalizedValue * (info.maxDisplay - info.minDisplay);
        }

        // Format string
        char buf[32];
        if (info.isTime) {
            if (displayVal >= 1000.0f) {
                snprintf(buf, sizeof(buf), "%s: %.1fs", info.name, displayVal / 1000.0f);
            } else {
                snprintf(buf, sizeof(buf), "%s: %.0f%s", info.name, displayVal, info.unit);
            }
        } else {
            snprintf(buf, sizeof(buf), "%s: %.0f%s", info.name, displayVal, info.unit);
        }

        display_.setFont(ArialMT_Plain_10);
        display_.setTextAlignment(TEXT_ALIGN_LEFT);

        int y = 20 + static_cast<int>(param) * 11;

        if (selected) {
            display_.fillRect(0, y - 1, 128, 11);
            display_.setColor(BLACK);
        }

        display_.drawString(2, y, buf);
        display_.setColor(WHITE);
    }

    void showStatus(float freq, float level, bool playing) {
        char buf[32];

        // Show frequency
        display_.setFont(ArialMT_Plain_10);
        display_.setTextAlignment(TEXT_ALIGN_RIGHT);
        snprintf(buf, sizeof(buf), "%.1fHz", freq);
        display_.drawString(126, 54, buf);

        // Show playing indicator
        if (playing) {
            display_.fillCircle(6, 58, 4);
        } else {
            display_.drawCircle(6, 58, 4);
        }

        // Show level bar
        int barWidth = static_cast<int>(level * 40.0f);
        display_.drawRect(20, 56, 42, 6);
        display_.fillRect(21, 57, barWidth, 4);
    }

    void update() {
        display_.display();
    }

private:
    SH1106Wire display_{0x3c, PIN_SDA, PIN_SCL};
};
