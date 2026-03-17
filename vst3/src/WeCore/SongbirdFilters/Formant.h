// Vendored from WE-Core / Songbird filter sources with minimal local trimming.
#pragma once

namespace WECore::Songbird {

class Formant {
public:
    Formant() : frequency(0.0), gaindB(0.0) {}
    Formant(double frequencyIn, double gaindBIn) : frequency(frequencyIn), gaindB(gaindBIn) {}

    double frequency;
    double gaindB;
};

} // namespace WECore::Songbird
