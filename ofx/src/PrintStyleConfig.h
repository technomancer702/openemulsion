// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <algorithm>
#include <array>

namespace printstyle {

enum Style { Full, Standard, Extended, Custom };
enum Control { Tone, Contrast, Rolloff, Color, Neutralize, Saturation, BlackPoint, ControlCount };

struct Parameter {
    const char* name;
    int setting;
};

inline constexpr std::array<const char*, 4> Labels {{"Full (Film Print)", "Standard", "Extended (Telecine)", "Custom"}};
inline constexpr std::array<Parameter, ControlCount> Parameters {{
    {"printTone",32}, {"printContrast",33}, {"printRolloff",34}, {"printColor",11},
    {"printNeutralize",35}, {"printSaturation",36}, {"blackPoint",12}
}};

// Original recipes expressed entirely in the same controls exposed by Custom.
// Standard retains the previous default print response; no hidden base profile.
inline constexpr std::array<std::array<double, ControlCount>, 3> Presets {{
    {-1.0, 1.0, 0.55, 0.25, 0.0, 1.0, 0.585},
    { 0.0, 1.0, 0.55, 0.43, 0.0, 1.0, 0.450},
    { 1.0, 1.0, 0.55, 0.79, 1.0, 1.0, 0.135}
}};

inline bool isCustom(int style)
{
    return std::clamp(style, 0, static_cast<int>(Custom)) == Custom;
}

template<class Writer>
inline void applyPreset(int style, Writer write)
{
    style = std::clamp(style, 0, static_cast<int>(Custom));
    if (style == Custom) return;
    for (int control = 0; control < ControlCount; ++control)
        write(control, Presets[style][control]);
}

inline std::array<float, ControlCount> resolve(const float* settings)
{
    std::array<float, ControlCount> values;
    for (int control = 0; control < ControlCount; ++control)
        values[control] = settings[Parameters[control].setting];
    applyPreset(static_cast<int>(settings[2]), [&](int control, double value) {
        values[control] = static_cast<float>(value);
    });
    return values;
}

} // namespace printstyle
