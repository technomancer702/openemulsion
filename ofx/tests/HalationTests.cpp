// SPDX-License-Identifier: MPL-2.0

#include <Windows.h>

#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "HalationBlur.h"
#include "HalationConfig.h"
#include "GrainConfig.h"
#include "ColorSpaceConfig.h"
#include "FilmResponseConfig.h"

extern bool RunOpenEmulsionOpenCL(void*, int, int, double, const float*, const float*, float*);

static void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

static halation::Blur cpuBlur(const std::vector<float>& input, int width, int height, const grain::PackedSettings& settings)
{
    const auto halo = halation::prepare(settings.data(), height);
    halation::Blur blur(width, height, halation::Filter(settings[13], settings[14], settings[15], halo.auraRadius, halo.radiusScale), halo.downsample);
    const auto color = color::prepare(settings.data());
    blur.extractRows(0, blur.height, [&](int x, int y) {
        const size_t i = (static_cast<size_t>(y) * width + x) * 4;
        const auto work = color_to_work({input[i], input[i + 1], input[i + 2]}, color);
        return halation_key(work.r, work.g, work.b, halo.key);
    });
    blur.blurRows(0, blur.height, true);
    blur.blurRows(0, blur.height, false);
    return blur;
}

static grain::PackedSettings settings(float radius, float amount = 1.0f, float aura = 1.0f, int mode = 2)
{
    grain::PackedSettings s {};
    s[0] = static_cast<float>(mode);
    s[4] = s[5] = s[6] = s[8] = s[10] = 1.0f;
    s[13] = amount;
    s[14] = radius;
    s[15] = aura;
    s[19] = static_cast<float>(film::All);
    s[20] = 0.25f;
    s[21] = 0.25f;
    s[22] = 1.15f;
    s[23] = 0.80f;
    s[24] = 0.42f;
    s[26] = color::SRGB;
    s[film::NegativeColorStrength] = s[film::NegativeToneStrength] = 1.0f;
    s[film::PrintColorStrength] = s[film::PrintToneStrength] = 1.0f;
    s[film::HalationThreshold] = 0.48f; s[film::HalationSoftness] = 0.52f;
    s[film::HalationColor] = 0.50f; s[film::AuraRadius] = 1.0f;
    return s;
}

static grain::PackedSettings filmSettings()
{
    auto s = settings(0.0f, 0.0f, 0.0f, 1);
    s[2] = 1.0f; s[7] = 0.18f; s[8] = 0.95f; s[9] = 0.16f; s[10] = 1.08f;
    s[11] = 0.43f; s[12] = 0.45f;
    s[28] = 0.50f; s[29] = 0.35f; s[30] = 0.50f;
    s[33] = s[36] = 1.0f; s[34] = 0.55f;
    return s;
}

static void requireNear(float actual, float expected, float tolerance, const char* message)
{
    if (!std::isfinite(actual) || std::abs(actual - expected) > tolerance) {
        std::fprintf(stderr, "%s: actual %.9g, expected %.9g, tolerance %.9g\n", message, actual, expected, tolerance);
        require(false, message);
    }
}

static void testColorSpaces()
{
    const std::array<float, color::SpaceCount> gray {0.39100683f, 0.27839584f, 420.0f/1023.0f, 420.0f/1023.0f,
        0.33604327f, 0.4894371f, 0.4613561f, 0.38356164f, 0.33333266f, 0.39825469f, 0.34338937f,
        0.42331145f, 0.4135884f, 0.18f, 0.18f};
    for (int space = 0; space < static_cast<int>(color::spaces().size()); ++space) {
        const int curve = color::spaces()[space].curve;
        requireNear(color_encode(0.18f, curve), gray[space], 3e-6f, color::spaces()[space].label);
        const auto p = color::prepare(space, 0, true);
        const auto white = color_matrix({1,1,1}, p.to709);
        requireNear(white.r, 1, 3e-6f, "White adaptation R");
        requireNear(white.g, 1, 3e-6f, "White adaptation G");
        requireNear(white.b, 1, 3e-6f, "White adaptation B");
        float previous = -1e10f;
        for (float x : {-1.0f, -0.05f, -0.015f, -0.01f, -0.001f, 0.0f, 0.001f, 0.00262409f, 0.0031308f,
                        0.005f, 0.0078125f, 0.01f, 0.010591f, 0.01125f, 0.0126f, 0.18f, 1.0f, 16.0f, 184.32f, 1000.0f}) {
            const float encoded = color_encode(x, curve);
            require(encoded >= previous, "Non-monotonic transfer curve");
            previous = encoded;
            requireNear(color_decode(encoded, curve), x, 8e-6f * std::max(1.0f, std::abs(x)), "Transfer round trip");
        }
        for (ColorRgb work : {ColorRgb{-0.12f, 0.20f, 1.8f}, ColorRgb{0.4f, 0.3f, 0.2f}, ColorRgb{1,1,1}}) {
            const auto encoded = color_from_work(work, p);
            const auto restored = color_to_work(encoded, p);
            requireNear(restored.r, work.r, 3e-5f, "Gamut round trip R");
            requireNear(restored.g, work.g, 3e-5f, "Gamut round trip G");
            requireNear(restored.b, work.b, 3e-5f, "Gamut round trip B");
        }
    }
    // External reference matrices catch incorrect primaries, not just matching inverses.
    const std::array<float, 9> vlogTo709 {1.806576f,-0.695697f,-0.110879f, -0.170090f,1.305955f,-0.135865f, -0.025206f,-0.154468f,1.179674f};
    const auto vlog = color::prepare(color::PanasonicVLog, 0, true);
    for (int i = 0; i < 9; ++i) requireNear(vlog.to709[i], vlogTo709[i], 3e-6f, "Panasonic reference matrix");
    const auto xyz = color::rgbToXYZ(color::spaces()[color::DaVinciIntermediate]);
    const std::array<float, 9> dwgXYZ {0.70062239f,0.14877482f,0.10105872f, 0.27411851f,0.87363190f,-0.14775041f, -0.09896291f,-0.13789533f,1.32591599f};
    for (int i = 0; i < 9; ++i) requireNear(static_cast<float>(xyz[i]), dwgXYZ[i], 3e-6f, "Blackmagic DWG reference matrix");
    const std::array<float, 9> awg3XYZ {0.638008f,0.214704f,0.097744f, 0.291954f,0.823841f,-0.115795f, 0.002798f,-0.067034f,1.153294f};
    const std::array<float, 9> awg4XYZ {0.70485832f,0.12976030f,0.11583731f, 0.25452418f,0.78147773f,-0.03600191f, 0,0,1.08905775f};
    const auto awg3 = color::rgbToXYZ(color::spaces()[color::AlexaLogC3]), awg4 = color::rgbToXYZ(color::spaces()[color::ArriLogC4]);
    for (int i = 0; i < 9; ++i) {
        requireNear(static_cast<float>(awg3[i]), awg3XYZ[i], 1e-6f, "ARRI AWG3 reference matrix");
        requireNear(static_cast<float>(awg4[i]), awg4XYZ[i], 1e-6f, "ARRI AWG4 reference matrix");
    }
    const float acesLimit = color_decode(2.0f, ColorACEScct);
    requireNear(acesLimit, 65504.0f, 0.2f, "ACEScct half-float limit");
    const auto grayWork = color_curve_rgb({0.18f, 0.18f, 0.18f}, ColorSRGB, 1);
    const auto plusStop = color_curve_rgb(color_balance(grayWork, {2,2,2}), ColorSRGB, 0);
    requireNear(plusStop.r, 0.36f, 1e-6f, "Managed one-stop exposure");
    std::puts("Color: manufacturer gray anchors, negative/HDR transfer/gamut round trips, monotonicity, white adaptation, and published reference matrices pass.");
}

