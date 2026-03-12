// Copyright (c) 2026 Brian R. Gunnison
// MIT License
#include "BreathalyzerProcessor.h"

#include "BreathalyzerController.h"
#include "BreathalyzerDefaults.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include <algorithm>
#include <vector>

namespace breathalyzer {
using namespace Steinberg;
using namespace Steinberg::Vst;

namespace {

void writeOutputSilence(ProcessData& data) {
    for (int32 bus = 0; bus < data.numOutputs; ++bus) {
        auto& out = data.outputs[bus];
        out.silenceFlags = 0;
        if (out.channelBuffers32 && data.symbolicSampleSize == kSample32) {
            for (uint32 channel = 0; channel < out.numChannels; ++channel) {
                std::fill_n(out.channelBuffers32[channel], data.numSamples, 0.0f);
            }
        }
        if (out.channelBuffers64 && data.symbolicSampleSize == kSample64) {
            for (uint32 channel = 0; channel < out.numChannels; ++channel) {
                std::fill_n(out.channelBuffers64[channel], data.numSamples, 0.0);
            }
        }
    }
}

uint64 silenceFlagsForChannels(uint32 numChannels) {
    if (numChannels == 0) {
        return 0;
    }
    if (numChannels >= 64) {
        return ~0ull;
    }
    return (1ull << numChannels) - 1ull;
}

} // namespace

BreathalyzerProcessor::BreathalyzerProcessor() {
    setControllerClass(kBreathalyzerControllerUID);
    setProcessing(true);
    buildParamOrder();
    resetToDefaults();
    syncSynth();
}

Steinberg::tresult PLUGIN_API BreathalyzerProcessor::initialize(FUnknown* context) {
    const tresult result = AudioEffect::initialize(context);
    if (result != kResultOk) {
        return result;
    }

    addAudioOutput(STR16("Main Out"), SpeakerArr::kStereo);
    addEventInput(STR16("MIDI In"), 1);

    return kResultOk;
}

Steinberg::tresult PLUGIN_API BreathalyzerProcessor::terminate() {
    return AudioEffect::terminate();
}

Steinberg::tresult PLUGIN_API BreathalyzerProcessor::setBusArrangements(SpeakerArrangement* inputs,
                                                                        int32 numIns,
                                                                        SpeakerArrangement* outputs,
                                                                        int32 numOuts) {
    if (numIns != 0 || numOuts != 1) {
        return kResultFalse;
    }
    if (outputs[0] != SpeakerArr::kStereo) {
        return kResultFalse;
    }
    return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
}

Steinberg::tresult PLUGIN_API BreathalyzerProcessor::setupProcessing(ProcessSetup& setup) {
    sampleRate_ = setup.sampleRate > 0.0 ? setup.sampleRate : 44100.0;
    maxBlockSize_ = setup.maxSamplesPerBlock;
    synth_.prepare(sampleRate_, maxBlockSize_);
    syncSynth();
    return AudioEffect::setupProcessing(setup);
}

Steinberg::tresult PLUGIN_API BreathalyzerProcessor::setActive(TBool state) {
    if (state) {
        synth_.reset();
    } else {
        synth_.allNotesOff();
    }
    return AudioEffect::setActive(state);
}

Steinberg::tresult PLUGIN_API BreathalyzerProcessor::canProcessSampleSize(int32 symbolicSampleSize) {
    if (symbolicSampleSize == kSample32 || symbolicSampleSize == kSample64) {
        return kResultOk;
    }
    return kResultFalse;
}

void BreathalyzerProcessor::buildParamOrder() {
    paramOrder_.clear();
    paramOrder_.push_back(kParamShape);
    paramOrder_.push_back(kParamBreath);
    paramOrder_.push_back(kParamVoice);
    paramOrder_.push_back(kParamRelease);
    paramOrder_.push_back(kParamHumanize);
    paramOrder_.push_back(kParamTone);

    paramState_.clear();
    for (const auto pid : paramOrder_) {
        paramState_[pid] = defaultNormalized(pid);
    }
}

ParamValue BreathalyzerProcessor::defaultNormalized(ParamID pid) const {
    switch (pid) {
        case kParamShape: return kDefaultShape;
        case kParamBreath: return kDefaultBreath;
        case kParamVoice: return kDefaultVoice;
        case kParamRelease: return kDefaultRelease;
        case kParamHumanize: return kDefaultHumanize;
        case kParamTone: return kDefaultTone;
        default: break;
    }
    return 0.0;
}

void BreathalyzerProcessor::resetToDefaults() {
    for (const auto pid : paramOrder_) {
        applyNormalizedParam(pid, defaultNormalized(pid));
    }
}

void BreathalyzerProcessor::handleParameterChanges(ProcessData& data) {
    if (!data.inputParameterChanges) {
        return;
    }

    const int32 count = data.inputParameterChanges->getParameterCount();
    for (int32 i = 0; i < count; ++i) {
        IParamValueQueue* queue = data.inputParameterChanges->getParameterData(i);
        if (!queue || queue->getPointCount() <= 0) {
            continue;
        }

        ParamValue value = 0.0;
        int32 sampleOffset = 0;
        if (queue->getPoint(queue->getPointCount() - 1, sampleOffset, value) == kResultOk) {
            applyNormalizedParam(queue->getParameterId(), value);
        }
    }
}

void BreathalyzerProcessor::applyNormalizedParam(ParamID pid, ParamValue value) {
    value = std::clamp(value, 0.0, 1.0);
    paramState_[pid] = value;
    switch (pid) {
        case kParamShape: params_.shape = value; break;
        case kParamBreath: params_.breath = value; break;
        case kParamVoice: params_.voice = value; break;
        case kParamRelease: params_.release = value; break;
        case kParamHumanize: params_.humanize = value; break;
        case kParamTone: params_.tone = value; break;
        default: break;
    }
}

void BreathalyzerProcessor::syncSynth() {
    synth_.setParams(params_);
}

void BreathalyzerProcessor::handleEvent(const Event& event) {
    switch (event.type) {
        case Event::kNoteOnEvent:
            if (event.noteOn.velocity <= 0.0f) {
                synth_.noteOff(event.noteOn.pitch, event.noteOn.noteId, event.noteOn.channel);
            } else {
                synth_.noteOn(event.noteOn.pitch, event.noteOn.velocity, event.noteOn.noteId, event.noteOn.channel);
            }
            break;
        case Event::kNoteOffEvent:
            synth_.noteOff(event.noteOff.pitch, event.noteOff.noteId, event.noteOff.channel);
            break;
        default:
            break;
    }
}

void BreathalyzerProcessor::handleAllEvents(IEventList* events) {
    if (!events) {
        return;
    }
    const int32 eventCount = events->getEventCount();
    Event event{};
    for (int32 i = 0; i < eventCount; ++i) {
        if (events->getEvent(i, event) == kResultOk) {
            handleEvent(event);
        }
    }
}

Steinberg::tresult PLUGIN_API BreathalyzerProcessor::process(ProcessData& data) {
    handleParameterChanges(data);
    syncSynth();

    if (data.numOutputs == 0 || !data.outputs) {
        handleAllEvents(data.inputEvents);
        return kResultOk;
    }

    if (data.numSamples <= 0) {
        return kResultOk;
    }

    auto& outBus = data.outputs[0];
    if (outBus.numChannels == 0) {
        return kResultOk;
    }

    if (data.symbolicSampleSize == kSample32) {
        if (!outBus.channelBuffers32) {
            writeOutputSilence(data);
            return kResultOk;
        }
        processAudio<float>(data, outBus.channelBuffers32, static_cast<int32_t>(outBus.numChannels), data.numSamples);
    } else if (data.symbolicSampleSize == kSample64) {
        if (!outBus.channelBuffers64) {
            writeOutputSilence(data);
            return kResultOk;
        }
        processAudio<double>(data, outBus.channelBuffers64, static_cast<int32_t>(outBus.numChannels), data.numSamples);
    }

    outBus.silenceFlags = synth_.isSilent() ? silenceFlagsForChannels(outBus.numChannels) : 0;
    return kResultOk;
}

template <typename SampleType>
void BreathalyzerProcessor::processAudio(ProcessData& data, SampleType** out, int32_t numChannels, int32_t numSamples) {
    const int32 eventCount = data.inputEvents ? data.inputEvents->getEventCount() : 0;
    int32 nextEvent = 0;
    Event event{};

    for (int32_t sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex) {
        while (data.inputEvents && nextEvent < eventCount) {
            if (data.inputEvents->getEvent(nextEvent, event) != kResultOk) {
                ++nextEvent;
                continue;
            }
            if (event.sampleOffset > sampleIndex) {
                break;
            }
            handleEvent(event);
            ++nextEvent;
        }

        double left = 0.0;
        double right = 0.0;
        synth_.processSample(left, right);

        out[0][sampleIndex] = static_cast<SampleType>(numChannels > 1 ? left : 0.5 * (left + right));
        if (numChannels > 1) {
            out[1][sampleIndex] = static_cast<SampleType>(right);
        }
    }

    while (data.inputEvents && nextEvent < eventCount) {
        if (data.inputEvents->getEvent(nextEvent, event) == kResultOk) {
            handleEvent(event);
        }
        ++nextEvent;
    }
}

Steinberg::tresult PLUGIN_API BreathalyzerProcessor::setState(IBStream* state) {
    if (!state) {
        return kResultFalse;
    }

    IBStreamer streamer(state, kLittleEndian);
    std::vector<double> values;
    values.reserve(kStateValueCount);
    double value = 0.0;
    while (streamer.readDouble(value)) {
        values.push_back(value);
    }

    for (size_t i = 0; i < paramOrder_.size(); ++i) {
        const ParamID pid = paramOrder_[i];
        const double loaded = i < values.size() ? values[i] : defaultNormalized(pid);
        applyNormalizedParam(pid, loaded);
    }
    syncSynth();
    return kResultOk;
}

Steinberg::tresult PLUGIN_API BreathalyzerProcessor::getState(IBStream* state) {
    if (!state) {
        return kResultFalse;
    }

    IBStreamer streamer(state, kLittleEndian);
    for (const auto pid : paramOrder_) {
        const auto it = paramState_.find(pid);
        const double value = it != paramState_.end() ? it->second : defaultNormalized(pid);
        streamer.writeDouble(value);
    }
    return kResultOk;
}

} // namespace breathalyzer
