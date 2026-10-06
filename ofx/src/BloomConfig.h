// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "BloomMath.h"
#include "FilmModules.h"

namespace bloom {

struct Configuration {
    BloomParameters parameters;
    int downsample;
    float sigma;
};

inline Configuration prepare(const float* s, int height)
{
    const float resolution = std::max(height / 1080.0f, 0.125f);
    const int step = std::clamp(4 * static_cast<int>(std::lround(resolution)), 4, 32);
    const float threshold = std::clamp(s[film::BloomThreshold], 0.0f, 2.0f);
    const float transition = std::clamp(s[film::BloomSoftness], 0.01f, 2.0f);
    const float low = color_decode(threshold, ColorSRGB);
    const float high = color_decode(threshold + transition, ColorSRGB);
    return {{low, 1.0f / (high - low),
             (film::modulesForSettings(s) & film::Bloom) ? std::clamp(s[film::BloomAmount], 0.0f, 2.0f) : 0.0f,
             std::clamp(s[film::BloomColor], 0.0f, 1.0f), std::clamp(s[film::BloomProtection], 0.0f, 1.0f)},
            step, std::max(0.5f, (1.5f + std::clamp(s[film::BloomRadius], 0.0f, 2.0f) * 5.0f) * resolution * 4.0f / step)};
}

} // namespace bloom
