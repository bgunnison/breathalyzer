// Copyright (c) 2026 Brian R. Gunnison
// MIT License
#pragma once

#include "../BreathalyzerDefaults.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace breathalyzer::dsp {

class BreathalyzerSynth {
public:
    struct Params {
        double shape{kDefaultShape};
        double breath{kDefaultBreath};
        double voice{kDefaultVoice};
        double release{kDefaultRelease};
        double humanize{kDefaultHumanize};
        double tone{kDefaultTone};
    };

    void prepare(double sampleRate, int /*maxBlockSize*/) {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
        reset();
    }

    void reset() {
        activeVoiceCount_ = 0;
        for (auto& voice : voices_) {
            voice = Voice{};
        }
    }

    void setParams(const Params& params) {
        params_ = params;
    }

    void noteOn(int pitch, double velocity, int32_t noteId, int16_t channel) {
        velocity = clamp01(velocity);
        Voice* voice = findVoiceForNoteOn(pitch, noteId, channel);
        if (!voice) {
            return;
        }

        const double seedA = rand01(randomState_);
        const double seedB = rand01(randomState_);
        const double seedC = rand01(randomState_);
        const double seedD = rand01(randomState_);

        voice->active = true;
        voice->held = true;
        voice->pitch = pitch;
        voice->noteId = noteId;
        voice->channel = channel;
        voice->velocity = velocity;
        voice->env = 0.0;
        voice->attackSeconds = 0.006 + 0.028 * (1.0 - velocity) + 0.02 * (1.0 - params_.breath);
        voice->phase = 2.0 * kPi * seedA;
        voice->phase2 = 2.0 * kPi * seedB;
        voice->driftPhase = 2.0 * kPi * seedC;
        voice->driftRate = 0.13 + 0.52 * seedD;
        voice->pan = (seedA * 2.0 - 1.0) * 0.18 * params_.humanize;
        voice->ampVariance = (seedB * 2.0 - 1.0);
        voice->opennessOffset = (seedC * 2.0 - 1.0) * 0.12;
        voice->brightnessOffset = (seedD * 2.0 - 1.0);
        voice->widthOffset = (rand01(randomState_) * 2.0 - 1.0);
        voice->noiseState = 0.0;
        voice->random = randomState_ ^ static_cast<uint32_t>(pitch * 7919 + noteId * 17 + channel * 131);
        voice->toneFilter.reset();
        voice->formant1.reset();
        voice->formant2.reset();
        voice->formant3.reset();
    }

    void noteOff(int pitch, int32_t noteId, int16_t channel) {
        for (auto& voice : voices_) {
            if (!voice.active) {
                continue;
            }
            const bool noteIdMatch = (noteId >= 0 && voice.noteId >= 0 && voice.noteId == noteId);
            const bool pitchMatch = (voice.pitch == pitch && voice.channel == channel);
            if (noteIdMatch || pitchMatch) {
                voice.held = false;
            }
        }
    }

    void allNotesOff() {
        for (auto& voice : voices_) {
            if (voice.active) {
                voice.held = false;
            }
        }
    }

    void processSample(double& left, double& right) {
        left = 0.0;
        right = 0.0;
        activeVoiceCount_ = 0;

        for (auto& voice : voices_) {
            if (!voice.active) {
                continue;
            }
            renderVoice(voice, left, right);
            if (voice.active) {
                ++activeVoiceCount_;
            }
        }

        if (activeVoiceCount_ > 0) {
            const double norm = 0.42 / std::sqrt(static_cast<double>(activeVoiceCount_));
            left = std::tanh(left * norm * 1.7);
            right = std::tanh(right * norm * 1.7);
        }
    }

    bool isSilent() const {
        return activeVoiceCount_ == 0;
    }

private:
    static constexpr double kPi = 3.14159265358979323846;
    static constexpr int kMaxVoices = 12;

    struct OnePole {
        double a0{1.0};
        double b1{0.0};
        double z{0.0};

        void setLowpass(double sampleRate, double cutoff) {
            cutoff = std::clamp(cutoff, 20.0, sampleRate * 0.45);
            const double x = std::exp(-2.0 * kPi * cutoff / sampleRate);
            a0 = 1.0 - x;
            b1 = x;
        }

        double process(double input) {
            z = a0 * input + b1 * z;
            return z;
        }

        void reset() {
            z = 0.0;
        }
    };

    struct Resonator {
        double c1{0.0};
        double c2{0.0};
        double gain{1.0};
        double y1{0.0};
        double y2{0.0};

