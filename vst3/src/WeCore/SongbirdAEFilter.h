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
        for (auto& filter : leftVowelFilters_) {
            filter.setSampleRate(sampleRate);
        }
        for (auto& filter : rightVowelFilters_) {
            filter.setSampleRate(sampleRate);
        }
        airLeft_.setSampleRate(sampleRate);
        airRight_.setSampleRate(sampleRate);
    }

    void reset() {
        for (auto& filter : leftVowelFilters_) {
            filter.reset();
        }
        for (auto& filter : rightVowelFilters_) {
            filter.reset();
        }
        airLeft_.reset();
        airRight_.reset();
    }

    void process(double inputLeft,
                 double inputRight,
                 double morph,
                 double utterance,
                 double& outputLeft,
                 double& outputRight) {
        morph = std::clamp(morph, 0.0, 1.0);
        utterance = std::clamp(utterance, 0.0, 1.0);

        std::array<double, kNumVowels> leftVowels{};
        std::array<double, kNumVowels> rightVowels{};
        for (std::size_t index = 0; index < kNumVowels; ++index) {
            leftVowels[index] = leftVowelFilters_[index].process(inputLeft);
            rightVowels[index] = rightVowelFilters_[index].process(inputRight);
        }

        const double airLeft = airLeft_.process(inputLeft);
        const double airRight = airRight_.process(inputRight);

        const double pairPosition = utterance * static_cast<double>(kNumPairs - 1);
        const std::size_t pairIndex =
            std::min<std::size_t>(static_cast<std::size_t>(pairPosition), kNumPairs - 1);
        const double pairBlend = pairPosition - static_cast<double>(pairIndex);

        const double pairLeftA = mixPair(leftVowels, pairIndex, morph);
        const double pairRightA = mixPair(rightVowels, pairIndex, morph);

        double vowelLeft = pairLeftA;
        double vowelRight = pairRightA;
        if (pairIndex + 1 < kNumPairs) {
            const double pairLeftB = mixPair(leftVowels, pairIndex + 1, morph);
            const double pairRightB = mixPair(rightVowels, pairIndex + 1, morph);
            vowelLeft = lerp(pairLeftA, pairLeftB, pairBlend);
            vowelRight = lerp(pairRightA, pairRightB, pairBlend);
        }

        outputLeft = vowelLeft + airLeft * kAirGain;
        outputRight = vowelRight + airRight * kAirGain;
    }

    SongbirdAEFilter() {
        for (std::size_t index = 0; index < kNumVowels; ++index) {
            leftVowelFilters_[index].setFormants(kVowels[index]);
            rightVowelFilters_[index].setFormants(kVowels[index]);
        }
        airLeft_.setFormants(kAirFormants);
        airRight_.setFormants(kAirFormants);
    }

private:
    static constexpr std::size_t kNumVowels = 5;
    static constexpr std::size_t kNumPairs = kNumVowels - 1;
    static constexpr double kAirGain = 0.35;

    static double lerp(double a, double b, double t) {
        return a + (b - a) * t;
    }

    static double mixPair(const std::array<double, kNumVowels>& vowels, std::size_t pairIndex, double morph) {
        return lerp(vowels[pairIndex], vowels[pairIndex + 1], morph);
    }

    inline static const std::array<std::array<WECore::Songbird::Formant, 2>, kNumVowels> kVowels{{
        {
            WECore::Songbird::Formant(800.0, 0.0),
            WECore::Songbird::Formant(1150.0, -4.0),
        },
        {
            WECore::Songbird::Formant(400.0, 0.0),
            WECore::Songbird::Formant(2100.0, -24.0),
        },
        {
            WECore::Songbird::Formant(350.0, 0.0),
            WECore::Songbird::Formant(2400.0, -20.0),
        },
        {
            WECore::Songbird::Formant(450.0, 0.0),
            WECore::Songbird::Formant(800.0, -9.0),
        },
        {
            WECore::Songbird::Formant(325.0, 0.0),
            WECore::Songbird::Formant(700.0, -12.0),
        },
    }};

    inline static const std::array<WECore::Songbird::Formant, 2> kAirFormants{
        WECore::Songbird::Formant(2700.0, -15.0),
        WECore::Songbird::Formant(3500.0, -25.0),
    };

    std::array<WECore::Songbird::SongbirdFormantFilter<double, 2>, kNumVowels> leftVowelFilters_{};
    std::array<WECore::Songbird::SongbirdFormantFilter<double, 2>, kNumVowels> rightVowelFilters_{};
    WECore::Songbird::SongbirdFormantFilter<double, 2> airLeft_{};
    WECore::Songbird::SongbirdFormantFilter<double, 2> airRight_{};
};

} // namespace breathalyzer::wecore
