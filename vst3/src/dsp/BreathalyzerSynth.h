// Copyright (c) 2026 Brian R. Gunnison
// MIT License
#pragma once

#include "../BreathalyzerDefaults.h"
#include "../WeCore/SongbirdAEFilter.h"
#include "../dasiydsp/FormantVoice.h"
#include "growlfx.h"

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
        double voiceGrowl{kDefaultVoiceGrowl};
        double voiceGrowlIntensity{kDefaultVoiceGrowlIntensity};
        double noiseGrowl{kDefaultNoiseGrowl};
        double noiseGrowlIntensity{kDefaultNoiseGrowlIntensity};
        double voiceSpeed{kDefaultVoiceSpeed};
    };

    BreathalyzerSynth() {
        prepare(sampleRate_, 0);
    }

    void prepare(double sampleRate, int /*maxBlockSize*/) {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
        leftVoice_.prepare(sampleRate_);
        rightVoice_.prepare(sampleRate_);
        voiceGrowlLeft_.setSampleRate(sampleRate_);
        voiceGrowlRight_.setSampleRate(sampleRate_);
        noiseGrowlLeft_.setSampleRate(sampleRate_);
        noiseGrowlRight_.setSampleRate(sampleRate_);
        breathFormant_.setSampleRate(sampleRate_);
        voiceGrowlLeft_.setRandomSeed(0x6A09E667u);
        voiceGrowlRight_.setRandomSeed(0xBB67AE85u);
        noiseGrowlLeft_.setRandomSeed(0x3C6EF372u);
        noiseGrowlRight_.setRandomSeed(0xA54FF53Au);
        prepareSmoothers();
        reset();
    }

    void reset() {
        leftVoice_.reset();
        rightVoice_.reset();
        noiseStateLeft_ = 0.0;
        noiseStateRight_ = 0.0;
        humanizePhaseA_ = 0.0;
        humanizePhaseB_ = 2.0943951023931953;
        voiceSpeedPhase_ = 0.0;
        voiceToneFilterLeft_.reset();
        voiceToneFilterRight_.reset();
        noiseToneFilterLeft_.reset();
        noiseToneFilterRight_.reset();
        voiceGrowlLeft_.reset();
        voiceGrowlRight_.reset();
        noiseGrowlLeft_.reset();
        noiseGrowlRight_.reset();
        breathFormant_.reset();
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
        voiceGrowlSmoother_.setTarget(clamp01(params_.voiceGrowl));
        voiceGrowlIntensitySmoother_.setTarget(clamp01(params_.voiceGrowlIntensity));
        noiseGrowlSmoother_.setTarget(clamp01(params_.noiseGrowl));
        noiseGrowlIntensitySmoother_.setTarget(clamp01(params_.noiseGrowlIntensity));
        voiceSpeedSmoother_.setTarget(clamp01(params_.voiceSpeed));
    }

    void noteOn(int pitch, double velocity, int32_t noteId, int16_t channel) {
        leftVoice_.noteOn(pitch, velocity, noteId, channel);
        rightVoice_.noteOn(pitch, velocity, noteId, channel);
        const double voiceGrowl = clamp01(params_.voiceGrowl);
        const double noiseGrowl = clamp01(params_.noiseGrowl);
        voiceGrowlLeft_.trigger(voiceGrowl);
        voiceGrowlRight_.trigger(voiceGrowl);
        noiseGrowlLeft_.trigger(noiseGrowl);
        noiseGrowlRight_.trigger(noiseGrowl);
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
        const double humanizeDepth = std::pow(p.humanize, 1.15);
        const double voiceSpeedHz = voiceSpeedHzFromNormalized(p.voiceSpeed);

        humanizePhaseA_ += (2.0 * 3.14159265358979323846 * (0.23 + 1.70 * humanizeDepth)) / sampleRate_;
        humanizePhaseB_ += (2.0 * 3.14159265358979323846 * (0.41 + 2.60 * humanizeDepth)) / sampleRate_;
        voiceSpeedPhase_ += (2.0 * 3.14159265358979323846 * voiceSpeedHz) / sampleRate_;
        if (humanizePhaseA_ >= 2.0 * 3.14159265358979323846) {
            humanizePhaseA_ -= 2.0 * 3.14159265358979323846;
        }
        if (humanizePhaseB_ >= 2.0 * 3.14159265358979323846) {
            humanizePhaseB_ -= 2.0 * 3.14159265358979323846;
        }
        if (voiceSpeedPhase_ >= 2.0 * 3.14159265358979323846) {
            voiceSpeedPhase_ -= 2.0 * 3.14159265358979323846;
        }
        const double humanizeMotion = (std::sin(humanizePhaseA_) + 0.6 * std::sin(humanizePhaseB_ + 0.7)) / 1.6;
        const double vowelMorph = 0.5 + 0.5 * std::sin(voiceSpeedPhase_);

        const double voicedLeft = leftVoice_.processSample();
        const double voicedRight = rightVoice_.processSample();
        const double envLeft = leftVoice_.lastEnvelope();
        const double envRight = rightVoice_.lastEnvelope();

        const double whiteLeft = randBipolar(randomLeft_);
        const double whiteRight = randBipolar(randomRight_);
        const double noiseFollow = 0.010 + 0.190 * p.breath;
        noiseStateLeft_ += noiseFollow * (whiteLeft - noiseStateLeft_);
        noiseStateRight_ += noiseFollow * (whiteRight - noiseStateRight_);

        const double breathNoiseAmount = 0.18 + 1.35 * p.breath;
        const double airyLeft = (whiteLeft - (0.86 - 0.28 * p.tone) * noiseStateLeft_) * breathNoiseAmount * envLeft;
        const double airyRight = (whiteRight - (0.86 - 0.28 * p.tone) * noiseStateRight_) * breathNoiseAmount * envRight;

        const double width = 0.90 + 1.05 * humanizeDepth;
        const double voicedBreathTrim = 1.10 - 0.45 * p.breath;
        const double leftSkew = 1.0 - 0.28 * humanizeDepth - 0.20 * humanizeDepth * humanizeMotion;
        const double rightSkew = 1.0 + 0.28 * humanizeDepth + 0.20 * humanizeDepth * humanizeMotion;
        const double noiseSourceLeft = airyLeft * (1.0 - 0.12 * humanizeDepth * humanizeMotion);
        const double noiseSourceRight = airyRight * (1.0 + 0.12 * humanizeDepth * humanizeMotion);
        double vowelNoiseLeft = 0.0;
        double vowelNoiseRight = 0.0;
        breathFormant_.process(noiseSourceLeft, noiseSourceRight, vowelMorph, vowelNoiseLeft, vowelNoiseRight);
        const double voiceSourceLeft = voicedLeft * width * voicedBreathTrim * leftSkew;
        const double voiceSourceRight = voicedRight * width * voicedBreathTrim * rightSkew;

        const double cutoffBase = 450.0 + std::pow(p.tone, 1.45) * 10800.0 + (1.0 - tonalBlend) * 1600.0 + 1800.0 * p.breath;
        const double leftCutoff = cutoffBase * (1.0 - 0.18 * humanizeDepth - 0.10 * humanizeDepth * humanizeMotion);
        const double rightCutoff = cutoffBase * (1.0 + 0.18 * humanizeDepth + 0.10 * humanizeDepth * humanizeMotion);
        voiceToneFilterLeft_.setLowpass(sampleRate_, leftCutoff);
        voiceToneFilterRight_.setLowpass(sampleRate_, rightCutoff);
        noiseToneFilterLeft_.setLowpass(sampleRate_, leftCutoff);
        noiseToneFilterRight_.setLowpass(sampleRate_, rightCutoff);

        voiceGrowlLeft_.setGrowl(p.voiceGrowl);
        voiceGrowlLeft_.setIntensity(p.voiceGrowlIntensity);
        voiceGrowlRight_.setGrowl(p.voiceGrowl);
        voiceGrowlRight_.setIntensity(p.voiceGrowlIntensity);
        noiseGrowlLeft_.setGrowl(p.noiseGrowl);
        noiseGrowlLeft_.setIntensity(p.noiseGrowlIntensity);
        noiseGrowlRight_.setGrowl(p.noiseGrowl);
        noiseGrowlRight_.setIntensity(p.noiseGrowlIntensity);

        const double gain = 0.88 + 0.28 * p.breath;
        const double processedNoiseLeft = noiseGrowlLeft_.process(noiseToneFilterLeft_.process(vowelNoiseLeft));
        const double processedNoiseRight = noiseGrowlRight_.process(noiseToneFilterRight_.process(vowelNoiseRight));
        const double processedVoiceLeft = voiceGrowlLeft_.process(voiceToneFilterLeft_.process(voiceSourceLeft));
        const double processedVoiceRight = voiceGrowlRight_.process(voiceToneFilterRight_.process(voiceSourceRight));
        left = std::tanh(lerp(processedNoiseLeft, processedVoiceLeft, tonalBlend) * gain);
        right = std::tanh(lerp(processedNoiseRight, processedVoiceRight, tonalBlend) * gain);
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
        double voiceGrowl{0.0};
        double voiceGrowlIntensity{0.0};
        double noiseGrowl{0.0};
        double noiseGrowlIntensity{0.0};
        double voiceSpeed{0.0};
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
        voiceGrowlSmoother_.prepare(sampleRate_, kControlSmoothSeconds);
        voiceGrowlIntensitySmoother_.prepare(sampleRate_, kControlSmoothSeconds);
        noiseGrowlSmoother_.prepare(sampleRate_, kControlSmoothSeconds);
        noiseGrowlIntensitySmoother_.prepare(sampleRate_, kControlSmoothSeconds);
        voiceSpeedSmoother_.prepare(sampleRate_, kControlSmoothSeconds);
    }

    void resetSmoothersToParams() {
        shapeSmoother_.reset(clamp01(params_.shape));
        breathSmoother_.reset(clamp01(params_.breath));
        voiceSmoother_.reset(clamp01(params_.voice));
        releaseSmoother_.reset(clamp01(params_.release));
        humanizeSmoother_.reset(clamp01(params_.humanize));
        toneSmoother_.reset(clamp01(params_.tone));
        attackSmoother_.reset(clamp01(params_.attack));
        voiceGrowlSmoother_.reset(clamp01(params_.voiceGrowl));
        voiceGrowlIntensitySmoother_.reset(clamp01(params_.voiceGrowlIntensity));
        noiseGrowlSmoother_.reset(clamp01(params_.noiseGrowl));
        noiseGrowlIntensitySmoother_.reset(clamp01(params_.noiseGrowlIntensity));
        voiceSpeedSmoother_.reset(clamp01(params_.voiceSpeed));
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
            voiceGrowlSmoother_.current,
            voiceGrowlIntensitySmoother_.current,
            noiseGrowlSmoother_.current,
            noiseGrowlIntensitySmoother_.current,
            voiceSpeedSmoother_.current,
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
            voiceGrowlSmoother_.next(),
            voiceGrowlIntensitySmoother_.next(),
            noiseGrowlSmoother_.next(),
            noiseGrowlIntensitySmoother_.next(),
            voiceSpeedSmoother_.next(),
        };
    }

    void applySmoothedParams(const SmoothedParams& p, bool force = false) {
        const float attack = static_cast<float>(attackSecondsFromNormalized(p.attack));
        const float release = static_cast<float>(releaseSecondsFromNormalized(p.release));
        const float decay = static_cast<float>(0.095 + 0.460 * (1.0 - p.breath) + 0.120 * (1.0 - p.tone));
        const float sustain = static_cast<float>(0.28 + 0.62 * p.voice);
        const float formantBase = static_cast<float>(380.0 + 1800.0 * p.shape + 260.0 * p.tone + 340.0 * p.breath);
        const float stereoSpread = static_cast<float>(1.00 + 0.45 * p.humanize);
        const float basePhase = static_cast<float>(0.02 + 0.66 * p.tone);
        const float phaseSpread = static_cast<float>(0.015 + 0.45 * p.humanize + 0.16 * p.breath);

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
    GrowlStage voiceGrowlLeft_{};
    GrowlStage voiceGrowlRight_{};
    GrowlStage noiseGrowlLeft_{};
    GrowlStage noiseGrowlRight_{};
    breathalyzer::wecore::SongbirdAEFilter breathFormant_{};
    FourPoleLowpass voiceToneFilterLeft_{};
    FourPoleLowpass voiceToneFilterRight_{};
    FourPoleLowpass noiseToneFilterLeft_{};
    FourPoleLowpass noiseToneFilterRight_{};
    SmoothedValue shapeSmoother_{};
    SmoothedValue breathSmoother_{};
    SmoothedValue voiceSmoother_{};
    SmoothedValue releaseSmoother_{};
    SmoothedValue humanizeSmoother_{};
    SmoothedValue toneSmoother_{};
    SmoothedValue attackSmoother_{};
    SmoothedValue voiceGrowlSmoother_{};
    SmoothedValue voiceGrowlIntensitySmoother_{};
    SmoothedValue noiseGrowlSmoother_{};
    SmoothedValue noiseGrowlIntensitySmoother_{};
    SmoothedValue voiceSpeedSmoother_{};
    double sampleRate_{44100.0};
    double noiseStateLeft_{0.0};
    double noiseStateRight_{0.0};
    double humanizePhaseA_{0.0};
    double humanizePhaseB_{2.0943951023931953};
    double voiceSpeedPhase_{0.0};
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
