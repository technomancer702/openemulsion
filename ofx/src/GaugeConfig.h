// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <algorithm>
#include <array>
#include "FilmModules.h"

namespace gauge {

struct Profile {
    const char* label;
    float scale, grainStrength;
};

// Creative format approximations, not measurements of specific stocks or gates.
inline constexpr std::array<Profile, 5> Profiles {{
    {"Custom", 1.0f, 1.0f}, {"8 mm", 2.60f, 1.35f},
    {"16 mm", 1.65f, 1.15f}, {"35 mm", 1.0f, 1.0f},
    {"65 mm", 0.70f, 0.80f}
}};

inline Profile prepare(const float* settings)
{
    return Profiles[std::clamp(static_cast<int>(settings[film::FilmGauge]), 0, 4)];
}

} // namespace gauge
