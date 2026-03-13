// Copyright (c) 2026 Brian R. Gunnison
// MIT License
#pragma once

#include "../BreathalyzerDefaults.h"
#include "../dasiydsp/FormantVoice.h"

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
        double attack{kDefaultAttack};
    };

    BreathalyzerSynth() {
        prepare(sampleRate_, 0);
    }

    void prepare(double sampleRate, int /*maxBlockSize*/) {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
        leftVoice_.prepare(sampleRate_);
        rightVoice_.prepare(sampleRate_);
        prepareSmoothers();
        reset();
    }

    void reset() {
        leftVoice_.reset();
        rightVoice_.reset();
        noiseStateLeft_ = 0.0;
        noiseStateRight_ = 0.0;
        toneFilterLeft_.reset();
        toneFilterRight_.reset();
        resetSmoothersToParams();
        applySmoothedParams(currentSmoothedParams(), true);
    }

    void setParams(const Params& params) {
        params_ = params;
        shapeSmoother_.setTarget(clamp01(params_.shape));
        breathSmoother_.setTarget(clamp01(params_.breath));
        voiceSmoother_.setTarget(clamp01(params_.voice));
        releaseSmoother_.setTarget(clamp01(params_.release));
        humanizeSmoother_.setTarget(clamp01(params_.humanize));
        toneSmoother_.setTarget(clamp01(params_.tone));
        attackSmoother_.setTarget(clamp01(params_.attack));
    }

    void noteOn(int pitch, double velocity, int32_t noteId, int16_t channel) {
        leftVoice_.noteOn(pitch, velocity, noteId, channel);
        rightVoice_.noteOn(pitch, velocity, noteId, channel);
    }

    void noteOff(int pitch, int32_t noteId, int16_t channel) {
        leftVoice_.noteOff(pitch, noteId, channel);
        rightVoice_.noteOff(pitch, noteId, channel);
    }

    void allNotesOff() {
        leftVoice_.allNotesOff();
        rightVoice_.allNotesOff();
    }

    void processSample(double& left, double& right) {
        const SmoothedParams p = advanceSmoothedParams();
        applySmoothedParams(p);

        const double tonalBlend = std::pow(p.voice, 2.6);

        const double voicedLeft = leftVoice_.processSample();
        const double voicedRight = rightVoice_.processSample();
        const double envLeft = leftVoice_.lastEnvelope();
        const double envRight = rightVoice_.lastEnvelope();

        const double whiteLeft = randBipolar(randomLeft_);
        const double whiteRight = randBipolar(randomRight_);
        const double noiseFollow = 0.025 + 0.085 * p.breath;
        noiseStateLeft_ += noiseFollow * (whiteLeft - noiseStateLeft_);
        noiseStateRight_ += noiseFollow * (whiteRight - noiseStateRight_);

        const double airyLeft = (whiteLeft - (0.82 - 0.20 * p.tone) * noiseStateLeft_) * (0.42 + 0.58 * p.breath) * envLeft;
        const double airyRight = (whiteRight - (0.82 - 0.20 * p.tone) * noiseStateRight_) * (0.42 + 0.58 * p.breath) * envRight;

        const double width = 1.0 + 0.2 * p.humanize;
        const double sourceLeft = lerp(airyLeft, voicedLeft * width, tonalBlend);
        const double sourceRight = lerp(airyRight, voicedRight * width, tonalBlend);

        const double cutoff = 450.0 + std::pow(p.tone, 1.45) * 10800.0 + (1.0 - tonalBlend) * 1600.0;
        toneFilterLeft_.setLowpass(sampleRate_, cutoff);
        toneFilterRight_.setLowpass(sampleRate_, cutoff * (1.0 + 0.02 * p.humanize));

        const double gain = 0.95 + 0.20 * p.breath;
        left = std::tanh(toneFilterLeft_.process(sourceLeft) * gain);
        right = std::tanh(toneFilterRight_.process(sourceRight) * gain);
    }

    bool isSilent() const {
        return leftVoice_.isSilent() && rightVoice_.isSilent();
    }

