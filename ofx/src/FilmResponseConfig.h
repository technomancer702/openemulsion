// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include "FilmResponseMath.h"
#include "FilmModules.h"
#include "PrintStyleConfig.h"

namespace response {

inline FilmResponseParameters prepare(const float* s)
{
    FilmResponseParameters p {};
    p.system = std::clamp(static_cast<int>(s[1]), 0, 5);
    p.density = s[7];
    p.colorStrength = std::clamp(s[film::NegativeColorStrength], 0.0f, 1.0f);
    p.toneStrength = std::clamp(s[film::NegativeToneStrength], 0.0f, 1.0f);
    p.printColorStrength = std::clamp(s[film::PrintColorStrength], 0.0f, 1.0f);
    p.printToneStrength = std::clamp(s[film::PrintToneStrength], 0.0f, 1.0f);
    p.push = std::clamp(s[film::PushPull], -3.0f, 3.0f);
    p.developmentContrast = std::exp2(p.push * 0.18f);
    p.richness = std::clamp(s[film::ColorRichness], -1.0f, 1.0f);
    p.splitAmount = std::clamp(s[film::SplitTone], 0.0f, 1.0f);
    p.splitPivot = std::clamp(s[film::SplitPivot], 0.2f, 0.8f);
    p.splitWidth = std::clamp(s[film::SplitWidth], 0.0f, 0.3f);
    p.splitShadows = std::clamp(s[film::SplitShadows], 0.0f, 2.0f);
    p.splitHighlights = std::clamp(s[film::SplitHighlights], 0.0f, 2.0f);
    const float hue = std::clamp(s[film::SplitHue], 0.0f, 360.0f) / 60.0f;
    auto hueChannel = [hue](float offset) {
        const float t = std::fmod(hue + offset, 6.0f);
        return std::clamp(std::abs(t - 3.0f) - 1.0f, 0.0f, 1.0f);
    };
    p.splitTint = {hueChannel(0), hueChannel(4), hueChannel(2)};
    const float tintLuma = response_luma(p.splitTint);
    p.splitTint.r -= tintLuma; p.splitTint.g -= tintLuma; p.splitTint.b -= tintLuma;

    // Original creative profiles, not measured or branded stock calibrations.
    const std::array<std::array<float, 9>, 6> matrices {{
        {0.86f,0.10f,0.04f, 0.035f,0.94f,0.025f, 0.02f,0.10f,0.88f},
        {0.88f,0.15f,-0.03f, 0.045f,0.94f,0.015f, 0.06f,0.16f,0.78f},
        {0.94f,0.04f,0.02f, 0.02f,0.96f,0.02f, 0.02f,0.05f,0.93f},
        {1.03f,-0.045f,0.015f, 0.02f,0.96f,0.02f, 0.035f,0.085f,0.88f},
        {1,0,0, 0,1,0, 0,0,1},
        {1.04f,-0.02f,-0.02f, -0.015f,1.035f,-0.02f, -0.01f,0.02f,0.99f}
    }};
    const std::array<float, 6> contrast {1.0f,1.035f,1.17f,1.025f,1.06f,1.16f};
    const std::array<float, 6> saturation {1.02f,0.96f,0.52f,1.04f,1.0f,1.12f};
    const float shoulder = std::clamp(s[28], 0.0f, 1.0f);
    p.negativeTone = {std::clamp(s[10], 0.5f, 2.0f) * contrast[p.system],
        std::clamp(s[9], 0.0f, 1.0f) * 0.85f, 0.88f - 0.18f * shoulder,
        2.1f - 0.90f * shoulder};
    const float crosstalk = std::clamp(s[29], 0.0f, 1.0f);
    for (int i = 0; i < 9; ++i) {
        const float identity = i % 4 == 0 ? 1.0f : 0.0f;
        p.negativeMatrix[i] = identity + (matrices[p.system][i] - identity) * crosstalk;
    }
    p.negativeSat = std::clamp(s[8], 0.0f, 2.0f) * saturation[p.system];
    p.negativeCompression = std::clamp(s[30], 0.0f, 1.0f);
    p.skinHue = std::clamp(s[31], -1.0f, 1.0f);

    const auto print = printstyle::resolve(s);
    const float tone = std::clamp(print[printstyle::Tone], -1.0f, 1.0f);
    const float rolloff = std::clamp(print[printstyle::Rolloff], 0.0f, 1.0f);
    // Piecewise-linear anchors keep Full/Standard/Extended reachable on one curve.
    const float baseContrast = 1.12f - tone * (tone < 0.0f ? 0.08f : 0.13f);
    p.printTone = {baseContrast * std::clamp(print[printstyle::Contrast], 0.5f, 2.0f),
        std::clamp(0.35f - tone * (tone < 0.0f ? 0.25f : 0.20f), 0.0f, 0.85f),
        std::clamp(0.73f + tone * (tone < 0.0f ? 0.06f : 0.07f) - (rolloff - 0.55f) * 0.25f, 0.55f, 0.93f), 1.0f};
    const float palette = 1.0f - std::clamp(print[printstyle::Color], 0.0f, 1.0f);
    const std::array<float, 9> printMatrix {0.91f,0.065f,0.025f, 0.025f,0.95f,0.025f, 0.01f,0.085f,0.905f};
    for (int i = 0; i < 9; ++i) {
        const float identity = i % 4 == 0 ? 1.0f : 0.0f;
        p.printMatrix[i] = identity + (printMatrix[i] - identity) * palette;
    }
    p.printSat = std::clamp(print[printstyle::Saturation], 0.0f, 2.0f) * (1.0f - palette * 0.10f);
    p.printCast = palette * (1.0f - std::clamp(print[printstyle::Neutralize], 0.0f, 1.0f));
    p.printLift = std::clamp(print[printstyle::BlackPoint], 0.0f, 1.0f) * 0.035f;
    p.printGamutKnee = 0.95f - palette * 0.30f;
    const float exposure = std::clamp(s[37], -2.0f, 2.0f);
    p.printGain = {std::exp2(exposure + std::clamp(s[38], -2.0f, 2.0f)),
                   std::exp2(exposure + std::clamp(s[39], -2.0f, 2.0f)),
                   std::exp2(exposure + std::clamp(s[40], -2.0f, 2.0f))};
    return p;
}

} // namespace response
