// SPDX-License-Identifier: MPL-2.0

#include <cstdio>
#include <set>
#include <stdexcept>
#include <string>

#include "ModuleControlState.h"
#include "ColorSpaceConfig.h"
#include "LookPresetConfig.h"

static void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

int main()
{
    try {
        const std::array<int, 7> modeMasks {{
            film::All, film::Negative | film::Development | film::Print | film::SelectiveColor,
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
                }, (enabled & film::SelectiveColor) != 0);
                const int expected = modeMasks[mode] & (film::DefaultModules | (enabled & film::SelectiveColor));
                require(writes == moduleui::Toggles.size() && changedMask == expected,"Mode does not synchronize toggles or preserve selective opt-in");
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
        // Default mode edits never enable Selective Color, even with retained amounts.
        int enabled = film::All;
        for (int mode : {3,4,6,5,2,1,0}) {
            moduleui::applyMode(mode,[&](size_t i, bool on) {
                const int bit = moduleui::Toggles[i].module;
                enabled = on ? enabled | bit : enabled & ~bit;
            });
            require(enabled == (modeMasks[mode] & film::DefaultModules),"Mode sequence opts into selective color");
        }
        enabled = film::SelectiveColor;
        for (int mode : {0,1,0,2,0,1,4,0}) {
            const int expected = modeMasks[mode] & (film::DefaultModules | (enabled & film::SelectiveColor));
            moduleui::applyMode(mode,[&](size_t i, bool on) {
                const int bit = moduleui::Toggles[i].module;
                enabled = on ? enabled | bit : enabled & ~bit;
            }, (enabled & film::SelectiveColor) != 0);
            require(enabled == expected,"Mode transition loses explicit selective opt-in or restores it after exclusion");
        }
        require(moduleui::controlEnabled(0,film::Development,film::Development),"Neutral Development cannot be edited");
        require(!moduleui::controlEnabled(3,film::All,film::Negative | film::Development | film::Print),"Texture-only output selector remains enabled");
        require(!moduleui::controlEnabled(6,film::All,film::Halation | film::Aura | film::Grain),"Bloom-only enables Film Gauge");
        for (int mode = 0; mode < 7; ++mode) for (int mask = 0; mask <= film::All; ++mask)
            require(moduleui::semanticControlEnabled("grainResponse",mode,mask,0,1) ==
                (mode == 0 && (mask & (film::Negative | film::Development | film::Print)) != 0),
                "Grain Response remains editable in texture-only or excluded modes");
        for (int mode = 0; mode < 7; ++mode) for (int mask = 0; mask <= film::All; ++mask)
            for (int system = 0; system < 6; ++system) for (double strength : {0.0,0.5,0.999,1.0}) {
                const bool mono = system == 4 && (mode == 0 || mode == 1) && (mask & film::Negative);
                for (const char* name : {"saturation","density","gamutCompression","grainColor","highlightRetention"})
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
        auto recipe=look::recipe(look::Neutral);
        std::array<float,film::SettingsCount> s {};
        for (size_t i=0; i<s.size(); ++i) s[i]=static_cast<float>(recipe[i]);
        s[26]=color::AlexaLogC3; s[27]=1; s[film::HighlightRetention]=1;
        for (int mode=0; mode<7; ++mode) for (int mask=0; mask<=film::All; ++mask)
            for (int rendering=0; rendering<3; ++rendering) for (int system : {0,4}) for (float strength : {.5f,1.0f}) {
                s[0]=static_cast<float>(mode); s[19]=static_cast<float>(mask); s[1]=static_cast<float>(system);
                s[film::OutputRendering]=static_cast<float>(rendering); s[film::NegativeColorStrength]=strength;
                const auto cp=color::prepare(s.data());
                const bool expected=(mode==0 || mode==1) && (mask & film::Negative) && rendering!=color::ConversionOnly &&
                    !(system==4 && strength==1);
                require((cp.highlightRetention>0)==expected,"Retention rendering policy mismatch");
                const auto view=color::prepare(color::AlexaLogC3,1,false,rendering);
                require(color::highlightRetentionEnabled(view,film::modulesForMode(mode,mask),system,strength,0)==expected,
                    "Retention UI disagrees with rendering");
            }
        s[0]=1; s[19]=film::Negative|film::SelectiveColor; s[1]=0;
        s[film::OutputRendering]=color::Automatic; s[film::SelectiveView]=1;
        require(color::prepare(s.data()).highlightRetention==0,"Retention affects selective matte");
        s[film::SDRContrast]=s[film::SDRRolloff]=s[film::SDRGamut]=1;
        s[film::HDRExposure]=s[film::HDRRolloff]=1;
        for (int mode=0; mode<7; ++mode) for (int output : {1,5,color::HDRPQOutput})
            for (int rendering=0; rendering<color::RenderingCount; ++rendering) for (int view : {0,1}) {
                s[0]=static_cast<float>(mode); s[27]=static_cast<float>(output);
                s[film::OutputRendering]=static_cast<float>(rendering); s[film::SelectiveView]=static_cast<float>(view);
                const auto cp=color::prepare(s.data());
                const int modules=film::modulesForSettings(s.data());
                const bool enabled=color::sdrControlsEnabled(cp,modules,view);
                require((cp.sdrContrast!=0)==enabled && (cp.sdrRolloff!=0)==enabled && (cp.sdrGamut!=0)==enabled,
                        "SDR controls UI/render policy mismatch");
                require(!enabled || ((mode==0 || mode==1) && output==1 && rendering!=color::ConversionOnly &&
                         rendering!=color::StandardHDR && view==0),"SDR controls leak into other paths");
                const bool hdrEnabled=color::hdrControlsEnabled(cp,modules,view);
                require((cp.hdrGain!=1)==hdrEnabled && (cp.hdrRolloff!=0)==hdrEnabled,
                        "HDR viewing UI/render policy mismatch");
                require(!hdrEnabled || ((mode==0 || mode==1) && output==color::HDRPQOutput &&
                        rendering!=color::ConversionOnly && rendering!=color::StandardSDR && view==0),
                        "HDR viewing controls leak into other paths");
            }
        std::puts("Module UI: every mode/mask, all control bindings, toggle synchronization, manual-disable policy, print locks, neutral Development, globals, mode transitions and retention UI/render policies pass.");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr,"FAILED: %s\n",error.what());
        return 1;
    }
}