static void testResponse()
{
    auto s = filmSettings();
    for (int system = 0; system < 6; ++system) {
        for (int style = 0; style < 4; ++style) {
            for (int extreme = 0; extreme < 4; ++extreme) {
                s[1] = static_cast<float>(system); s[2] = static_cast<float>(style);
                s[9] = s[28] = s[34] = extreme & 1 ? 1.0f : 0.0f;
                s[10] = s[33] = extreme & 2 ? 2.0f : 0.5f;
                s[32] = extreme & 1 ? 1.0f : -1.0f;
                const auto p = response::prepare(s.data());
                for (ResponseTone tone : {p.negativeTone, p.printTone}) {
                    float previous = -1.0f;
                    for (int i = 0; i <= 8000; ++i) {
                        const float x = i / 1000.0f;
                        const float y = response_tone(x, tone);
                        require(std::isfinite(y) && y >= previous && y >= 0 && y <= tone.ceiling, "Film curve non-monotonic/unbounded");
                        if (i == 1) require(y > 0.0f, "Film curve crushes nonzero shadows");
                        previous = y;
                    }
                    requireNear(response_tone(0.46135613f, tone), 0.46135613f, 1e-6f, "Middle gray moves with contrast");
                    for (float x : {16.0f, 1000.0f, 1000000.0f}) {
                        const float y = response_tone(x, tone);
                        require(std::isfinite(y) && y >= previous && y <= tone.ceiling, "Extreme HDR folds/overflows tone curve");
                        previous = y;
                    }
                    const float pivot = 0.46135613f;
                    const float toeJoin = 0.23f * tone.contrast * pivot / (pivot + 0.23f * (tone.contrast - 1.0f));
                    const float shoulderJoin = pivot + (tone.knee - pivot) / tone.contrast;
                    for (float join : {toeJoin, pivot, shoulderJoin}) {
                        const float step = 0.0001f, center = response_tone(join, tone);
                        const float left = (center - response_tone(join - step, tone)) / step;
                        const float right = (response_tone(join + step, tone) - center) / step;
                        require(std::abs(left - right) < 0.012f, "Film curve has a slope discontinuity");
                    }
                }
            }
        }
    }
    s = filmSettings(); s[35] = 1.0f;
    for (int system = 0; system < 6; ++system) {
        for (int style = 0; style < 4; ++style) {
            s[1] = static_cast<float>(system); s[2] = static_cast<float>(style);
            printstyle::applyPreset(style, [&](int control, double value) {
                s[printstyle::Parameters[control].setting] = static_cast<float>(value);
            });
            s[2] = printstyle::Custom; s[35] = 1.0f;
            const auto p = response::prepare(s.data());
            for (float v : {0.0f, 0.001f, 0.05f, 0.18f, 0.46135613f, 0.9f, 4.0f, 1000.0f}) {
                const auto n = response_negative({v,v,v}, p), t = response_print(n, p);
                requireNear(n.r, n.g, 3e-6f, "Negative neutral axis R/G");
                requireNear(n.g, n.b, 3e-6f, "Negative neutral axis G/B");
                requireNear(t.r, t.g, 3e-6f, "Neutralized print axis R/G");
                requireNear(t.g, t.b, 3e-6f, "Neutralized print axis G/B");
            }
        }
    }
    for (ColorRgb c : {ColorRgb{0.12f,0.18f,0.28f}, ColorRgb{2.0f,-0.2f,0.1f}, ColorRgb{0.1f,-0.15f,4.0f}}) {
        if (response_luma(c) <= 0 || response_luma(c) >= 1) continue;
        const auto out = response_gamut(c, 1.0f, 0.70f);
        require(out.r >= -2e-6f && out.g >= -2e-6f && out.b >= -2e-6f && out.r <= 1.000002f && out.g <= 1.000002f && out.b <= 1.000002f, "Gamut escape");
        requireNear(response_luma(out), response_luma(c), 3e-6f, "Gamut compression changes luminance");
        const float y = response_luma(c);
        requireNear((out.r-y)*(c.g-y), (out.g-y)*(c.r-y), 3e-6f, "Gamut compression rotates hue");
    }
    const ColorRgb skin {0.60f,0.46f,0.34f};
    require(response_skin(skin, 1).g < response_skin(skin, -1).g, "Skin Hue is ineffective/reversed");
    for (ColorRgb c : {ColorRgb{0.5f,0.5f,0.5f}, ColorRgb{0.1f,0.3f,0.9f}, ColorRgb{0.1f,0.8f,0.2f}}) {
        const auto a = response_skin(c, 1);
        require(a.r == c.r && a.g == c.g && a.b == c.b, "Skin Hue changes neutral/blue/green");
    }
    s = filmSettings(); s[29] = 0.0f;
    s[7] = 0.0f;
    const auto p0 = response::prepare(s.data());
    s[7] = 0.9f;
    const auto p1 = response::prepare(s.data());
    const auto a = response_negative({0.5f,0.5f,0.5f}, p0), b = response_negative({0.5f,0.5f,0.5f}, p1);
    requireNear(a.r, b.r, 1e-6f, "Density shifts neutrals");
    require(response_luma(response_negative({0.85f,0.12f,0.10f}, p1)) < response_luma(response_negative({0.85f,0.12f,0.10f}, p0)), "Density does not enrich saturated colors");
    const std::array<ColorRgb, 6> chips {{{0.08f,0.08f,0.08f}, {0.5f,0.5f,0.5f}, {3,3,3}, skin, {0.8f,0.05f,0.01f}, {-0.3f,0.15f,1.2f}}};
    for (int style = 0; style < 4; ++style) {
        s = filmSettings(); s[2] = static_cast<float>(style);
        const auto original = response::prepare(s.data());
        const auto floor = response_print({0,0,0}, original);
        requireNear(floor.r, original.printLift, 1e-6f, "Print floor R");
        requireNear(floor.g, original.printLift, 1e-6f, "Print floor G");
        requireNear(floor.b, original.printLift, 1e-6f, "Print floor B");
        for (const auto control : std::array<std::array<float, 2>, 17>{{{9,1},{10,1.8f},{28,1},{29,1},{30,1},{31,1},
                 {11,1},{12,1},{32,1},{33,1.8f},{34,1},{35,1},{36,1.8f},{37,1},{38,1},{39,1},{40,1}}}) {
            auto changed = s; changed[static_cast<int>(control[0])] = control[1];
            const auto adjusted = response::prepare(changed.data());
            double difference = 0;
            for (auto c : chips) {
                const auto before = control[0] <= 10 || (control[0] >= 28 && control[0] <= 31) ? response_negative(c, original) : response_print(c, original);
                const auto after = control[0] <= 10 || (control[0] >= 28 && control[0] <= 31) ? response_negative(c, adjusted) : response_print(c, adjusted);
                difference += std::abs(before.r-after.r) + std::abs(before.g-after.g) + std::abs(before.b-after.b);
            }
            const bool recipe = std::any_of(printstyle::Parameters.begin(), printstyle::Parameters.end(),
                [&](const auto& parameter) { return parameter.setting == static_cast<int>(control[0]); });
            if (!printstyle::isCustom(style) && recipe) {
                require(difference == 0, "Fixed print recipe accepts a locked slider");
            } else {
                if (difference <= 0.00001) std::fprintf(stderr, "Dead control %.0f, print style %d\n", control[0], style);
                require(difference > 0.00001, "Editable response slider has no effect");
            }
        }
    }
    std::puts("Response: monotonic HDR curves, smooth joins, stable gray, gamut hue/luminance, skin isolation, density, fixed recipes, and editable Custom/balance controls pass.");
}

static void requireRgbNear(ColorRgb a, ColorRgb b, float tolerance, const char* message)
{
    requireNear(a.r, b.r, tolerance, message);
    requireNear(a.g, b.g, tolerance, message);
    requireNear(a.b, b.b, tolerance, message);
}

static void testPrintPresets()
{
    const std::array<ColorRgb, 6> chips {{{0,0,0},{0.08f,0.08f,0.08f},{0.5f,0.5f,0.5f},
        {0.8f,0.2f,0.1f},{-0.2f,0.3f,1.8f},{5,5,5}}};
    for (int style = printstyle::Full; style < printstyle::Custom; ++style) {
        require(!printstyle::isCustom(style), "Named print style is editable");
        auto s = filmSettings(); s[2] = static_cast<float>(style);
        s[37] = 0.25f; s[38] = -0.10f; s[film::PrintColorStrength] = 0.71f;
        const auto beforeSelection = s;
        int writes = 0;
        printstyle::applyPreset(style, [&](int control, double value) {
            s[printstyle::Parameters[control].setting] = static_cast<float>(value);
            ++writes;
        });
        require(writes == printstyle::ControlCount, "Preset does not populate every recipe knob");
        for (int index = 0; index < film::SettingsCount; ++index) {
            const bool recipe = std::any_of(printstyle::Parameters.begin(), printstyle::Parameters.end(),
                [&](const auto& parameter) { return parameter.setting == index; });
            if (!recipe) require(s[index] == beforeSelection[index], "Preset resets independent settings");
        }
        const auto named = response::prepare(s.data());
        s[2] = printstyle::Custom;
        const auto inherited = s;
        printstyle::applyPreset(printstyle::Custom, [&](int, double) { ++writes; });
        require(s == inherited && writes == printstyle::ControlCount, "Custom overwrites inherited knobs");
        const auto custom = response::prepare(s.data());
        for (auto chip : chips)
            requireRgbNear(response_print(chip,named), response_print(chip,custom), 0, "Custom changes the inherited preset look");
        auto dirty = s; dirty[2] = static_cast<float>(style);
        for (int control = 0; control < printstyle::ControlCount; ++control)
            dirty[printstyle::Parameters[control].setting] = control % 2 ? 1.8f : -1;
        const auto fixed = response::prepare(dirty.data());
        for (auto chip : chips)
            requireRgbNear(response_print(chip,named), response_print(chip,fixed), 0, "Stored knobs change a fixed recipe");
    }
    require(printstyle::isCustom(printstyle::Custom), "Custom print style remains locked");
    auto s = filmSettings();
    const auto standard = response::prepare(s.data());
    requireNear(standard.printTone.contrast,1.12f,1e-7f,"Standard contrast changed");
    requireNear(standard.printTone.toe,0.35f,1e-7f,"Standard toe changed");
    requireNear(standard.printTone.knee,0.73f,1e-7f,"Standard knee changed");
    requireNear(standard.printCast,0.57f,1e-7f,"Standard print character changed");
    requireNear(standard.printLift,0.45f*0.035f,1e-7f,"Standard black lift changed");
    for (int style : {printstyle::Full,printstyle::Extended}) {
        s[2] = static_cast<float>(style);
        const auto p = response::prepare(s.data());
        require(std::abs(p.printTone.contrast-standard.printTone.contrast) > 0.05f, "Print presets lack distinct tone");
        require(std::abs(p.printLift-standard.printLift) > 0.004f, "Print presets lack distinct black points");
    }
    s[2] = printstyle::Extended;
    const auto extended = response::prepare(s.data());
    for (float gray : {0.05f,0.20f,0.50f,0.80f}) {
        const auto c = response_print({gray,gray,gray},extended);
        requireNear(c.r,c.g,2e-6f,"Extended gray axis R/G");
        requireNear(c.g,c.b,2e-6f,"Extended gray axis G/B");
    }
    std::puts("Print presets: fixed recipes, populated knobs, exact Custom inheritance, independent settings, preserved Standard, and neutral Extended pass.");
}

static void testMonochrome()
{
    const std::array<ColorRgb, 4> chips {{{0.2f,0.4f,0.8f},{1.4f,0.6f,0.2f},{-0.2f,0.3f,1.8f},{8,2,1}}};
    for (int mode = 0; mode <= 5; ++mode) for (int mask = 0; mask <= film::All; ++mask) {
        for (float strength : {0.0f,0.37f,1.0f}) {
            auto s = filmSettings();
            s[0] = static_cast<float>(mode); s[1] = 4; s[19] = static_cast<float>(mask);
            s[film::NegativeColorStrength] = strength; s[21] = 1; s[16] = 0.7f;
            const int modules = film::modulesForMode(mode,mask);
            const float expectedStrength = modules & film::Negative ? strength : 0;
            requireNear(film::monochromeStrength(s.data()),expectedStrength,0,"Mono strength ignores module/mode gating");
            const auto p = response::prepare(s.data());
            const auto grain = grain::prepare(s.data(),1080,37);
            requireNear(grain.color,1-expectedStrength,0,"Mono grain color is not gated/blended");
            for (auto chip : chips) {
                const auto c = response_finish(chip,modules,p);
                const float y = response_luma(chip);
                requireRgbNear(c,response_mix(chip,{y,y,y},expectedStrength),0,"Mono finalization blend");
                requireNear(response_luma(c),y,2e-6f,"Mono finalization changes luminance");
                if (expectedStrength == 1) require(c.r == c.g && c.g == c.b,"Full Mono finalization has chroma");
                if (expectedStrength == 0) require(c.r == chip.r && c.g == chip.g && c.b == chip.b,"Inactive Mono alters pixels");
            }
            if (expectedStrength == 1) {
                const auto delta = grain_delta(29,57,0.45f,grain);
                require(delta.r == delta.g && delta.g == delta.b,"Full Mono retains colored grain");
            }
        }
    }
    for (int system : {0,1,2,3,5}) {
        auto s = filmSettings(); s[1] = static_cast<float>(system); s[21] = 1;
        const auto p = response::prepare(s.data());
        require(grain::prepare(s.data(),1080,37).color == 1,"Non-Mono loses colored grain");
        for (auto chip : chips) requireRgbNear(response_finish(chip,film::All,p),chip,0,"Non-Mono finalization changes pixels");
    }
    std::puts("Monochrome: neutral final composite, luminance preservation, continuous strengths, mono grain, and exact inactive/non-Mono isolation pass.");
}

