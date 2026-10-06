// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include "FilmResponseMath.h"

namespace response {

inline FilmResponseParameters prepare(const float* s)
{
    FilmResponseParameters p {};
    p.system = std::clamp(static_cast<int>(s[1]), 0, 5);
    const int printStyle = std::clamp(static_cast<int>(s[2]), 0, 3);
    p.density = s[7];

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

    const std::array<float, 4> printContrast {1.20f,1.12f,0.99f,1.08f};
    const std::array<float, 4> printToe {0.60f,0.35f,0.15f,0.35f};
    const std::array<float, 4> printKnee {0.67f,0.73f,0.80f,0.73f};
    const std::array<float, 4> printColor {1.0f,0.76f,0.28f,0.76f};
    const std::array<float, 4> lift {1.30f,1.0f,0.30f,1.0f};
    const float tone = std::clamp(s[32], -1.0f, 1.0f), rolloff = std::clamp(s[34], 0.0f, 1.0f);
    p.printTone = {printContrast[printStyle] * std::clamp(s[33], 0.5f, 2.0f) * (1.0f - 0.15f * tone),
        std::clamp(printToe[printStyle] - 0.25f * tone, 0.0f, 0.85f),
        std::clamp(printKnee[printStyle] + 0.06f * tone - (rolloff - 0.55f) * 0.25f, 0.55f, 0.93f), 1.0f};
    const float palette = (1.0f - std::clamp(s[11], 0.0f, 1.0f)) * printColor[printStyle];
    const std::array<float, 9> printMatrix {0.91f,0.065f,0.025f, 0.025f,0.95f,0.025f, 0.01f,0.085f,0.905f};
    for (int i = 0; i < 9; ++i) {
        const float identity = i % 4 == 0 ? 1.0f : 0.0f;
        p.printMatrix[i] = identity + (printMatrix[i] - identity) * palette;
    }
    p.printSat = std::clamp(s[36], 0.0f, 2.0f) * (1.0f - palette * 0.10f);
    p.printCast = palette * (1.0f - std::clamp(s[35], 0.0f, 1.0f));
    p.printLift = std::clamp(s[12], 0.0f, 1.0f) * 0.035f * lift[printStyle];
    p.printGamutKnee = 0.95f - palette * 0.30f;
    const float exposure = std::clamp(s[37], -2.0f, 2.0f);
    p.printGain = {std::exp2(exposure + std::clamp(s[38], -2.0f, 2.0f)),
                   std::exp2(exposure + std::clamp(s[39], -2.0f, 2.0f)),
                   std::exp2(exposure + std::clamp(s[40], -2.0f, 2.0f))};
    return p;
}

} // namespace response
