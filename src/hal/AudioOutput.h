#pragma once

#include <cstddef>
#include <cstdint>
#include <driver/i2s.h>
#include "Config.h"

class AudioOutput {
public:
    bool init() {
        i2s_config_t config{};
        config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN);
        config.sample_rate = SAMPLE_RATE;
        config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
        config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
        config.communication_format = I2S_COMM_FORMAT_STAND_MSB;
        config.intr_alloc_flags = 0;
        config.dma_buf_count = 8;
        config.dma_buf_len = AUDIO_BLOCK_SIZE;
        config.use_apll = false;
        config.tx_desc_auto_clear = true;
        config.fixed_mclk = 0;

        if (i2s_driver_install(I2S_NUM_0, &config, 0, nullptr) != ESP_OK) {
            return false;
        }

        if (i2s_set_pin(I2S_NUM_0, nullptr) != ESP_OK) {
            return false;
        }

        if (i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN) != ESP_OK) {
            return false;
        }

        return true;
    }

    // Write a buffer of samples
    // buffer: array of 16-bit samples (interleaved L/R for stereo)
    // length: size in bytes
    bool write(const uint16_t* buffer, size_t length, size_t* bytesWritten) {
        return i2s_write(I2S_NUM_0, buffer, length, bytesWritten, portMAX_DELAY) == ESP_OK;
    }

    // Convert float sample to DAC format
    // Input: -1.0 to 1.0
    // Output: 16-bit value for I2S DAC
    static uint16_t floatToSample(float sample) {
        // Clamp to valid range
        if (sample < -1.0f) sample = -1.0f;
        if (sample > 1.0f) sample = 1.0f;
        // Convert to unsigned 16-bit (DAC expects unsigned)
        // The internal DAC uses the upper 8 bits
        float unipolar = (sample + 1.0f) * 0.5f;
        return static_cast<uint16_t>(unipolar * 65535.0f);
    }
};