static void testStrengthControls()
{
    const std::array<ColorRgb, 5> chips {{{0.08f,0.08f,0.08f}, {0.5f,0.5f,0.5f},
        {0.8f,0.2f,0.1f}, {-0.2f,0.3f,1.8f}, {5,5,5}}};
    for (int system = 0; system < 6; ++system) {
        for (int style = 0; style < 4; ++style) {
            auto s = filmSettings();
            s[1] = static_cast<float>(system); s[2] = static_cast<float>(style);
            s[film::NegativeColorStrength] = s[film::NegativeToneStrength] = 0;
            s[film::PrintColorStrength] = s[film::PrintToneStrength] = 0;
            auto p = response::prepare(s.data());
            for (auto c : chips) {
                requireRgbNear(response_negative(c, p), c, 0, "Zero negative strengths change pixels");
                requireRgbNear(response_print(c, p), c, 0, "Zero print strengths change pixels");
            }
            // Tone-only processing ignores palette, saturation, density, and print casts.
            s[film::NegativeToneStrength] = s[film::PrintToneStrength] = 1;
            p = response::prepare(s.data());
            auto changed = s;
            changed[7] = 0.9f; changed[8] = 2; changed[29] = changed[30] = changed[31] = 1;
            changed[11] = changed[35] = 1; changed[36] = 2;
            auto q = response::prepare(changed.data());
            for (auto c : chips) {
                requireRgbNear(response_negative(c, p), response_negative(c, q), 0, "Disabled negative color leaks");
                requireRgbNear(response_print(c, p), response_print(c, q), 0, "Disabled print color leaks");
            }
            // Color-only processing ignores the toe, contrast, shoulder, and lifted floor.
            s[film::NegativeColorStrength] = s[film::PrintColorStrength] = 1;
            s[film::NegativeToneStrength] = s[film::PrintToneStrength] = 0;
            p = response::prepare(s.data());
            changed = s;
            changed[9] = changed[28] = 1; changed[10] = 2;
            changed[12] = changed[32] = changed[34] = 1; changed[33] = 2;
            q = response::prepare(changed.data());
            for (auto c : chips) {
                requireRgbNear(response_negative(c, p), response_negative(c, q), 0, "Disabled negative tone leaks");
                requireRgbNear(response_print(c, p), response_print(c, q), 0, "Disabled print tone leaks");
                require(std::isfinite(response_print(c, p).r), "Color-only HDR overflows");
            }
            // Partial tone strength interpolates the luminance response, not the source encoding.
            s[film::NegativeColorStrength] = 0; s[film::NegativeToneStrength] = 0.37f;
            p = response::prepare(s.data());
            const ColorRgb gray {0.6f,0.6f,0.6f};
            requireRgbNear(response_negative(gray, p),
                response_mix(gray, response_luminance_curve(gray, p.negativeTone), 0.37f), 1e-6f, "Tone strength interpolation");
        }
    }
    auto s = filmSettings();
    s[film::PrintColorStrength] = s[film::PrintToneStrength] = 0;
    s[37] = 1;
    requireRgbNear(response_print({0.4f,0.3f,0.2f}, response::prepare(s.data())),
        color_balance({0.4f,0.3f,0.2f}, {2,2,2}), 2e-6f, "Print exposure disabled by strengths");
    std::puts("Strengths: exact zero response, independent tone/color controls, partial tone, and retained print exposure pass.");
}

static void testHalationControls()
{
    auto s = settings(1);
    auto base = halation::prepare(s.data(), 1080);
    const float reference = halation_key(0.60f,0.60f,0.60f,base.key);
    s[film::HalationThreshold] = 0.70f;
    require(halation_key(0.60f,0.60f,0.60f,halation::prepare(s.data(),1080).key) < reference, "Threshold is ineffective");
    s[film::HalationThreshold] = 0.48f; s[film::HalationSoftness] = 0.80f;
    require(halation_key(0.60f,0.60f,0.60f,halation::prepare(s.data(),1080).key) < reference, "Transition is ineffective");
    float previous = -1;
    for (int i = 0; i <= 3000; ++i) {
        const float v = i / 1000.0f, key = halation_key(v,v,v,base.key);
        require(key >= previous && key >= 0 && key <= 1, "Highlight key folds/escapes");
        previous = key;
    }
    for (float hue : {0.0f,0.5f,1.0f}) {
        s[film::HalationColor] = hue;
        const auto p = halation::prepare(s.data(),1080).key;
        requireNear(p.red*0.2126f+p.green*0.7152f+p.blue*0.0722f, 0.291827f, 1e-6f, "Hue changes halo luminance");
    }
    s = settings(1);
    s[film::AuraRadius] = 0;
    auto narrow = halation::prepare(s.data(),1080);
    halation::Filter a(1,1,1,narrow.auraRadius,narrow.radiusScale);
    s[film::AuraRadius] = 2;
    auto wide = halation::prepare(s.data(),1080);
    halation::Filter b(1,1,1,wide.auraRadius,wide.radiusScale);
    require(b.radius > a.radius, "Aura radius is ineffective");
    requireNear(a.weights[a.radius].local,b.weights[b.radius].local,1e-7f,"Aura radius changes local halation");
    s[16] = 1;
    auto p = grain::prepare(s.data(),1080,37);
    for (int format = 0; format < 5; ++format) {
        s[film::FilmGauge] = static_cast<float>(format);
        auto g = grain::prepare(s.data(),1080,37);
        const auto halo = halation::prepare(s.data(),1080);
        requireNear(g.inverseSize * gauge::Profiles[format].scale, p.inverseSize, 1e-6f, "Gauge grain scale");
        requireNear(g.amount, gauge::Profiles[format].grainStrength, 1e-6f, "Gauge grain strength");
        requireNear(halo.radiusScale, gauge::Profiles[format].scale, 1e-6f, "Gauge halation scale");
        s[16] = 0;
        require(grain::prepare(s.data(),1080,37).amount == 0, "Gauge enables zero grain");
        s[16] = 1;
    }
    std::array<double,2> variance {};
    for (int scale = 1; scale <= 2; ++scale) {
        const int width = 160 * scale, height = 1080 * scale;
        s = settings(1);
        const auto halo = halation::prepare(s.data(),height);
        halation::Blur blur(width,height,halation::Filter(1,1,1,halo.auraRadius,halo.radiusScale),halo.downsample);
        blur.extractRows(0,blur.height,[&](int x,int y) { return x == width/2 && y == height/2 ? 1.0f : 0.0f; });
        blur.blurRows(0,blur.height,true); blur.blurRows(0,blur.height,false);
        double mass = 0, center = 0, moment = 0;
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
            const auto h = blur.sample(x,y);
            mass += h.local; center += h.local*x; moment += h.local*x*x;
        }
        require(std::abs(mass-1) < 1e-4, "Resolution-scaled blur loses small lights");
        variance[scale-1] = (moment/mass-(center/mass)*(center/mass))/(height*height);
    }
    require(std::abs(variance[1]/variance[0]-1) < 0.025, "Halation size changes between HD and 4K");
    std::puts("Halation: threshold/transition, bounded smooth key, luminance-stable hue, independent aura, gauge scaling, and HD/4K spread pass.");
}

static void testImpulse()
{
    const int width = 131, height = 131;
    std::vector<float> input(width * height * 4, 0.0f);
    const int i = (64 * width + 64) * 4;
    input[i] = input[i + 1] = input[i + 2] = 1.0f;
    for (float radius : {0.0f, 0.25f, 1.0f, 2.0f}) {
        auto blur = cpuBlur(input, width, height, settings(radius));
        double massLocal = 0.0, massAura = 0.0;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                auto p = blur.sample(x, y);
                require(std::isfinite(p.local) && std::isfinite(p.aura) && p.local >= 0.0f && p.aura >= 0.0f, "Invalid halo sample");
                massLocal += p.local;
                massAura += p.aura;
            }
        }
        require(std::abs(massLocal - 1.0) < 0.001 && std::abs(massAura - 1.0) < 0.001, "Blur loses highlight energy");
        for (const auto direction : std::array<std::array<int, 2>, 3>{{{1, 0}, {0, 1}, {1, 1}}}) {
            auto previous = blur.sample(65, 65);
            for (int distance = 1; distance < 60; ++distance) {
                const auto p = blur.sample(65 + direction[0] * distance, 65 + direction[1] * distance);
                require(p.local <= previous.local + 1e-8f && p.aura <= previous.aura + 1e-8f, "Halo has repeated peaks/dots away from the light");
                previous = p;
            }
        }
        for (int d = 0; d < 60; ++d) {
            auto a = blur.sample(64 - d, 64), b = blur.sample(65 + d, 65);
            require(std::abs(a.local - b.local) < 1e-8f && std::abs(a.aura - b.aura) < 1e-8f, "Halo is asymmetric");
        }
    }
    std::puts("CPU: isolated lights preserve energy, symmetry, and a continuous halo at all tested radii.");
}