        void set(double sampleRate, double frequency, double bandwidth) {
            frequency = std::clamp(frequency, 40.0, sampleRate * 0.45);
            bandwidth = std::clamp(bandwidth, 30.0, sampleRate * 0.25);
            const double radius = std::exp(-kPi * bandwidth / sampleRate);
            const double theta = 2.0 * kPi * frequency / sampleRate;
            c1 = 2.0 * radius * std::cos(theta);
            c2 = -(radius * radius);
            gain = 1.0 - radius;
        }

        double process(double input) {
            const double y = gain * input + c1 * y1 + c2 * y2;
            y2 = y1;
            y1 = y;
            return y;
        }

        void reset() {
            y1 = 0.0;
            y2 = 0.0;
        }
    };

    struct FormantSet {
        double f1;
        double f2;
        double f3;
    };

    struct Voice {
        bool active{false};
        bool held{false};
        int pitch{60};
        int32_t noteId{-1};
        int16_t channel{0};
        double velocity{0.0};
        double env{0.0};
        double attackSeconds{0.01};
        double phase{0.0};
        double phase2{0.0};
        double driftPhase{0.0};
        double driftRate{0.25};
        double pan{0.0};
        double ampVariance{0.0};
        double opennessOffset{0.0};
        double brightnessOffset{0.0};
        double widthOffset{0.0};
        double noiseState{0.0};
        uint32_t random{0x9E3779B9u};
        OnePole toneFilter{};
        Resonator formant1{};
        Resonator formant2{};
        Resonator formant3{};
    };

    static double clamp01(double value) {
        return std::clamp(value, 0.0, 1.0);
    }

    static double lerp(double a, double b, double t) {
        return a + (b - a) * t;
    }

