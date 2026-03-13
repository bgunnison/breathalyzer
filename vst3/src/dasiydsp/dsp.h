/*
Copyright (c) 2020 Electrosmith, Corp, Emilie Gillet

Use of this source code is governed by an MIT-style
license that can be found in the LICENSE file or at
https://opensource.org/licenses/MIT.
*/
#pragma once

#include <cmath>

#ifndef PI_F
#define PI_F 3.1415927410125732421875f
#endif

#ifndef TWOPI_F
#define TWOPI_F (2.0f * PI_F)
#endif

namespace daisysp {

inline float mtof(float midiNote) {
    return std::pow(2.0f, (midiNote - 69.0f) / 12.0f) * 440.0f;
}

} // namespace daisysp
