#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "PinConfig.h"

class Display {
public:
    bool init() {
        Wire.begin(PIN_SDA, PIN_SCL);
        if (!display_.begin(0x3C, true)) {
            return false;
        }
        display_.clearDisplay();
        display_.setTextSize(1);
        display_.setTextColor(SH110X_WHITE);
        display_.display();
        return true;
    }

    void clear() {
        display_.clearDisplay();
    }

    void showTitle(const char* title) {
        display_.setTextSize(2);
        display_.setCursor(10, 0);
        display_.print(title);
        display_.setTextSize(1);
    }

    void showMenuLine(const char* text, int row, bool selected) {
        int y = row * 10;
        if (selected) {
            display_.fillRect(0, y, 128, 10, SH110X_WHITE);
            display_.setTextColor(SH110X_BLACK);
        } else {
            display_.setTextColor(SH110X_WHITE);
        }
        display_.setCursor(2, y + 1);
        display_.print(text);
        display_.setTextColor(SH110X_WHITE);
    }

    void showStatus(float freq, float level, bool playing) {
        char buf[32];

        // Status line at bottom (y=56)
        // Show playing indicator
        if (playing) {
            display_.fillCircle(6, 60, 3, SH110X_WHITE);
        } else {
            display_.drawCircle(6, 60, 3, SH110X_WHITE);
        }

        // Show level bar
        int barWidth = static_cast<int>(level * 36.0f);
        display_.drawRect(14, 57, 38, 6, SH110X_WHITE);
        if (barWidth > 0) {
            display_.fillRect(15, 58, barWidth, 4, SH110X_WHITE);
        }

        // Show frequency
        snprintf(buf, sizeof(buf), "%.0fHz", freq);
        display_.setCursor(56, 56);
        display_.print(buf);
    }

    void update() {
        display_.display();
    }

private:
    Adafruit_SH1106G display_{128, 64, &Wire};
};
