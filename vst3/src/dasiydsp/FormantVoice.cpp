// Adapted from C:\projects\daisypod\spring\formantvoice.cpp and voice.h
#include "FormantVoice.h"

#include <algorithm>
#include <cmath>

namespace breathalyzer::daisydsp {

namespace {

float clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

} // namespace

void FormantVoice::prepare(double sampleRate) {
    sampleRate_ = sampleRate > 0.0 ? static_cast<float>(sampleRate) : 44100.0f;
    for (auto& voice : voices_) {
        voice.oscillator.Init(sampleRate_);
        voice.adsr.Init(sampleRate_);
        applyEnvelopeParams(voice);
        applyOscillatorParams(voice);
    }
    reset();
}

void FormantVoice::reset() {
    activeVoiceCount_ = 0;
    lastEnvelope_ = 0.0f;
    for (auto& voice : voices_) {
        voice.active = false;
        voice.held = false;
        voice.pitch = 60;
        voice.noteId = -1;
        voice.channel = 0;
        voice.amplitude = 0.0f;
        voice.env = 0.0f;
        voice.oscillator.Init(sampleRate_);
        voice.adsr.Init(sampleRate_);
        applyEnvelopeParams(voice);
        applyOscillatorParams(voice);
    }
}

void FormantVoice::noteOn(int pitch, double velocity, int32_t noteId, int16_t channel) {
    VoiceSlot* voice = findVoiceForNoteOn(pitch, noteId, channel);
    if (!voice) {
        return;
    }
    startVoice(*voice, pitch, velocity, noteId, channel);
}

void FormantVoice::noteOff(int pitch, int32_t noteId, int16_t channel) {
    for (auto& voice : voices_) {
        if (!voice.active) {
            continue;
        }

        const bool noteIdMatch = noteId >= 0 && voice.noteId >= 0 && voice.noteId == noteId;
        const bool pitchMatch = voice.pitch == pitch && voice.channel == channel;
        if (noteIdMatch || pitchMatch) {
            voice.held = false;
            voice.noteId = -1;
        }
    }
}

void FormantVoice::allNotesOff() {
    for (auto& voice : voices_) {
        if (voice.active) {
            voice.held = false;
            voice.noteId = -1;
        }
    }
}

void FormantVoice::setAttackSeconds(float seconds) {
    attackSeconds_ = std::max(seconds, 0.001f);
    for (auto& voice : voices_) {
        voice.adsr.SetAttackTime(attackSeconds_);
    }
}

void FormantVoice::setDecaySeconds(float seconds) {
    decaySeconds_ = std::max(seconds, 0.001f);
    for (auto& voice : voices_) {
        voice.adsr.SetDecayTime(decaySeconds_);
    }
}

void FormantVoice::setSustainLevel(float level) {
    sustainLevel_ = clamp01(level);
    for (auto& voice : voices_) {
        voice.adsr.SetSustainLevel(sustainLevel_);
    }
}

void FormantVoice::setReleaseSeconds(float seconds) {
    releaseSeconds_ = std::max(seconds, 0.001f);
    for (auto& voice : voices_) {
        voice.adsr.SetReleaseTime(releaseSeconds_);
    }
}

void FormantVoice::setFormantFreq(float hz) {
    formantFreq_ = std::max(hz, 20.0f);
    for (auto& voice : voices_) {
        voice.oscillator.SetFormantFreq(formantFreq_);
    }
}

void FormantVoice::setPhaseShift(float shift) {
    phaseShift_ = shift;
    for (auto& voice : voices_) {
        voice.oscillator.SetPhaseShift(phaseShift_);
    }
}

float FormantVoice::processSample() {
    float sum = 0.0f;
    float envSum = 0.0f;
    activeVoiceCount_ = 0;

    for (auto& voice : voices_) {
        if (!voice.active) {
            continue;
        }

        voice.env = voice.adsr.Process(voice.held);
        if (!voice.held && !voice.adsr.IsRunning() && voice.env <= 0.0f) {
            voice.active = false;
            voice.amplitude = 0.0f;
            voice.pitch = 60;
            voice.channel = 0;
            continue;
        }

        sum += voice.oscillator.Process() * voice.env * voice.amplitude;
        envSum += voice.env * voice.amplitude;
        ++activeVoiceCount_;
    }

    lastEnvelope_ = activeVoiceCount_ > 0 ? std::clamp(envSum / static_cast<float>(kPolyphony), 0.0f, 1.0f) : 0.0f;
    return activeVoiceCount_ > 0 ? (sum / static_cast<float>(kPolyphony)) : 0.0f;
}

FormantVoice::VoiceSlot* FormantVoice::findVoiceForNoteOn(int pitch, int32_t noteId, int16_t channel) {
    VoiceSlot* releasedCandidate = nullptr;
    float releasedLevel = 2.0f;
    VoiceSlot* quietest = &voices_[0];
    float quietestLevel = 2.0f;

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

void FormantVoice::startVoice(VoiceSlot& voice, int pitch, double velocity, int32_t noteId, int16_t channel) {
    voice.active = true;
    voice.held = true;
    voice.pitch = pitch;
    voice.noteId = noteId;
    voice.channel = channel;
    voice.amplitude = clamp01(static_cast<float>(velocity));
    voice.env = 0.0f;
    voice.oscillator.Init(sampleRate_);
    applyOscillatorParams(voice);
    voice.oscillator.SetCarrierFreq(daisysp::mtof(static_cast<float>(pitch)));
    voice.adsr.Init(sampleRate_);
    applyEnvelopeParams(voice);
    voice.adsr.Retrigger(false);
    activeVoiceCount_ = std::max(activeVoiceCount_, 1);
}

void FormantVoice::applyEnvelopeParams(VoiceSlot& voice) {
    voice.adsr.SetAttackTime(attackSeconds_);
    voice.adsr.SetDecayTime(decaySeconds_);
    voice.adsr.SetSustainLevel(sustainLevel_);
    voice.adsr.SetReleaseTime(releaseSeconds_);
}

void FormantVoice::applyOscillatorParams(VoiceSlot& voice) {
    voice.oscillator.SetFormantFreq(formantFreq_);
    voice.oscillator.SetPhaseShift(phaseShift_);
}

} // namespace breathalyzer::daisydsp
