// Vendored from WE-Core / Songbird filter sources with minimal local trimming.
#pragma once

#include <cmath>
#include <limits>

namespace WECore::CoreMath {

constexpr long double LONG_PI = 3.14159265358979323846264338327950288L;
constexpr double DOUBLE_PI = static_cast<double>(LONG_PI);

constexpr long double LONG_TAU = 2.0L * LONG_PI;
constexpr double DOUBLE_TAU = static_cast<double>(LONG_TAU);

constexpr long double LONG_E = 2.71828182845904523536028747135266250L;
constexpr double DOUBLE_E = static_cast<double>(LONG_E);

template <typename T>
bool compareFloatsEqual(T x, T y, T tolerance = std::numeric_limits<T>::epsilon()) {
    return std::abs(x - y) < tolerance;
}

constexpr double MINUS_INF_DB = -200.0;

inline double linearTodB(double value) {
    return value > 0.0 ? std::fmax(20.0 * std::log10(value), MINUS_INF_DB) : MINUS_INF_DB;
}

inline double dBToLinear(double value) {
    return std::pow(10.0, value / 20.0);
}

} // namespace WECore::CoreMath
