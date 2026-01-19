# Claudius - AI Assistant Guide

## Project Overview

Claudius is a Eurorack synthesizer voice module running on ESP32. It implements **Harmonic Cascade Synthesis** where multiple harmonics have independent decay rates.

## Architecture

### Dual-Core Design

- **Core 0**: UI task - reads inputs, updates display, sends parameters
- **Core 1**: DSP task - audio generation, must never block

Communication via FreeRTOS queues:
- `gParamQueue`: UI → DSP (parameters)
- `gStatusQueue`: DSP → UI (level, frequency, playing state)

### Directory Structure

```
src/
├── main.cpp           # Entry point, task creation
├── hal/               # Hardware abstraction
│   ├── Adc.h          # CV and pot reading
│   ├── AudioOutput.h  # I2C DAC output
│   ├── Display.h      # SH1106 OLED
│   ├── Encoder.h      # Rotary encoder
│   └── Gate.h         # Gate in/out
├── dsp/               # Digital signal processing
│   ├── DspTask.h      # Audio loop, parameter handling
│   ├── ClaudiusEngine.h  # Main synthesis orchestrator
│   ├── HarmonicCascade.h # Core synthesis algorithm
│   └── Envelope.h     # AR envelope generator
└── ui/
    └── UiTask.h       # Display, encoder, ADC polling

include/
├── Config.h           # Sample rate, timing constants
├── PinConfig.h        # ESP32 GPIO assignments
├── Parameters.h       # Message structs, param metadata
└── Calibration.h      # ADC normalization
```

### Key Classes

**ClaudiusEngine** (`src/dsp/ClaudiusEngine.h`)
- Owns HarmonicCascade and Envelope
- `process()` returns one audio sample
- `gate(bool)` triggers/releases envelope
- Parameters: frequency, attack, decay, wavefold, chaos, harmonicSpread, cascadeRate

**HarmonicCascade** (`src/dsp/HarmonicCascade.h`)
- 8 harmonics with independent phases and decay multipliers
- `trigger()` resets all harmonic envelopes
- `process(freq, spread, cascadeRate, wavefold, chaos)` generates sample
- Uses Lorenz attractor for chaos modulation

**Envelope** (`src/dsp/Envelope.h`)
- Attack-Decay envelope with exponential curves
- States: IDLE, ATTACK, DECAY
- `gate(bool)` to trigger/release
- `process()` returns envelope value 0-1

## Audio Signal Flow

```
Gate In → Envelope Trigger
                ↓
HarmonicCascade.process()
    ├── 8 harmonic oscillators (sine)
    ├── Per-harmonic decay (cascade)
    ├── Wave folder
    └── Chaos modulation (Lorenz)
                ↓
         × Envelope
                ↓
         × Master Gain
                ↓
         Soft clip (tanh)
                ↓
         DAC Output
```

## Parameter Routing

| Param | CV | Knob | Range |
|-------|-----|------|-------|
| Harmonic Spread | CV0 | Pot0 | 1-8 harmonics |
| Cascade Rate | CV1 | Pot1 | 0-1 (higher = faster high-harmonic decay) |
| Pitch | CV2 | Pot2 | 27.5-880 Hz (exponential) |
| Attack | encoder | - | 1-2000ms |
| Decay | encoder | - | 10-8000ms |
| Wavefold | encoder | - | 0-100% |
| Chaos | encoder | - | 0-100% |

## Common Modifications

### Adding a new encoder parameter

1. Add to `ParamIndex` enum in `Parameters.h`
2. Add metadata to `PARAM_INFO[]` array
3. Add field to `ParamMessage` struct
4. Handle in `UiTask::updateParams()`
5. Add setter in `ClaudiusEngine`

### Changing harmonic count

Edit `NUM_HARMONICS` in `HarmonicCascade.h`. More harmonics = richer sound but higher CPU.

### Adjusting pitch range

Edit `MIN_FREQ` and `MAX_FREQ` in `Config.h`.

## Build Commands

```bash
pio run                 # Build
pio run -t upload       # Upload to ESP32
pio device monitor      # Serial monitor
```

## Hardware Pins (PinConfig.h)

- **I2C**: SDA=21, SCL=22
- **Encoder**: CLK=34, DT=35, SW=16
- **CV**: CV0=36, CV1=39, CV2=32
- **Pots**: Pot0=4, Pot1=27, Pot2=33
- **Gate**: In=17, Out=5

## Notes

- All DSP runs at 44.1kHz sample rate
- Audio block size is 64 samples
- CV inputs are inverted in hardware (handled in Calibration.h)
- Envelope attack=0 AND decay=0 enables drone mode (continuous output)
- Wave folder uses `sin(x * PI)` for smooth folding