static void testFlatEdges()
{
    for (auto dimensions : std::array<std::array<int, 2>, 4>{{{1, 1}, {3, 5}, {128, 130}, {129, 131}}}) {
        const int width = dimensions[0], height = dimensions[1];
        std::vector<float> input(width * height * 4, 1.0f);
        for (auto s : {settings(2.0f), settings(2.0f, 1.0f, 0.0f), settings(2.0f, 0.0f, 1.0f)}) {
            auto blur = cpuBlur(input, width, height, s);
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    auto p = blur.sample(x, y);
                    require(std::abs(p.local - (s[13] > 0.0f ? 1.0f : 0.0f)) < 2e-6f, "Flat halation/edge normalization failed");
                    require(std::abs(p.aura - (s[15] > 0.0f ? 1.0f : 0.0f)) < 2e-6f, "Flat aura/edge normalization failed");
                }
            }
        }
    }
    std::puts("CPU: odd dimensions, tiny images, edges, and independent Halation/Aura controls pass.");
}

struct GrainStatistics {
    double mean, rms, correlation;
};

static GrainStatistics grainStatistics(GrainParameters p)
{
    double sum = 0.0, energy = 0.0, neighbors = 0.0;
    const int width = 512, height = 256;
    for (int y = 0; y < height; ++y) {
        float previous = grain_delta(0, y, 0.45f, p).r;
        for (int x = 1; x < width; ++x) {
            const float n = grain_delta(x, y, 0.45f, p).r;
            sum += n;
            energy += n * n;
            neighbors += n * previous;
            previous = n;
        }
    }
    const double count = (width - 1) * height;
    return {sum / count, std::sqrt(energy / count), neighbors / energy};
}

static void testGrain()
{
    auto s = settings(0.0f, 0.0f, 0.0f, 3);
    s[16] = 1.0f;
    s[17] = 0.45f;
    s[18] = 0.32f;
    s[21] = 0.0f;
    std::array<GrainStatistics, 3> styles;
    for (int style = 0; style < 3; ++style) {
        s[3] = static_cast<float>(style);
        styles[style] = grainStatistics(grain::prepare(s.data(), 1080, 37.0));
        require(std::abs(styles[style].mean) < 0.003, "Grain creates a brightness bias");
        require(styles[style].rms > 0.015 && styles[style].rms < 0.060, "Unexpected grain strength");
        std::printf("Grain style %d: RMS %.4f, neighboring-pixel correlation %.3f.\n", style, styles[style].rms, styles[style].correlation);
    }
    require(styles[0].correlation + 0.10 < styles[1].correlation && styles[1].correlation + 0.10 < styles[2].correlation,
            "Fine, Classic, and Rough do not have distinct spatial structure");
    s[3] = 1.0f;
    s[17] = 0.0f;
    auto small = grainStatistics(grain::prepare(s.data(), 1080, 37.0));
    s[17] = 1.0f;
    auto large = grainStatistics(grain::prepare(s.data(), 1080, 37.0));
    require(large.correlation > small.correlation + 0.25, "Size does not increase granule scale");
    require(large.rms / small.rms > 0.8 && large.rms / small.rms < 1.2, "Size changes amplitude excessively");
    s[20] = 0.0f;
    auto hard = grainStatistics(grain::prepare(s.data(), 1080, 37.0));
    s[20] = 1.0f;
    auto soft = grainStatistics(grain::prepare(s.data(), 1080, 37.0));
    require(soft.correlation > hard.correlation + 0.05, "Softness does not suppress fine detail");
    require(soft.rms / hard.rms > 0.8 && soft.rms / hard.rms < 1.2, "Softness is merely an amplitude control");
    s[17] = 0.45f;
    const auto mono = grain::prepare(s.data(), 1080, -12.25);
    const auto repeated = grain::prepare(s.data(), 1080, -12.25);
    const auto otherFrame = grain::prepare(s.data(), 1080, -11.25);
    s[25] = 123456.0f;
    const auto otherSeed = grain::prepare(s.data(), 1080, -12.25);
    s[21] = 1.0f;
    const auto colored = grain::prepare(s.data(), 1080, -12.25);
    double frameDifference = 0.0, seedDifference = 0.0, chroma = 0.0;
    for (int x = 0; x < 1024; ++x) {
        const auto a = grain_delta(x, 57, 0.45f, mono);
        const auto b = grain_delta(x, 57, 0.45f, repeated);
        require(a.r == a.g && a.g == a.b, "Monochrome grain adds a color cast");
        require(a.r == b.r && a.g == b.g && a.b == b.b, "Grain does not repeat exactly");
        frameDifference += std::abs(a.r - grain_delta(x, 57, 0.45f, otherFrame).r);
        seedDifference += std::abs(a.r - grain_delta(x, 57, 0.45f, otherSeed).r);
        const auto c = grain_delta(x, 57, 0.45f, colored);
        chroma += std::abs(c.r - c.g);
    }
    require(frameDifference > 10.0 && seedDifference > 10.0 && chroma > 10.0, "Frame, seed, or color control is ineffective");
    for (int zone = 0; zone < 3; ++zone) {
        s[22] = s[23] = s[24] = 0.0f;
        s[22 + zone] = 1.0f;
        const auto p = grain::prepare(s.data(), 1080, 37.0);
        for (int tone = 0; tone < 3; ++tone) {
            auto value = grain_delta(29, 67, std::array<float, 3>{0.0f, 0.45f, 1.0f}[tone], p);
            if (tone != zone) require(value.r == 0.0f && value.g == 0.0f && value.b == 0.0f, "Tonal grain controls affect an isolated other zone");
        }
    }
    const auto hd = grain::prepare(s.data(), 1080, 37.0);
    const auto uhd = grain::prepare(s.data(), 2160, 37.0);
    require(hd.inverseSize == uhd.inverseSize * 2.0f, "Grain size is not resolution independent");
    require(film::isIdentity(4, film::All, 2.0f, 1.0f, 1.0f), "Bypass is not identity");
    require(film::isIdentity(0, 0, 2.0f, 1.0f, 1.0f), "Disabled modules are not identity");
    require(!film::isIdentity(5, 0, 0.0f, 0.0f, 0.0f), "Empty matte incorrectly returns source");
    require(!film::isIdentity(0, film::Negative, 0.0f, 0.0f, 0.0f), "Active color falsely reports identity");
    std::puts("Grain: style/size/softness structure, stable amplitude, neutral mean, mono/color, tonal controls, seeds, and HD/4K scaling pass.");
}

static void writePreview(const char* path)
{
    const int width = 960, height = 540, rowBytes = width * 3;
    std::array<unsigned char, 54> header {};
    header[0] = 'B'; header[1] = 'M'; header[10] = 54; header[14] = 40; header[26] = 1; header[28] = 24;
    auto putInt = [&](int offset, unsigned value) {
        for (int byte = 0; byte < 4; ++byte) header[offset + byte] = static_cast<unsigned char>(value >> (8 * byte));
    };
    putInt(2, 54 + rowBytes * height); putInt(18, width); putInt(22, height);
    std::ofstream file(path, std::ios::binary);
    require(file.good(), "Cannot write grain preview");
    file.write(reinterpret_cast<const char*>(header.data()), header.size());
    std::vector<unsigned char> row(rowBytes);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            auto s = settings(0.0f, 0.0f, 0.0f, 3);
            s[3] = static_cast<float>(x / 320);
            s[16] = 0.65f;
            s[17] = y < 270 ? 0.20f : 0.75f;
            s[18] = 0.32f;
            const float base = 0.15f + (y % 270) / 269.0f * 0.60f;
            const auto delta = grain_delta(x, y, base, grain::prepare(s.data(), 1080, 37.0));
            const std::array<float, 3> rgb {base + delta.b, base + delta.g, base + delta.r};
            for (int channel = 0; channel < 3; ++channel)
                row[x * 3 + channel] = static_cast<unsigned char>(std::clamp(rgb[channel], 0.0f, 1.0f) * 255.0f + 0.5f);
        }
        file.write(reinterpret_cast<const char*>(row.data()), row.size());
    }
    std::printf("Preview written: %s (columns Fine/Classic/Rough; rows small/large grain).\n", path);
}

static void writeResponsePreview(const char* path)
{
    const int width = 1024, height = 512, rowBytes = width * 3;
    std::array<unsigned char, 54> header {};
    header[0] = 'B'; header[1] = 'M'; header[10] = 54; header[14] = 40; header[26] = 1; header[28] = 24;
    auto putInt = [&](int offset, unsigned value) {
        for (int byte = 0; byte < 4; ++byte) header[offset + byte] = static_cast<unsigned char>(value >> (8 * byte));
    };
    putInt(2, 54 + rowBytes * height); putInt(18, width); putInt(22, height);
    std::array<FilmResponseParameters, 4> columns;
    for (int column = 0; column < 4; ++column) {
        auto s = filmSettings();
        s[2] = static_cast<float>(column);
        columns[column] = response::prepare(s.data());
    }
    std::ofstream file(path, std::ios::binary);
    require(file.good(), "Cannot write response preview");
    file.write(reinterpret_cast<const char*>(header.data()), header.size());
    std::vector<unsigned char> row(rowBytes);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            const float t = (x % 256) / 255.0f;
            ColorRgb c;
            if (y < 128) c = {t,t,t};
            else if (y < 256) c = {0.12f + t * 0.70f, 0.075f + t * 0.54f, 0.045f + t * 0.40f};
            else if (y < 384) {
                const std::array<ColorRgb, 8> hues {{{1,0,0},{1,0.45f,0},{1,1,0},{0,1,0},{0,1,1},{0,0,1},{1,0,1},{0.4f,0.2f,0.1f}}};
                c = hues[(y - 256) / 16];
                const float scale = 0.05f + t * 1.5f;
                c.r *= scale; c.g *= scale; c.b *= scale;
            } else {
                const float linear = 0.18f * std::exp2(t * 16.0f - 6.0f);
                const float v = color_encode(linear, ColorSRGB);
                c = {v,v,v};
            }
            c = response_print(response_negative(c, columns[x / 256]), columns[x / 256]);
            const std::array<float, 3> bgr {c.b,c.g,c.r};
            for (int channel = 0; channel < 3; ++channel)
                row[x * 3 + channel] = static_cast<unsigned char>(std::clamp(bgr[channel], 0.0f, 1.0f) * 255.0f + 0.5f);
            if (x % 256 == 0 || y % 128 == 0) row[x*3] = row[x*3+1] = row[x*3+2] = 32;
        }
        file.write(reinterpret_cast<const char*>(row.data()), row.size());
    }
    std::printf("Response preview: %s (columns Full/Standard/Extended/Custom; bands gray/skin/colors/HDR).\n", path);
}

