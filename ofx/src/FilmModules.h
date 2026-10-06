// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <algorithm>

namespace film {

enum Module {
    Negative = 1, Print = 2, Halation = 4, Aura = 8, Grain = 16, All = 31
};

enum SettingIndex {
    NegativeColorStrength = 41, NegativeToneStrength,
    PrintColorStrength, PrintToneStrength, FilmGauge,
    HalationThreshold, HalationSoftness, HalationColor, AuraRadius,
    SettingsCount
};
constexpr int ModuleIndex = 19;

inline int modulesForMode(int mode, int enabled)
{
    switch (mode) {
    case 0: return enabled & All;
    case 1: return enabled & (Negative | Print);
    case 2: return enabled & (Halation | Aura | Grain);
    case 3: return enabled & Grain;
    case 5: return enabled & (Halation | Aura);
    default: return 0;
    }
}

inline float monochromeStrength(const float* settings)
{
    const int modules = modulesForMode(static_cast<int>(settings[0]), static_cast<int>(settings[ModuleIndex]));
    return (modules & Negative) && static_cast<int>(settings[1]) == 4 ?
        std::clamp(settings[NegativeColorStrength], 0.0f, 1.0f) : 0.0f;
}

inline bool isIdentity(int mode, int modules, float halation, float aura, float grain)
{
    if (mode == 5) return false;
    const int active = modulesForMode(mode, modules);
    return !(active & (Negative | Print)) &&
           (!(active & Halation) || halation <= 0.0f) &&
           (!(active & Aura) || aura <= 0.0f) &&
           (!(active & Grain) || grain <= 0.0f);
}

} // namespace film
