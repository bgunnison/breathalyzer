// Copyright (c) 2026 Brian R. Gunnison
// MIT License
#include "BreathalyzerController.h"

#include "BreathalyzerDefaults.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"
#include "vstgui/plugin-bindings/vst3editor.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace breathalyzer {
using namespace Steinberg;
using namespace Steinberg::Vst;

namespace {

void addAsciiParameter(ParameterContainer& parameters, const char* title, ParamID id, ParamValue defaultValue) {
    UString128 parameterTitle;
    UString128 parameterUnits;
    parameterTitle.fromAscii(title);
    parameterUnits.fromAscii("");
    parameters.addParameter(parameterTitle, parameterUnits, 0, defaultValue, ParameterInfo::kCanAutomate, id);
}

bool parseAsciiFloat(TChar* string, double& value) {
    UString128 ustring(string);
    char ascii[64]{};
    ustring.toAscii(ascii, static_cast<int32>(sizeof(ascii)));
    char* end = nullptr;
    value = std::strtod(ascii, &end);
    return end != ascii;
}

std::vector<double> upgradeStateValues(std::vector<double> values) {
    if (values.size() == kLegacyStateValueCount) {
        std::vector<double> upgraded;
        upgraded.reserve(kSplitGrowlStateValueCount);
        for (size_t i = 0; i < 7; ++i) {
            upgraded.push_back(values[i]);
        }

        const double legacyGrowl = values[7];
        const double legacyGrowlIntensity = values[8];
        upgraded.push_back(legacyGrowl);
        upgraded.push_back(legacyGrowlIntensity);
        upgraded.push_back(legacyGrowl);
        upgraded.push_back(legacyGrowlIntensity);
        values = upgraded;
    }

    if (values.size() == kSplitGrowlStateValueCount) {
        values.push_back(kDefaultVoiceSpeed);
    }

    return values;
}

} // namespace

Steinberg::tresult PLUGIN_API BreathalyzerController::initialize(FUnknown* context) {
    const tresult result = EditControllerEx1::initialize(context);
    if (result != kResultOk) {
        return result;
    }

    buildParamOrder();
    addAsciiParameter(parameters, "SHAPE", kParamShape, defaultNormalized(kParamShape));
    addAsciiParameter(parameters, "BREATH", kParamBreath, defaultNormalized(kParamBreath));
    addAsciiParameter(parameters, "VOICE", kParamVoice, defaultNormalized(kParamVoice));
    addAsciiParameter(parameters, "RELEASE", kParamRelease, defaultNormalized(kParamRelease));
    addAsciiParameter(parameters, "HUMANIZE", kParamHumanize, defaultNormalized(kParamHumanize));
    addAsciiParameter(parameters, "TONE", kParamTone, defaultNormalized(kParamTone));
    addAsciiParameter(parameters, "ATTACK", kParamAttack, defaultNormalized(kParamAttack));
    addAsciiParameter(parameters, "VOICE GROWL", kParamVoiceGrowl, defaultNormalized(kParamVoiceGrowl));
    addAsciiParameter(parameters,
                      "VOICE GROWL INTENSITY",
                      kParamVoiceGrowlIntensity,
                      defaultNormalized(kParamVoiceGrowlIntensity));
    addAsciiParameter(parameters, "NOISE GROWL", kParamNoiseGrowl, defaultNormalized(kParamNoiseGrowl));
    addAsciiParameter(parameters,
                      "NOISE GROWL INTENSITY",
                      kParamNoiseGrowlIntensity,
                      defaultNormalized(kParamNoiseGrowlIntensity));
    addAsciiParameter(parameters, "VOICE SPEED", kParamVoiceSpeed, defaultNormalized(kParamVoiceSpeed));

    for (const auto pid : paramOrder_) {
        paramState_[pid] = defaultNormalized(pid);
    }

    return kResultOk;
}

IPlugView* PLUGIN_API BreathalyzerController::createView(FIDString name) {
    if (FIDStringsEqual(name, ViewType::kEditor)) {
        return new VSTGUI::VST3Editor(this, "view", "breathalyzer.uidesc");
    }
    return EditControllerEx1::createView(name);
}

Steinberg::tresult PLUGIN_API BreathalyzerController::setComponentState(IBStream* state) {
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
    values = upgradeStateValues(values);

    for (size_t i = 0; i < paramOrder_.size(); ++i) {
        const ParamID pid = paramOrder_[i];
        const ParamValue loaded = i < values.size() ? values[i] : defaultNormalized(pid);
        setParamNormalized(pid, loaded);
    }

    return kResultOk;
}