class Gpu {
public:
    using UInt = unsigned int;
    using Bits = unsigned long long;
    using Handle = void*;
    Handle context = nullptr, queue = nullptr, unordered = nullptr;

    int (__stdcall *getPlatforms)(UInt, Handle*, UInt*) = nullptr;
    int (__stdcall *getDevices)(Handle, Bits, UInt, Handle*, UInt*) = nullptr;
    int (__stdcall *getDeviceInfo)(Handle, UInt, size_t, void*, size_t*) = nullptr;
    Handle (__stdcall *createContext)(const intptr_t*, UInt, const Handle*, void*, void*, int*) = nullptr;
    Handle (__stdcall *createQueue)(Handle, Handle, Bits, int*) = nullptr;
    Handle (__stdcall *createBuffer)(Handle, Bits, size_t, void*, int*) = nullptr;
    int (__stdcall *readBuffer)(Handle, Handle, UInt, size_t, size_t, void*, UInt, const Handle*, Handle*) = nullptr;
    int (__stdcall *finish)(Handle) = nullptr;
    int (__stdcall *releaseMem)(Handle) = nullptr;
    int (__stdcall *releaseQueue)(Handle) = nullptr;
    int (__stdcall *releaseContext)(Handle) = nullptr;

    Gpu()
    {
        HMODULE dll = LoadLibraryA("OpenCL.dll");
        if (!dll) return;
#define LOAD(member, symbol) member = reinterpret_cast<decltype(member)>(GetProcAddress(dll, symbol)); require(member != nullptr, symbol)
        LOAD(getPlatforms, "clGetPlatformIDs");
        LOAD(getDevices, "clGetDeviceIDs");
        LOAD(getDeviceInfo, "clGetDeviceInfo");
        LOAD(createContext, "clCreateContext");
        LOAD(createQueue, "clCreateCommandQueue");
        LOAD(createBuffer, "clCreateBuffer");
        LOAD(readBuffer, "clEnqueueReadBuffer");
        LOAD(finish, "clFinish");
        LOAD(releaseMem, "clReleaseMemObject");
        LOAD(releaseQueue, "clReleaseCommandQueue");
        LOAD(releaseContext, "clReleaseContext");
#undef LOAD
        UInt count = 0;
        if (getPlatforms(0, nullptr, &count) != 0 || count == 0) return;
        std::vector<Handle> platforms(count);
        require(getPlatforms(count, platforms.data(), nullptr) == 0, "Cannot list OpenCL platforms");
        Handle device = nullptr;
        for (Handle platform : platforms) {
            if (getDevices(platform, 4, 1, &device, nullptr) == 0) break;
        }
        if (!device) return;
        int error = 0;
        context = createContext(nullptr, 1, &device, nullptr, nullptr, &error);
        require(error == 0 && context, "Cannot create GPU context");
        queue = createQueue(context, device, 0, &error);
        require(error == 0 && queue, "Cannot create GPU queue");
        unordered = createQueue(context, device, 1, &error);
        if (error != 0) unordered = nullptr;
        char name[256] {};
        getDeviceInfo(device, 0x102B, sizeof(name), name, nullptr);
        std::printf("OpenCL device: %s\n", name);
    }

    ~Gpu()
    {
        if (unordered) { finish(unordered); releaseQueue(unordered); }
        if (queue) { finish(queue); releaseQueue(queue); }
        if (context) releaseContext(context);
    }

