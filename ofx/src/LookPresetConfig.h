// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <array>
#include <string_view>

#include "FilmModules.h"
#include "GaugeConfig.h"
#include "ModuleControlState.h"

namespace look {

enum Preset { Custom, Daylight50, Daylight250, Tungsten200, Tungsten500,
              ReversalMono, ClassicCinema, SoftPortrait, NeonNights,
              SeventiesPrint, BleachBypass, Super8HomeMovie, SilverNoir, Count };

inline constexpr std::array<const char*, Count> Labels {{
    "Custom", "50D Daylight (Inspired)", "250D Daylight (Inspired)",
    "200T Tungsten (Inspired)", "500T Tungsten (Inspired)",
    "B&W Reversal (Inspired)", "Classic Cinema", "Soft Portrait", "Neon Nights",
    "Seventies Print", "Bleach Bypass", "Super 8 Home Movie", "Silver Noir"
}};

enum Kind { Choice, Double };
struct Control {
    const char* name;
    int setting;
    Kind kind;
    double initial, minimum, maximum;
};

// Complete recipes replace creative tuning, but never camera balance, encoding, or grain seed.
inline constexpr Control Controls[] {
    {"mode",0,Choice,0,0,6}, {"system",1,Choice,0,0,5},
    {"printStyle",2,Choice,printstyle::Custom,0,3}, {"grainStyle",3,Choice,1,0,3},
    {"density",7,Double,.18,-.6,.9}, {"saturation",8,Double,.95,0,2},
    {"toe",9,Double,.16,0,1}, {"contrast",10,Double,1.08,.5,2},
    {"printColor",11,Double,.43,0,1}, {"blackPoint",12,Double,.45,0,1},
    {"halation",13,Double,0,0,2}, {"halationRadius",14,Double,1,0,2},
    {"aura",15,Double,0,0,1}, {"grain",16,Double,.16,0,2},
    {"grainSize",17,Double,.45,0,1}, {"grainRoughness",18,Double,.32,0,1},
    {"grainSoftness",20,Double,.25,0,1}, {"grainColor",21,Double,.25,0,1},
    {"grainShadows",22,Double,1.15,0,2}, {"grainMidtones",23,Double,.8,0,2},
    {"grainHighlights",24,Double,.42,0,2},
    {"negativeShoulder",28,Double,.5,0,1}, {"negativeCrosstalk",29,Double,.35,0,1},
    {"gamutCompression",30,Double,.5,0,1}, {"skinHue",31,Double,0,-1,1},
    {"printTone",32,Double,0,-1,1}, {"printContrast",33,Double,1,.5,2},
    {"printRolloff",34,Double,.55,0,1}, {"printNeutralize",35,Double,0,0,1},
    {"printSaturation",36,Double,1,0,2}, {"printExposure",37,Double,0,-2,2},
    {"printRed",38,Double,0,-2,2}, {"printGreen",39,Double,0,-2,2},
    {"printBlue",40,Double,0,-2,2},
    {"negativeColorStrength",film::NegativeColorStrength,Double,1,0,1},
    {"negativeToneStrength",film::NegativeToneStrength,Double,1,0,1},
    {"printColorStrength",film::PrintColorStrength,Double,1,0,1},
    {"printToneStrength",film::PrintToneStrength,Double,1,0,1},
    {"filmGauge",film::FilmGauge,Choice,gauge::Standard35,0,gauge::Count-1},
    {"halationThreshold",film::HalationThreshold,Double,.48,0,2},
    {"halationSoftness",film::HalationSoftness,Double,.52,.01,2},
    {"halationColor",film::HalationColor,Double,.5,0,1},
    {"auraRadius",film::AuraRadius,Double,1,0,2},
    {"pushPull",film::PushPull,Double,0,-3,3},
    {"colorRichness",film::ColorRichness,Double,0,-1,1},
    {"splitTone",film::SplitTone,Double,0,0,1},
    {"splitHue",film::SplitHue,Double,220,0,360},
    {"splitPivot",film::SplitPivot,Double,.46135613,.2,.8},
    {"splitWidth",film::SplitWidth,Double,.1,0,.3},
    {"splitShadows",film::SplitShadows,Double,1,0,2},
    {"splitHighlights",film::SplitHighlights,Double,1,0,2},
    {"grainStretch",film::GrainStretch,Double,1,.5,2},
    {"grainRed",film::GrainRed,Double,1,0,2},
    {"grainGreen",film::GrainGreen,Double,1,0,2},
    {"grainBlue",film::GrainBlue,Double,1,0,2},
    {"bloom",film::BloomAmount,Double,0,0,2},
    {"bloomRadius",film::BloomRadius,Double,1,0,2},
    {"bloomThreshold",film::BloomThreshold,Double,.65,0,2},
    {"bloomSoftness",film::BloomSoftness,Double,.35,.01,2},
    {"bloomColor",film::BloomColor,Double,1,0,1},
    {"bloomProtection",film::BloomProtection,Double,.8,0,1}
};

using Recipe = std::array<double, film::SettingsCount>;

inline Recipe recipe(int preset)
{
    Recipe s {};
    for (const auto& control : Controls) s[control.setting] = control.initial;
    s[film::ModuleIndex] = film::All;
    // Original artistic recipes, not digitized stock measurements or movie grades.
    switch (preset) {
    case Daylight50:
        s[3]=0; s[7]=.10; s[8]=1.02; s[9]=.10; s[10]=1.04;
        s[16]=.08; s[17]=.28; s[18]=.18; s[28]=.62; s[29]=.22;
        s[32]=.35; s[35]=.6; s[13]=.08;
        break;
    case Daylight250:
        s[7]=.14; s[8]=1; s[10]=1.06; s[16]=.13; s[17]=.38;
        s[28]=.58; s[32]=.2; s[13]=.13;
        break;
    case Tungsten200:
        s[7]=.16; s[8]=.97; s[16]=.12; s[17]=.36; s[28]=.6;
        s[13]=.16; s[15]=.015; s[34]=.65;
        break;
    case Tungsten500:
        s[7]=.20; s[9]=.20; s[16]=.22; s[17]=.52; s[18]=.4;
        s[22]=1.35; s[28]=.66; s[13]=.24; s[15]=.025; s[34]=.68;
        break;
    case ReversalMono:
        s[1]=4; s[8]=1; s[9]=.26; s[10]=1.18; s[16]=.24; s[17]=.5;
        s[21]=0; s[32]=-.5; s[11]=1; s[12]=.14;
        s[film::ModuleIndex] = film::Negative | film::Print | film::Grain;
        break;
    case ClassicCinema:
        s[7]=.22; s[8]=1.04; s[32]=-.65; s[11]=.3; s[12]=.25;
        s[13]=.18; s[15]=.02; s[16]=.14; s[17]=.4;
        s[film::ColorRichness]=.12;
        break;
    case SoftPortrait:
        s[7]=.10; s[8]=.9; s[9]=.08; s[10]=.95; s[28]=.72;
        s[31]=.08; s[32]=.65; s[33]=.94; s[34]=.75; s[35]=.65;
        s[16]=.07; s[17]=.3; s[20]=.48;
        s[film::BloomAmount]=.12; s[film::BloomRadius]=.65;
        break;
    case NeonNights:
        s[7]=.22; s[8]=1.05; s[28]=.72; s[30]=.7; s[32]=-.3; s[12]=.18;
        s[13]=.36; s[14]=.85; s[15]=.04; s[16]=.2; s[17]=.5;
        s[film::SplitTone]=.18; s[film::SplitHue]=205;
        s[film::SplitHighlights]=.55; s[film::ColorRichness]=.15;
        break;
    case SeventiesPrint:
        s[1]=1; s[7]=.25; s[8]=.84; s[9]=.24; s[29]=.7;
        s[32]=-.55; s[11]=.18; s[12]=.65; s[38]=.06; s[40]=-.08;
        s[16]=.25; s[17]=.58; s[18]=.5; s[13]=.22; s[15]=.03;
        break;
    case BleachBypass:
        s[1]=2; s[7]=.23; s[8]=.75; s[9]=.25; s[10]=1.16;
        s[32]=-.5; s[33]=1.05; s[11]=.8; s[12]=.08; s[35]=1;
        s[16]=.25; s[17]=.5; s[21]=.08;
        s[film::ModuleIndex] = film::Negative | film::Print | film::Grain;
        break;
    case Super8HomeMovie:
        s[1]=1; s[7]=.18; s[8]=.88; s[9]=.24; s[10]=.98; s[29]=.6;
        s[32]=.45; s[11]=.25; s[12]=.68; s[38]=.06; s[40]=-.06;
        s[16]=.2; s[17]=.48; s[18]=.55; s[20]=.4;
        s[13]=.16; s[15]=.025; s[film::FilmGauge]=gauge::Super8;
        s[film::BloomAmount]=.08;
        break;
    case SilverNoir:
        s[1]=4; s[9]=.3; s[10]=1.25; s[32]=-.8; s[33]=1.05;
        s[11]=1; s[12]=.05; s[16]=.3; s[17]=.55; s[21]=0;
        s[film::PushPull]=.5;
        s[film::ModuleIndex] = film::Negative | film::Development | film::Print | film::Grain;
        break;
    default: break;
    }
    return s;
}

template<class Writer, class ToggleWriter>
inline void applyPreset(int preset, Writer write, ToggleWriter toggle)
{
    if (preset <= Custom || preset >= Count) return;
    const auto s = recipe(preset);
    for (size_t i = 0; i < std::size(Controls); ++i) write(i, s[Controls[i].setting]);
    for (size_t i = 0; i < moduleui::Toggles.size(); ++i)
        toggle(i, (static_cast<int>(s[film::ModuleIndex]) & moduleui::Toggles[i].module) != 0);
}

inline bool ownsControl(std::string_view name)
{
    for (const auto& control : Controls) if (name == control.name) return true;
    for (const auto& toggle : moduleui::Toggles) if (name == toggle.name) return true;
    return false;
}

enum EditAction { None, Apply, MarkCustom };
inline EditAction editAction(std::string_view name, bool userEdit, bool applying, int selected)
{
    if (!userEdit || applying) return None;
    if (name == "lookPreset") return selected > Custom && selected < Count ? Apply : None;
    return selected != Custom && ownsControl(name) ? MarkCustom : None;
}

} // namespace look
