// SPDX-License-Identifier: MPL-2.0

#include <cmath>
#include <cstdio>
#include <set>
#include <stdexcept>
#include <string>

#include "LookPresetConfig.h"
#include "FilmResponseConfig.h"
#include "ColorSpaceConfig.h"
#include "GrainConfig.h"

static void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

static std::array<float,film::SettingsCount> packedRecipe(int preset)
{
    std::array<float,film::SettingsCount> s {};
    const auto recipe = look::recipe(preset);
    for (const auto& control : look::Controls) s[control.setting] = static_cast<float>(recipe[control.setting]);
    s[film::ModuleIndex] = static_cast<float>(recipe[film::ModuleIndex]);
    s[4] = s[5] = s[6] = 1;
    s[26] = color::LinearRec709; s[27] = 1;
    return s;
}

static ColorRgb renderColorOnly(ColorRgb linear, int preset)
{
    const auto s = packedRecipe(preset);
    const auto cp = color::prepare(s.data());
    const auto rp = response::prepare(s.data());
    const int modules = film::modulesForSettings(s.data());
    auto c = color_to_work(linear,cp);
    if (modules & film::Negative) c = response_negative_stage(c,rp);
    if (modules & film::Development) c = response_development(c,rp);
    if (modules & film::Print) c = response_print(c,rp);
    c=response_finish(c,modules,rp);
    if (modules & film::SelectiveColor) c=response_selective(c,color_to_work(linear,cp),rp);
    return color_from_work(c,cp);
}

static double colorDistance(int a, int b)
{
    const std::array<ColorRgb,9> chips {{{.38f,.20f,.12f},{.16f,.065f,.032f},
        {.25f,.095f,.048f},{.28f,.02f,.008f},{.015f,.18f,.03f},{.008f,.045f,.28f},
        {.27f,.015f,.18f},{.30f,.22f,.012f},{.012f,.21f,.21f}}};
    double total = 0;
    for (float exposure : {.125f,.5f,1.0f,2.0f,8.0f}) for (const auto& chip : chips) {
        const ColorRgb input {chip.r*exposure,chip.g*exposure,chip.b*exposure};
        const auto x = renderColorOnly(input,a), y = renderColorOnly(input,b);
        const std::array<float,3> xx {x.r,x.g,x.b}, yy {y.r,y.g,y.b};
        for (int channel = 0; channel < 3; ++channel) {
            require(std::isfinite(xx[channel]) && std::isfinite(yy[channel]),"Nonfinite preset color sample");
            total += std::abs(std::clamp(xx[channel],0.0f,1.0f)-std::clamp(yy[channel],0.0f,1.0f))*255;
        }
    }
    return total/(chips.size()*5*3);
}

static void testLookIntent()
{
    // Synthetic display-RGB overlap guard, not a perceptual or stock-accuracy metric.
    const std::array<int,5> creative {look::ClassicCinema,look::SoftPortrait,look::NeonNights,
        look::SeventiesPrint,look::Super8HomeMovie};
    for (size_t a = 0; a < creative.size(); ++a) for (size_t b = a+1; b < creative.size(); ++b)
        require(colorDistance(creative[a],creative[b]) > 3,"Creative presets collapse to similar color/tone");
    require(colorDistance(look::SeventiesPrint,look::Super8HomeMovie) > 8,"Faded and reversal home-movie palettes overlap");
    require(colorDistance(look::Daylight50,look::Daylight250) < 4,"Daylight family diverges excessively");
    require(colorDistance(look::Tungsten200,look::Tungsten500) < 4,"Tungsten family diverges excessively");
    float previousAmount = 0, previousSize = 0;
    for (int preset : {look::Daylight50,look::Tungsten200,look::Daylight250,look::Tungsten500}) {
        const auto s = packedRecipe(preset);
        const auto g = grain::prepare(s.data(),1080,0);
        require(g.amount > previousAmount && 1/g.inverseSize > previousSize,"Stock texture hierarchy is inconsistent");
        previousAmount = g.amount; previousSize = 1/g.inverseSize;
        const auto gray = renderColorOnly({.18f,.18f,.18f},preset);
        require(std::max({gray.r,gray.g,gray.b})-std::min({gray.r,gray.g,gray.b}) < .006f,
            "Stock-inspired look introduces a strong middle-gray cast");
        require(std::abs(response_luma(gray)-.498f) < .006f,"Stock-inspired look shifts exposure pivot");
    }
    for (int preset : creative) for (float exposure : {.5f,1.0f,2.0f}) {
        const auto skin = renderColorOnly({.38f*exposure,.20f*exposure,.12f*exposure},preset);
        require(skin.r > skin.g && skin.g > skin.b,"Creative look reverses representative midtone skin hue");
    }
    const auto neonShadow = renderColorOnly({.018f,.018f,.018f},look::NeonNights);
    const auto neonHigh = renderColorOnly({1,1,1},look::NeonNights);
    require(neonShadow.b > neonShadow.r+.005f,"Neon Nights loses its cool shadow identity");
    require(neonHigh.r > neonHigh.b+.005f,"Neon Nights loses its warm highlight identity");
    const auto classicShadow = renderColorOnly({.018f,.018f,.018f},look::ClassicCinema);
    const auto vintageShadow = renderColorOnly({.018f,.018f,.018f},look::SeventiesPrint);
    require(response_luma(vintageShadow) > response_luma(classicShadow)+.015f,"Vintage shadows are not softer than cinema print");
    const auto vintageGray = renderColorOnly({.18f,.18f,.18f},look::SeventiesPrint);
    require(vintageGray.r > vintageGray.b+.02f,"Seventies Print loses its warm print balance");
    require(look::recipe(look::Super8HomeMovie)[1] == 5,"Home-movie look no longer uses the reversal palette");
    std::printf("Synthetic color separation (mean 8-bit-equivalent RGB codes, no grain/glow): Classic/Neon %.3f; Seventies/Super8 %.3f; 200T/500T %.3f.\n",
        colorDistance(look::ClassicCinema,look::NeonNights),colorDistance(look::SeventiesPrint,look::Super8HomeMovie),
        colorDistance(look::Tungsten200,look::Tungsten500));
}

