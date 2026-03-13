/*
Copyright (c) 2020 Electrosmith, Corp, Emilie Gillet

Use of this source code is governed by an MIT-style
license that can be found in the LICENSE file or at
https://opensource.org/licenses/MIT.
*/
#include "formantosc.h"

#include "dsp.h"

#include <cmath>

namespace daisysp {

void FormantOscillator::Init(float sampleRate) {
    carrierPhase_ = 0.0f;
    formantPhase_ = 0.0f;
    nextSample_ = 0.0f;
    carrierFrequency_ = 0.0f;
    formantFrequency_ = 100.0f;
    phaseShift_ = 0.0f;
    phaseShiftIncrement_ = 0.0f;
    sampleRate_ = sampleRate;
}

float FormantOscillator::Process() {
    float thisSample = nextSample_;
    float nextSample = 0.0f;
    carrierPhase_ += carrierFrequency_;

    if (carrierPhase_ >= 1.0f) {
        carrierPhase_ -= 1.0f;
        const float resetTime = carrierPhase_ / carrierFrequency_;
        const float formantPhaseAtReset = formantPhase_ + (1.0f - resetTime) * formantFrequency_;
        const float before = Sine(formantPhaseAtReset + phaseShift_
                                  + (phaseShiftIncrement_ * (1.0f - resetTime)));
        const float after = Sine(phaseShift_ + phaseShiftIncrement_);
        const float discontinuity = after - before;
        thisSample += discontinuity * ThisBlepSample(resetTime);
        nextSample += discontinuity * NextBlepSample(resetTime);
        formantPhase_ = resetTime * formantFrequency_;
    } else {
        formantPhase_ += formantFrequency_;
        if (formantPhase_ >= 1.0f) {
            formantPhase_ -= 1.0f;
        }
    }

    phaseShift_ += phaseShiftIncrement_;
    phaseShiftIncrement_ = 0.0f;

    nextSample += Sine(formantPhase_ + phaseShift_);
    nextSample_ = nextSample;
    return thisSample;
}

void FormantOscillator::SetFormantFreq(float freq) {
    formantFrequency_ = freq / sampleRate_;
    if (formantFrequency_ > 0.25f) {
        formantFrequency_ = 0.25f;
    } else if (formantFrequency_ < -0.25f) {
        formantFrequency_ = -0.25f;
    }
}

void FormantOscillator::SetCarrierFreq(float freq) {
    carrierFrequency_ = freq / sampleRate_;
    if (carrierFrequency_ > 0.25f) {
        carrierFrequency_ = 0.25f;
    } else if (carrierFrequency_ < -0.25f) {
        carrierFrequency_ = -0.25f;
    }
}

void FormantOscillator::SetPhaseShift(float phaseShift) {
    phaseShiftIncrement_ = phaseShift - phaseShift_;
}

float FormantOscillator::Sine(float phase) {
    return std::sinf(phase * TWOPI_F);
}

float FormantOscillator::ThisBlepSample(float t) {
    return 0.5f * t * t;
}

float FormantOscillator::NextBlepSample(float t) {
    t = 1.0f - t;
    return -0.5f * t * t;
}

} // namespace daisysp
