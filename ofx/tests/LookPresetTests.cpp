// SPDX-License-Identifier: MPL-2.0

#include <cmath>
#include <cstdio>
#include <set>
#include <stdexcept>
#include <string>

#include "LookPresetConfig.h"
#include "FilmResponseConfig.h"

static void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

int main()
{
    try {
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
        std::puts("Looks: complete recipes, valid ranges, preserved camera/encoding/seed, Custom inheritance, repeatability, gray ramps, and edit/restore/recursion policy pass.");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr,"FAILED: %s\n",error.what());
        return 1;
    }
}