static void testCategoriesAndNeutral()
{
    require(look::Count == 53,"Expected 52 named recipes plus Custom");
    require(look::Neutral == 13 && look::FadedInstant == 37 && look::AmberWasteland == 46 &&
        look::Creative == 7 && look::ScienceFiction == 10,
        "Existing preset/category IDs changed");
    require(look::optionCount(look::Thriller) == 5 && look::optionCount(look::Horror) == 5 &&
        look::optionCount(look::ScienceFiction) == 5 && look::optionCount(look::GraphicNoir) == 4,"Genre categories omit recipes");
    const std::set<int> order(look::MenuOrder.begin(),look::MenuOrder.end());
    require(order.size() == look::Count && *order.begin() == 0 && *order.rbegin() == look::Count-1,
        "Menu order duplicates or omits a stable preset ID");
    require(look::presetAt(look::AllLooks,1) == look::Neutral,"Neutral is not first in All Presets");
    for (int category = 0; category < look::CategoryCount; ++category) {
        require(look::optionCount(category) > 1,"Empty preset category");
        require(look::presetAt(category,0) == look::Custom,"Category has no non-destructive Custom choice");
        for (int option = 0; option < look::optionCount(category); ++option) {
            const int preset = look::presetAt(category,option);
            require(look::contains(category,preset),"Filtered menu leaks another category");
            require(look::optionFor(category,preset) == option,"Filtered option/stable-ID mapping fails");
        }
        for (int selected = 0; selected < look::Count; ++selected) {
            const int displayed = look::presetAt(category,look::optionFor(category,selected));
            require(displayed == (look::contains(category,selected) ? selected : look::Custom),
                "Category browsing misidentifies the stored recipe");
            require(look::editAction("presetCategory",true,false,selected) == look::None,
                "Category browsing edits the current recipe");
        }
        require(look::presetAt(category,-1) == look::Custom &&
            look::presetAt(category,look::optionCount(category)) == look::Custom,"Invalid menu position applies a look");
    }
    for (int category : std::array<int,2> {-1,look::CategoryCount}) {
        require(look::optionCount(category) == 0 && look::presetAt(category,1) == look::Custom,
            "Invalid category applies a look");
    }
    const auto neutral = packedRecipe(look::Neutral);
    for (int index : std::array<int,11> {film::NegativeColorStrength,film::NegativeToneStrength,film::PrintColorStrength,
        film::PrintToneStrength,film::PushPull,film::ColorRichness,film::SplitTone,13,15,16,film::BloomAmount})
        require(neutral[index] == 0,"Neutral retains a creative effect");
    require(neutral[film::FilmGauge] == gauge::Custom,"Neutral retains a format override");
    const auto cp = color::prepare(neutral.data());
    for (const auto input : std::array<ColorRgb,5> {{{0,0,0},{.18f,.18f,.18f},{.38f,.2f,.12f},
        {-.01f,.02f,.3f},{4,.01f,1.2f}}}) {
        const auto actual = renderColorOnly(input,look::Neutral);
        const auto expected = color_from_work(color_to_work(input,cp),cp);
        require(std::abs(actual.r-expected.r) < 1e-6f && std::abs(actual.g-expected.g) < 1e-6f &&
            std::abs(actual.b-expected.b) < 1e-6f,"Neutral changes color beyond input/output conversion");
    }
    for (int a = 1; a < look::Count; ++a) for (int b = a+1; b < look::Count; ++b)
        require(look::recipe(a) != look::recipe(b),"Two library entries contain identical recipes");
    const auto p160=packedRecipe(look::Portra160), p400=packedRecipe(look::Portra400), p800=packedRecipe(look::Portra800);
    require(p160[16] < p400[16] && p400[16] < p800[16] &&
        p160[17] < p400[17] && p400[17] < p800[17],"Still-negative grain hierarchy is inconsistent");
    for (int pushed : {look::Portra800Push1,look::Portra800Push2}) {
        const auto p=packedRecipe(pushed);
        require(p[17] == p800[17] && p[film::PushPull] == pushed-look::Portra800,
            "Pushed recipe changes grain pitch or misses development amount");
    }
    for (int preset : {look::Kodachrome64,look::Ektachrome100,look::Velvia100,look::Provia100}) {
        const auto s=packedRecipe(preset);
        require(s[1] == 5 && s[film::PrintColorStrength] == 0 && s[film::PrintToneStrength] == 0,
            "Reversal stock look includes a second print response");
    }
    for (int preset : {look::Print2383,look::Print2393}) {
        const auto s=packedRecipe(preset);
        require(s[film::NegativeColorStrength] == 0 && s[film::NegativeToneStrength] == 0 && s[16] == 0,
            "Print-inspired look includes negative shaping or camera grain");
    }
    for (int preset : {look::TriX400,look::Hp5Plus400}) {
        const auto c=renderColorOnly({.38f,.2f,.12f},preset);
        require(std::abs(c.r-c.g) < 1e-6f && std::abs(c.g-c.b) < 1e-6f,"B&W stock look is colored");
    }
    const std::array<int,4> creative {look::DesertChrome,look::ArcticDusk,look::GoldenHour,look::FadedInstant};
    for (size_t a=0; a<creative.size(); ++a) for (size_t b=a+1; b<creative.size(); ++b)
        require(colorDistance(creative[a],creative[b]) > 3,"New creative looks have insufficient color/tone separation");
}

