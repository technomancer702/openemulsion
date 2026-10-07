// SPDX-License-Identifier: MPL-2.0

#include <cstdio>
#include <limits>
#include <stdexcept>
#include "ColorSpaceConfig.h"
#include "FilmResponseConfig.h"
#include "LookPresetConfig.h"

extern "C" {
int oe_settings_count();
int oe_rendering_index();
int oe_retention_index();
int oe_sdr_contrast_index();
int oe_sdr_rolloff_index();
int oe_sdr_gamut_index();
int oe_hdr_exposure_index();
int oe_hdr_rolloff_index();
int oe_preset_settings(int, float*, size_t);
int oe_render(const float*, float*, size_t, const float*, size_t, float);
}

static void require(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}

int main()
{
    try {
        require(oe_settings_count()==film::SettingsCount && oe_rendering_index()==film::OutputRendering && oe_retention_index()==film::HighlightRetention,
                "Native settings ABI mismatch");
        require(oe_sdr_contrast_index()==film::SDRContrast && oe_sdr_rolloff_index()==film::SDRRolloff &&
                oe_sdr_gamut_index()==film::SDRGamut,"Native SDR control indices mismatch");
        require(oe_hdr_exposure_index()==film::HDRExposure && oe_hdr_rolloff_index()==film::HDRRolloff,
                "Native HDR control indices mismatch");
        const std::array<float,16> input {0,0,0,.1f, .15f,.25f,.3f,.5f, .6f,.7f,.5f,.7f, .9f,.6f,.5f,1};
        std::array<float,16> actual {};
        std::array<float,film::SettingsCount> s {};
        for (int preset=0; preset<look::Count; ++preset) {
            require(oe_preset_settings(preset,s.data(),s.size())==0,"Recipe loading failed");
            s[19]=static_cast<float>(static_cast<int>(s[19]) & (film::Negative|film::Development|film::Print|film::SelectiveColor));
            for (int source=0; source<color::SpaceCount; ++source) for (int output : {1,color::HDRPQOutput})
                for (int rendering=0; rendering<color::RenderingCount; ++rendering) for (float retention : {0.0f,.5f,1.0f}) {
                s[27]=static_cast<float>(output);
                s[26]=static_cast<float>(source); s[film::OutputRendering]=static_cast<float>(rendering);
                s[film::HighlightRetention]=retention;
                const auto cp=color::prepare(s.data());
                const auto rp=response::prepare(s.data());
                const int modules=film::modulesForSettings(s.data());
                require(oe_render(input.data(),actual.data(),4,s.data(),s.size(),0)==0,"Baseline rendering failed");
                for (size_t i=0; i<4; ++i) {
                    auto c=color_to_work({input[i*4],input[i*4+1],input[i*4+2]},cp);
                    if (modules & film::Negative) c=color_balance(c,{s[4],s[5],s[6]});
                    const auto key=c;
                    c=color_render_work(c,cp);
                    if (modules & film::Negative) c=response_negative_stage(c,rp);
                    if (modules & film::Development) c=response_development(c,rp);
                    if (modules & film::Print) c=response_print(c,rp);
                    c=response_finish(c,modules,rp);
                    if (modules & film::SelectiveColor) c=response_selective(c,key,rp);
                    if (response_clamps_negative(modules,rp)) c={std::max(c.r,0.0f),std::max(c.g,0.0f),std::max(c.b,0.0f)};
                    if (!((modules & film::SelectiveColor) && rp.selectiveView)) c=color_from_work(c,cp);
                    for (const auto pair : {std::pair<float,float>{actual[i*4],c.r}, {actual[i*4+1],c.g}, {actual[i*4+2],c.b}})
                        require(std::isfinite(pair.first) && std::abs(pair.first-pair.second)<1e-6f,"Bench baseline differs from production stages");
                    require(actual[i*4+3]==input[i*4+3],"Bench changes alpha");
                }
            }
        }
        s[film::SelectiveView]=std::numeric_limits<float>::max();
        require(oe_render(input.data(),actual.data(),4,s.data(),s.size(),0)!=0,"Out-of-range selective view accepted");
        s[film::SelectiveView]=0;
        for (int control : {film::SDRContrast,film::SDRRolloff,film::SDRGamut}) {
            s[control]=2;
            require(oe_render(input.data(),actual.data(),4,s.data(),s.size(),0)!=0,"Invalid SDR control accepted");
            s[control]=0;
        }
        for (const auto entry : {std::pair<int,float>{film::HDRExposure,4.01f},{film::HDRRolloff,1.01f}}) {
            s[film::OutputRendering]=color::StandardHDR; s[entry.first]=entry.second;
            require(oe_render(input.data(),actual.data(),4,s.data(),s.size(),0)!=0,"Invalid HDR viewing control accepted");
            s[entry.first]=0;
        }
        s[film::OutputRendering]=std::numeric_limits<float>::quiet_NaN();
        require(oe_render(input.data(),actual.data(),4,s.data(),s.size(),0)!=0,"NaN settings accepted");
        require(oe_render(nullptr,actual.data(),4,s.data(),s.size(),0)!=0,"Null pixels accepted");
        std::puts("Color bench: all recipes/spaces/rendering policies match production color stages; alpha/invalid input pass.");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr,"FAILED: %s\n",error.what());
        return 1;
    }
}
