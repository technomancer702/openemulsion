// SPDX-License-Identifier: MPL-2.0

#pragma once

namespace film {

enum Module {
    Negative = 1, Print = 2, Halation = 4, Aura = 8, Grain = 16, All = 31
};

constexpr int SettingsCount = 41;
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
