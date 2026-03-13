// Adapted from C:\projects\daisypod\spring\formantvoice.cpp and voice.h
#pragma once

#include "adsr.h"
#include "dsp.h"
#include "formantosc.h"

#include <array>
#include <cstdint>

namespace breathalyzer::daisydsp {

class FormantVoice {
public:
    static constexpr std::size_t kPolyphony = 8;

    void prepare(double sampleRate);
    void reset();

    void noteOn(int pitch, double velocity, int32_t noteId, int16_t channel);
    void noteOff(int pitch, int32_t noteId, int16_t channel);
    void allNotesOff();

    void setAttackSeconds(float seconds);
    void setDecaySeconds(float seconds);
    void setSustainLevel(float level);
    void setReleaseSeconds(float seconds);
    void setFormantFreq(float hz);
    void setPhaseShift(float shift);

    float processSample();
    float lastEnvelope() const { return lastEnvelope_; }
    bool isSilent() const { return activeVoiceCount_ == 0; }

private:
    struct VoiceSlot {
        bool active{false};
        bool held{false};
        int pitch{60};
        int32_t noteId{-1};
        int16_t channel{0};
        float amplitude{0.0f};
        float env{0.0f};
        daisysp::FormantOscillator oscillator{};
        daisysp::Adsr adsr{};
    };

    VoiceSlot* findVoiceForNoteOn(int pitch, int32_t noteId, int16_t channel);
    void startVoice(VoiceSlot& voice, int pitch, double velocity, int32_t noteId, int16_t channel);
    void applyEnvelopeParams(VoiceSlot& voice);
    void applyOscillatorParams(VoiceSlot& voice);

    std::array<VoiceSlot, kPolyphony> voices_{};
    float sampleRate_{44100.0f};
    float attackSeconds_{0.1f};
    float decaySeconds_{0.5f};
    float sustainLevel_{1.0f};
    float releaseSeconds_{0.2f};
    float formantFreq_{1000.0f};
    float phaseShift_{0.0f};
    float lastEnvelope_{0.0f};
    int activeVoiceCount_{0};
};

} // namespace breathalyzer::daisydsp