    void test(int width, int height, const grain::PackedSettings& s, bool outOfOrder, double time = 0.0)
    {
        const size_t count = static_cast<size_t>(width) * height * 4;
        std::vector<float> input(count), output(count);
        for (size_t i = 0; i < count; i += 4) {
            input[i] = static_cast<float>((i * 13) % 101) / 40.0f - 0.2f;
            input[i + 1] = static_cast<float>((i * 17) % 109) / 60.0f - 0.3f;
            input[i + 2] = static_cast<float>((i * 31) % 113) / 90.0f - 0.4f;
            input[i + 3] = static_cast<float>(i % 17) / 16.0f;
            {
                const auto encoded = color_from_work({input[i], input[i + 1], input[i + 2]}, color::prepare(static_cast<int>(s[26]), 0, true));
                input[i] = encoded.r; input[i + 1] = encoded.g; input[i + 2] = encoded.b;
            }
        }
        int error = 0;
        Handle src = createBuffer(context, 4 | 32, count * sizeof(float), input.data(), &error);
        require(error == 0, "Cannot create test input");
        Handle dst = createBuffer(context, 1, count * sizeof(float), nullptr, &error);
        require(error == 0, "Cannot create test output");
        Handle q = outOfOrder ? unordered : queue;
        // Queue several renders before reading to exercise scratch reuse and mode switching.
        for (int render = 0; render < 3; ++render) {
            auto current = render == 2 ? s : settings(2.0f, 1.0f, 1.0f, render == 0 ? 2 : 4);
            require(RunOpenEmulsionOpenCL(q, width, height, time, current.data(),
                                          reinterpret_cast<const float*>(src), reinterpret_cast<float*>(dst)), "GPU render failed");
        }
        require(finish(q) == 0, "GPU execution failed");
        require(readBuffer(q, dst, 1, 0, count * sizeof(float), output.data(), 0, nullptr, nullptr) == 0, "Cannot read GPU output");
        const int modules = film::modulesForMode(static_cast<int>(s[0]), static_cast<int>(s[19]));
        auto effective = s;
        if (!(modules & film::Halation)) effective[13] = 0.0f;
        if (!(modules & film::Aura)) effective[15] = 0.0f;
        const auto blur = cpuBlur(input, width, height, effective);
        const auto grainParameters = grain::prepare(s.data(), height, time);
        const auto colorParameters = color::prepare(s.data());
        const auto responseParameters = response::prepare(s.data());
        const auto halo = halation::prepare(s.data(), height);
        const bool identity = film::isIdentity(static_cast<int>(s[0]), modules, effective[13], effective[15], s[16]);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const size_t i = (static_cast<size_t>(y) * width + x) * 4;
                float h = 0.0f;
                const ColorRgb original {input[i], input[i + 1], input[i + 2]};
                const auto work = color_to_work(original, colorParameters);
                const bool spatial = effective[13] > 0.0f || effective[15] > 0.0f;
                if (spatial)
                    h = halation::signal(blur.sample(x, y), halation_key(work.r, work.g, work.b, halo.key), effective[13], effective[15]);
                ColorRgb processed = work;
                if (modules & film::Negative) processed = response_negative(color_balance(processed, {s[4],s[5],s[6]}), responseParameters);
                if (modules & film::Print) processed = response_print(processed, responseParameters);
                std::array<float, 3> rgb {processed.r + h * halo.key.red, processed.g + h * halo.key.green, processed.b + h * halo.key.blue};
                if (s[0] == 5.0f) {
                    const float matte = std::clamp(h * 2.0f, 0.0f, 1.0f);
                    rgb = {matte, matte * 0.55f, matte * 0.10f};
                }
                if ((modules & film::Grain) && s[16] > 0.0f) {
                    const float luma = rgb[0] * 0.2126f + rgb[1] * 0.7152f + rgb[2] * 0.0722f;
                    const auto delta = grain_delta(x, y, luma, grainParameters);
                    rgb[0] += delta.r;
                    rgb[1] += delta.g;
                    rgb[2] += delta.b;
                }
                if (s[0] != 5.0f) {
                    const auto finished = response_finish({rgb[0],rgb[1],rgb[2]}, modules, responseParameters);
                    rgb = {finished.r,finished.g,finished.b};
                    if (response_clamps_negative(modules, responseParameters)) for (auto& v : rgb) v = std::max(v, 0.0f);
                    const bool unchanged = identity || (!(modules & 3) && rgb[0] == work.r && rgb[1] == work.g && rgb[2] == work.b);
                    const auto encoded = unchanged ? original : color_from_work({rgb[0], rgb[1], rgb[2]}, colorParameters);
                    rgb = {encoded.r, encoded.g, encoded.b};
                }
                for (int channel = 0; channel < 4; ++channel) {
                    const float expected = channel < 3 ? rgb[channel] : input[i + channel];
                    const float tolerance = 3e-5f * std::max(1.0f, std::abs(expected));
                    if (!std::isfinite(output[i + channel]) || std::abs(output[i + channel] - expected) >= tolerance) {
                        std::fprintf(stderr, "Mismatch at (%d,%d), channel %d, source %.0f, mode %.0f, mask %.0f, style %.0f, time %.2f, strengths %.2f/%.2f/%.2f/%.2f: GPU %.8f, CPU %.8f\n",
                                     x, y, channel, s[26], s[0], s[19], s[3], time, s[41], s[42], s[43], s[44], output[i + channel], expected);
                        require(false, "CPU/GPU mismatch or broken mode/alpha");
                    }
                }
            }
        }
        releaseMem(src);
        releaseMem(dst);
    }

    void writeTexturePreview(const char* path, bool monochrome = false)
    {
        const int panelWidth = 256, height = 640, width = panelWidth * 5, rowBytes = width * 3;
        std::vector<float> input(panelWidth * height * 4, 1.0f);
        for (int y = 0; y < height; ++y) for (int x = 0; x < panelWidth; ++x) {
            const bool light = (x-128)*(x-128) + (y-150)*(y-150) < 64 ||
                               (x >= 120 && x < 136 && y >= 315 && y < 365);
            const float value = y < 440 ? (light ? 1.4f : 0.025f) : (y < 540 ? 0.30f : 0.60f);
            const size_t i = (static_cast<size_t>(y) * panelWidth + x) * 4;
            input[i] = input[i+1] = input[i+2] = value;
        }
        const auto grainInput = input;
        for (int y = 440; y < height; ++y) for (int x = 0; x < panelWidth; ++x) {
            const size_t i = (static_cast<size_t>(y) * panelWidth + x) * 4;
            input[i] = input[i+1] = input[i+2] = 0.025f;
        }
        std::array<std::vector<float>, 5> columns;
        for (int format = 0; format < 5; ++format) {
            auto s = settings(1.2f,1.2f,0.6f,2);
            s[film::FilmGauge] = static_cast<float>(format);
            if (monochrome) { s[0] = 0; s[1] = 4; }
            s[film::AuraRadius] = 1.3f;
            columns[format] = render(input,panelWidth,height,s);
            s[0] = monochrome ? 0 : 3; s[16] = 0.70f; s[17] = 0.65f; s[3] = 1;
            if (monochrome) s[19] = film::Negative | film::Print | film::Grain;
            s[21] = monochrome ? 1 : 0; s[22] = s[24] = 0; s[23] = 1;
            const auto grain = render(grainInput,panelWidth,height,s);
            const size_t begin = static_cast<size_t>(440) * panelWidth * 4;
            std::copy(grain.begin()+begin,grain.end(),columns[format].begin()+begin);
        }
        std::array<unsigned char, 54> header {};
        header[0] = 'B'; header[1] = 'M'; header[10] = 54; header[14] = 40; header[26] = 1; header[28] = 24;
        auto putInt = [&](int offset, unsigned value) {
            for (int byte = 0; byte < 4; ++byte) header[offset + byte] = static_cast<unsigned char>(value >> (8 * byte));
        };
        putInt(2,54+rowBytes*height); putInt(18,width); putInt(22,height);
        std::ofstream file(path,std::ios::binary);
        require(file.good(),"Cannot write texture preview");
        file.write(reinterpret_cast<const char*>(header.data()),header.size());
        std::vector<unsigned char> row(rowBytes);
        for (int y = height-1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                const auto& column = columns[x/panelWidth];
                const size_t i = (static_cast<size_t>(y)*panelWidth + x%panelWidth)*4;
                for (int channel = 0; channel < 3; ++channel)
                    row[x*3+channel] = static_cast<unsigned char>(std::clamp(column[i+2-channel],0.0f,1.0f)*255.0f+0.5f);
                if (x%panelWidth == 0 || y == 440 || y == 540) row[x*3] = row[x*3+1] = row[x*3+2] = 32;
            }
            file.write(reinterpret_cast<const char*>(row.data()),row.size());
        }
        require(file.good(),"Texture preview write failed");
        std::printf("Texture preview: %s (columns Custom/8mm/16mm/35mm/65mm; bands lights/grain).\n",path);
    }

    void benchmark()
    {
        const int width = 3840, height = 2160;
        const size_t bytes = static_cast<size_t>(width) * height * 4 * sizeof(float);
        std::vector<float> input(bytes / sizeof(float), 0.7f);
        int error = 0;
        Handle src = createBuffer(context, 4 | 32, bytes, input.data(), &error);
        require(error == 0, "Cannot allocate benchmark input");
        Handle dst = createBuffer(context, 1, bytes, nullptr, &error);
        require(error == 0, "Cannot allocate benchmark output");
        std::vector<grain::PackedSettings> cases;
        for (int source : {color::Rec709Gamma24, color::AlexaLogC3, color::DaVinciIntermediate, color::ACEScct}) {
            for (int mode : {1, 3, 2, 0}) {
                auto s = filmSettings();
                s[0] = static_cast<float>(mode); s[26] = static_cast<float>(source);
                s[27] = mode == 0 || mode == 1 ? 1.0f : 0.0f;
                s[13] = mode == 3 ? 0.0f : 1.0f; s[14] = 2.0f; s[15] = s[13];
                s[film::AuraRadius] = 2.0f;
                cases.push_back(s);
            }
        }
        for (int format : {1,2,4}) {
            auto s = filmSettings();
            s[0] = 0; s[26] = color::AlexaLogC3; s[27] = 1;
            s[13] = s[15] = 1; s[14] = s[film::AuraRadius] = 2;
            s[film::FilmGauge] = static_cast<float>(format);
            cases.push_back(s);
        }
        for (int mode : {0,1}) {
            auto s = filmSettings();
            s[0] = static_cast<float>(mode); s[1] = 4;
            s[26] = color::AlexaLogC3; s[27] = 1;
            s[13] = s[15] = 1; s[14] = s[film::AuraRadius] = 2;
            cases.push_back(s);
        }
        for (auto s : cases) {
            s[16] = 0.16f;
            s[17] = 0.45f;
            s[18] = 0.32f;
            require(RunOpenEmulsionOpenCL(queue, width, height, 0.0f, s.data(), reinterpret_cast<const float*>(src), reinterpret_cast<float*>(dst)), "Warmup failed");
            require(finish(queue) == 0, "Warmup execution failed");
            const auto start = std::chrono::steady_clock::now();
            for (int frame = 0; frame < 12; ++frame)
                require(RunOpenEmulsionOpenCL(queue, width, height, static_cast<float>(frame), s.data(), reinterpret_cast<const float*>(src), reinterpret_cast<float*>(dst)), "Benchmark render failed");
            require(finish(queue) == 0, "Benchmark execution failed");
            const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / 12.0;
            std::printf("4K %s, mode %.0f, film %.0f, gauge %s, radius %.1f, aura %.1f/radius %.1f: %.2f ms/frame (GPU resident, excludes Resolve/transfers).\n", color::spaces()[static_cast<int>(s[26])].label, s[0], s[1], gauge::prepare(s.data()).label, s[14], s[15], s[film::AuraRadius], ms);
        }
        releaseMem(src);
        releaseMem(dst);
    }

    std::vector<float> render(const std::vector<float>& input, int width, int height, const grain::PackedSettings& s)
    {
        const size_t bytes = input.size() * sizeof(float);
        int error = 0;
        Handle src = createBuffer(context, 4 | 32, bytes, const_cast<float*>(input.data()), &error);
        require(error == 0, "Cannot create color test input");
        Handle dst = createBuffer(context, 1, bytes, nullptr, &error);
        require(error == 0, "Cannot create color test output");
        require(RunOpenEmulsionOpenCL(queue, width, height, 0.0, s.data(), reinterpret_cast<const float*>(src), reinterpret_cast<float*>(dst)), "Color module render failed");
        std::vector<float> output(input.size());
        require(readBuffer(queue, dst, 1, 0, bytes, output.data(), 0, nullptr, nullptr) == 0, "Cannot read color result");
        releaseMem(src); releaseMem(dst);
        return output;
    }

    void testColorModules()
    {
        const int width = 128, height = 64;
        std::vector<float> input(width * height * 4);
        for (size_t i = 0; i < input.size(); i += 4) {
            input[i] = static_cast<float>(i % 131) / 70.0f - 0.2f;
            input[i + 1] = static_cast<float>(i % 97) / 60.0f - 0.1f;
            input[i + 2] = static_cast<float>(i % 173) / 80.0f - 0.3f;
            input[i + 3] = 0.5f;
        }
        auto s = filmSettings();
        s[13] = 2.0f; s[14] = 2.0f; s[15] = 1.0f;
        s[1] = 3.0f; s[2] = 1.0f;
        s[4] = 1.08f; s[5] = 0.98f; s[6] = 0.95f;
        s[7] = 0.18f; s[8] = 0.95f; s[9] = 0.16f; s[10] = 1.08f;
        s[16] = 1.0f;
        const auto combined = render(input, width, height, s);
        s[19] = film::Negative;
        const auto negative = render(input, width, height, s);
        s[19] = film::Print;
        const auto composed = render(negative, width, height, s);
        const auto print = render(input, width, height, s);
        s[4] = s[5] = s[6] = 3.0f;
        s[7] = 0.9f; s[8] = 1.5f; s[9] = 1.0f;
        const auto printWithIgnoredControls = render(input, width, height, s);
        s[19] = 0;
        const auto bypass = render(input, width, height, s);
        for (size_t i = 0; i < input.size(); ++i) {
            require(std::abs(combined[i] - composed[i]) < 3e-5f, "Negative/Print stages do not compose independently");
            require(print[i] == printWithIgnoredControls[i], "Disabled Film Color still influences Print");
            require(input[i] == bypass[i], "All-disabled color mode alters HDR/negative values or alpha");
        }
        std::puts("OpenCL: independent Film Color/Print compose correctly; disabled color controls have no effect; exact float pass-through passes.");
    }

    void testManagedColor()
    {
        const int width = 33, height = 31;
        std::vector<float> work(width * height * 4);
        for (size_t i = 0; i < work.size(); i += 4) {
            work[i] = static_cast<float>(i % 53) / 50.0f;
            work[i + 1] = static_cast<float>(i % 47) / 45.0f;
            work[i + 2] = static_cast<float>(i % 31) / 29.0f;
            work[i + 3] = static_cast<float>(i % 19) / 18.0f;
        }
        for (int source = 0; source < static_cast<int>(color::spaces().size()); ++source) {
            const auto sourceParameters = color::prepare(source, 0, true);
            auto input = work;
            for (size_t i = 0; i < input.size(); i += 4) {
                const auto rgb = color_from_work({work[i], work[i+1], work[i+2]}, sourceParameters);
                input[i] = rgb.r; input[i+1] = rgb.g; input[i+2] = rgb.b;
            }
            for (int mask : {film::Negative, film::Print, film::All}) {
              for (int balance : {0, 1}) {
                auto s = filmSettings();
                s[0] = 0.0f; s[13] = 0.7f; s[14] = 1.0f; s[15] = 0.3f;
                s[19] = static_cast<float>(mask);
                s[16] = 0.2f; s[3] = 1.0f; s[17] = 0.5f;
                s[9] = 0.16f; s[10] = 1.08f; s[7] = 0.18f; s[8] = 0.95f;
                if (balance) { s[4] = 2.0f; s[5] = 1.9f; s[6] = 2.1f; }
                // Compare managed rendering with the common film response in its work domain.
                auto normalized = input;
                for (size_t i = 0; i < input.size(); i += 4) {
                    auto rgb = color_to_work({input[i], input[i+1], input[i+2]}, sourceParameters);
                    if (mask & film::Negative) rgb = color_balance(rgb, {s[4], s[5], s[6]});
                    normalized[i] = rgb.r; normalized[i+1] = rgb.g; normalized[i+2] = rgb.b;
                }
                auto referenceSettings = s;
                referenceSettings[4] = referenceSettings[5] = referenceSettings[6] = 1.0f;
                if (balance && (mask & film::Halation)) {
                    // Halation is keyed from unbalanced input, so isolate color for this exposure check.
                    s[13] = s[15] = referenceSettings[13] = referenceSettings[15] = 0.0f;
                }
                const auto reference = render(normalized, width, height, referenceSettings);
                s[26] = static_cast<float>(source);
                for (int output = 0; output < static_cast<int>(color::OutputSpaces.size()); ++output) {
                    s[27] = static_cast<float>(output);
                    const auto p = color::prepare(s.data());
                    const auto result = render(input, width, height, s);
                    for (size_t i = 0; i < result.size(); i += 4) {
                        const auto expected = color_from_work({reference[i], reference[i+1], reference[i+2]}, p);
                        requireNear(result[i], expected.r, 4e-5f, "Managed film R");
                        requireNear(result[i+1], expected.g, 4e-5f, "Managed film G");
                        requireNear(result[i+2], expected.b, 4e-5f, "Managed film B");
                        require(result[i+3] == input[i+3], "Managed alpha changed");
                    }
                }
              }
            }
            for (int mode : {4, 0, 2, 3}) {
                auto s = settings(2.0f, 1.0f, 1.0f, mode);
                s[26] = static_cast<float>(source); s[27] = 1.0f;
                if (mode != 4) s[19] = 0.0f;
                // Deliberately invalid-looking encoded values still must survive bypass bit-for-bit.
                const auto result = render(work, width, height, s);
                require(result == work, "Managed bypass/all-disabled changes pixels");
            }
            auto s = settings(0.0f, 0.0f, 0.0f, 3);
            s[26] = static_cast<float>(source); s[16] = 0.3f;
            const auto reference = render(input, width, height, s);
            s[27] = 1.0f;
            require(reference == render(input, width, height, s), "Texture-only output override changed image");
            s[16] = 0.0f;
            require(input == render(input, width, height, s), "Zero-grain managed mode changes image");
        }
        std::puts("OpenCL: all input/output spaces, independent film stages, managed halation/grain, exact bypass/zero-grain, alpha, and texture-only output preservation pass.");
    }

    void testPrintPresetTransitions()
    {
        const int width = 33, height = 31;
        std::vector<float> work(width*height*4,0.5f);
        for (size_t i = 0; i < work.size(); i += 4) {
            work[i] = static_cast<float>(i%131)/100.0f-0.15f;
            work[i+1] = static_cast<float>(i%79)/75.0f-0.05f;
            work[i+2] = static_cast<float>(i%107)/60.0f;
            work[i+3] = static_cast<float>(i%17)/16.0f;
        }
        for (int source = 0; source < color::SpaceCount; ++source) {
            auto input = work;
            for (size_t i = 0; i < input.size(); i += 4) {
                const auto encoded = color_from_work({work[i],work[i+1],work[i+2]},color::prepare(source,0,true));
                input[i] = encoded.r; input[i+1] = encoded.g; input[i+2] = encoded.b;
            }
            for (int style = printstyle::Full; style < printstyle::Custom; ++style) {
                for (int mode : {0,1,2,3,4,5}) {
                    auto s = filmSettings();
                    s[0] = static_cast<float>(mode); s[2] = static_cast<float>(style);
                    s[26] = static_cast<float>(source); s[27] = 1;
                    s[13] = 0.7f; s[15] = 0.3f; s[16] = 0.3f;
                    s[37] = 0.2f; s[38] = -0.1f;
                    s[film::PrintColorStrength] = 0.71f; s[film::PrintToneStrength] = 0.83f;
                    const auto fixed = render(input,width,height,s);
                    auto dirty = s;
                    for (int control = 0; control < printstyle::ControlCount; ++control)
                        dirty[printstyle::Parameters[control].setting] = control % 2 ? 1.8f : -1;
                    require(fixed == render(input,width,height,dirty), "GPU named preset accepts stored recipe changes");
                    printstyle::applyPreset(style, [&](int control, double value) {
                        s[printstyle::Parameters[control].setting] = static_cast<float>(value);
                    });
                    s[2] = printstyle::Custom;
                    require(fixed == render(input,width,height,s), "GPU preset-to-Custom transition changes pixels");
                    if (mode == 4) require(fixed == input, "Print presets break exact bypass");
                }
            }
        }
        // Exercise actual Custom tuning across every space, not just inherited endpoints.
        for (int source = 0; source < color::SpaceCount; ++source) {
            auto s = filmSettings();
            s[2] = printstyle::Custom; s[26] = static_cast<float>(source); s[27] = 5;
            s[32] = -0.37f; s[33] = 1.26f; s[34] = 0.83f;
            s[11] = 0.61f; s[35] = 0.43f; s[36] = 1.35f; s[12] = 0.72f;
            test(width,height,s,false,37);
        }
        std::puts("OpenCL: exact preset-to-Custom transitions in all spaces/modes, locked recipe immunity, Custom tuning parity, and bypass pass.");
    }

    void testMonochromeOutput()
    {
        const int width = 33, height = 31;
        std::vector<float> work(width*height*4);
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
            const size_t i = (static_cast<size_t>(y)*width+x)*4;
            work[i] = 0.05f + x / 100.0f;
            work[i+1] = 0.10f + y / 80.0f;
            work[i+2] = 0.15f + (x+y) / 120.0f;
            work[i+3] = static_cast<float>((x+y)%17)/16;
            if (x >= 14 && x <= 18 && y >= 13 && y <= 17) {
                work[i] = 3; work[i+1] = 2; work[i+2] = 1;
            }
        }
        auto bright = filmSettings();
        bright[0] = 0; bright[1] = 4;
        bright[13] = 2; bright[14] = 2; bright[15] = 1; bright[film::AuraRadius] = 2;
        bright[16] = 0.7f; bright[21] = 1;
        bright[37] = 0.25f; bright[38] = 0.6f; bright[39] = -0.3f; bright[40] = 0.4f;
        for (int source = 0; source < color::SpaceCount; ++source) {
            auto input = work;
            for (size_t i = 0; i < input.size(); i += 4) {
                const auto encoded = color_from_work({work[i],work[i+1],work[i+2]},color::prepare(source,0,true));
                input[i] = encoded.r; input[i+1] = encoded.g; input[i+2] = encoded.b;
            }
            for (int output = 0; output < static_cast<int>(color::OutputSpaces.size()); ++output) {
                for (int style = 0; style < 4; ++style) {
                    auto s = bright;
                    s[26] = static_cast<float>(source); s[27] = static_cast<float>(output);
                    s[2] = static_cast<float>(style); s[3] = static_cast<float>(style);
                    s[film::FilmGauge] = static_cast<float>(style+1);
                    s[film::HalationColor] = style % 2 ? 1 : 0;
                    for (int mode : {0,1}) {
                        s[0] = static_cast<float>(mode);
                        const auto result = render(input,width,height,s);
                        for (size_t i = 0; i < result.size(); i += 4) {
                            const float tolerance = 3e-5f * std::max(1.0f,std::abs(result[i]));
                            requireNear(result[i],result[i+1],tolerance,"GPU Mono output R/G");
                            requireNear(result[i+1],result[i+2],tolerance,"GPU Mono output G/B");
                            require(result[i+3] == input[i+3],"Mono changes alpha");
                        }
                    }
                }
            }
            for (float strength : {0.0f,0.37f,0.99f,1.0f}) {
                auto s = bright; s[26] = static_cast<float>(source); s[27] = 5;
                s[film::NegativeColorStrength] = strength;
                test(width,height,s,false,37);
            }
        }
        for (int mode : {0,1,2,3,4,5}) {
            auto s = bright; s[0] = static_cast<float>(mode);
            if (mode < 2) s[19] = film::All & ~film::Negative;
            const auto reference = render(work,width,height,s);
            s[1] = 0;
            require(reference == render(work,width,height,s),"Mono selection leaks into disabled-negative/texture/matte/bypass modes");
        }
        // Verify that neutrality retains the effects, not just their absence.
        auto s = bright;
        s[19] = film::Negative | film::Halation | film::Aura;
        s[13] = s[15] = 0;
        const auto withoutHalo = render(work,width,height,s);
        s[13] = 2; s[15] = 1;
        const auto withHalo = render(work,width,height,s);
        double haloDifference = 0;
        for (size_t i = 0; i < withHalo.size(); i += 4) haloDifference += std::abs(withHalo[i]-withoutHalo[i]);
        require(haloDifference > 0.01,"Mono disables the halo instead of neutralizing it");
        s[19] = film::Negative | film::Grain; s[21] = 1;
        const auto monoGrain = render(work,width,height,s);
        s[21] = 0;
        require(monoGrain == render(work,width,height,s),"Grain Color changes active full Mono");
        s[16] = 0;
        const auto withoutGrain = render(work,width,height,s);
        double grainDifference = 0;
        for (size_t i = 0; i < monoGrain.size(); i += 4) grainDifference += std::abs(monoGrain[i]-withoutGrain[i]);
        require(grainDifference > 0.01,"Mono suppresses grain");
        std::puts("OpenCL: neutral Mono with print/halation/aura/grain across all input/output spaces, partial strengths, alpha, active effects, and exact mode/module isolation pass.");
    }

    void testUpgradeControls()
    {
        for (int source = 0; source < color::SpaceCount; ++source) {
            for (float color : {0.0f,0.37f,1.0f}) for (float tone : {0.0f,0.63f,1.0f}) {
                auto s = filmSettings();
                // Linear output isolates response parity from Gamma 2.4's singular
                // slope at zero when arbitrary wide-gamut inputs cancel to black.
                s[26] = static_cast<float>(source); s[27] = 5;
                s[film::NegativeColorStrength] = s[film::PrintColorStrength] = color;
                s[film::NegativeToneStrength] = s[film::PrintToneStrength] = tone;
                test(33,31,s,false,37);
                s[0] = 0; s[13] = 1; s[15] = 0.5f; s[16] = 0.3f;
                s[film::HalationThreshold] = 0.2f; s[film::HalationSoftness] = 0.7f;
                test(33,31,s,false,37);
            }
        }
        for (float color : {0.0f,0.37f,1.0f}) for (float tone : {0.0f,0.63f,1.0f}) {
            auto s = filmSettings();
            s[26] = color::SRGB; s[27] = 1;
            s[film::NegativeColorStrength] = s[film::PrintColorStrength] = color;
            s[film::NegativeToneStrength] = s[film::PrintToneStrength] = tone;
            test(33,31,s,false,37);
        }
        for (int flags = 0; flags < 16; ++flags) {
            auto s = filmSettings();
            s[26] = color::AlexaLogC3; s[27] = 5;
            for (int control = 0; control < 4; ++control)
                s[film::NegativeColorStrength + control] = flags & (1 << control) ? 0.67f : 0.0f;
            test(33,31,s,false,37);
        }
        for (bool outOfOrder : {false,true}) {
            if (outOfOrder && !unordered) continue;
            for (int format = 0; format < 5; ++format) {
                for (int mode : {0,2,3,4,5}) {
                    auto s = filmSettings();
                    s[0] = static_cast<float>(mode); s[film::FilmGauge] = static_cast<float>(format);
                    s[13] = 1; s[14] = 2; s[15] = 0.7f; s[16] = 0.3f;
                    s[film::AuraRadius] = 2; s[film::HalationColor] = format / 4.0f;
                    test(129,131,s,outOfOrder,37);
                }
            }
            for (auto dimensions : std::array<std::array<int,2>,4>{{{33,1081},{35,1621},{65,2161},{17,4321}}}) {
                auto s = settings(2);
                s[film::FilmGauge] = 1; s[film::AuraRadius] = 2; s[film::HalationColor] = 1;
                s[16] = 0.3f;
                test(dimensions[0],dimensions[1],s,outOfOrder,37);
            }
        }
        const int width = 17, height = 19;
        std::vector<float> input(width*height*4,0.4f);
        for (int mode : {2,3,4,5}) {
            auto s = settings(1,1,0.5f,mode);
            s[16] = 0.3f;
            const auto reference = render(input,width,height,s);
            s[film::NegativeColorStrength] = s[film::NegativeToneStrength] = 0;
            s[film::PrintColorStrength] = s[film::PrintToneStrength] = 0;
            require(reference == render(input,width,height,s), "Strengths leak into texture modes");
        }
        auto s = settings(1,0,0,3);
        s[16] = 0.3f;
        const auto reference = render(input,width,height,s);
        s[film::HalationThreshold] = 2; s[film::HalationSoftness] = 0.01f;
        s[film::HalationColor] = 1; s[film::AuraRadius] = 2;
        require(reference == render(input,width,height,s), "Halation controls affect grain-only mode");
        s[0] = 0; s[19] = 0; s[film::FilmGauge] = 1;
        require(input == render(input,width,height,s), "Gauge breaks exact all-disabled identity");
        std::puts("OpenCL: partial/zero color-tone parity in all spaces, all gauges/modes, enlarged/odd grids, and control isolation pass.");
    }

    void testFilmResponse()
    {
        for (int source = 0; source < static_cast<int>(color::spaces().size()); ++source) {
            for (int system = 0; system < 6; ++system) {
                for (int style = 0; style < 4; ++style) {
                    auto s = filmSettings();
                    s[26] = static_cast<float>(source); s[1] = static_cast<float>(system); s[2] = static_cast<float>(style);
                    s[35] = style % 2 ? 1.0f : 0.0f;
                    test(33, 31, s, false);
                    s[0] = 0.0f; s[13] = 1.0f; s[14] = 1.5f; s[15] = 0.5f;
                    s[16] = 0.3f; s[3] = 1.0f; s[17] = 0.45f; s[31] = 0.6f;
                    s[32] = -0.5f; s[33] = 1.2f; s[34] = 0.75f; s[36] = 1.15f;
                    s[37] = -0.25f; s[38] = 0.1f; s[39] = -0.05f; s[40] = 0.05f;
                    test(33, 31, s, false, 37.0);
                }
            }
        }
        auto s = filmSettings(); s[26] = color::AlexaLogC3; s[27] = 1.0f;
        for (int mask = 0; mask <= film::All; ++mask) {
            s[19] = static_cast<float>(mask); s[0] = 0.0f;
            s[13] = 1.0f; s[15] = 0.5f; s[16] = 0.4f;
            test(33, 31, s, false, 37.0);
        }
        const int width = 17, height = 19;
        std::vector<float> input(width * height * 4, 0.4f);
        for (int mode : {2, 3, 4, 5}) {
            auto baseline = settings(1.0f, 1.0f, 0.5f, mode);
            baseline[26] = color::AlexaLogC3; baseline[16] = 0.3f;
            auto updated = baseline;
            updated[28] = updated[29] = updated[30] = updated[31] = 1.0f;
            updated[32] = 1.0f; updated[33] = 2.0f; updated[37] = updated[38] = updated[39] = updated[40] = 2.0f;
            require(render(input, width, height, baseline) == render(input, width, height, updated), "Film response controls leak into texture/matte/bypass");
        }
        s = filmSettings(); s[19] = 0.0f; s[0] = 0.0f;
        require(render(input, width, height, s) == input, "New response breaks all-disabled identity");
        for (size_t i = 0; i < input.size(); i += 4) {
            input[i] = static_cast<float>(i % 131) / 100.0f - 0.15f;
            input[i+1] = static_cast<float>(i % 79) / 75.0f - 0.05f;
            input[i+2] = static_cast<float>(i % 107) / 60.0f;
        }
        s = filmSettings();
        const auto combined = render(input, width, height, s);
        s[19] = film::Negative;
        const auto negative = render(input, width, height, s);
        s[19] = film::Print;
        const auto composed = render(negative, width, height, s);
        const auto print = render(input, width, height, s);
        s[28] = s[29] = s[30] = s[31] = 1.0f;
        s[7] = 0.9f; s[8] = 2.0f; s[9] = 1.0f; s[10] = 2.0f;
        require(print == render(input, width, height, s), "Disabled upgraded negative affects print");
        for (size_t i = 0; i < input.size(); ++i)
            requireNear(combined[i], composed[i], 3e-5f, "Upgraded negative/print do not compose independently");
        s = filmSettings(); s[19] = film::Negative;
        const auto isolatedNegative = render(input, width, height, s);
        s[32] = 1.0f; s[33] = 2.0f; s[34] = s[35] = 1.0f;
        s[36] = 2.0f; s[37] = s[38] = s[39] = s[40] = 2.0f;
        require(isolatedNegative == render(input, width, height, s), "Disabled upgraded print affects negative");
        std::puts("OpenCL: Film Response CPU parity across all spaces, six negatives, four prints, all module masks, HDR/negative input, balance, and texture isolation pass.");
    }
};

