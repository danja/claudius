# Claudius - Harmonic Cascade Synthesizer

## Overview

Claudius is a Eurorack voice module based on the ESP32. It features a unique **Harmonic Cascade** synthesis algorithm where multiple harmonics are generated simultaneously, but each harmonic has its own decay envelope - with higher harmonics decaying faster than lower ones.

This creates naturally evolving timbres that start bright and complex, then mellow into simpler, warmer tones. Perfect for plucked sounds, percussion, and evolving pads.

## Synthesis Concept

### Harmonic Cascade
- Generates up to 8 harmonics of the fundamental frequency
- Each harmonic has an independent amplitude envelope
- Higher harmonics decay faster based on the **Cascade Rate** parameter
- Creates organic, naturally evolving timbres

### Wave Folding
- Adds additional harmonic content through wave folding
- Folds the waveform back when it exceeds thresholds
- Creates rich, buzzy tones at higher settings

### Chaos Modulation
- Uses a Lorenz-like chaotic oscillator for modulation
- Adds subtle pitch and amplitude variations
- Creates organic, living textures

## Hardware

Same hardware as disyn-esp32:

* Controller: ESP32 DevKit V1 DOIT module
* Display: SH1106 based 1.3" I2C OLED
* Rotary Controller with Pushbutton Switch
* Digital Input: GateIn from buffer
* Analog Inputs: CV0, CV1, CV2 from buffers
* Analog Inputs: Pot0, Pot1, Pot2 from potentiometers
* Digital Output: GateOut to buffer
* Analog Outputs: DAC1, DAC2 to buffers

## Controls

### CV & Knob Inputs

| Input | Function |
|-------|----------|
| CV0 + Pot0 | Harmonic Spread - Controls how many harmonics are active and their relative amplitudes |
| CV1 + Pot1 | Cascade Rate - How much faster higher harmonics decay |
| CV2 + Pot2 | Pitch - Fundamental frequency (27.5Hz to 880Hz) |

### Encoder Parameters

Press the encoder button to cycle through parameters, rotate to adjust:

1. **Attack** (1ms - 2s) - Envelope attack time
2. **Decay** (10ms - 8s) - Envelope decay time
3. **Wavefold** (0-100%) - Wave folding amount
4. **Chaos** (0-100%) - Chaos modulation depth

### Gate I/O

- **Gate In**: Triggers the envelope and harmonic cascade
- **Gate Out**: Active (inverted) while the voice is playing

## Sound Design Tips

### Plucked Sounds
- Short attack (1-10ms)
- Medium decay (200-500ms)
- High cascade rate (70-100%)
- Some harmonic spread (40-60%)

### Percussion
- Zero attack
- Short decay (50-150ms)
- Maximum cascade rate
- High wavefold for metallic tones

### Evolving Pads
- Slow attack (100-500ms)
- Long decay (2-8s)
- Low cascade rate (10-30%)
- Add chaos for movement

### Buzzy Leads
- Short attack
- Medium decay
- High wavefold (50-80%)
- Full harmonic spread

## Architecture

Dual-core design:
- **Core 0**: User interface (display, encoder, ADC reading)
- **Core 1**: Audio DSP (synthesis, envelope, audio output)

Communication via FreeRTOS queues with latest-value semantics.

## Building

```bash
cd /home/danny/github/claudius
pio run
pio run -t upload
```
