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
              SeventiesPrint, BleachBypass, Super8HomeMovie, SilverNoir,
              Neutral, Verita200, Portra160, Portra400, Portra800, Portra800Push1,
              Portra800Push2, Ektar100, Gold200, Ultramax400, Pro400H, Xtra400,
              C200, Kodachrome64, Ektachrome100, Velvia100, Provia100,
              Print2383, Print2393, TriX400, Hp5Plus400,
              DesertChrome, ArcticDusk, GoldenHour, FadedInstant,
              ArchiveThriller, SilverThriller, SodiumNoir, FolkDread, DaylightDread,
              GialloCrimson, CrimsonDream, SimulationGreen, AmberWasteland,
              WinterCrime, SteelBlue, Nostromo, ComicRed, ComicBlue, ComicYellow, Count };

inline constexpr std::array<const char*, Count> Labels {{
    "Custom", "50D Daylight", "250D Daylight",
    "200T Tungsten", "500T Tungsten",
    "B&W Reversal", "Classic Cinema", "Soft Portrait", "Neon Nights",
    "Seventies Print", "Bleach Bypass", "Super 8 Home Movie", "Silver Noir",
    "Neutral / Clean Slate", "VERITA 200D",
    "Portra 160", "Portra 400", "Portra 800",
    "Portra 800 Push +1", "Portra 800 Push +2",
    "Ektar 100", "Gold 200", "Ultramax 400",
    "PRO 400H", "Superia X-TRA 400", "C200",
    "Kodachrome 64", "Ektachrome 100",
    "Velvia 100", "Provia 100F",
    "2383 Print", "2393 Print",
    "Tri-X 400", "HP5 Plus 400",
    "Desert Chrome", "Arctic Dusk", "Golden Hour", "Faded Instant",
    "Archive Thriller (Zodiac)", "Silver Thriller (Se7en)", "Sodium Noir (Nightcrawler)",
    "Folk Dread (The Witch)", "Daylight Dread (Midsommar)",
    "Giallo Crimson (Suspiria 1977)", "Crimson Dream (Mandy)",
    "Simulation Green (The Matrix)", "Amber Wasteland (2049 Vegas)",
    "Winter Crime (Fargo 1996)", "Steel Blue (Terminator 2)", "Nostromo (Alien 1979)",
    "Graphic Noir / Red (Sin City)", "Graphic Noir / Blue (Sin City)", "Graphic Noir / Yellow (Sin City)"
}};

enum Category { AllLooks, StartingPoints, CinemaNegative, StillNegative,
                ReversalFilm, Monochrome, PrintLooks, Creative,
                Thriller, Horror, ScienceFiction, GraphicNoir, CategoryCount };
inline constexpr std::array<const char*, CategoryCount> CategoryLabels {{
    "All Presets", "Starting Points", "Cinema Negative", "Still Negative",
    "Reversal Film", "Monochrome", "Print Looks", "Creative Looks",
    "Thriller", "Horror", "Sci-Fi", "Graphic Noir"
}};

inline constexpr std::array<int,Count> MenuOrder {{
    Custom, Neutral, Daylight50, Daylight250, Tungsten200, Tungsten500, Verita200,
    Portra160, Portra400, Portra800, Portra800Push1, Portra800Push2, Ektar100,
    Gold200, Ultramax400, Pro400H, Xtra400, C200,
    Kodachrome64, Ektachrome100, Velvia100, Provia100,
    ReversalMono, TriX400, Hp5Plus400, SilverNoir, Print2383, Print2393,
    ClassicCinema, SoftPortrait, NeonNights, SeventiesPrint, BleachBypass,
    Super8HomeMovie, DesertChrome, ArcticDusk, GoldenHour, FadedInstant,
    ArchiveThriller, SilverThriller, SodiumNoir, FolkDread, DaylightDread,
    GialloCrimson, CrimsonDream, SimulationGreen, AmberWasteland,
    WinterCrime, SteelBlue, Nostromo, ComicRed, ComicBlue, ComicYellow
}};

