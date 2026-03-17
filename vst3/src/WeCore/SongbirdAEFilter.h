// Breathalyzer-specific wrapper around the vendored Songbird formant filters.
#pragma once

#include "SongbirdFilters/Formant.h"
#include "SongbirdFilters/SongbirdFormantFilter.h"

#include <array>
#include <algorithm>

namespace breathalyzer::wecore {

class SongbirdAEFilter {
public:
    void setSampleRate(double sampleRate) {
        filterALeft_.setSampleRate(sampleRate);
        filterARight_.setSampleRate(sampleRate);
        filterELeft_.setSampleRate(sampleRate);
        filterERight_.setSampleRate(sampleRate);
        airLeft_.setSampleRate(sampleRate);
        airRight_.setSampleRate(sampleRate);
    }

    void reset() {
        filterALeft_.reset();
        filterARight_.reset();
        filterELeft_.reset();
        filterERight_.reset();
        airLeft_.reset();
        airRight_.reset();
    }

    void process(double inputLeft, double inputRight, double morph, double& outputLeft, double& outputRight) {
        morph = std::clamp(morph, 0.0, 1.0);

        const double vowelALeft = filterALeft_.process(inputLeft);
        const double vowelARight = filterARight_.process(inputRight);
        const double vowelELeft = filterELeft_.process(inputLeft);
        const double vowelERight = filterERight_.process(inputRight);
        const double airLeft = airLeft_.process(inputLeft);
        const double airRight = airRight_.process(inputRight);

        outputLeft = vowelALeft * (1.0 - morph) + vowelELeft * morph + airLeft * kAirGain;
        outputRight = vowelARight * (1.0 - morph) + vowelERight * morph + airRight * kAirGain;
    }

    SongbirdAEFilter() {
        filterALeft_.setFormants(kVowelA);
        filterARight_.setFormants(kVowelA);
        filterELeft_.setFormants(kVowelE);
        filterERight_.setFormants(kVowelE);
        airLeft_.setFormants(kAirFormants);
        airRight_.setFormants(kAirFormants);
    }

private:
    static constexpr double kAirGain = 0.35;

    inline static const std::array<WECore::Songbird::Formant, 2> kVowelA{
        WECore::Songbird::Formant(800.0, 0.0),
        WECore::Songbird::Formant(1150.0, -4.0),
    };

    inline static const std::array<WECore::Songbird::Formant, 2> kVowelE{
        WECore::Songbird::Formant(400.0, 0.0),
        WECore::Songbird::Formant(2100.0, -24.0),
    };

    inline static const std::array<WECore::Songbird::Formant, 2> kAirFormants{
        WECore::Songbird::Formant(2700.0, -15.0),
        WECore::Songbird::Formant(3500.0, -25.0),
    };

    WECore::Songbird::SongbirdFormantFilter<double, 2> filterALeft_{};
    WECore::Songbird::SongbirdFormantFilter<double, 2> filterARight_{};
    WECore::Songbird::SongbirdFormantFilter<double, 2> filterELeft_{};
    WECore::Songbird::SongbirdFormantFilter<double, 2> filterERight_{};
    WECore::Songbird::SongbirdFormantFilter<double, 2> airLeft_{};
    WECore::Songbird::SongbirdFormantFilter<double, 2> airRight_{};
};

} // namespace breathalyzer::wecore
