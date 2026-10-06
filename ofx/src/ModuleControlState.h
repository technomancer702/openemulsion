// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <array>
#include <string_view>

#include "FilmModules.h"
#include "PrintStyleConfig.h"

namespace moduleui {

struct Toggle {
    const char* name;
    int module;
};

inline constexpr std::array<Toggle, 7> Toggles {{
    {"enableNegative", film::Negative}, {"enableDevelopment", film::Development},
    {"enablePrint", film::Print}, {"enableHalation", film::Halation},
    {"enableAura", film::Aura}, {"enableBloom", film::Bloom}, {"enableGrain", film::Grain}
}};

struct Control {
    const char* name;
    int modules;
};

inline constexpr Control Controls[] {
    {"system", film::Negative}, {"negativeColorStrength", film::Negative},
    {"negativeToneStrength", film::Negative}, {"exposure", film::Negative},
    {"temperature", film::Negative}, {"tint", film::Negative}, {"density", film::Negative},
    {"saturation", film::Negative}, {"toe", film::Negative}, {"contrast", film::Negative},
    {"negativeShoulder", film::Negative}, {"negativeCrosstalk", film::Negative},
    {"gamutCompression", film::Negative}, {"skinHue", film::Negative},
    {"pushPull", film::Development}, {"colorRichness", film::Development},
    {"splitTone", film::Development}, {"splitHue", film::Development},
    {"splitPivot", film::Development}, {"splitWidth", film::Development},
    {"splitShadows", film::Development}, {"splitHighlights", film::Development},
    {"printStyle", film::Print}, {"printColorStrength", film::Print},
    {"printToneStrength", film::Print}, {"printExposure", film::Print},
    {"printRed", film::Print}, {"printGreen", film::Print}, {"printBlue", film::Print},
    {"halation", film::Halation}, {"halationRadius", film::Halation},
    {"halationThreshold", film::Halation}, {"halationSoftness", film::Halation},
    {"halationColor", film::Halation}, {"aura", film::Aura}, {"auraRadius", film::Aura},
    {"bloom", film::Bloom}, {"bloomRadius", film::Bloom}, {"bloomThreshold", film::Bloom},
    {"bloomSoftness", film::Bloom}, {"bloomColor", film::Bloom}, {"bloomProtection", film::Bloom},
    {"grainStyle", film::Grain}, {"grain", film::Grain}, {"grainSize", film::Grain},
    {"grainSoftness", film::Grain}, {"grainRoughness", film::Grain}, {"grainColor", film::Grain},
    {"grainShadows", film::Grain}, {"grainMidtones", film::Grain}, {"grainHighlights", film::Grain},
    {"grainSeed", film::Grain}, {"grainStretch", film::Grain}, {"grainRed", film::Grain},
    {"grainGreen", film::Grain}, {"grainBlue", film::Grain},
    {"sourceSpace", film::All}, {"outputSpace", film::Negative | film::Development | film::Print},
    {"filmGauge", film::Halation | film::Aura | film::Grain}
};

// Unlike rendering, the UI keeps a neutral Development module editable.
inline bool controlEnabled(int mode, int enabled, int modules)
{
    return (film::modulesForMode(mode, enabled) & modules) != 0;
}

inline bool printRecipeEnabled(int mode, int enabled, int style)
{
    return controlEnabled(mode, enabled, film::Print) && printstyle::isCustom(style);
}

inline bool semanticControlEnabled(std::string_view name, int mode, int enabled, int system, double colorStrength)
{
    const bool mono = system == 4 && controlEnabled(mode, enabled, film::Negative);
    if (!mono) return true;
    if (name == "negativeCrosstalk" || name == "skinHue") return false;
    if (colorStrength < 1.0) return true;
    return name != "saturation" && name != "density" && name != "gamutCompression" && name != "grainColor";
}

template<class Writer>
inline void applyMode(int mode, Writer write)
{
    const int modules = film::modulesForMode(mode, film::All);
    for (size_t i = 0; i < Toggles.size(); ++i)
        write(i, (modules & Toggles[i].module) != 0);
}

} // namespace moduleui