static void testGenreIntent()
{
    const std::array<int,9> genres {look::ArchiveThriller,look::SilverThriller,look::SodiumNoir,
        look::FolkDread,look::DaylightDread,look::GialloCrimson,look::CrimsonDream,
        look::SimulationGreen,look::AmberWasteland};
    double closest = 255;
    for (size_t a=0; a<genres.size(); ++a) for (size_t b=a+1; b<genres.size(); ++b) {
        const double distance = colorDistance(genres[a],genres[b]);
        std::printf("Genre separation: %s / %s: %.3f codes.\n",look::Labels[genres[a]],look::Labels[genres[b]],distance);
        closest = std::min(closest,distance);
        require(distance > 3,"Genre looks overlap without grain/glow");
    }
    for (const auto pair : std::array<std::array<int,2>,5> {{{look::SilverThriller,look::BleachBypass},
        {look::SodiumNoir,look::NeonNights},{look::DaylightDread,look::SoftPortrait},
        {look::FolkDread,look::ArcticDusk},{look::AmberWasteland,look::DesertChrome}}})
        require(colorDistance(pair[0],pair[1]) > 3,"Genre recipe merely renames a general creative look");
    const auto silver=renderColorOnly({.018f,.018f,.018f},look::SilverThriller);
    const auto archive=renderColorOnly({.018f,.018f,.018f},look::ArchiveThriller);
    const auto folk=renderColorOnly({.018f,.018f,.018f},look::FolkDread);
    require(response_luma(archive) > response_luma(silver)+.015f &&
        response_luma(folk) > response_luma(silver)+.015f,"Restrained thrillers/folk horror crush like silver print");
    const auto daylight=renderColorOnly({.18f,.18f,.18f},look::DaylightDread);
    const auto folkGray=renderColorOnly({.18f,.18f,.18f},look::FolkDread);
    require(response_luma(daylight) > response_luma(folkGray)+.025f,"Daylight horror loses its luminous midtones");
    const auto green=renderColorOnly({.018f,.018f,.018f},look::SimulationGreen);
    require(green.g > green.r+.015f && green.g > green.b+.015f,"Simulation loses green shadows");
    const auto amber=renderColorOnly({.18f,.18f,.18f},look::AmberWasteland);
    require(amber.r > amber.g+.06f && amber.g > amber.b+.06f,"Wasteland loses amber print balance");
    const auto crimson=renderColorOnly({.018f,.018f,.018f},look::CrimsonDream);
    require(crimson.b > crimson.g+.015f,"Crimson Dream loses violet shadow separation");
    for (int preset : genres) {
        float previous=-1;
        for (float level : {0.0f,.0001f,.001f,.003f,.01f,.018f,.04f,.08f,.18f}) {
            const auto rgb=renderColorOnly({level,level,level},preset);
            const float luma=response_luma(rgb);
            require(std::isfinite(luma) && luma > previous+1e-6f,"Genre shadows flatten or reverse");
            previous=luma;
        }
    }
    require(look::recipe(look::CrimsonDream)[film::BloomAmount] >
        look::recipe(look::GialloCrimson)[film::BloomAmount],"Dream and giallo diffusion identities collapse");
    std::printf("Closest genre color/tone separation: %.3f codes (synthetic, not film-match validation).\n",closest);
    for (const auto pair : std::array<std::array<int,2>,5>{{{look::WinterCrime,look::FolkDread},
        {look::WinterCrime,look::ArcticDusk},{look::SteelBlue,look::NeonNights},
        {look::Nostromo,look::SteelBlue},{look::Nostromo,look::SilverThriller}}})
        require(colorDistance(pair[0],pair[1])>3,"New movie direction overlaps a related existing recipe");
    for (int preset : {look::ComicRed,look::ComicBlue,look::ComicYellow}) {
        const auto recipe=look::recipe(preset);
        require(recipe[1] != 4 && recipe[film::SelectiveAmount] == 1 &&
            recipe[film::SelectiveView] == 0 && (static_cast<int>(recipe[film::ModuleIndex])&film::SelectiveColor),
            "Graphic noir loses color before selection or loads a matte");
        const ColorRgb kept = preset == look::ComicRed ? ColorRgb{.8f,.01f,.01f} :
            preset == look::ComicBlue ? ColorRgb{.01f,.01f,.8f} : ColorRgb{.8f,.8f,.01f};
        const ColorRgb rejected{.01f,.8f,.01f};
        const auto accent=renderColorOnly(kept,preset), mono=renderColorOnly(rejected,preset);
        require(std::max({accent.r,accent.g,accent.b})-std::min({accent.r,accent.g,accent.b})>.1f,
            "Graphic noir loses selected accent");
        require(std::abs(mono.r-mono.g)<1e-6f && std::abs(mono.g-mono.b)<1e-6f,
            "Graphic noir recolors rejected pixels");
    }
}