private:
    struct OnePole {
        double a0{1.0};
        double b1{0.0};
        double z{0.0};

        void setLowpass(double sampleRate, double cutoff) {
            cutoff = std::clamp(cutoff, 20.0, sampleRate * 0.45);
            const double x = std::exp(-2.0 * 3.14159265358979323846 * cutoff / sampleRate);
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

    struct FourPoleLowpass {
        std::array<OnePole, 4> stages{};

        void setLowpass(double sampleRate, double cutoff) {
            for (auto& stage : stages) {
                stage.setLowpass(sampleRate, cutoff);
            }
        }

        double process(double input) {
            for (auto& stage : stages) {
                input = stage.process(input);
            }
            return input;
        }

        void reset() {
            for (auto& stage : stages) {
                stage.reset();
            }
        }
    };

    struct SmoothedValue {
        double current{0.0};
        double target{0.0};
        double coeff{0.0};

        void prepare(double sampleRate, double timeSeconds) {
            const double time = std::max(timeSeconds, 1.0e-4);
            coeff = std::exp(-1.0 / (time * sampleRate));
        }

        void reset(double value) {
            current = value;
            target = value;
        }

        void setTarget(double value) {
            target = value;
        }

        double next() {
            current = target + coeff * (current - target);
            return current;
        }
    };

    struct SmoothedParams {
        double shape{0.0};
        double breath{0.0};
        double voice{0.0};
        double release{0.0};
        double humanize{0.0};
        double tone{0.0};
        double attack{0.0};
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

    void prepareSmoothers() {
        constexpr double kControlSmoothSeconds = 0.020;
        shapeSmoother_.prepare(sampleRate_, kControlSmoothSeconds);
        breathSmoother_.prepare(sampleRate_, kControlSmoothSeconds);
        voiceSmoother_.prepare(sampleRate_, kControlSmoothSeconds);
        releaseSmoother_.prepare(sampleRate_, kControlSmoothSeconds);
        humanizeSmoother_.prepare(sampleRate_, kControlSmoothSeconds);
        toneSmoother_.prepare(sampleRate_, kControlSmoothSeconds);
        attackSmoother_.prepare(sampleRate_, kControlSmoothSeconds);
    }

    void resetSmoothersToParams() {
        shapeSmoother_.reset(clamp01(params_.shape));
        breathSmoother_.reset(clamp01(params_.breath));
        voiceSmoother_.reset(clamp01(params_.voice));
        releaseSmoother_.reset(clamp01(params_.release));
        humanizeSmoother_.reset(clamp01(params_.humanize));
        toneSmoother_.reset(clamp01(params_.tone));
        attackSmoother_.reset(clamp01(params_.attack));
    }

    SmoothedParams currentSmoothedParams() const {
        return {
            shapeSmoother_.current,
            breathSmoother_.current,
            voiceSmoother_.current,
            releaseSmoother_.current,
            humanizeSmoother_.current,
            toneSmoother_.current,
            attackSmoother_.current,
        };
    }

    SmoothedParams advanceSmoothedParams() {
        return {
            shapeSmoother_.next(),
            breathSmoother_.next(),
            voiceSmoother_.next(),
            releaseSmoother_.next(),
            humanizeSmoother_.next(),
            toneSmoother_.next(),
            attackSmoother_.next(),
        };
    }

    void applySmoothedParams(const SmoothedParams& p, bool force = false) {
        const float attack = static_cast<float>(attackSecondsFromNormalized(p.attack));
        const float release = static_cast<float>(releaseSecondsFromNormalized(p.release));
        const float decay = static_cast<float>(0.110 + 0.310 * (1.0 - p.breath) + 0.120 * (1.0 - p.tone));
        const float sustain = static_cast<float>(0.28 + 0.62 * p.voice);
        const float formantBase = static_cast<float>(460.0 + 1700.0 * p.shape + 420.0 * p.tone);
        const float stereoSpread = static_cast<float>(1.06 + 0.12 * p.humanize);
        const float basePhase = static_cast<float>(0.04 + 0.62 * p.tone);
        const float phaseSpread = static_cast<float>(0.03 + 0.12 * p.humanize + 0.08 * p.breath);

        if (force || std::abs(attack - lastAttackSeconds_) > 1.0e-5f) {
            leftVoice_.setAttackSeconds(attack);
            rightVoice_.setAttackSeconds(attack);
            lastAttackSeconds_ = attack;
        }
        if (force || std::abs(decay - lastDecaySeconds_) > 1.0e-5f) {
            leftVoice_.setDecaySeconds(decay);
            rightVoice_.setDecaySeconds(decay);
            lastDecaySeconds_ = decay;
        }
        if (force || std::abs(sustain - lastSustainLevel_) > 1.0e-5f) {
            leftVoice_.setSustainLevel(sustain);
            rightVoice_.setSustainLevel(sustain);
            lastSustainLevel_ = sustain;
        }
        if (force || std::abs(release - lastReleaseSeconds_) > 1.0e-5f) {
            leftVoice_.setReleaseSeconds(release);
            rightVoice_.setReleaseSeconds(release);
            lastReleaseSeconds_ = release;
        }

        const float leftFormant = formantBase / stereoSpread;
        const float rightFormant = formantBase * stereoSpread;
        if (force || std::abs(leftFormant - lastLeftFormantHz_) > 1.0e-4f) {
            leftVoice_.setFormantFreq(leftFormant);
            lastLeftFormantHz_ = leftFormant;
        }
        if (force || std::abs(rightFormant - lastRightFormantHz_) > 1.0e-4f) {
            rightVoice_.setFormantFreq(rightFormant);
            lastRightFormantHz_ = rightFormant;
        }

        const float leftPhase = std::max(0.0f, basePhase - phaseSpread);
        const float rightPhase = basePhase + phaseSpread;
        if (force || std::abs(leftPhase - lastLeftPhase_) > 1.0e-5f) {
            leftVoice_.setPhaseShift(leftPhase);
            lastLeftPhase_ = leftPhase;
        }
        if (force || std::abs(rightPhase - lastRightPhase_) > 1.0e-5f) {
            rightVoice_.setPhaseShift(rightPhase);
            lastRightPhase_ = rightPhase;
        }
    }

    Params params_{};
    breathalyzer::daisydsp::FormantVoice leftVoice_{};
    breathalyzer::daisydsp::FormantVoice rightVoice_{};
    FourPoleLowpass toneFilterLeft_{};
    FourPoleLowpass toneFilterRight_{};
    SmoothedValue shapeSmoother_{};
    SmoothedValue breathSmoother_{};
    SmoothedValue voiceSmoother_{};
    SmoothedValue releaseSmoother_{};
    SmoothedValue humanizeSmoother_{};
    SmoothedValue toneSmoother_{};
    SmoothedValue attackSmoother_{};
    double sampleRate_{44100.0};
    double noiseStateLeft_{0.0};
    double noiseStateRight_{0.0};
    uint32_t randomLeft_{0x13572468u};
    uint32_t randomRight_{0x24681357u};
    float lastAttackSeconds_{-1.0f};
    float lastDecaySeconds_{-1.0f};
    float lastSustainLevel_{-1.0f};
    float lastReleaseSeconds_{-1.0f};
    float lastLeftFormantHz_{-1.0f};
    float lastRightFormantHz_{-1.0f};
    float lastLeftPhase_{-1.0f};
    float lastRightPhase_{-1.0f};
};

} // namespace breathalyzer::dsp
