// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <algorithm>
#include <array>
#include <cmath>

#include "FilmModules.h"
#include "GaugeConfig.h"
#include "GrainMath.h"

namespace grain {

using PackedSettings = std::array<float, film::SettingsCount>;

inline GrainParameters prepare(const float* settings, int height, double time)
{
    const int style = static_cast<int>(settings[3]);
    const float size = std::clamp(settings[17], 0.0f, 1.0f);
    const float softness = std::clamp(settings[20], 0.0f, 1.0f);
    const float scale = std::max(height / 1080.0f, 0.125f);
    const float styleSize = style == 0 ? 0.65f : (style == 2 ? 1.55f : 1.0f);
    const float detail = style == 0 ? 0.65f : (style == 2 ? 0.32f : 0.50f);
    const auto frame = static_cast<int64_t>(std::llround(time * 256.0));
    const auto seed = static_cast<GrainUInt>(settings[25]);
    const auto format = gauge::prepare(settings);
    return {1.0f / ((0.55f + size * 2.65f) * styleSize * scale * format.scale),
            detail * (1.0f - softness), std::clamp(settings[18], 0.0f, 1.0f),
            std::clamp(settings[21], 0.0f, 1.0f) * (1.0f - film::monochromeStrength(settings)),
            settings[16] * format.grainStrength, settings[22], settings[23], settings[24],
            grain_hash(static_cast<GrainUInt>(frame) ^ grain_hash(seed + 0x9e3779b9u)), style == 3 ? 1 : 0};
}

} // namespace grain
