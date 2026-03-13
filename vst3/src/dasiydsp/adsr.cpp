/*
Copyright (c) 2020 Electrosmith, Corp, Paul Batchelor

Use of this source code is governed by an MIT-style
license that can be found in the LICENSE file or at
https://opensource.org/licenses/MIT.
*/
#include "adsr.h"

#include <cmath>

namespace daisysp {

void Adsr::Init(float sampleRate, int blockSize) {
    sampleRate_ = sampleRate / static_cast<float>(blockSize);
    attackShape_ = -1.0f;
    attackTarget_ = 0.0f;
    attackTime_ = -1.0f;
    decayTime_ = -1.0f;
    releaseTime_ = -1.0f;
    sustainLevel_ = 0.7f;
    x_ = 0.0f;
    gate_ = false;
    mode_ = ADSR_SEG_IDLE;

    SetTime(ADSR_SEG_ATTACK, 0.1f);
    SetTime(ADSR_SEG_DECAY, 0.1f);
    SetTime(ADSR_SEG_RELEASE, 0.1f);
}

void Adsr::Retrigger(bool hard) {
    mode_ = ADSR_SEG_ATTACK;
    if (hard) {
        x_ = 0.0f;
    }
}

void Adsr::SetTime(int seg, float time) {
    switch (seg) {
        case ADSR_SEG_ATTACK:
            SetAttackTime(time, 0.0f);
            break;
        case ADSR_SEG_DECAY:
            SetTimeConstant(time, decayTime_, decayD0_);
            break;
        case ADSR_SEG_RELEASE:
            SetTimeConstant(time, releaseTime_, releaseD0_);
            break;
        default:
            break;
    }
}

void Adsr::SetAttackTime(float timeInS, float shape) {
    if (timeInS == attackTime_ && shape == attackShape_) {
        return;
    }

    attackTime_ = timeInS;
    attackShape_ = shape;
    if (timeInS > 0.0f) {
        const float target = 9.0f * std::pow(shape, 10.0f) + 0.3f * shape + 1.01f;
        attackTarget_ = target;
        const float logTarget = std::log(1.0f - (1.0f / target));
        attackD0_ = 1.0f - std::exp(logTarget / (timeInS * sampleRate_));
    } else {
        attackD0_ = 1.0f;
    }
}

void Adsr::SetDecayTime(float timeInS) {
    SetTimeConstant(timeInS, decayTime_, decayD0_);
}

void Adsr::SetReleaseTime(float timeInS) {
    SetTimeConstant(timeInS, releaseTime_, releaseD0_);
}

void Adsr::SetSustainLevel(float sustainLevel) {
    sustainLevel_ = sustainLevel <= 0.0f ? -0.01f : (sustainLevel > 1.0f ? 1.0f : sustainLevel);
}

void Adsr::SetTimeConstant(float timeInS, float& time, float& coeff) {
    if (timeInS == time) {
        return;
    }

    time = timeInS;
    if (time > 0.0f) {
        const float target = std::log(1.0f / static_cast<float>(std::exp(1.0f)));
        coeff = 1.0f - std::exp(target / (time * sampleRate_));
    } else {
        coeff = 1.0f;
    }
}

float Adsr::Process(bool gate) {
    if (gate && !gate_) {
        mode_ = ADSR_SEG_ATTACK;
    } else if (!gate && gate_) {
        mode_ = ADSR_SEG_RELEASE;
    }
    gate_ = gate;

    float coeff = attackD0_;
    if (mode_ == ADSR_SEG_DECAY) {
        coeff = decayD0_;
    } else if (mode_ == ADSR_SEG_RELEASE) {
        coeff = releaseD0_;
    }

    const float target = mode_ == ADSR_SEG_DECAY ? sustainLevel_ : -0.01f;

    switch (mode_) {
        case ADSR_SEG_IDLE:
            return 0.0f;
        case ADSR_SEG_ATTACK:
            x_ += coeff * (attackTarget_ - x_);
            if (x_ > 1.0f) {
                x_ = 1.0f;
                mode_ = ADSR_SEG_DECAY;
            }
            return x_;
        case ADSR_SEG_DECAY:
        case ADSR_SEG_RELEASE:
            x_ += coeff * (target - x_);
            if (x_ < 0.0f) {
                x_ = 0.0f;
                mode_ = ADSR_SEG_IDLE;
            }
            return x_;
        default:
            return 0.0f;
    }
}

} // namespace daisysp
