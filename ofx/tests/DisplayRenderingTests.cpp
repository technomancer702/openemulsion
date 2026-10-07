// SPDX-License-Identifier: MPL-2.0

#include <cstdio>
#include <stdexcept>
#include "ColorSpaceConfig.h"
#include "LookPresetConfig.h"
#include "FilmResponseConfig.h"

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

static void near(float a, float b, float tolerance, const char* message)
{
    require(std::isfinite(a) && std::abs(a-b) <= tolerance,message);
}

static float luma(ColorRgb c) { return .2126f*c.r + .7152f*c.g + .0722f*c.b; }

static ColorRgb renderLinear(ColorRgb c)
{
    const auto p=color::prepare(color::LinearRec709,1,false,color::Automatic);
    return color_curve_rgb(color_render_work(color_curve_rgb(c,ColorSRGB,1),p),ColorSRGB,0);
}

int main()
{
    try {
        near(color_sdr_tone(0),0,0,"SDR adds a black offset");
        near(color_sdr_tone(.18f),.12f,1e-7f,"SDR gray anchor");
        near(color_sdr_tone(.6f),.484f,1e-7f,"SDR shoulder anchor");
        near(color_sdr_tone(1),.69135703f,1e-6f,"SDR diffuse white anchor");
        float previous=-1;
        for (int i=0; i<=20000; ++i) {
            const float x=i*.001f, y=color_sdr_tone(x);
            require(std::isfinite(y) && y >= previous && y >= 0 && y < 1,"SDR curve is not bounded/monotonic");
            previous=y;
        }
        for (float join : {.18f,.6f}) {
            const float h=1e-4f;
            const float left=(color_sdr_tone(join)-color_sdr_tone(join-h))/h;
            const float right=(color_sdr_tone(join+h)-color_sdr_tone(join))/h;
            near(left,right,.003f,"SDR curve has a slope discontinuity");
        }
        for (float y : {0.0f,1e-8f,.0001f,.002f,.018f,.09f,.18f,.6f,1.0f,4.0f,64.0f,10000.0f}) {
            const auto c=renderLinear({y,y,y});
            near(c.r,c.g,2e-6f,"SDR tints a neutral");
            near(c.g,c.b,2e-6f,"SDR tints a neutral");
            near(c.r,color_sdr_tone(y),2e-6f,"Neutral SDR disagrees with luminance curve");
            if (y > 0 && y < .18f) require(c.r < y && c.r > 0,"SDR crushes or lifts positive shadows");
        }
        // Test positive and negative wide-gamut channels, skin, and overbright LEDs.
        for (const auto chip : std::array<ColorRgb,9>{{{.38f,.2f,.12f},{.3f,.22f,.012f},
            {1,.01f,.025f},{4,-.05f,.25f},{.02f,4,.05f},{.01f,.03f,4},
            {-.03f,.15f,.08f},{.12f,.11f,-.08f},{1,1,1}}}) {
            for (float exposure : {.01f,.1f,1.0f,4.0f,64.0f}) {
                const ColorRgb source {chip.r*exposure,chip.g*exposure,chip.b*exposure};
                const auto c=renderLinear(source);
                for (float v : {c.r,c.g,c.b}) require(std::isfinite(v) && v>=0 && v<=1.000001f,"SDR gamut boundary");
                near(luma(c),color_sdr_tone(luma(source)),3e-6f,"SDR gamut mapping changes target luminance");
                const float sourceY=luma(source), outputY=luma(c);
                const float cross=(source.r-sourceY)*(c.g-outputY)-(source.g-sourceY)*(c.r-outputY);
                near(cross,0,5e-5f,"SDR gamut mapping rotates linear chroma");
            }
        }
        const auto skin=renderLinear({.38f,.2f,.12f});
        require(skin.r>skin.g && skin.g>skin.b,"SDR reverses warm skin ordering");
        const auto red=renderLinear({1,.01f,.025f}), hotRed=renderLinear({64,.64f,1.6f});
        require((hotRed.r-hotRed.g)/hotRed.r < (red.r-red.g)/red.r,"Overbright red cannot approach display white");
        for (int source=0; source<color::SpaceCount; ++source) for (int output=0; output<6; ++output)
            for (int rendering=0; rendering<3; ++rendering) for (bool texture : {false,true}) {
                const auto p=color::prepare(source,output,texture,rendering);
                const int destination=texture || output==0 ? source : color::OutputSpaces[output];
                const bool expected=!texture && (destination==color::Rec709Gamma24 || destination==color::SRGB) &&
                    (rendering==color::StandardSDR || (rendering==color::Automatic && source!=color::SRGB && source!=color::Rec709Gamma24));
                require(bool(p.renderSDR)==expected,"Automatic/explicit output policy");
                const ColorRgb original {-.02f,.3f,4};
                if (!p.renderSDR) {
                    const auto c=color_render_work(original,p);
                    require(c.r==original.r && c.g==original.g && c.b==original.b,"Disabled SDR is not exact identity");
                }
            }
        for (int preset=0; preset<look::Count; ++preset) {
            const auto recipe=look::recipe(preset);
            std::array<float,film::SettingsCount> s {};
            for (size_t i=0; i<s.size(); ++i) s[i]=static_cast<float>(recipe[i]);
            s[26]=color::AlexaLogC3; s[27]=1;
            for (int mode=0; mode<7; ++mode) for (int mask=0; mask<=film::All; ++mask) {
                s[0]=static_cast<float>(mode); s[19]=static_cast<float>(mask);
                const bool colorActive=film::modulesForSettings(s.data()) & (film::Negative|film::Print|film::Development|film::SelectiveColor);
                require(bool(color::prepare(s.data()).renderSDR)==colorActive,"Rendering leaks into bypass/texture/matte");
            }
        }
        for (int preset=1; preset<look::Count; ++preset) {
            const auto recipe=look::recipe(preset);
            std::array<float,film::SettingsCount> s {};
            for (size_t i=0; i<s.size(); ++i) s[i]=static_cast<float>(recipe[i]);
            s[26]=color::LinearRec709; s[27]=1;
            const auto p=color::prepare(s.data());
            const auto r=response::prepare(s.data());
            const int modules=film::modulesForSettings(s.data());
            for (float y : {0.0f,.002f,.018f,.18f,1.0f,4.0f}) {
                auto c=color_render_work(color_to_work({y,y,y},p),p);
                if (modules & film::Negative) c=response_negative_stage(c,r);
                if (modules & film::Development) c=response_development(c,r);
                if (modules & film::Print) c=response_print(c,r);
                c=response_finish(c,modules,r);
                c=color_from_work(c,p);
                require(std::isfinite(c.r) && std::isfinite(c.g) && std::isfinite(c.b),"Recipe produces invalid rendered gray");
            }
        }
        std::printf("SDR Gamma 2.4 gray: old %.4f, new %.4f; 1%% scene shadow: old %.4f, new %.4f.\n",
            color_encode(.18f,ColorGamma24),color_encode(.12f,ColorGamma24),
            color_encode(.01f,ColorGamma24),color_encode(color_sdr_tone(.01f),ColorGamma24));
        std::puts("SDR rendering: gray/white/black anchors, smooth monotonic HDR, preserved shadow gradation, gamut/luminance/chroma, skin/LEDs, all-space policies, all-recipe module isolation and exact disabled behavior pass.");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr,"FAILED: %s\n",error.what());
        return 1;
    }
}
