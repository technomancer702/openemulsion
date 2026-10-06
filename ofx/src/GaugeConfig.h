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

enum Format {
    Custom, Standard8, Super8, Standard16, Super16,
    Standard35, Super35, Standard65, LargeFormat70, Count
};

// Creative format approximations, not measurements of specific stocks or gates.
inline constexpr std::array<Profile, Count> Profiles {{
    {"Custom", 1.0f, 1.0f}, {"8 mm", 2.60f, 1.35f},
    {"Super 8", 2.35f, 1.28f}, {"16 mm", 1.65f, 1.15f},
    {"Super 16", 1.45f, 1.10f}, {"35 mm", 1.0f, 1.0f},
    {"Super 35", 0.90f, 0.95f}, {"65 mm", 0.70f, 0.80f},
    {"70 mm (15-perf)", 0.50f, 0.70f}
}};

inline Profile prepare(const float* settings)
{
    return Profiles[std::clamp(static_cast<int>(settings[film::FilmGauge]), 0, Count - 1)];
}

} // namespace gauge
