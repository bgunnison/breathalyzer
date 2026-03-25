# Breathalyzer

Breathalyzer is a VST3 instrument that turns MIDI notes into playable breath-and-voice gestures. It is not a sample player and not a speech synthesizer. The design goal is a compact expressive instrument whose mouth color, noise, growl, and vowel motion can all be performed from a small control surface.

## Intent

- Keep the sound synthetic enough to stay musical, but realistic enough to read as breath and utterance.
- Let MIDI notes behave like short performed exhales, phrases, and vocalized air bursts.
- Use a small single-page editor with performance-oriented labels instead of a deep synthesis panel.
- Treat vowel motion as a core part of the sound rather than a static filter color.

Typical uses:

- airy melodic phrases
- whispered rhythmic beds
- creature or tension textures
- stylized vocal-breath layers

## UI

The current editor is a single page with 13 sliders, one checkbox, and an always-spinning logo in the upper left.

The visible labels map to the internal parameters like this:

- `SHAPE`: overall mouth color and base formant positioning.
- `UTTERANCE`: chooses which neighboring vowel pair the utterance LFO moves through. Low settings stay near `A-E`; higher settings move further through `E-I`, `I-O`, and `O-U`.
- `VOLUME` under `BREATH`: internal `BREATH`; controls airy level, turbulence amount, and brightness lift in the noise path.
- `VOLUME` under `VOICE`: internal `VOICE`; blends from mostly breath/noise toward the voiced branch.
- `ATTACK`: note onset time. This affects the voiced envelopes directly, and the breath path follows those same envelopes.
- `GROWL` under `VOICE`: voiced-branch growl character.
- `ANGER` under `VOICE`: voiced-branch growl wet/dry amount.
- `RELEASE`: note-off decay time.
- `HUMANIZE`: slow stereo and spectral drift, formant spread, and left/right asymmetry.
- `TONE`: overall brightness through the shared four-pole lowpass stages.
- `UTTERANCE RATE`: internal `VOICE SPEED`; LFO rate for vowel motion.
- `GROWL` under `BREATH`: noise-branch growl character.
- `ANGER` under `BREATH`: noise-branch growl wet/dry amount.
- `UTTERANCE SYNC`: if enabled, the utterance LFO restarts on each note-on. If disabled, the LFO free-runs across notes.

Notes about the editor:

- The labels are intentionally performance-oriented rather than engineering-oriented.
- The two `VOLUME` sliders are different controls: one is breath amount, one is voice blend.
- The two `ANGER` sliders are the voiced and noise growl intensities.

## MIDI Behavior

- Note on starts the gesture.
- Note off releases it naturally through the shared envelope.
- Note pitch influences mouth and formant color more than conventional oscillator pitch.
- Velocity affects pressure, brightness, and perceived breath energy.
- If `UTTERANCE SYNC` is on, note-on also restarts the utterance LFO phase.

## DSP Summary

At a high level:

- a Daisy-based `FormantVoice` engine generates the voiced body
- a white-noise turbulence path generates the airy branch
- both branches pass through Songbird-derived A/E/I/O/U formant filters
- `UTTERANCE RATE` moves the vowel morph over time
- `UTTERANCE` selects which vowel region the morph operates in
- each branch has its own growl processor and wet/dry intensity
- the two branches are blended with the `VOICE` control and soft-clipped at the output

The breath and voice branches intentionally share the same utterance motion so they read like one mouth shape rather than two unrelated sound sources.

## Build Requirements

Current local workflow is Windows-focused.

- Windows 10/11
- CMake 3.20 or newer
- Visual Studio 2022 with the Desktop C++ workload and MSBuild
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

If deployment fails, close Ableton Live first so the installed plugin bundle is not locked.

## Release Zip

Use:

```powershell
release.bat
```

This creates `Breathalyzer.vst3.zip` in the repo root from `build\VST3\Release\Breathalyzer.vst3`.

## Repo Layout

- `project.md`: original product and sound-design brief
- `README.md`: project overview, UI, and build notes
- `dspflow.md`: current signal-flow summary
- `vst3\`: VST3 project, UI description, and resources
- `vst3\src\dasiydsp\`: vendored Daisy-based voiced generator code
- `vst3\src\WeCore\`: vendored Songbird/WE-Core formant filter subset
- `build.bat`: configure and build Debug/Release
- `deploy.bat`: copy built bundles into the local plugin folder
- `release.bat`: create the release zip from the Release bundle
