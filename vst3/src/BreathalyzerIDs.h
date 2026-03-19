// Copyright (c) 2026 Brian R. Gunnison
// MIT License
#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace breathalyzer {

#ifdef BREATHALYZER_DEBUG_UIDS
// {A6B98552-6F58-4D75-8A8A-9319F2432F53}
static const Steinberg::FUID kBreathalyzerProcessorUID(0xA6B98552, 0x6F584D75, 0x8A8A9319, 0xF2432F53);
// {08D5862A-3227-41F4-BB09-FB461F4B8A62}
static const Steinberg::FUID kBreathalyzerControllerUID(0x08D5862A, 0x322741F4, 0xBB09FB46, 0x1F4B8A62);
#else
// {72E418B1-3DE1-4D7E-BF67-6D4F6CA5FAD5}
static const Steinberg::FUID kBreathalyzerProcessorUID(0x72E418B1, 0x3DE14D7E, 0xBF676D4F, 0x6CA5FAD5);
// {6D747494-5600-40EE-A03F-682FC2A4991A}
static const Steinberg::FUID kBreathalyzerControllerUID(0x6D747494, 0x560040EE, 0xA03F682F, 0xC2A4991A);
#endif

#ifdef BREATHALYZER_DEBUG_NAME
constexpr Steinberg::FIDString kBreathalyzerVst3Name = "Debug Breathalyzer";
#else
constexpr Steinberg::FIDString kBreathalyzerVst3Name = "Breathalyzer";
#endif
constexpr Steinberg::FIDString kBreathalyzerVst3Vendor = "VirtualRobot";
constexpr Steinberg::FIDString kBreathalyzerVst3Version = "0.1.0 (" __DATE__ " " __TIME__ ")";
constexpr const char kBreathalyzerVst3Build[] = __DATE__ " " __TIME__;
constexpr Steinberg::FIDString kBreathalyzerVst3Url = "https://ableplugs.local/breathalyzer";
constexpr Steinberg::FIDString kBreathalyzerVst3Email = "support@ableplugs.local";

enum ParamIDs : Steinberg::Vst::ParamID {
    kParamShape = 0,
    kParamBreath = 1,
    kParamVoice = 2,
    kParamRelease = 3,
    kParamHumanize = 4,
    kParamTone = 5,
    kParamAttack = 6,
    kParamVoiceGrowl = 7,
    kParamVoiceGrowlIntensity = 8,
    kParamNoiseGrowl = 9,
    kParamNoiseGrowlIntensity = 10,
    kParamVoiceSpeed = 11,
    kParamUtterance = 12,
    kParamUtteranceSync = 13
};

} // namespace breathalyzer
