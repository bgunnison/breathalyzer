// Vendored from WE-Core / Songbird filter sources with minimal local trimming.
#pragma once

#include "General/CoreMath.h"
#include "WEFilters/TPTSVFilterParameters.h"

#include <cmath>
#include <cstddef>
#include <type_traits>

namespace WECore::TPTSVF {

template <typename T>
class TPTSVFilter {
    static_assert(std::is_floating_point<T>::value, "TPTSVFilter requires floating point sample types");

public:
    TPTSVFilter()
        : sampleRate_(44100.0),
          cutoffHz_(Parameters::CUTOFF.defaultValue),
          q_(Parameters::Q.defaultValue),
          gain_(Parameters::GAIN.defaultValue),
          s1_(0),
          s2_(0),
          g_(0),
          h_(0),
          mode_(Parameters::FILTER_MODE.BYPASS) {
        calculateCoefficients();
    }

    void processBlock(T* samples, std::size_t numSamples) {
        if (mode_ == Parameters::FILTER_MODE.BYPASS) {
            return;
        }

        for (std::size_t index = 0; index < numSamples; ++index) {
            const T input = samples[index];

            const T yH = static_cast<T>(h_ * (input - (1.0 / q_ + g_) * s1_ - s2_));
            const T yB = static_cast<T>(g_ * yH + s1_);
            s1_ = g_ * yH + yB;

            const T yL = static_cast<T>(g_ * yB + s2_);
            s2_ = g_ * yB + yL;

            switch (mode_) {
                case Parameters::ModeParameter::PEAK:
                    samples[index] = yB * static_cast<T>(gain_);
                    break;
                case Parameters::ModeParameter::HIGHPASS:
                    samples[index] = yH * static_cast<T>(gain_);
                    break;
                default:
                    samples[index] = yL * static_cast<T>(gain_);
                    break;
            }
        }
    }

    void reset() {
        s1_ = 0;
        s2_ = 0;
    }

    int getMode() const { return mode_; }
    double getCutoff() const { return cutoffHz_; }
    double getQ() const { return q_; }
    double getGain() const { return gain_; }

    void setMode(int value) { mode_ = Parameters::FILTER_MODE.BoundsCheck(value); }

    void setCutoff(double value) {
        cutoffHz_ = Parameters::CUTOFF.BoundsCheck(value);
        calculateCoefficients();
    }

    void setQ(double value) {
        q_ = Parameters::Q.BoundsCheck(value);
        calculateCoefficients();
    }

    void setGain(double value) {
        gain_ = Parameters::GAIN.BoundsCheck(value);
    }

    void setSampleRate(double value) {
        sampleRate_ = value;
        calculateCoefficients();
    }

private:
    void calculateCoefficients() {
        g_ = static_cast<T>(std::tan(CoreMath::DOUBLE_PI * cutoffHz_ / sampleRate_));
        h_ = static_cast<T>(1.0 / (1.0 + g_ / q_ + g_ * g_));
    }

    double sampleRate_;
    double cutoffHz_;
    double q_;
    double gain_;

    T s1_;
    T s2_;
    T g_;
    T h_;
    int mode_;
};

} // namespace WECore::TPTSVF