int main(int argc, char** argv)
{
    try {
        testImpulse();
        testFlatEdges();
        testGrain();
        testColorSpaces();
        testResponse();
        testPrintPresets();
        testMonochrome();
        testStrengthControls();
        testHalationControls();
        if (argc > 1) writePreview(argv[1]);
        if (argc > 2) writeResponsePreview(argv[2]);
        Gpu gpu;
        if (!gpu.queue) {
            std::puts("OpenCL tests skipped: no GPU device available.");
            return 0;
        }
        for (bool unordered : {false, true}) {
            if (unordered && !gpu.unordered) continue;
            for (auto dimensions : std::array<std::array<int, 2>, 6>{{{1, 1}, {3, 5}, {128, 130}, {129, 131}, {257, 255}, {17, 19}}}) {
                for (float radius : {0.0f, 0.25f, 1.0f, 2.0f}) {
                    for (auto s : {settings(radius), settings(radius, 1.0f, 0.0f), settings(radius, 0.0f, 1.0f),
                                   settings(radius, 1.0f, 1.0f, 5), settings(radius, 1.0f, 1.0f, 4), settings(radius, 1.0f, 1.0f, 3),
                                   settings(radius, 0.0f, 0.0f)})
                        gpu.test(dimensions[0], dimensions[1], s, unordered);
                }
            }
            std::printf("OpenCL: CPU parity, independent controls, mode switches, alpha, and buffer resizing pass (%s queue).\n", unordered ? "out-of-order" : "in-order");
            for (int style = 0; style < 4; ++style) {
                for (double time : {-12.25, 37.0, 16777217.25}) {
                    auto s = settings(1.0f, 1.0f, 1.0f, 2);
                    s[3] = static_cast<float>(style);
                    s[16] = 0.65f; s[17] = 0.75f; s[18] = 0.60f; s[25] = 123457.0f;
                    gpu.test(257, 129, s, unordered, time);
                    s[20] = 1.0f; s[21] = 0.0f;
                    gpu.test(257, 129, s, unordered, time);
                }
            }
            for (int mask = 0; mask <= film::All; ++mask) {
                auto s = settings(1.0f, 1.0f, 1.0f, 2);
                s[16] = 0.65f;
                s[19] = static_cast<float>(mask);
                gpu.test(129, 131, s, unordered, 37.0);
                s[0] = 5.0f;
                gpu.test(129, 131, s, unordered, 37.0);
                s[0] = 1.0f;
                s[19] = static_cast<float>(mask & ~(film::Negative | film::Print));
                gpu.test(17, 19, s, unordered, 37.0);
                s[0] = 0.0f;
                gpu.test(17, 19, s, unordered, 37.0);
            }
            std::puts("OpenCL: grain CPU/GPU parity, large/negative frame times, all module masks, and empty matte pass.");
        }
        if (!gpu.unordered) std::puts("Out-of-order queue checks skipped: unsupported by this GPU driver.");
        gpu.testColorModules();
        for (int source = 0; source < static_cast<int>(color::spaces().size()); ++source) {
            for (int mode : {2, 3, 5}) {
                auto s = settings(2.0f, 1.0f, 0.5f, mode);
                s[26] = static_cast<float>(source); s[27] = 1.0f;
                s[16] = 0.65f; s[3] = 1.0f; s[17] = 0.45f;
                gpu.test(65, 63, s, false, 37.0);
            }
        }
        gpu.testManagedColor();
        gpu.testFilmResponse();
        gpu.testPrintPresetTransitions();
        gpu.testMonochromeOutput();
        gpu.testUpgradeControls();
        if (argc > 3) gpu.writeTexturePreview(argv[3]);
        if (argc > 4) gpu.writeTexturePreview(argv[4],true);
        gpu.benchmark();
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAILED: %s\n", error.what());
        return 1;
    }
}
