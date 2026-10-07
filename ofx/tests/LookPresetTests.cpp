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
    return color_from_work(response_finish(c,modules,rp),cp);
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

int main()
{
    try {
        testLookIntent();
        std::set<std::string> names, labels;
        std::set<int> settings;
        for (const auto* label : look::Labels)
            require(labels.insert(label).second, "Duplicate preset label");
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
        require(look::Count == 13, "Expected twelve named presets plus Custom");
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
            require(writes == std::size(look::Controls) && toggles == 7, "Incomplete recipe application");
            require(mask == static_cast<int>(look::recipe(preset)[film::ModuleIndex]), "Incorrect module switches");
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
