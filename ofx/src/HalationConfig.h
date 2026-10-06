// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "GaugeConfig.h"
#include "HalationMath.h"

namespace halation {

struct Configuration {
    HalationParameters key;
    int downsample;
    float radiusScale, auraRadius;
};

inline Configuration prepare(const float* settings, int height)
{
    const float resolution = std::max(height / 1080.0f, 0.125f);
    // Scale the work grid with frame size to keep blur sampling cost bounded.
    const int step = std::clamp(2 * static_cast<int>(std::lround(resolution)), 2, 16);
    const float hue = std::clamp(settings[film::HalationColor], 0.0f, 1.0f);
    const float green = 0.08f + hue * 0.32f, blue = 0.015f + hue * 0.06f;
    const float normalization = 0.291827f / (0.55f * 0.2126f + green * 0.7152f + blue * 0.0722f);
    return {{std::clamp(settings[film::HalationThreshold], 0.0f, 2.0f),
             1.0f / std::clamp(settings[film::HalationSoftness], 0.01f, 2.0f),
             0.55f * normalization, green * normalization, blue * normalization},
            step, resolution * gauge::prepare(settings).scale * 2.0f / step,
            std::clamp(settings[film::AuraRadius], 0.0f, 2.0f)};
}

} // namespace halation
