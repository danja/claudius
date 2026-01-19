# Claudius

A Eurorack synthesizer voice module for ESP32 featuring **Harmonic Cascade Synthesis** - a technique where higher harmonics decay faster than lower ones, creating naturally evolving timbres.

## Sound Character

Claudius generates up to 8 harmonics, each with its own decay envelope. When a note triggers, all harmonics sound at full volume, then progressively fade - with higher harmonics disappearing first. This mimics the acoustic behavior of plucked strings and struck metal, producing sounds that start bright and mellow over time.

Additional timbral features:
- **Wave Folding** - adds edge and complexity by folding the waveform back on itself
- **Chaos Modulation** - subtle pitch and amplitude variation from a Lorenz attractor for organic, living textures

## Hardware

Same platform as disyn-esp32:
- ESP32 DevKit V1
- SH1106 128x64 OLED display (I2C)
- Rotary encoder with push button
- MCP4725 DAC (I2C) for audio output
- 3x CV inputs (0-5V)
- 3x potentiometers
- Gate in/out

## Controls

### CV + Knob Inputs

| Input | Control |
|-------|---------|
| CV0 + Pot0 | **Harmonic Spread** - Number of active harmonics (1-8) |
| CV1 + Pot1 | **Cascade Rate** - How fast higher harmonics decay relative to lower |
| CV2 + Pot2 | **Pitch** - Fundamental frequency (27.5Hz - 880Hz, 5 octaves) |

### Encoder Menu

Rotate to select, press to edit:
- **Attack** - 1ms to 2s
- **Decay** - 10ms to 8s
- **Wavefold** - 0-100% fold intensity
- **Chaos** - 0-100% modulation depth

### Gate

- **Gate In** - Triggers envelope on rising edge
- **Gate Out** - High while voice is sounding

## Building

```bash
pio run
pio run -t upload
```

## Sound Design Tips

- **Plucks/Keys**: Short attack, medium decay, high cascade rate
- **Bells**: Zero attack, long decay, medium cascade, some wavefold
- **Pads**: Slow attack, very long decay, low cascade, add chaos
- **Percussive**: Zero attack, short decay, max cascade, high wavefold
- **Drones**: Set attack and decay to 0 for continuous output, modulate spread with CV

## License

MIT
