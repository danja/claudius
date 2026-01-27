# Claudius - AI Assistant Guide

## Project Overview

Claudius is a Eurorack synthesizer voice module running on ESP32. It now supports three voice algorithms (Cascade, Orbit FM, PitchVerb) selectable via the menu system.

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
│   ├── OrbitFm.h       # 2-operator FM voice
│   ├── PitchedVerb.h   # Tuned resonator voice
│   └── Envelope.h     # AR envelope generator
└── ui/
    └── UiTask.h       # Display, encoder, ADC polling

include/
├── Config.h           # Sample rate, timing constants
├── PinConfig.h        # ESP32 GPIO assignments
├── Parameters.h       # Message structs, voice selection
└── Calibration.h      # ADC normalization
```

### Key Classes

**ClaudiusEngine** (`src/dsp/ClaudiusEngine.h`)
- Owns HarmonicCascade, OrbitFm, PitchedVerb, and Envelope
- `process()` returns one audio sample
- `gate(bool)` triggers/releases envelope
- Parameters: frequency, attack, decay, plus per-voice controls and voice selection

**HarmonicCascade** (`src/dsp/HarmonicCascade.h`)
- 8 harmonics with independent phases and decay multipliers
- `trigger()` resets all harmonic envelopes
- `process(spread, cascadeRate, wavefold, chaos, envelope)` generates sample
- Uses Lorenz attractor for chaos modulation

**OrbitFm** (`src/dsp/OrbitFm.h`)
- 2-operator FM voice
- Parameters: index, ratio, feedback, fold

**PitchedVerb** (`src/dsp/PitchedVerb.h`)
- Tuned comb/allpass resonator voice
- Parameters: feedback, damp, mix, excite

**Envelope** (`src/dsp/Envelope.h`)
- Attack-Decay envelope with exponential curves
- States: IDLE, ATTACK, DECAY
- `gate(bool)` to trigger/release
- `process()` returns envelope value 0-1

## Audio Signal Flow

```
Gate In → Envelope Trigger
                ↓
Voice Select
    ├── Cascade (additive harmonics + wavefold + chaos)
    ├── Orbit FM (2-op FM + feedback + fold)
    └── PitchVerb (tuned comb/allpass resonator)
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

Menu system follows `docs/menu-system.md`:
- Title line (row 0): rotate to change page, click to advance selection
- Encoder click cycles items; rotation edits selected item

Pages:
- **VOICE**: select Cascade / Orbit FM / PitchVerb
- **SHAPE**: per-voice parameters
  - Cascade: Wavefold, Chaos
  - Orbit FM: Feedback, Fold
  - PitchVerb: Mix, Excite
- **ENV**: Attack, Decay

Pot/CV mapping:
- Pot0/Pot1: voice-specific timbre controls (Cascade: Spread/Cascade, Orbit FM: Index/Ratio, PitchVerb: Feedback/Damp)
- Pot2 + CV2: pitch (exponential, 27.5–880 Hz)
- CV0/CV1 currently unused (reserved)

## Common Modifications

### Adding a new menu parameter

1. Add field to `ParamMessage` in `Parameters.h`
2. Update menu handling in `src/ui/UiTask.h`
3. Add setter/usage in `ClaudiusEngine` and `DspTask`

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
