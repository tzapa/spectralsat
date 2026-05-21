# SpectralSat

4-sávos multiband saturator VST3/AU plugin – JUCE/C++ + CMake.

## Features

- **4 sáv:** Low (0–300 Hz), Low Mid (300–1 kHz), High Mid (1–7 kHz), High (7 kHz+)
- **Parallel saturation:** a delta (harmonics) signal keveredik vissza a dry jelbe
- **Tape-stílusú saturator:** atan soft-knee shaper, természetes hangzás
- **0–12 dB gain** sávonként (toló fader)
- **Q faktor** sávonként a crossover resonance-hoz (forgó knob)
- **4x oversampling** opcionálisan
- Linkwitz–Riley LR4 crossoverek

## Build

### Előfeltételek
- CMake ≥ 3.22
- C++17 képes compiler (Xcode 14+, MSVC 2022, GCC 11+)
- Internetkapcsolat (JUCE FetchContent)

### macOS / Linux

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Windows

```powershell
cmake -B build
cmake --build build --config Release --parallel
```

A lefordított VST3 a `build/SpectralSat_artefacts/Release/VST3/` mappában lesz.

## CI/CD

A `.github/workflows/build.yml` automatikusan buildel macOS-en és Windows-on
minden push után. Az artifacts letölthető a GitHub Actions oldalról.

A macOS builden automatikus `codesign --force --deep --sign -` fut
(ad-hoc signing, azonos a RhythmPanner pipelinenal).

## Testreszabás

| Fájl | Mit módosíts |
|------|-------------|
| `Source/MultibandSaturator.h` | Crossover frekvenciák (`kCross1/2/3`) |
| `Source/MultibandSaturator.cpp` | Saturation algoritmus (`BandSaturator::saturate`) |
| `Source/PluginEditor.cpp` | GUI színek, méretek |
| `CMakeLists.txt` | `COMPANY_NAME`, `PLUGIN_MANUFACTURER_CODE`, `PLUGIN_CODE` |
