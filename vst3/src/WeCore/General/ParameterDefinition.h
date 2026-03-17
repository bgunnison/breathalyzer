// Vendored from WE-Core / Songbird filter sources with minimal local trimming.
#pragma once

namespace ParameterDefinition {

template <typename T>
T BoundsCheck(T value, T minimum, T maximum) {
    if (value < minimum) {
        value = minimum;
    }
    if (value > maximum) {
        value = maximum;
    }
    return value;
}

template <typename T>
class BaseParameter {
public:
    const T minValue;
    const T maxValue;
    const T defaultValue;

    BaseParameter() = delete;

    BaseParameter(T minimum, T maximum, T defaultValueIn)
        : minValue(minimum), maxValue(maximum), defaultValue(defaultValueIn) {}

    T BoundsCheck(T value) const {
        return ParameterDefinition::BoundsCheck(value, minValue, maxValue);
    }
};

template <typename T>
class RangedParameter : public BaseParameter<T> {
public:
    using BaseParameter<T>::BaseParameter;

    T NormalisedToInternal(T value) const {
        return value * (this->maxValue - this->minValue) + this->minValue;
    }

    T InternalToNormalised(T value) const {
        return (value - this->minValue) / (this->maxValue - this->minValue);
    }
};

} // namespace ParameterDefinition
