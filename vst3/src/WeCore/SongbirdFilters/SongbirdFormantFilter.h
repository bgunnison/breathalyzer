// Vendored from WE-Core / Songbird filter sources with minimal local trimming.
#pragma once

#include "SongbirdFilters/Formant.h"
#include "WEFilters/TPTSVFilter.h"

#include <array>
#include <cstddef>
#include <type_traits>

namespace WECore::Songbird {

template <typename T, std::size_t NUM_FORMANTS>
class SongbirdFormantFilter {
    static_assert(std::is_floating_point<T>::value, "SongbirdFormantFilter requires floating point sample types");

public:
    SongbirdFormantFilter() {
        for (std::size_t index = 0; index < NUM_FORMANTS; ++index) {
            filters_[index].setMode(TPTSVF::Parameters::FILTER_MODE.PEAK);
            filters_[index].setQ(10.0);
        }
    }

    T process(T input) {
        T output = 0;
        for (std::size_t index = 0; index < filters_.size(); ++index) {
            T sample = input;
            filters_[index].processBlock(&sample, 1);
            output += sample;
        }
        return output;
    }

    bool setFormants(const std::array<Formant, NUM_FORMANTS>& formants) {
        if (filters_.size() != formants.size()) {
            return false;
        }

        for (std::size_t index = 0; index < filters_.size(); ++index) {
            filters_[index].setCutoff(formants[index].frequency);
            filters_[index].setGain(CoreMath::dBToLinear(formants[index].gaindB));
        }
        return true;
    }

    void setSampleRate(double sampleRate) {
        for (auto& filter : filters_) {
            filter.setSampleRate(sampleRate);
        }
    }

    void reset() {
        for (auto& filter : filters_) {
            filter.reset();
        }
    }

private:
    std::array<TPTSVF::TPTSVFilter<T>, NUM_FORMANTS> filters_{};
};

} // namespace WECore::Songbird