    static double rand01(uint32_t& state) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<double>(state & 0x00FFFFFFu) / static_cast<double>(0x01000000u);
    }

    static double randBipolar(uint32_t& state) {
        return rand01(state) * 2.0 - 1.0;
    }

    static double envelopeCoeff(double seconds, double sampleRate) {
        seconds = std::max(seconds, 0.001);
        return std::exp(-1.0 / (seconds * sampleRate));
    }

    Voice* findVoiceForNoteOn(int pitch, int32_t noteId, int16_t channel) {
        Voice* releasedCandidate = nullptr;
        double releasedLevel = 2.0;
        Voice* quietest = &voices_[0];
        double quietestLevel = 2.0;

        for (auto& voice : voices_) {
            if (!voice.active) {
                return &voice;
            }
            if (voice.noteId >= 0 && noteId >= 0 && voice.noteId == noteId && voice.channel == channel) {
                return &voice;
            }
            if (voice.pitch == pitch && voice.channel == channel) {
                return &voice;
            }
            if (!voice.held && voice.env < releasedLevel) {
                releasedLevel = voice.env;
                releasedCandidate = &voice;
            }
            if (voice.env < quietestLevel) {
                quietestLevel = voice.env;
                quietest = &voice;
            }
        }

        return releasedCandidate ? releasedCandidate : quietest;
    }

    FormantSet formantsForMorph(double morph) const {
        const FormantSet tight{320.0, 2750.0, 3900.0};
        const FormantSet airy{760.0, 1850.0, 2900.0};
        const FormantSet open{980.0, 1450.0, 2450.0};
        const FormantSet round{520.0, 940.0, 2100.0};

        if (morph < 0.33) {
            const double t = morph / 0.33;
            return {lerp(tight.f1, airy.f1, t), lerp(tight.f2, airy.f2, t), lerp(tight.f3, airy.f3, t)};
        }
        if (morph < 0.66) {
            const double t = (morph - 0.33) / 0.33;
            return {lerp(airy.f1, open.f1, t), lerp(airy.f2, open.f2, t), lerp(airy.f3, open.f3, t)};
        }
        const double t = (morph - 0.66) / 0.34;
        return {lerp(open.f1, round.f1, t), lerp(open.f2, round.f2, t), lerp(open.f3, round.f3, t)};
    }

    void renderVoice(Voice& voice, double& left, double& right) {
        const double humanize = clamp01(params_.humanize);
        const double breath = clamp01(params_.breath);
        const double releaseSeconds = releaseSecondsFromNormalized(params_.release) * (1.0 + 0.2 * humanize * voice.ampVariance);
        const double coeff = voice.held ? envelopeCoeff(voice.attackSeconds, sampleRate_)
                                        : envelopeCoeff(releaseSeconds, sampleRate_);
        const double target = voice.held ? 1.0 : 0.0;
        voice.env = target + coeff * (voice.env - target);
        if (!voice.held && voice.env < 1.0e-4) {
            voice.active = false;
            return;
        }

        voice.driftPhase += (2.0 * kPi * voice.driftRate) / sampleRate_;
        if (voice.driftPhase >= 2.0 * kPi) {
            voice.driftPhase -= 2.0 * kPi;
        }
        const double drift = std::sin(voice.driftPhase) + 0.5 * std::sin(voice.driftPhase * 0.37 + voice.phase2 * 0.13);

        const double noteNorm = clamp01((static_cast<double>(voice.pitch) - 36.0) / 48.0);
        const double range = 0.7 + 0.6 * clamp01(params_.shape);
        const double shapeLift = (params_.shape - 0.5) * 0.55;
        const double morph = clamp01(0.5 + (noteNorm - 0.5) * range + shapeLift + voice.opennessOffset * humanize + 0.03 * humanize * drift);
        const FormantSet formants = formantsForMorph(morph);

        const double brightness = clamp01(params_.tone + 0.14 * humanize * voice.brightnessOffset + 0.05 * (voice.velocity - 0.5));
        const double voiceAmount = clamp01(params_.voice);
        const double tonalBlend = std::pow(voiceAmount, 2.4);
        const double pressure = 0.18 + 0.82 * voice.velocity;

        const double white = randBipolar(voice.random);
        const double noiseFollow = 0.02 + 0.08 * (0.3 + breath * 0.7);
        voice.noiseState += noiseFollow * (white - voice.noiseState);
        const double turbulence = white - (0.82 - 0.24 * brightness) * voice.noiseState;

        const double bodyFreq = std::clamp(95.0 * std::pow(2.0, (static_cast<double>(voice.pitch) - 60.0) / 28.0), 70.0, 320.0);
        const double wobble = 1.0 + 0.004 * humanize * drift;
        voice.phase += 2.0 * kPi * bodyFreq * wobble / sampleRate_;
        voice.phase2 += 2.0 * kPi * bodyFreq * 2.01 / sampleRate_;
        if (voice.phase >= 2.0 * kPi) {
            voice.phase -= 2.0 * kPi;
        }
        if (voice.phase2 >= 2.0 * kPi) {
            voice.phase2 -= 2.0 * kPi;
        }
        const double body = std::sin(voice.phase) + 0.25 * std::sin(voice.phase2 + 0.35 * std::sin(voice.phase));

        const double widthScale = 1.0 + 0.2 * humanize * voice.widthOffset;
        const double baseWidth = (90.0 + 260.0 * (0.3 + breath * 0.7)) * widthScale;
        voice.formant1.set(sampleRate_, formants.f1 * (1.0 + 0.02 * humanize * drift), baseWidth * (0.75 + 0.45 * morph));
        voice.formant2.set(sampleRate_, formants.f2 * (1.0 + 0.018 * humanize * drift), baseWidth * (0.95 + 0.2 * (1.0 - morph)));
        voice.formant3.set(sampleRate_, formants.f3 * (1.0 + 0.015 * humanize * drift), baseWidth * 1.3);

        const double noiseCore = 0.62 * turbulence + 0.38 * white;
        const double excitation = turbulence * (0.18 + 0.42 * breath + 0.18 * pressure) * (0.12 + 0.88 * tonalBlend)
                                + body * (0.03 + 0.39 * tonalBlend);
        const double f1 = voice.formant1.process(excitation);
        const double f2 = voice.formant2.process(excitation);
        const double f3 = voice.formant3.process(excitation);
        const double shaped = 0.95 * f1 + 0.6 * f2 + 0.35 * f3;
        const double airyNoise = noiseCore * (0.52 + 0.42 * breath + 0.12 * brightness + 0.10 * pressure);
        const double voicedTexture = shaped
                                   + turbulence * (0.05 + 0.15 * (1.0 - morph) + 0.12 * brightness)
                                   + body * (0.01 + 0.15 * tonalBlend);

        voice.toneFilter.setLowpass(sampleRate_, 1400.0 + std::pow(brightness, 1.35) * 7400.0 + (1.0 - tonalBlend) * 2400.0);
        double sample = voice.toneFilter.process(lerp(airyNoise, voicedTexture, tonalBlend));
        sample *= voice.env * pressure * (0.3 + 0.7 * breath) * (0.92 + 0.18 * humanize * voice.ampVariance);

        const double pan = std::clamp(voice.pan, -0.95, 0.95);
        const double leftGain = std::sqrt(0.5 * (1.0 - pan));
        const double rightGain = std::sqrt(0.5 * (1.0 + pan));
        left += sample * leftGain;
        right += sample * rightGain;
    }

    std::array<Voice, kMaxVoices> voices_{};
    Params params_{};
    double sampleRate_{44100.0};
    uint32_t randomState_{0x13572468u};
    int activeVoiceCount_{0};
};

} // namespace breathalyzer::dsp
