/*
Copyright (c) 2020 Electrosmith, Corp, Emilie Gillet

Use of this source code is governed by an MIT-style
license that can be found in the LICENSE file or at
https://opensource.org/licenses/MIT.
*/
#pragma once

namespace daisysp {

class FormantOscillator {
public:
    void Init(float sampleRate);
    float Process();
    void SetFormantFreq(float freq);
    void SetCarrierFreq(float freq);
    void SetPhaseShift(float phaseShift);

private:
    static float Sine(float phase);
    static float ThisBlepSample(float t);
    static float NextBlepSample(float t);

    float carrierPhase_{0.0f};
    float formantPhase_{0.0f};
    float nextSample_{0.0f};
    float carrierFrequency_{0.0f};
    float formantFrequency_{100.0f};
    float phaseShift_{0.0f};
    float phaseShiftIncrement_{0.0f};
    float sampleRate_{44100.0f};
};

} // namespace daisysp