inline constexpr int categoryFor(int preset)
{
    switch (preset) {
    case Neutral: return StartingPoints;
    case Daylight50: case Daylight250: case Tungsten200: case Tungsten500:
    case Verita200: return CinemaNegative;
    case Portra160: case Portra400: case Portra800: case Portra800Push1: case Portra800Push2:
    case Ektar100: case Gold200: case Ultramax400: case Pro400H: case Xtra400: case C200:
        return StillNegative;
    case Kodachrome64: case Ektachrome100: case Velvia100: case Provia100: return ReversalFilm;
    case ReversalMono: case TriX400: case Hp5Plus400: case SilverNoir: return Monochrome;
    case Print2383: case Print2393: return PrintLooks;
    case ArchiveThriller: case SilverThriller: case SodiumNoir: case WinterCrime: return Thriller;
    case FolkDread: case DaylightDread: case GialloCrimson: case CrimsonDream: return Horror;
    case SimulationGreen: case AmberWasteland: case SteelBlue: case Nostromo: return ScienceFiction;
    case ComicRed: case ComicBlue: case ComicYellow: return GraphicNoir;
    default: return Creative;
    }
}

inline constexpr bool contains(int category, int preset)
{
    return category >= 0 && category < CategoryCount && preset >= Custom && preset < Count &&
        (preset == Custom || category == AllLooks || categoryFor(preset) == category);
}

inline constexpr int optionCount(int category)
{
    int count = 0;
    for (int preset : MenuOrder) if (contains(category,preset)) ++count;
    return count;
}

// Filtered menu positions are presentation only; stored IDs never change with category order.
inline constexpr int presetAt(int category, int option)
{
    if (option < 0) return Custom;
    for (int preset : MenuOrder)
        if (contains(category,preset) && option-- == 0) return preset;
    return Custom;
}

