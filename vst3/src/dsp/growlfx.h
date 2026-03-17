// Copyright (c) 2026 Brian R. Gunnison
// MIT License
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace breathalyzer::dsp {

    // Growl / wet-gurgle post-tone processor.
    //
    // Intended use:
    // - Place after a clean formant or breath tone source.
    // - Adds an organic throat-like distortion character.
    // - Uses moving asymmetric saturation plus a short-memory smear.
    // - Keeps enough clean signal in parallel so the underlying vowel / mouth
    //   identity remains intact.
    //
    // The sound is built from four parts:
    // 1) low-mid body emphasis before nonlinearity
    // 2) asymmetric soft clipping
    // 3) slow drive wobble from sine + smoothed noise
    // 4) short filtered smear to create a wet/gurgly texture
    //
    // Control intent:
    // - growl controls aggression/character
    // - intensity is wet/dry blend only
    class GrowlStage {
    public:
        static constexpr double DEFAULT_GROWL = 0.0;      // Start fully clean.
        static constexpr double DEFAULT_INTENSITY = 0.0;  // Start fully clean.

        void setSampleRate(double sampleRate) {
            sampleRate_ = (sampleRate > MIN_SAMPLE_RATE) ? sampleRate : DEFAULT_SAMPLE_RATE;
        }

        void setRandomSeed(std::uint32_t seed) {
            initialRandomSeed_ = seed != 0 ? seed : DEFAULT_RANDOM_SEED;
            randomState_ = initialRandomSeed_;
        }

        void reset() {
            lfoPhase_ = 0.0;
            preLpState_ = 0.0;
            dcRejectX1_ = 0.0;
            dcRejectY1_ = 0.0;
            smearState1_ = 0.0;
            smearState2_ = 0.0;
            postLpState_ = 0.0;
            dryLevel_ = 0.0;
            wetLevel_ = 0.0;
            noiseSmooth_ = 0.0;
            randomState_ = initialRandomSeed_;
            wobbleRateHz_ = DEFAULT_WOBBLE_RATE_HZ;
            wobbleRateTargetHz_ = DEFAULT_WOBBLE_RATE_HZ;
        }

        void setGrowl(double growl) { growl_ = clamp01(growl); }
        void setIntensity(double intensity) { intensity_ = clamp01(intensity); }
        void trigger(double growl) {
            wobbleRateTargetHz_ = computeWobbleRateTargetHz(clamp01(growl), nextUnitRandom());
        }

        double process(double x) {
            const double growl = growl_;
            const double intensity = intensity_;
            wobbleRateHz_ += WOBBLE_RATE_SMOOTH_COEFF * (wobbleRateTargetHz_ - wobbleRateHz_);

            const double drive = computeDrive(growl);
            const double bias = computeBias(growl);
            const double wobbleDepth = computeWobbleDepth(growl);
            const double wobbleRateHz = wobbleRateHz_;
            const double smearAmount = computeSmearAmount(growl);
            const double wetMix = computeWetMix(intensity);
            const double bodyAmount = computeBodyAmount(growl);
            const double postLpCoeff = computePostLowpassCoeff(growl);

            // Broad low-mid emphasis. Subtracting a slow lowpass from the input
            // gives a gentle body band without adding sharp resonances.
            const double low = onePoleLowpass(x, preLpState_, PRE_BODY_LOW_PASS_COEFF);
            const double body = x + bodyAmount * (x - low);

            // Growl movement comes from modulation of drive, not from a static
            // clipper. Mix periodic motion with smoothed noise so repeated notes
            // do not sound too mechanically identical.
            const double lfo = nextLfoSample(wobbleRateHz);
            const double noise = nextSmoothedNoise(NOISE_SMOOTH_COEFF);
            const double wobble = 1.0 + wobbleDepth * (LFO_WOBBLE_MIX * lfo + NOISE_WOBBLE_MIX * noise);

            // Asymmetric soft clipper. Bias produces the throat-like asymmetry.
            const double driven = body * drive * wobble + bias;
            const double shapedPrimary = std::tanh(driven) - std::tanh(bias);

            // A second chew stage makes high growl settings substantially nastier
            // without relying on raw output gain.
            const double chewDrive = 1.0 + 9.0 * growl;
            const double chewBias = 0.12 + 0.42 * growl;
            const double chewed = std::tanh((shapedPrimary + chewBias) * chewDrive)
                                - std::tanh(chewBias * chewDrive);
            const double shapedRaw = shapedPrimary * (0.75 - 0.35 * growl)
                                   + chewed * (0.25 + 0.85 * growl);

            // Remove the DC component created by asymmetry so the smear section
            // reacts to texture rather than offset.
            const double shaped = dcReject(shapedRaw, DC_REJECT_COEFF);

            // Very short-memory smear. This is meant to feel wet/internal rather
            // than like an audible delay or room effect.
            const double smearIn = shaped + SMEAR_FEEDBACK * smearState2_;
            smearState1_ += smearAmount * (smearIn - smearState1_);
            smearState2_ += (SMEAR_SECOND_STAGE_RATIO * smearAmount) * (smearState1_ - smearState2_);
            double wet = shaped + SMEAR_ENHANCE * (smearState1_ - smearState2_);

            // Dampen top-end fizz so the branch stays throat-like rather than
            // turning into generic bright synth distortion.
            wet = onePoleLowpass(wet, postLpState_, postLpCoeff);

            // Match the wet branch level back toward the dry branch so wet/dry
            // mix does not read as a loudness control.
            dryLevel_ += LEVEL_TRACK_COEFF * (std::abs(x) - dryLevel_);
            wetLevel_ += LEVEL_TRACK_COEFF * (std::abs(wet) - wetLevel_);
            const double wetComp = std::clamp((dryLevel_ + 1.0e-4) / (wetLevel_ + 1.0e-4), 0.35, 2.4);
            wet *= wetComp * (0.96 - 0.16 * growl);

            // Equal-power blend keeps the control feeling like wet/dry instead
            // of level.
            const double dryGain = std::cos(HALF_PI * wetMix);
            const double wetGain = std::sin(HALF_PI * wetMix);
            return dryGain * x + wetGain * wet;
        }

    private:
        static constexpr double DEFAULT_SAMPLE_RATE = 48000.0; // Safe default for offline construction.
        static constexpr double MIN_SAMPLE_RATE = 1.0;         // Reject invalid sample rates.

        static constexpr std::uint32_t DEFAULT_RANDOM_SEED = 0x12345678u; // Deterministic startup behavior.
        static constexpr double DEFAULT_WOBBLE_RATE_HZ = 22.0;

        static constexpr double PRE_BODY_LOW_PASS_COEFF = 0.035; // Slow enough to isolate low content; fast enough to track dynamics.
        static constexpr double NOISE_SMOOTH_COEFF = 0.004;      // Keeps random motion slow and fluid, not buzzy.

        static constexpr double DC_REJECT_COEFF = 0.995; // Strong DC removal with very low audible impact.

        static constexpr double LFO_WOBBLE_MIX = 0.65;   // Mostly periodic so the effect feels controlled.
        static constexpr double NOISE_WOBBLE_MIX = 0.35; // Enough randomness to avoid repetitive chewing.

        static constexpr double SMEAR_FEEDBACK = 0.18;          // Small memory boost without obvious ringing.
        static constexpr double SMEAR_SECOND_STAGE_RATIO = 0.45; // Second stage slower than first for wet/internal character.
        static constexpr double SMEAR_ENHANCE = 0.65;           // Amount of differential smear added back into the wet branch.
        static constexpr double LEVEL_TRACK_COEFF = 0.01;       // Tracks branch energy for wet compensation.
        static constexpr double WOBBLE_RATE_SMOOTH_COEFF = 0.002;
        static constexpr double HALF_PI = 1.57079632679489661923;
        static constexpr double TWO_PI = 6.28318530717958647692;

        static double clamp01(double x) {
            return std::clamp(x, 0.0, 1.0);
        }

        static double onePoleLowpass(double x, double& state, double coeff) {
            state += coeff * (x - state);
            return state;
        }

        double dcReject(double x, double coeff) {
            const double y = x - dcRejectX1_ + coeff * dcRejectY1_;
            dcRejectX1_ = x;
            dcRejectY1_ = y;
            return y;
        }

        double nextLfoSample(double rateHz) {
            const double phaseIncrement = (TWO_PI * rateHz) / sampleRate_;
            lfoPhase_ += phaseIncrement;
            if (lfoPhase_ >= TWO_PI) {
                lfoPhase_ -= TWO_PI;
            }
            return std::sin(lfoPhase_);
        }

        double nextSmoothedNoise(double smoothCoeff) {
            randomState_ = 1664525u * randomState_ + 1013904223u;

            // Convert to roughly [-1, 1).
            const double white =
                static_cast<double>((randomState_ >> 8) & 0x00ffffffu) * (1.0 / 8388608.0) - 1.0;

            noiseSmooth_ += smoothCoeff * (white - noiseSmooth_);
            return noiseSmooth_;
        }

        double nextUnitRandom() {
            randomState_ = 1664525u * randomState_ + 1013904223u;
            return static_cast<double>((randomState_ >> 8) & 0x00ffffffu) * (1.0 / 16777216.0);
        }

        static double computeDrive(double growl) {
            // Push substantially harder at the top of the range so high growl
            // becomes obviously feral instead of politely dirty.
            return 1.0 + 26.0 * std::pow(growl, 2.2);
        }

        static double computeBias(double growl) {
            return 0.55 * growl;
        }

        static double computeWobbleDepth(double growl) {
            return 0.03 + 0.55 * growl;
        }

        static double computeWobbleRateTargetHz(double growl, double randomUnit) {
            const double randomRateHz = 20.0 + 40.0 * randomUnit;
            const double growlDropHz = 2.75 * std::pow(growl, 0.9);
            return std::clamp(randomRateHz - growlDropHz, 8.5, 32.0);
        }

        static double computeSmearAmount(double growl) {
            return 0.05 + 0.55 * growl;
        }

        static double computeWetMix(double intensity) {
            return intensity;
        }

        static double computeBodyAmount(double growl) {
            return 0.25 + 0.90 * growl;
        }

        static double computePostLowpassCoeff(double growl) {
            return 0.14 + 0.46 * growl;
        }

        double sampleRate_{ DEFAULT_SAMPLE_RATE };

        double growl_{ DEFAULT_GROWL };
        double intensity_{ DEFAULT_INTENSITY };

        double lfoPhase_{ 0.0 };
        double preLpState_{ 0.0 };

        double dcRejectX1_{ 0.0 };
        double dcRejectY1_{ 0.0 };

        double smearState1_{ 0.0 };
        double smearState2_{ 0.0 };
        double postLpState_{ 0.0 };
        double dryLevel_{ 0.0 };
        double wetLevel_{ 0.0 };

        double noiseSmooth_{ 0.0 };
        std::uint32_t initialRandomSeed_{ DEFAULT_RANDOM_SEED };
        std::uint32_t randomState_{ DEFAULT_RANDOM_SEED };
        double wobbleRateHz_{ DEFAULT_WOBBLE_RATE_HZ };
        double wobbleRateTargetHz_{ DEFAULT_WOBBLE_RATE_HZ };
    };

} // namespace breathalyzer::dsp

