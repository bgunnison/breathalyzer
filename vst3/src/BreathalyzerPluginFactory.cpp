// Copyright (c) 2026 Brian R. Gunnison
// MIT License
#include "BreathalyzerController.h"
#include "BreathalyzerIDs.h"
#include "BreathalyzerProcessor.h"

#include "public.sdk/source/main/pluginfactory.h"

#ifdef BREATHALYZER_DEBUG_NAME
#define stringPluginName "Debug Breathalyzer"
#else
#define stringPluginName "Breathalyzer"
#endif

using namespace breathalyzer;
using namespace Steinberg;
using namespace Steinberg::Vst;

BEGIN_FACTORY_DEF(kBreathalyzerVst3Vendor, kBreathalyzerVst3Url, kBreathalyzerVst3Email)

    DEF_CLASS2(INLINE_UID_FROM_FUID(kBreathalyzerProcessorUID),
               PClassInfo::kManyInstances,
               kVstAudioEffectClass,
               stringPluginName,
               Vst::kDistributable,
               PlugType::kInstrumentSynth,
               kBreathalyzerVst3Version,
               kVstVersionString,
               BreathalyzerProcessor::createInstance)

    DEF_CLASS2(INLINE_UID_FROM_FUID(kBreathalyzerControllerUID),
               PClassInfo::kManyInstances,
               kVstComponentControllerClass,
               stringPluginName " Controller",
               0,
               "",
               kBreathalyzerVst3Version,
               kVstVersionString,
               BreathalyzerController::createInstance)

END_FACTORY