inline constexpr int optionFor(int category, int selected)
{
    int option = 0;
    for (int preset : MenuOrder) if (contains(category,preset)) {
        if (preset == selected) return option;
        ++option;
    }
    return 0;
}

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
    {"grainResponse",film::GrainResponse,Choice,0,0,1},
    {"density",7,Double,.18,-1.2,1.5}, {"saturation",8,Double,.95,0,2},
    {"toe",9,Double,.16,0,1}, {"contrast",10,Double,1.08,.5,2},
    {"printColor",11,Double,.43,0,1}, {"blackPoint",12,Double,.45,0,1},
    {"halation",13,Double,0,0,2}, {"halationRadius",14,Double,1,0,2},
    {"aura",15,Double,0,0,1}, {"grain",16,Double,.16,0,2},
    {"grainSize",17,Double,.45,0,1}, {"grainRoughness",18,Double,.32,0,1},
    {"grainSoftness",20,Double,.25,0,2}, {"grainColor",21,Double,.25,0,1},
    {"grainShadows",22,Double,1.15,0,2}, {"grainMidtones",23,Double,.8,0,2},
    {"grainHighlights",24,Double,.42,0,2},
    {"negativeShoulder",28,Double,.5,0,1}, {"negativeCrosstalk",29,Double,.35,0,3},
    {"gamutCompression",30,Double,.5,0,1}, {"skinHue",31,Double,0,-3,3},
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
    {"splitTone",film::SplitTone,Double,0,0,3},
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
    {"bloomProtection",film::BloomProtection,Double,.8,0,1},
    {"selectiveAmount",film::SelectiveAmount,Double,0,0,1},
    {"selectiveHue",film::SelectiveHue,Double,0,0,360},
    {"selectiveRange",film::SelectiveRange,Double,15,0,180},
    {"selectiveSoftness",film::SelectiveSoftness,Double,15,0,90},
    {"selectiveSaturation",film::SelectiveSaturation,Double,.25,0,1},
    {"selectiveView",film::SelectiveView,Choice,0,0,1}
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
        s[7]=.16; s[8]=1; s[9]=.14; s[10]=1.06; s[16]=.13; s[17]=.38;
        s[28]=.65; s[32]=.25; s[35]=.5; s[13]=.13;
        break;
    case Tungsten200:
        s[7]=.16; s[8]=.97; s[9]=.12; s[10]=1.04; s[16]=.11; s[17]=.34; s[28]=.7;
        s[13]=.16; s[15]=.015; s[32]=.35; s[34]=.65; s[35]=.55;
        break;
    case Tungsten500:
        s[7]=.20; s[9]=.18; s[16]=.20; s[17]=.52; s[18]=.4;
        s[22]=1.25; s[28]=.76; s[13]=.24; s[15]=.025; s[32]=.15; s[34]=.72; s[35]=.4;
        break;
    case ReversalMono:
        s[1]=4; s[8]=1; s[9]=.26; s[10]=1.18; s[16]=.24; s[17]=.5;
        s[21]=0; s[32]=-.5; s[11]=1; s[12]=.14;
        s[film::ModuleIndex] = film::Negative | film::Print | film::Grain;
        break;
    case ClassicCinema:
        s[7]=.28; s[8]=1.07; s[9]=.19; s[10]=1.10; s[29]=.55;
        s[32]=-.8; s[33]=1.04; s[11]=.2; s[12]=.16; s[35]=.3; s[36]=1.04; s[34]=.6;
        s[13]=.18; s[15]=.02; s[16]=.16; s[17]=.4;
        s[film::ColorRichness]=.25;
        break;
    case SoftPortrait:
        s[7]=.10; s[8]=.9; s[9]=.08; s[10]=.95; s[28]=.72;
        s[31]=.08; s[32]=.65; s[33]=.94; s[34]=.75; s[35]=.65;
        s[16]=.07; s[17]=.3; s[20]=.48;
        s[film::BloomAmount]=.12; s[film::BloomRadius]=.65;
        break;
    case NeonNights:
        s[7]=.20; s[8]=1.06; s[9]=.12; s[10]=1.03; s[28]=.82; s[30]=.6;
        s[32]=-.2; s[33]=1.05; s[11]=.65; s[12]=.18; s[35]=.65;
        s[13]=.36; s[14]=.85; s[15]=.04; s[16]=.2; s[17]=.5;
        s[film::SplitTone]=.9; s[film::SplitHue]=205; s[film::SplitShadows]=1.4;
        s[film::SplitHighlights]=.55; s[film::ColorRichness]=.18;
        break;
    case SeventiesPrint:
        s[1]=1; s[7]=.22; s[8]=.78; s[9]=.14; s[10]=1.02; s[28]=.58; s[29]=.95;
        s[32]=-.25; s[33]=.98; s[11]=.1; s[12]=.95; s[35]=.55; s[36]=.92;
        s[38]=.14; s[39]=.01; s[40]=-.18; s[film::ColorRichness]=-.15;
        s[16]=.25; s[17]=.58; s[18]=.5; s[13]=.22; s[15]=.03;
        break;
    case BleachBypass:
        s[1]=2; s[7]=.23; s[8]=.75; s[9]=.25; s[10]=1.16;
        s[32]=-.5; s[33]=1.05; s[11]=.8; s[12]=.08; s[35]=1;
        s[16]=.25; s[17]=.5; s[21]=.08;
        s[film::ModuleIndex] = film::Negative | film::Print | film::Grain;
        break;
    case Super8HomeMovie:
        s[1]=5; s[7]=.18; s[8]=1; s[9]=.18; s[10]=.98; s[28]=.65; s[29]=.5;
        s[32]=.6; s[33]=.92; s[11]=.42; s[12]=.7; s[35]=.65; s[36]=1.02;
        s[38]=.08; s[39]=.02; s[40]=-.06;
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
    case Neutral:
        s[7]=0; s[8]=1; s[9]=0; s[10]=1; s[11]=1; s[12]=0;
        s[16]=0; s[28]=0; s[29]=0; s[30]=0;
        s[32]=1; s[34]=0; s[35]=1;
        s[film::NegativeColorStrength]=s[film::NegativeToneStrength]=0;
        s[film::PrintColorStrength]=s[film::PrintToneStrength]=0;
        s[film::FilmGauge]=gauge::Custom;
        break;
    case Verita200:
        s=recipe(Daylight250); s[7]=.13; s[8]=.97; s[9]=.1; s[10]=1.03;
        s[28]=.72; s[29]=.28; s[16]=.11; s[17]=.35; s[32]=.4; s[35]=.65;
        break;
    case Portra160: case Portra400: case Portra800: case Portra800Push1: case Portra800Push2:
        s[1]=3; s[7]=.1; s[8]=.91; s[9]=.08; s[10]=.95; s[28]=.78; s[29]=.28;
        s[32]=.75; s[33]=.95; s[34]=.78; s[35]=.85; s[11]=.7; s[12]=.38;
        s[16]=.08; s[17]=.29; s[18]=.2; s[20]=.4; s[13]=.06;
        if (preset != Portra160) {
            s[7]=.14; s[10]=.99; s[16]=.13; s[17]=.39; s[18]=.3; s[13]=.1;
        }
        if (preset >= Portra800) {
            s[7]=.18; s[8]=.95; s[10]=1.02; s[16]=.20; s[17]=.49; s[18]=.4;
            s[28]=.72; s[32]=.5; s[13]=.14;
        }
        if (preset == Portra800Push1 || preset == Portra800Push2) {
            s[film::PushPull]=preset == Portra800Push1 ? 1 : 2;
            s[8]=preset == Portra800Push1 ? .92 : .88;
            s[18]=preset == Portra800Push1 ? .46 : .52;
        }
        break;
    case Ektar100:
        s[1]=3; s[3]=0; s[7]=.2; s[8]=1.18; s[9]=.12; s[10]=1.08; s[28]=.62; s[29]=.15;
        s[32]=.15; s[35]=.9; s[11]=.85; s[12]=.18;
        s[16]=.055; s[17]=.23; s[18]=.15; s[13]=.05;
        break;
    case Gold200: case Ultramax400:
        s[1]=3; s[7]=.18; s[8]=1.03; s[9]=.16; s[10]=1.05; s[29]=.55;
        s[32]=.25; s[35]=.6; s[12]=.5; s[38]=.07; s[40]=-.07;
        s[16]=.17; s[17]=.45; s[18]=.4; s[13]=.12;
        if (preset == Ultramax400) {
            s[8]=1.10; s[10]=1.1; s[38]=.035; s[40]=-.035;
            s[16]=.23; s[17]=.53; s[18]=.48; s[32]=.05; s[13]=.17;
        }
        break;
    case Pro400H:
        s[1]=3; s[7]=.1; s[8]=.87; s[9]=.08; s[10]=.96; s[28]=.8; s[29]=.18;
        s[32]=.8; s[33]=.96; s[35]=1; s[11]=.9; s[12]=.4;
        s[38]=-.025; s[39]=.025; s[40]=.035;
        s[16]=.12; s[17]=.37; s[20]=.45; s[13]=.07;
        break;
    case Xtra400: case C200:
        s[1]=3; s[7]=.19; s[8]=1.08; s[9]=.14; s[10]=1.08; s[29]=.18;
        s[32]=.2; s[35]=1; s[11]=.85; s[12]=.3;
        s[38]=-.025; s[39]=.025; s[40]=.04;
        s[16]=.21; s[17]=.5; s[18]=.42; s[13]=.12;
        if (preset == C200) {
            s[7]=.14; s[8]=1; s[10]=1.02; s[32]=.45;
            s[16]=.15; s[17]=.41; s[18]=.32; s[13]=.08;
        }
        break;
    case Kodachrome64: case Ektachrome100: case Velvia100: case Provia100:
        s[1]=5; s[3]=0; s[7]=.2; s[8]=1.02; s[9]=.18; s[10]=1.06;
        s[28]=.55; s[29]=.2; s[32]=.55; s[33]=.96; s[35]=1; s[11]=.9; s[12]=.08;
        s[16]=.07; s[17]=.25; s[18]=.18; s[13]=.07;
        // Reversal looks do not include a second creative print curve or palette.
        s[film::PrintColorStrength]=s[film::PrintToneStrength]=0;
        if (preset == Kodachrome64) {
            s[7]=.28; s[8]=1.05; s[9]=.24; s[10]=1.12; s[29]=.4;
            s[16]=.1; s[17]=.3; s[38]=.04; s[40]=-.055;
        } else if (preset == Velvia100) {
            s[7]=.28; s[8]=1.25; s[9]=.22; s[10]=1.18; s[28]=.45;
            s[film::ColorRichness]=.25; s[16]=.06; s[17]=.24;
        } else if (preset == Provia100) {
            s[7]=.15; s[8]=.94; s[9]=.12; s[10]=.97; s[28]=.65;
            s[16]=.06; s[17]=.24;
        } else {
            s[38]=-.015; s[40]=.025;
        }
        break;
    case Print2383: case Print2393:
        s[film::NegativeColorStrength]=s[film::NegativeToneStrength]=0;
        s[16]=0; s[32]=-.75; s[33]=1.02; s[11]=.3; s[12]=.12; s[35]=.7;
        if (preset == Print2393) {
            s[32]=-1; s[33]=1.15; s[11]=.2; s[12]=.04; s[36]=1.08; s[34]=.65;
        }
        break;
    case TriX400: case Hp5Plus400:
        s[1]=4; s[8]=1; s[9]=.19; s[10]=1.1; s[28]=.64;
        s[32]=.15; s[33]=1; s[35]=1; s[11]=1; s[12]=.14;
        s[16]=.24; s[17]=.51; s[18]=.44; s[21]=0;
        s[film::ModuleIndex]=film::Negative | film::Development | film::Print | film::Grain;
        if (preset == Hp5Plus400) {
            s[9]=.1; s[10]=.98; s[28]=.76; s[32]=.55; s[12]=.32;
            s[16]=.20; s[17]=.47; s[18]=.36;
        }
        break;
    case DesertChrome:
        s[1]=2; s[7]=.3; s[8]=.85; s[9]=.2; s[10]=1.12;
        s[32]=-.6; s[33]=1.07; s[11]=.75; s[12]=.12; s[35]=.8;
        s[38]=.18; s[40]=-.16; s[film::SplitTone]=.65; s[film::SplitHue]=200;
        s[film::SplitShadows]=1.3; s[film::SplitHighlights]=.25;
        s[16]=.2; s[17]=.46; s[13]=.15;
        break;
    case ArcticDusk:
        s[7]=.2; s[8]=.7; s[9]=.13; s[10]=1.07; s[28]=.8;
        s[32]=.35; s[35]=1; s[11]=.85; s[12]=.26; s[38]=-.08; s[40]=.08;
        s[film::SplitTone]=1.1; s[film::SplitHue]=215; s[film::SplitHighlights]=.15;
        s[16]=.1; s[17]=.34; s[13]=.08;
        break;
    case GoldenHour:
        s[1]=3; s[7]=.12; s[8]=1.06; s[9]=.08; s[10]=.98; s[28]=.8;
        s[32]=.7; s[35]=.8; s[12]=.45; s[38]=.12; s[40]=-.14;
        s[film::SplitTone]=.75; s[film::SplitHue]=205; s[film::SplitShadows]=.3;
        s[film::SplitHighlights]=1.4; s[film::BloomAmount]=.16; s[film::BloomRadius]=.7;
        s[16]=.09; s[17]=.32; s[13]=.12; s[15]=.025;
        break;
    case FadedInstant:
        s[1]=1; s[7]=.06; s[8]=.68; s[9]=.05; s[10]=.88; s[29]=.8;
        s[32]=1; s[33]=.85; s[11]=.25; s[12]=1; s[35]=.55;
        s[38]=.1; s[39]=.045; s[40]=-.12;
        s[film::ColorRichness]=-.3; s[film::BloomAmount]=.09;
        s[16]=.19; s[17]=.54; s[20]=.55; s[13]=.08;
        break;
    case ArchiveThriller:
        s[1]=1; s[7]=.12; s[8]=.72; s[9]=.07; s[10]=1.01; s[28]=.72; s[29]=.55;
        s[32]=.55; s[33]=.98; s[11]=.8; s[12]=.22; s[34]=.65; s[35]=1;
        s[38]=.06; s[39]=.035; s[40]=-.14;
        s[film::SplitTone]=.55; s[film::SplitHue]=85; s[film::SplitShadows]=.75;
        s[film::SplitHighlights]=0; s[film::ColorRichness]=-.15;
        s[16]=.045; s[17]=.28; s[18]=.18; s[20]=.4; s[21]=.08;
        s[film::ModuleIndex]=film::Negative | film::Development | film::Print | film::Grain;
        break;
    case SilverThriller:
        // Print-led silver-retention interpretation, not another negative bleach recipe.
        s[7]=.26; s[8]=.64; s[9]=.18; s[10]=1.02; s[28]=.68; s[29]=.4;
        s[32]=-1; s[33]=1.28; s[11]=.45; s[12]=.025; s[34]=.68; s[35]=1; s[36]=.9;
        s[38]=.12; s[39]=.10; s[40]=-.15;
        s[film::SplitTone]=.65; s[film::SplitHue]=185; s[film::SplitShadows]=1;
        s[film::SplitHighlights]=.15;
        s[16]=.22; s[17]=.48; s[18]=.46; s[20]=.25; s[21]=.04;
        s[film::ModuleIndex]=film::Negative | film::Development | film::Print | film::Grain;
        break;
    case SodiumNoir:
        s[7]=.24; s[8]=.86; s[9]=.1; s[10]=1.04; s[28]=.85; s[29]=.2;
        s[32]=-.35; s[33]=1.08; s[11]=.9; s[12]=.14; s[34]=.72; s[35]=1;
        s[38]=.20; s[39]=.11; s[40]=-.26;
        s[film::SplitTone]=1.3; s[film::SplitHue]=220; s[film::SplitShadows]=1.4;
        s[film::SplitHighlights]=.8; s[film::ColorRichness]=.12;
        s[13]=.14; s[14]=.65; s[16]=.035; s[17]=.27; s[18]=.2;
        s[21]=.08;
        break;
    case FolkDread:
        s[7]=.08; s[8]=.38; s[9]=.06; s[10]=1.01; s[28]=.82; s[29]=.15;
        s[32]=.7; s[33]=.98; s[11]=1; s[12]=.18; s[34]=.8; s[35]=1;
        s[38]=-.04; s[39]=.015; s[40]=.045;
        s[film::ColorRichness]=-.25;
        s[16]=.025; s[17]=.28; s[20]=.5; s[21]=0;
        s[film::ModuleIndex]=film::Negative | film::Development | film::Print | film::Grain;
        break;
    case DaylightDread:
        s[1]=3; s[7]=.04; s[8]=.84; s[9]=.025; s[10]=.9; s[28]=.9; s[29]=.15;
        s[32]=.95; s[33]=.9; s[11]=1; s[12]=.20; s[34]=.78; s[35]=1; s[37]=.18;
        s[38]=.025; s[39]=.035; s[40]=0;
        s[film::ColorRichness]=-.25;
        s[film::BloomAmount]=.08; s[film::BloomRadius]=.6; s[film::BloomProtection]=.9;
        s[16]=.02; s[17]=.25; s[20]=.5; s[21]=.1;
        break;
    case GialloCrimson:
        s[1]=5; s[7]=.30; s[8]=1.3; s[9]=.16; s[10]=1.12; s[28]=.62; s[29]=.7; s[30]=.4;
        s[32]=-.75; s[33]=1.07; s[11]=.9; s[12]=.07; s[34]=.68; s[35]=1; s[36]=1.1;
        s[38]=.22; s[39]=-.08; s[40]=.08;
        s[film::SplitTone]=1.1; s[film::SplitHue]=240; s[film::SplitShadows]=1.2;
        s[film::SplitHighlights]=.15; s[film::ColorRichness]=.35;
        s[13]=.12; s[14]=.65; s[film::BloomAmount]=.06; s[film::BloomRadius]=.5;
        s[16]=.11; s[17]=.35; s[18]=.28; s[20]=.3;
        break;
    case CrimsonDream:
        s[7]=.32; s[8]=1.04; s[9]=.13; s[10]=1.03; s[28]=.9; s[29]=.35;
        s[32]=-.1; s[33]=1.03; s[11]=1; s[12]=.10; s[34]=.86; s[35]=1;
        s[38]=.46; s[39]=-.22; s[40]=.02;
        s[film::SplitTone]=2; s[film::SplitHue]=255; s[film::SplitShadows]=1.5;
        s[film::SplitHighlights]=.10; s[film::ColorRichness]=.25;
        s[13]=.38; s[14]=.85; s[15]=.025;
        s[film::BloomAmount]=.28; s[film::BloomRadius]=1.05; s[film::BloomProtection]=.85;
        s[16]=.16; s[17]=.45; s[18]=.45; s[20]=.5; s[21]=.12;
        break;
    case SimulationGreen:
        s[7]=.24; s[8]=.76; s[9]=.16; s[10]=1.07; s[28]=.68; s[29]=.15;
        s[32]=-.5; s[33]=1.08; s[11]=1; s[12]=.09; s[34]=.65; s[35]=1;
        s[38]=-.06; s[39]=.10; s[40]=-.10;
        s[film::SplitTone]=1.65; s[film::SplitHue]=115; s[film::SplitShadows]=1.3;
        s[film::SplitHighlights]=0; s[film::SplitPivot]=.58;
        s[16]=.12; s[17]=.39; s[18]=.3; s[21]=.12;
        s[film::FilmGauge]=gauge::Super35;
        s[film::ModuleIndex]=film::Negative | film::Development | film::Print | film::Grain;
        break;
    case AmberWasteland:
        s[1]=1; s[7]=.20; s[8]=.65; s[9]=.07; s[10]=.98; s[28]=.92; s[29]=.4;
        s[32]=.55; s[33]=.95; s[11]=1; s[12]=.16; s[34]=.9; s[35]=1;
        s[38]=.60; s[39]=.12; s[40]=-.80;
        s[film::SplitTone]=.8; s[film::SplitHue]=32; s[film::SplitShadows]=.85;
        s[film::SplitHighlights]=0; s[film::ColorRichness]=-.2;
        s[film::BloomAmount]=.18; s[film::BloomRadius]=1.1; s[film::BloomProtection]=.9;
        s[16]=.015; s[17]=.25; s[20]=.55; s[21]=.05;
        break;
    case WinterCrime:
        s[7]=.09; s[8]=.82; s[9]=.06; s[10]=1.02; s[28]=.8; s[29]=.2;
        s[32]=.6; s[33]=1.01; s[11]=1; s[12]=.14; s[34]=.78; s[35]=1;
        s[38]=-.015; s[39]=0; s[40]=.035;
        s[16]=.085; s[17]=.34; s[18]=.22; s[20]=.35; s[21]=.12;
        s[film::ModuleIndex]=film::Negative | film::Print | film::Grain;
        break;
    case SteelBlue:
        s[7]=.25; s[8]=1.02; s[9]=.15; s[10]=1.12; s[28]=.78; s[29]=.2;
        s[32]=-.65; s[33]=1.12; s[11]=.9; s[12]=.09; s[34]=.72; s[35]=1; s[36]=1.05;
        s[film::SplitTone]=1.85; s[film::SplitHue]=215; s[film::SplitShadows]=1.5;
        s[film::SplitHighlights]=1.25; s[film::ColorRichness]=.16;
        s[13]=.16; s[14]=.6; s[16]=.13; s[17]=.39; s[18]=.3;
        s[film::FilmGauge]=gauge::Super35;
        break;
    case Nostromo:
        s[1]=1; s[7]=.22; s[8]=.68; s[9]=.17; s[10]=1.09; s[28]=.78; s[29]=.65;
        s[32]=-.4; s[33]=1.09; s[11]=.85; s[12]=.12; s[34]=.74; s[35]=1;
        s[38]=-.04; s[39]=.035; s[40]=.025;
        s[film::SplitTone]=.85; s[film::SplitHue]=175; s[film::SplitShadows]=1.2;
        s[film::SplitHighlights]=.45;
        s[13]=.09; s[14]=.6; s[16]=.15; s[17]=.41; s[18]=.34; s[21]=.1;
        break;
    case ComicRed: case ComicBlue: case ComicYellow:
        // Keep a color negative; Mono Negative would remove the accent before selection.
        s[7]=.12; s[8]=1.05; s[9]=.22; s[10]=1.10; s[28]=.55; s[29]=.15;
        s[32]=-1; s[33]=1.5; s[11]=1; s[12]=0; s[34]=.4; s[35]=1; s[36]=1.05;
        s[16]=.03; s[17]=.27; s[18]=.18; s[21]=0;
        s[film::SelectiveAmount]=1;
        s[film::SelectiveHue]=preset == ComicRed ? 0 : preset == ComicBlue ? 240 : 60;
        s[film::SelectiveRange]=12; s[film::SelectiveSoftness]=14; s[film::SelectiveSaturation]=.45;
        s[film::ModuleIndex]=film::Negative | film::Print | film::Grain | film::SelectiveColor;
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
