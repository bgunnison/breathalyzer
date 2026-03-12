// Copyright (c) 2026 Brian R. Gunnison
// MIT License
#pragma once

#include <algorithm>
#include <cmath>

namespace breathalyzer {

inline constexpr double kDefaultShape = 0.5;
inline constexpr double kDefaultBreath = 0.65;
inline constexpr double kDefaultVoice = 0.35;
inline constexpr double kDefaultRelease = 0.45;
inline constexpr double kDefaultHumanize = 0.25;
inline constexpr double kDefaultTone = 0.55;

inline constexpr double kMinReleaseSeconds = 0.04;
inline constexpr double kMaxReleaseSeconds = 1.84;
inline constexpr size_t kStateValueCount = 6;

inline double releaseSecondsFromNormalized(double normalized) {
    normalized = std::clamp(normalized, 0.0, 1.0);
    return kMinReleaseSeconds + (normalized * normalized) * (kMaxReleaseSeconds - kMinReleaseSeconds);
}

inline double normalizedFromReleaseSeconds(double seconds) {
    seconds = std::clamp(seconds, kMinReleaseSeconds, kMaxReleaseSeconds);
    return std::sqrt((seconds - kMinReleaseSeconds) / (kMaxReleaseSeconds - kMinReleaseSeconds));
}

} // namespace breathalyzer
