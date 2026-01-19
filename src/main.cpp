// Claudius - Harmonic Cascade Synthesizer
// A Eurorack voice module for ESP32
//
// Features:
// - Harmonic cascade synthesis with up to 8 harmonics
// - Higher harmonics decay faster for evolving timbres
// - Wave folding for additional harmonic content
// - Chaos modulation for organic movement
// - Attack/Decay envelope
// - CV + Knob control for pitch, harmonic spread, and cascade rate
//
// Hardware: ESP32 DevKit V1 with OLED display, encoder, CV inputs, gate I/O

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "Parameters.h"
#include "dsp/DspTask.h"
#include "ui/UiTask.h"

// Inter-core communication queues
QueueHandle_t gParamQueue = nullptr;
QueueHandle_t gStatusQueue = nullptr;

// Task instances
static DspTask dspTask;
static UiTask uiTask;

// Task functions for FreeRTOS
void dspTaskFunc(void* param) {
    dspTask.init();
    dspTask.run();
}

void uiTaskFunc(void* param) {
    uiTask.init();
    uiTask.run();
}

void setup() {
    Serial.begin(115200);
    Serial.println("Claudius - Harmonic Cascade Synthesizer");
    Serial.println("Starting...");

    // Create communication queues
    // Using queue size 1 with overwrite for latest-value semantics
    gParamQueue = xQueueCreate(1, sizeof(ParamMessage));
    gStatusQueue = xQueueCreate(1, sizeof(StatusMessage));

    if (!gParamQueue || !gStatusQueue) {
        Serial.println("Failed to create queues!");
        while (true) delay(1000);
    }

    // Create DSP task on Core 1 (high priority, audio processing)
    xTaskCreatePinnedToCore(
        dspTaskFunc,
        "DSP",
        4096,           // Stack size
        nullptr,        // Parameters
        configMAX_PRIORITIES - 1,  // Highest priority
        nullptr,        // Task handle
        1               // Core 1
    );

    // Create UI task on Core 0 (lower priority, user interface)
    xTaskCreatePinnedToCore(
        uiTaskFunc,
        "UI",
        4096,           // Stack size
        nullptr,        // Parameters
        1,              // Lower priority
        nullptr,        // Task handle
        0               // Core 0
    );

    Serial.println("Tasks started.");
}

void loop() {
    // Main loop not used - tasks handle everything
    vTaskDelay(portMAX_DELAY);
}
