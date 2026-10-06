// SPDX-License-Identifier: MPL-2.0

#include <cstdio>
#include <set>
#include <stdexcept>
#include <string>

#include "ModuleControlState.h"

static void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

int main()
{
    try {
        const std::array<int, 7> modeMasks {{
            film::All, film::Negative | film::Development | film::Print,
            film::Halation | film::Aura | film::Bloom | film::Grain, film::Grain,
            0, film::Halation | film::Aura, film::Bloom
        }};
        int toggleMask = 0;
        std::set<std::string> names;
        for (const auto& toggle : moduleui::Toggles) {
            require((toggleMask & toggle.module) == 0,"Duplicate module toggle");
            toggleMask |= toggle.module;
            require(names.insert(toggle.name).second,"Duplicate toggle name");
        }
        require(toggleMask == film::All,"Missing module toggle");
        int controlMask = 0;
        for (const auto& control : moduleui::Controls) {
            require(names.insert(control.name).second,"Duplicate module control");
            require(control.modules != 0 && (control.modules & ~film::All) == 0,"Unknown control module");
            controlMask |= control.modules;
        }
        require(controlMask == film::All,"Missing module controls");
        for (const auto& recipe : printstyle::Parameters)
            require(names.insert(recipe.name).second,"Print recipe has conflicting enable rules");
        for (size_t mode = 0; mode < modeMasks.size(); ++mode) {
            for (int enabled = 0; enabled <= film::All; ++enabled) {
                int changedMask = enabled, writes = 0;
                moduleui::applyMode(static_cast<int>(mode),[&](size_t i, bool on) {
                    const int bit = moduleui::Toggles[i].module;
                    changedMask = on ? changedMask | bit : changedMask & ~bit;
                    ++writes;
                });
                require(writes == 7 && changedMask == modeMasks[mode],"Mode does not synchronize all toggles");
                const int active = modeMasks[mode] & enabled;
                for (const auto& toggle : moduleui::Toggles) {
                    require(moduleui::controlEnabled(static_cast<int>(mode),film::All,toggle.module) ==
                            ((modeMasks[mode] & toggle.module) != 0),"Excluded module toggle remains editable");
                    require(moduleui::controlEnabled(static_cast<int>(mode),enabled,toggle.module) ==
                            ((active & toggle.module) != 0),"Disabled module controls remain editable");
                }
                for (const auto& control : moduleui::Controls)
                    require(moduleui::controlEnabled(static_cast<int>(mode),enabled,control.modules) ==
                            ((active & control.modules) != 0),"Mode/module control state mismatch");
                for (int style = 0; style < 4; ++style)
                    require(moduleui::printRecipeEnabled(static_cast<int>(mode),enabled,style) ==
                            ((active & film::Print) != 0 && style == printstyle::Custom),"Print preset locking bypasses module enable");
            }
        }
        // Re-entering Full restores every toggle, without changing print or numeric settings.
        int enabled = film::All;
        for (int mode : {3,4,6,5,2,1,0}) {
            moduleui::applyMode(mode,[&](size_t i, bool on) {
                const int bit = moduleui::Toggles[i].module;
                enabled = on ? enabled | bit : enabled & ~bit;
            });
            require(enabled == modeMasks[mode],"Mode sequence retains stale disabled toggles");
        }
        require(moduleui::controlEnabled(0,film::Development,film::Development),"Neutral Development cannot be edited");
        require(!moduleui::controlEnabled(3,film::All,film::Negative | film::Development | film::Print),"Texture-only output selector remains enabled");
        require(!moduleui::controlEnabled(6,film::All,film::Halation | film::Aura | film::Grain),"Bloom-only enables Film Gauge");
        for (int mode = 0; mode < 7; ++mode) for (int mask = 0; mask <= film::All; ++mask)
            for (int system = 0; system < 6; ++system) for (double strength : {0.0,0.5,0.999,1.0}) {
                const bool mono = system == 4 && (mode == 0 || mode == 1) && (mask & film::Negative);
                for (const char* name : {"saturation","density","gamutCompression","grainColor"})
                    require(moduleui::semanticControlEnabled(name,mode,mask,system,strength) == !(mono && strength >= 1),
                            "Mono semantic greying ignores mode/module/system/partial strength");
                for (const char* name : {"negativeCrosstalk","skinHue"})
                    require(moduleui::semanticControlEnabled(name,mode,mask,system,strength) == !mono,
                            "Mono leaves unused palette/skin controls editable at partial strength");
                for (const char* name : {"system","negativeColorStrength","negativeToneStrength","contrast",
                                         "temperature","printColor","splitTone","halationColor","grainSize"})
                    require(moduleui::semanticControlEnabled(name,mode,mask,system,strength),
                            "Mono semantic greying disables an effective or recovery control");
            }
        std::puts("Module UI: every mode/mask, all control bindings, toggle synchronization, manual-disable policy, print locks, neutral Development, globals, and mode transitions pass.");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr,"FAILED: %s\n",error.what());
        return 1;
    }
}