int main()
{
    try {
        testLookIntent();
        testCategoriesAndNeutral();
        testGenreIntent();
        std::set<std::string> names, labels;
        std::set<int> settings;
        for (const auto* label : look::Labels) {
            require(labels.insert(label).second, "Duplicate preset label");
            require(std::string(label).find("(Inspired)") == std::string::npos,"UI retains Inspired suffix");
        }
        for (const auto& control : look::Controls) {
            require(names.insert(control.name).second, "Duplicate preset binding");
            require(settings.insert(control.setting).second, "Duplicate packed binding");
            require(control.setting >= 0 && control.setting < film::SettingsCount, "Invalid packed binding");
        }
        for (int preserved : {4,5,6,film::ModuleIndex,25,26,27})
            require(settings.count(preserved) == 0, "Preset overwrites a preserved setting");
        for (const auto* name : {"sourceSpace","outputSpace","exposure","temperature","tint","grainSeed"})
            require(!look::ownsControl(name), "Preset owns encoding/camera balance/seed");
        for (const auto& control : moduleui::Controls) {
            const std::string name = control.name;
            if (name == "sourceSpace" || name == "outputSpace" || name == "exposure" ||
                name == "temperature" || name == "tint" || name == "grainSeed") continue;
            require(look::ownsControl(name), "Preset leaves stale module tuning");
        }
        for (const auto& control : printstyle::Parameters)
            require(look::ownsControl(control.name), "Preset leaves stale print tuning");
        for (int preset = -1; preset <= look::Count; ++preset) {
            std::array<float, film::SettingsCount> packed;
            packed.fill(.123f);
            packed[4]=1.2f; packed[5]=.9f; packed[6]=.8f; packed[25]=12345; packed[26]=5; packed[27]=2;
            const auto before = packed;
            int writes = 0, toggles = 0, mask = 0;
            look::applyPreset(preset, [&](size_t i, double value) {
                const auto& control = look::Controls[i];
                require(std::isfinite(value) && value >= control.minimum && value <= control.maximum,
                        "Preset value outside the exposed control range");
                if (control.kind == look::Choice) require(value == std::floor(value), "Non-integer preset choice");
                packed[control.setting] = static_cast<float>(value);
                ++writes;
            }, [&](size_t i, bool on) {
                if (on) mask |= moduleui::Toggles[i].module;
                ++toggles;
            });
            if (preset <= look::Custom || preset >= look::Count) {
                require(writes == 0 && toggles == 0 && packed == before, "Custom/invalid preset changes settings");
                continue;
            }
            require(writes == std::size(look::Controls) && toggles == moduleui::Toggles.size(), "Incomplete recipe application");
            require(mask == static_cast<int>(look::recipe(preset)[film::ModuleIndex]), "Incorrect module switches");
            const bool selectiveLook = preset == look::ComicRed || preset == look::ComicBlue || preset == look::ComicYellow;
            require(((mask & film::SelectiveColor) != 0) == selectiveLook,"Ordinary preset enables Selective Color");
            require(selectiveLook || (packed[film::SelectiveAmount] == 0 && packed[film::SelectiveView] == 0),
                    "Ordinary preset retains selective amount or matte view");
            require(packed[0] == 0 && packed[2] == printstyle::Custom, "Preset is masked by mode or locked print");
            for (int preserved : {4,5,6,25,26,27})
                require(packed[preserved] == before[preserved], "Preserved setting changed");
            packed[film::ModuleIndex] = static_cast<float>(mask);
            const auto response = response::prepare(packed.data());
            float previous = -1;
            for (int step = 0; step <= 2000; ++step) {
                const float gray = step / 200.0f;
                auto rgb = response_negative_stage({gray,gray,gray}, response);
                if (mask & film::Development) rgb = response_development(rgb,response);
                rgb = response_print(rgb,response);
                rgb = response_finish(rgb,mask,response);
                const float luma = response_luma(rgb);
                require(std::isfinite(luma) && luma >= previous-1e-6f, "Preset gray ramp is not finite/monotonic");
                previous = luma;
            }
            // Reapplying a recipe replaces stale values rather than accumulating effects.
            auto replaced = packed;
            replaced[8]=1.8f; replaced[film::SplitTone]=.9f;
            look::applyPreset(preset, [&](size_t i, double value) {
                replaced[look::Controls[i].setting]=static_cast<float>(value);
            }, [&](size_t, bool) {});
            require(replaced == packed, "Preset application accumulates old tuning");
        }
        for (int selected = 0; selected < look::Count; ++selected) {
            for (const auto& control : look::Controls) {
                require(look::editAction(control.name,true,false,selected) ==
                        (selected == look::Custom ? look::None : look::MarkCustom), "Manual edits do not mark Custom");
                require(look::editAction(control.name,false,false,selected) == look::None,
                        "Undo/reload/time notification rewrites recipe");
                require(look::editAction(control.name,true,true,selected) == look::None,
                        "Preset application recursively marks Custom");
            }
            for (const auto& toggle : moduleui::Toggles)
                require(look::editAction(toggle.name,true,false,selected) ==
                        (selected == look::Custom ? look::None : look::MarkCustom), "Module edits retain misleading preset label");
            require(look::editAction("lookPreset",true,false,selected) ==
                    (selected == look::Custom ? look::None : look::Apply), "Preset selection policy mismatch");
            require(look::editAction("lookPreset",false,false,selected) == look::None,
                    "Restoring a preset selector overwrites restored tuning");
        }
        std::puts("Looks: creative separation, stock-family restraint/texture/pivot, representative skin hue, split-tone identity, complete recipes, valid ranges, preserved context, Custom inheritance, repeatability, gray ramps, and edit/restore/recursion policy pass.");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr,"FAILED: %s\n",error.what());
        return 1;
    }
}
