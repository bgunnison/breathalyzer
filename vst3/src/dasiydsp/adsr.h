/*
Copyright (c) 2020 Electrosmith, Corp, Paul Batchelor

Use of this source code is governed by an MIT-style
license that can be found in the LICENSE file or at
https://opensource.org/licenses/MIT.
*/
#pragma once

#include <cstdint>

namespace daisysp {

enum {
    ADSR_SEG_IDLE = 0,
    ADSR_SEG_ATTACK = 1,
    ADSR_SEG_DECAY = 2,
    ADSR_SEG_RELEASE = 4
};

class Adsr {
public:
    void Init(float sampleRate, int blockSize = 1);
    void Retrigger(bool hard);
    float Process(bool gate);
    void SetTime(int seg, float time);
    void SetAttackTime(float timeInS, float shape = 0.0f);
    void SetDecayTime(float timeInS);
    void SetReleaseTime(float timeInS);
    void SetSustainLevel(float sustainLevel);

    uint8_t GetCurrentSegment() const { return mode_; }
    bool IsRunning() const { return mode_ != ADSR_SEG_IDLE; }

private:
    void SetTimeConstant(float timeInS, float& time, float& coeff);

    float sustainLevel_{0.0f};
    float x_{0.0f};
    float attackShape_{-1.0f};
    float attackTarget_{0.0f};
    float attackTime_{-1.0f};
    float decayTime_{-1.0f};
    float releaseTime_{-1.0f};
    float attackD0_{0.0f};
    float decayD0_{0.0f};
    float releaseD0_{0.0f};
    float sampleRate_{44100.0f};
    uint8_t mode_{ADSR_SEG_IDLE};
    bool gate_{false};
};

} // namespace daisysp
