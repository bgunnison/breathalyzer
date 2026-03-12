# Breathalyzer

## Goal

Build a simple audio plugin prototype called **Breathalyzer** that synthesizes expressive breath sounds driven by MIDI notes in a DAW.

This is a **pure synthesizer**, not a Foley simulator.

The plugin should create breath-like gestures where:
- each MIDI note triggers or sustains a breath event
- note pitch selects a tonal mouth/vocal shape
- note velocity controls loudness and turbulence
- the result feels expressive with very few controls

## Core Idea

A MIDI clip in the DAW controls the breathing performance.

Breathalyzer interprets MIDI musically rather than as a conventional pitched instrument:

- **Note timing** controls when breaths happen
- **Note length** controls breath duration or sustain
- **Note pitch** controls the tonal mouth shape
- **Note velocity** controls loudness and turbulence intensity

The plugin should feel like a playable breath instrument.

## Sound Model

The sound should be based on:
- synthetic air noise
- smooth breath envelopes
- mouth-shape coloration
- optional faint tonal body

Do not include:
- lip smacks
- clicks
- saliva sounds
- consonants
- spoken syllables
- samples

The result should stay synthetic, smooth, and continuous.

## MIDI Behavior

### Note On
A note starts a breath gesture.

### Note Length
The note duration should shape how long the breath is held or sustained.

### Note Off
The note release should end the breath naturally rather than cutting off abruptly.

### Note Pitch
Pitch does **not** need to behave like a normal instrument oscillator.

Instead, note pitch selects or shifts the mouth/vocal shape, moving between breath colors such as:

- more closed, narrow, hiss-like
- more open, warm, airy
- more “hee”
- more “ha”
- more “ho”

This should be a continuous expressive mapping, not hard-switched phonemes.

### Note Velocity
Velocity controls:
- output level
- breath pressure
- turbulence
- brightness / urgency

Low velocity:
- soft
- smooth
- quieter
- less turbulent

High velocity:
- louder
- breathier
- more turbulent
- more urgent

## Main Expressive Axis

The main expressive tone should come from **mouth openness / vocal shape**.

One end:
- mouth almost closed
- hiss-like
- narrow
- tight
- brighter and more constricted

Other end:
- mouth more open
- “ha” / “ho” quality
- rounder
- warmer
- more open and breathy

Pitch should let the user play across this range naturally.

## Minimal User Controls

Keep the control set very small.

### 1. Shape
Controls the overall tonal family or range of mouth-shape behavior.

This sets the general character of the playable note mapping.

Examples:
- tighter / hiss-oriented
- neutral
- more open / warm

### 2. Breath
Controls the baseline breath amount.

This influences:
- air presence
- softness vs density
- how strongly the patch reads as breath

### 3. Voice
Controls the amount of faint tonal human presence mixed into the breath.

At minimum:
- low = mostly air / hiss
- higher = more “hee / ha / ho” body

This should remain subtle and embedded in the breath.

### 4. Release
Controls how the breath decays after note-off.

Short:
- clipped
- tight
- rhythmic

Long:
- soft trailing exhale
- more natural release

### 5. Humanize
Adds small natural variation.

Should affect:
- envelope variation
- slight spectral movement
- subtle instability
- variation between repeated notes

### 6. Tone
A simple global brightness / warmth control.

This helps tune the patch toward:
- darker, warmer breath
- brighter, hissier breath

## Performance Expectations

A user should be able to write a MIDI clip and get phrases like:
- soft repeating hiss breaths
- open airy exhale pulses
- more tonal “hee” patterns
- rounded “ho” breathing textures
- dynamic breath accents from velocity

Breathalyzer should work well for:
- ambient pulsing breath textures
- emotional rhythmic layers
- stylized vocal-air instruments
- cinematic tension beds

## Sound Character Requirements

The plugin should not sound like:
- plain filtered white noise
- whispered speech
- vocal imitation with words
- a realistic recorded person
- a wind machine

It should sound like:
- a synthetic human-air instrument
- breath with playable mouth shape
- expressive air with tonal color

## Success Criteria

The prototype is successful if:

1. MIDI notes clearly drive the breath phrases
2. note pitch produces useful “hee / ha / ho” style tonal variation
3. velocity clearly affects loudness and turbulence
4. repeated MIDI notes create musically usable rhythmic breathing
5. the sound remains synthetic and clean
6. a small set of controls is enough to shape the instrument

## UI Expectations

Keep the UI very simple:
- one page
- six controls
- no advanced editor
- clear labels
- instrument-like feel

## Design Principle

Favor:
- expressive MIDI playability
- smooth synthetic breath tone
- simple mapping
- clear musical response

Over:
- realism
- large control count
- detailed anatomical simulation
- speech-like articulation

## Final Instruction

Implement the smallest possible playable prototype that proves this idea:

**Breathalyzer is a MIDI-driven synthetic breath instrument where notes perform rhythmic breaths, pitch changes the mouth/vocal color, and velocity controls loudness and turbulence.**