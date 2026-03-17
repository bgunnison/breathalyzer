// Vendored from WE-Core / Songbird filter sources with minimal local trimming.
#pragma once

#include "General/ParameterDefinition.h"

namespace WECore::TPTSVF::Parameters {

class ModeParameter : public ParameterDefinition::BaseParameter<int> {
public:
    ModeParameter() : ParameterDefinition::BaseParameter<int>(BYPASS, HIGHPASS, BYPASS) {}

    static constexpr int BYPASS = 1;
    static constexpr int LOWPASS = 2;
    static constexpr int PEAK = 3;
    static constexpr int HIGHPASS = 4;
};

inline const ModeParameter FILTER_MODE{};

inline const ParameterDefinition::RangedParameter<double> CUTOFF(0.0, 20000.0, 20.0);
inline const ParameterDefinition::RangedParameter<double> Q(0.1, 20.0, 0.5);
inline const ParameterDefinition::RangedParameter<double> GAIN(0.0, 2.0, 1.0);

} // namespace WECore::TPTSVF::Parameters
