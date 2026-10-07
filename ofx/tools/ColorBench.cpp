// SPDX-License-Identifier: MPL-2.0

#include <algorithm>
#include <cmath>
#include <thread>
#include <vector>
#include "ColorSpaceConfig.h"
#include "FilmResponseConfig.h"
#include "LookPresetConfig.h"

// Independent bench-only blend, with more range than production retention.
// Compare from zero production retention to avoid stacking the two controls.
static ColorRgb candidate(ColorRgb work, ColorParameters p, float amount)
{
    const auto baseline=color_render_work(work,p);
    if (!p.renderSDR || amount<=0) return baseline;
    const auto linear=color_curve_rgb(work,ColorSRGB,0);
    const float peak=std::max({linear.r,linear.g,linear.b});
    const float y=response_luma(linear);
    if (peak<=.6f || y<=0) return baseline;
    const float weight=amount*response_smooth(.6f,2.0f,peak);
    const float gain=color_sdr_tone(peak)/peak;
    ColorRgb scaled {linear.r*gain,linear.g*gain,linear.b*gain};
    // Soft compression of negative channels around the scaled luminance.
    scaled=response_gamut(scaled,1.0f,.8f);
    const auto original=color_curve_rgb(baseline,ColorSRGB,0);
    auto c=response_mix(original,scaled,weight);
    c.r=std::clamp(c.r,0.0f,1.0f); c.g=std::clamp(c.g,0.0f,1.0f); c.b=std::clamp(c.b,0.0f,1.0f);
    return color_curve_rgb(c,ColorSRGB,1);
}

template<class F> static void pixels(size_t count, F operation)
{
    const size_t workers=std::min<size_t>(8,std::max(1u,std::thread::hardware_concurrency()));
    if (count<32768) { for (size_t i=0; i<count; ++i) operation(i); return; }
    std::vector<std::thread> threads;
    try {
        for (size_t worker=0; worker<workers; ++worker) {
            const size_t begin=count*worker/workers, end=count*(worker+1)/workers;
            threads.emplace_back([=,&operation] { for (size_t i=begin; i<end; ++i) operation(i); });
        }
    } catch (...) {
        for (auto& thread : threads) thread.join();
        throw;
    }
    for (auto& thread : threads) thread.join();
}

extern "C" {
__declspec(dllexport) int oe_settings_count() { return film::SettingsCount; }
__declspec(dllexport) int oe_rendering_index() { return film::OutputRendering; }
__declspec(dllexport) int oe_retention_index() { return film::HighlightRetention; }
__declspec(dllexport) int oe_hdr_peak_index() { return film::HDRPeak; }
__declspec(dllexport) int oe_hdr_white_index() { return film::HDRWhite; }
__declspec(dllexport) int oe_hdr_output_index() { return color::HDRPQOutput; }
__declspec(dllexport) int oe_hdr_rendering_index() { return color::StandardHDR; }
__declspec(dllexport) int oe_preset_count() { return look::Count; }
__declspec(dllexport) const char* oe_preset_label(int preset)
{
    return preset>=0 && preset<look::Count ? look::Labels[preset] : nullptr;
}
__declspec(dllexport) int oe_preset_settings(int preset, float* out, size_t count)
{
    if (!out || count!=film::SettingsCount || preset<0 || preset>=look::Count) return 1;
    const auto recipe=look::recipe(preset);
    for (size_t i=0; i<count; ++i) out[i]=static_cast<float>(recipe[i]);
    out[0]=1; out[4]=out[5]=out[6]=1;
    out[26]=color::Rec709Gamma24; out[27]=1;
    if (preset==look::Custom) out[2]=printstyle::Standard;
    return 0;
}
__declspec(dllexport) int oe_decode_linear(const float* input, float* output, size_t count, int source)
try
{
    if (!input || !output || source<0 || source>=color::SpaceCount) return 1;
    const auto p=color::prepare(source,5,false);
    pixels(count,[&](size_t i) {
        const auto c=color_matrix(color_curve_rgb({input[i*4],input[i*4+1],input[i*4+2]},p.sourceCurve,0),p.to709);
        output[i*4]=c.r; output[i*4+1]=c.g; output[i*4+2]=c.b; output[i*4+3]=input[i*4+3];
    });
    return 0;
}
catch (...) { return 2; }
__declspec(dllexport) int oe_render(const float* input, float* output, size_t count,
                                  const float* settings, size_t settingCount, float peakBlend)
try
{
    if (!input || !output || !settings || settingCount!=film::SettingsCount ||
        !std::isfinite(peakBlend) || peakBlend<0 || peakBlend>1) return 1;
    for (size_t i=0; i<settingCount; ++i) if (!std::isfinite(settings[i])) return 1;
    if (settings[0]!=1 || settings[1]<0 || settings[1]>5 || settings[2]<0 || settings[2]>printstyle::Custom ||
        settings[19]<0 || settings[19]>film::All || settings[26]<0 || settings[26]>=color::SpaceCount ||
        settings[27]<0 || settings[27]>=color::OutputSpaces.size() ||
        settings[film::SelectiveView]<0 || settings[film::SelectiveView]>1 ||
        settings[film::OutputRendering]<0 || settings[film::OutputRendering]>color::StandardHDR) return 1;
    const int modules=film::modulesForSettings(settings);
    if (modules & ~(film::Negative|film::Development|film::Print|film::SelectiveColor)) return 1;
    const auto cp=color::prepare(settings);
    const auto rp=response::prepare(settings);
    const ColorRgb gain {settings[4],settings[5],settings[6]};
    const bool identity=film::isIdentity(1,modules,0,0,0,0,rp.selectiveAmount,rp.selectiveView);
    pixels(count,[&](size_t i) {
        if (identity) { std::copy_n(input+i*4,4,output+i*4); return; }
        auto c=color_to_work({input[i*4],input[i*4+1],input[i*4+2]},cp);
        if (modules & film::Negative) c=color_balance(c,gain);
        const auto key=c;
        c=candidate(c,cp,peakBlend);
        if (modules & film::Negative) c=response_negative_stage(c,rp);
        if (modules & film::Development) c=response_development(c,rp);
        if (modules & film::Print) c=response_print(c,rp);
        c=response_finish(c,modules,rp);
        if (modules & film::SelectiveColor) c=response_selective(c,key,rp);
        if (response_clamps_negative(modules,rp)) {
            c.r=std::max(c.r,0.0f); c.g=std::max(c.g,0.0f); c.b=std::max(c.b,0.0f);
        }
        if (!((modules & film::SelectiveColor) && rp.selectiveView)) c=color_from_work(c,cp);
        output[i*4]=c.r; output[i*4+1]=c.g; output[i*4+2]=c.b; output[i*4+3]=input[i*4+3];
    });
    return 0;
}
catch (...) { return 2; }
}
