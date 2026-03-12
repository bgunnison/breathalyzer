# Breathalyzer

Breathalyzer is a VST3 instrument prototype that turns MIDI notes into synthetic breath gestures.

The goal is a playable breath synth, not a foley simulator and not a sample-based vocal effect. MIDI timing shapes when breaths happen, note pitch shifts the mouth and vocal color, note length sustains the gesture, and velocity controls loudness, brightness, and turbulence.

## Intent

- Keep the sound synthetic, smooth, and expressive.
- Let a MIDI clip perform breath phrases like a compact instrument.
- Use a very small control surface instead of a deep editor.
- Favor playable mouth-shape color and breath texture over realism or speech imitation.

Typical use cases:

- ambient breath pulses
- airy rhythmic layers
- stylized vocal-air textures
- cinematic tension beds

## UI

The plugin uses a single-page editor with six horizontal sliders.

- `SHAPE`: shifts the overall mouth and vowel family from tighter, hissier colors toward more open and rounded breath tones.
- `BREATH`: controls how much air and softness is in the sound.
- `VOICE`: blends from mostly airy noise toward more voiced and formant-colored body.
- `RELEASE`: sets the decay after note-off.
- `HUMANIZE`: adds small motion and per-note variation.
- `TONE`: global brightness and warmth.

The UI is intentionally minimal:

- one page
- six primary controls
- no advanced pages
- no numeric value fields beside the sliders

## MIDI Behavior

- Note on starts a breath gesture.
- Note length determines how long the breath is held.
- Note off releases the breath naturally instead of cutting hard.
- Note pitch changes mouth and vocal color rather than behaving like a conventional oscillator pitch.
- Velocity increases pressure, level, turbulence, and brightness.

## Build Requirements

Current local workflow is Windows-focused.

- Windows 10/11
- CMake 3.20 or newer
- Visual Studio 2022 with the C++ desktop toolchain and MSBuild
- Steinberg VST3 SDK checkout
- `VST3_SDK_ROOT` environment variable pointing at that SDK

Example:

```powershell
set VST3_SDK_ROOT=C:\projects\ableplugs\vst3sdk
```

## Build

Quick path:

```powershell
build.bat
```

Manual path:

```powershell
set VST3_SDK_ROOT=C:\projects\ableplugs\vst3sdk
cmake -S vst3 -B build
cmake --build build --config Debug
cmake --build build --config Release
```

Outputs land under:

- `build\VST3\Debug\`
- `build\VST3\Release\`

## Deploy

The repo includes a Windows deploy helper:

```powershell
deploy.bat
```

It copies the built bundles to:

- `C:\ProgramData\vstplugins\DebugBreathalyzer.vst3`
- `C:\ProgramData\vstplugins\Breathalyzer.vst3`

If deployment fails, make sure Ableton Live is closed so the existing plugin folders are not locked.

## Repo Layout

- `project.md`: original product and sound-design brief
- `vst3\`: VST3 source, UI description, and CMake project
- `build.bat`: configure and build Debug/Release
- `deploy.bat`: copy built bundles into the local plugin folder