Steinberg::tresult PLUGIN_API BreathalyzerController::getState(IBStream* state) {
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

Steinberg::tresult PLUGIN_API BreathalyzerController::setState(IBStream* state) {
    return setComponentState(state);
}

Steinberg::tresult PLUGIN_API BreathalyzerController::setParamNormalized(ParamID pid, ParamValue value) {
    paramState_[pid] = value;
    return EditControllerEx1::setParamNormalized(pid, value);
}

Steinberg::tresult PLUGIN_API BreathalyzerController::getParamStringByValue(ParamID pid,
                                                                            ParamValue valueNormalized,
                                                                            String128 string) {
    UString128 result(string);
    char text[32]{};
    if (pid == kParamAttack) {
        std::snprintf(text, sizeof(text), "%.3f s", attackSecondsFromNormalized(valueNormalized));
        result.fromAscii(text);
        return kResultOk;
    }

    if (pid == kParamRelease) {
        std::snprintf(text, sizeof(text), "%.2f s", releaseSecondsFromNormalized(valueNormalized));
        result.fromAscii(text);
        return kResultOk;
    }

    if (pid == kParamVoiceSpeed) {
        std::snprintf(text, sizeof(text), "%.2f Hz", voiceSpeedHzFromNormalized(valueNormalized));
        result.fromAscii(text);
        return kResultOk;
    }

    switch (pid) {
        case kParamShape:
        case kParamBreath:
        case kParamVoice:
        case kParamHumanize:
        case kParamTone:
        case kParamVoiceGrowl:
        case kParamVoiceGrowlIntensity:
        case kParamNoiseGrowl:
        case kParamNoiseGrowlIntensity:
            std::snprintf(text, sizeof(text), "%.0f", valueNormalized * 100.0);
            result.fromAscii(text);
            return kResultOk;
        default:
            break;
    }

    return EditControllerEx1::getParamStringByValue(pid, valueNormalized, string);
}

Steinberg::tresult PLUGIN_API BreathalyzerController::getParamValueByString(ParamID pid,
                                                                            TChar* string,
                                                                            ParamValue& valueNormalized) {
    double parsed = 0.0;
    if (!parseAsciiFloat(string, parsed)) {
        return kResultFalse;
    }

    if (pid == kParamAttack) {
        valueNormalized = normalizedFromAttackSeconds(parsed);
        return kResultOk;
    }

    if (pid == kParamRelease) {
        valueNormalized = normalizedFromReleaseSeconds(parsed);
        return kResultOk;
    }

    if (pid == kParamVoiceSpeed) {
        valueNormalized = normalizedFromVoiceSpeedHz(parsed);
        return kResultOk;
    }

    switch (pid) {
        case kParamShape:
        case kParamBreath:
        case kParamVoice:
        case kParamHumanize:
        case kParamTone:
        case kParamVoiceGrowl:
        case kParamVoiceGrowlIntensity:
        case kParamNoiseGrowl:
        case kParamNoiseGrowlIntensity:
            valueNormalized = std::clamp(parsed / 100.0, 0.0, 1.0);
            return kResultOk;
        default:
            break;
    }

    return EditControllerEx1::getParamValueByString(pid, string, valueNormalized);
}

void BreathalyzerController::buildParamOrder() {
    paramOrder_.clear();
    paramOrder_.push_back(kParamShape);
    paramOrder_.push_back(kParamBreath);
    paramOrder_.push_back(kParamVoice);
    paramOrder_.push_back(kParamRelease);
    paramOrder_.push_back(kParamHumanize);
    paramOrder_.push_back(kParamTone);
    paramOrder_.push_back(kParamAttack);
    paramOrder_.push_back(kParamVoiceGrowl);
    paramOrder_.push_back(kParamVoiceGrowlIntensity);
    paramOrder_.push_back(kParamNoiseGrowl);
    paramOrder_.push_back(kParamNoiseGrowlIntensity);
    paramOrder_.push_back(kParamVoiceSpeed);
}

ParamValue BreathalyzerController::defaultNormalized(ParamID pid) const {
    switch (pid) {
        case kParamShape: return kDefaultShape;
        case kParamBreath: return kDefaultBreath;
        case kParamVoice: return kDefaultVoice;
        case kParamRelease: return kDefaultRelease;
        case kParamHumanize: return kDefaultHumanize;
        case kParamTone: return kDefaultTone;
        case kParamAttack: return kDefaultAttack;
        case kParamVoiceGrowl: return kDefaultVoiceGrowl;
        case kParamVoiceGrowlIntensity: return kDefaultVoiceGrowlIntensity;
        case kParamNoiseGrowl: return kDefaultNoiseGrowl;
        case kParamNoiseGrowlIntensity: return kDefaultNoiseGrowlIntensity;
        case kParamVoiceSpeed: return kDefaultVoiceSpeed;
        default: break;
    }
    return 0.0;
}

} // namespace breathalyzer
