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
inline constexpr double kDefaultAttack = 0.30;
inline constexpr double kDefaultGrowl = 0.0;
inline constexpr double kDefaultGrowlIntensity = 0.0;

inline constexpr double kMinAttackSeconds = 0.002;
inline constexpr double kMaxAttackSeconds = 0.90;
inline constexpr double kMinReleaseSeconds = 0.04;
inline constexpr double kMaxReleaseSeconds = 1.84;
inline constexpr size_t kStateValueCount = 9;

inline double attackSecondsFromNormalized(double normalized) {
    normalized = std::clamp(normalized, 0.0, 1.0);
    return kMinAttackSeconds + (normalized * normalized) * (kMaxAttackSeconds - kMinAttackSeconds);
}

inline double normalizedFromAttackSeconds(double seconds) {
    seconds = std::clamp(seconds, kMinAttackSeconds, kMaxAttackSeconds);
    return std::sqrt((seconds - kMinAttackSeconds) / (kMaxAttackSeconds - kMinAttackSeconds));
}

inline double releaseSecondsFromNormalized(double normalized) {
    normalized = std::clamp(normalized, 0.0, 1.0);
    return kMinReleaseSeconds + (normalized * normalized) * (kMaxReleaseSeconds - kMinReleaseSeconds);
}

inline double normalizedFromReleaseSeconds(double seconds) {
    seconds = std::clamp(seconds, kMinReleaseSeconds, kMaxReleaseSeconds);
    return std::sqrt((seconds - kMinReleaseSeconds) / (kMaxReleaseSeconds - kMinReleaseSeconds));
}

} // namespace breathalyzer
