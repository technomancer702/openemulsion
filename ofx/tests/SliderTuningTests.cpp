// SPDX-License-Identifier: MPL-2.0

#include <cmath>
#include <cstdio>
#include <stdexcept>

#include "LookPresetConfig.h"
#include "ColorSpaceConfig.h"
#include "FilmResponseConfig.h"
#include "GrainConfig.h"

static void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

static void near(float a, float b, float tolerance, const char* message)
{
    require(std::isfinite(a) && std::abs(a-b) <= tolerance,message);
}

static grain::PackedSettings settings(int preset = look::Custom)
{
    const auto recipe = look::recipe(preset);
    grain::PackedSettings s {};
    for (const auto& control : look::Controls) s[control.setting] = static_cast<float>(recipe[control.setting]);
    s[19] = static_cast<float>(recipe[19]);
    s[4] = s[5] = s[6] = 1;
    return s;
}

static ColorRgb colorStage(ColorRgb c, const grain::PackedSettings& s)
{
    const auto p = response::prepare(s.data());
    const int modules = film::modulesForSettings(s.data());
    c = response_negative_stage(c,p);
    if (modules & film::Development) c = response_development(c,p);
    return response_finish(response_print(c,p),modules,p);
}

static void testAnchors()
{
    // Captured from committed v0.24 math: two color chips and one grain sample per recipe.
    const std::array<std::array<float,9>,look::Count> anchors {{
        { .619741440f,.452860475f,.359768122f,.848246515f,.103808984f,.0846692473f,-.00756785041f,-.00681879232f,-.00770107191f },
        { .635291100f,.451151282f,.350920796f,.899802089f,.105129004f,.0831860453f,-.000962815946f,.0000508178746f,-.000907741312f },
        { .627854705f,.451949358f,.353923231f,.879237413f,.103384629f,.0830402225f,-.00702754362f,-.00934132095f,-.0065630828f },
        { .623492479f,.452858508f,.357702076f,.857706428f,.102232680f,.082711190f,.00342236622f,.00359656126f,.00256306888f },
        { .618797243f,.452183485f,.359260380f,.845931828f,.103451550f,.0843893439f,.00309426640f,.00631570350f,.00391769921f },
        { .506651461f,.506651461f,.506651461f,.353699267f,.353699267f,.353699267f,-.00495648989f,-.00495648989f,-.00495648989f },
        { .626889884f,.445224971f,.343291372f,.836081922f,.091742292f,.0720656663f,.00971436035f,.00684141880f,.00770231476f },
        { .612976968f,.452835053f,.368443727f,.901436210f,.131118476f,.107832238f,-.00426559011f,-.00338761695f,-.00295395567f },
        { .632135451f,.442124248f,.336497873f,.832423508f,.0936749801f,.0760228708f,-.00675071264f,-.00195840863f,-.00422327546f },
        { .599490523f,.463598847f,.390624076f,.717596650f,.135527357f,.129439890f,-.00742487749f,-.00747785810f,-.00557462266f },
        { .546825349f,.469790339f,.429388493f,.473109245f,.146509215f,.139949247f,-.00707618613f,-.00504512200f,-.00603310578f },
        { .607475638f,.461386412f,.385042965f,.820365489f,.138846785f,.129965976f,-.00100113801f,-.000395533105f,.00337315700f },
        { .514006555f,.514006555f,.514006555f,.335305929f,.335305929f,.335305929f,-.00973842479f,-.00973842479f,-.00973842479f }
    }};
    for (int preset=0; preset<look::Count; ++preset) {
        auto s=settings(preset);
        // v0.29 changes recipes, not math. Freeze the revised looks' original inputs.
        if (preset == look::Daylight250 || preset == look::Tungsten200 || preset == look::Tungsten500 ||
            preset == look::ClassicCinema || preset == look::NeonNights || preset == look::SeventiesPrint ||
            preset == look::Super8HomeMovie) {
            s = settings();
            switch (preset) {
            case look::Daylight250:
                s[7]=.14f; s[8]=1; s[10]=1.06f; s[16]=.13f; s[17]=.38f;
                s[28]=.58f; s[32]=.2f; s[13]=.13f;
                break;
            case look::Tungsten200:
                s[7]=.16f; s[8]=.97f; s[16]=.12f; s[17]=.36f; s[28]=.6f;
                s[13]=.16f; s[15]=.015f; s[34]=.65f;
                break;
            case look::Tungsten500:
                s[7]=.20f; s[9]=.20f; s[16]=.22f; s[17]=.52f; s[18]=.4f;
                s[22]=1.35f; s[28]=.66f; s[13]=.24f; s[15]=.025f; s[34]=.68f;
                break;
            case look::ClassicCinema:
                s[7]=.22f; s[8]=1.04f; s[32]=-.65f; s[11]=.3f; s[12]=.25f;
                s[13]=.18f; s[15]=.02f; s[16]=.14f; s[17]=.4f;
                s[film::ColorRichness]=.12f;
                break;
            case look::NeonNights:
                s[7]=.22f; s[8]=1.05f; s[28]=.72f; s[30]=.7f; s[32]=-.3f; s[12]=.18f;
                s[13]=.36f; s[14]=.85f; s[15]=.04f; s[16]=.2f; s[17]=.5f;
                s[film::SplitTone]=.18f; s[film::SplitHue]=205;
                s[film::SplitHighlights]=.55f; s[film::ColorRichness]=.15f;
                break;
            case look::SeventiesPrint:
                s[1]=1; s[7]=.25f; s[8]=.84f; s[9]=.24f; s[29]=.7f;
                s[32]=-.55f; s[11]=.18f; s[12]=.65f; s[38]=.06f; s[40]=-.08f;
                s[16]=.25f; s[17]=.58f; s[18]=.5f; s[13]=.22f; s[15]=.03f;
                break;
            case look::Super8HomeMovie:
                s[1]=1; s[7]=.18f; s[8]=.88f; s[9]=.24f; s[10]=.98f; s[29]=.6f;
                s[32]=.45f; s[11]=.25f; s[12]=.68f; s[38]=.06f; s[40]=-.06f;
                s[16]=.2f; s[17]=.48f; s[18]=.55f; s[20]=.4f;
                s[13]=.16f; s[15]=.025f; s[film::FilmGauge]=gauge::Super8;
                s[film::BloomAmount]=.08f;
                break;
            }
        }
        const auto warm=colorStage({.65f,.44f,.33f},s), red=colorStage({1.2f,.03f,.01f},s);
        const auto grain=grain_delta(29,67,response_luma(red),grain::prepare(s.data(),1080,37));
        const std::array<float,9> actual {warm.r,warm.g,warm.b,red.r,red.g,red.b,grain.r,grain.g,grain.b};
        for (size_t i=0; i<actual.size(); ++i)
            near(actual[i],anchors[preset][i],5e-7f,"Default controls or historical response math changed from v0.24 anchor");
    }
}

static void testExpandedColor()
{
    const auto neutral=color::cameraBalance(0,0,0);
    require(neutral.r==1 && neutral.g==1 && neutral.b==1,"Camera balance neutral changed");
    for (double exposure : {-4.0,0.0,4.0}) for (double temperature : {-3.0,-1.0,0.0,1.0,3.0})
        for (double tint : {-3.0,-1.0,0.0,1.0,3.0}) {
            const auto gain=color::cameraBalance(exposure,temperature,tint);
            require(gain.r>0 && gain.g>0 && gain.b>0 && std::isfinite(gain.r+gain.g+gain.b),"Expanded camera gains nonpositive/nonfinite");
        }
    auto s=settings(); s[19]=film::Negative;
    const ColorRgb warm {.65f,.44f,.33f}, neutralWork {.46135613f,.46135613f,.46135613f};
    for (int system=0; system<6; ++system) {
        s[1]=static_cast<float>(system);
        for (int setting : std::array<int,4>{7,29,31,film::SplitTone}) {
            const float low=setting==7 ? -1.2f : setting==31 ? -3.0f : 0.0f;
            const float high=setting==7 ? 1.5f : 3.0f;
            for (int step=0; step<=100; ++step) {
                auto sweep=s; sweep[setting]=low+(high-low)*step/100;
                const auto p=response::prepare(sweep.data());
                for (ColorRgb c : {warm,neutralWork,ColorRgb{0,0,0},ColorRgb{1.2f,.02f,-.1f},ColorRgb{-.2f,2,.4f},ColorRgb{20,20,20}}) {
                    c=response_development(response_negative_stage(c,p),p);
                    c=response_finish(response_print(c,p),film::Negative|film::Development|film::Print,p);
                    require(std::isfinite(c.r+c.g+c.b),"Expanded color sweep nonfinite");
                }
            }
        }
    }
    s=settings(); s[7]=0; s[29]=0; s[30]=0; s[film::NegativeToneStrength]=0;
    s[31]=1; const auto skin1=response_negative_stage(warm,response::prepare(s.data()));
    s[31]=3; const auto skin3=response_negative_stage(warm,response::prepare(s.data()));
    s[31]=0; const auto skin0=response_negative_stage(warm,response::prepare(s.data()));
    require(std::abs(skin3.g-skin0.g)>2.9f*std::abs(skin1.g-skin0.g),"Skin endpoint still clamped to old range");
    for (ColorRgb c : {neutralWork,ColorRgb{.2f,.6f,.8f}}) {
        s[31]=0; const auto zero=response_negative_stage(c,response::prepare(s.data()));
        s[31]=3; const auto strong=response_negative_stage(c,response::prepare(s.data()));
        near(strong.r,zero.r,1e-7f,"Skin Hue leaks onto cool/neutral colors");
        near(strong.g,zero.g,1e-7f,"Skin Hue leaks onto cool/neutral colors");
        near(strong.b,zero.b,1e-7f,"Skin Hue leaks onto cool/neutral colors");
    }
    s=settings(); s[29]=1; const auto mix1=response::prepare(s.data());
    s[29]=3; const auto mix3=response::prepare(s.data());
    near(mix3.negativeMatrix[1],mix1.negativeMatrix[1]*3,1e-7f,"Crosstalk endpoint still clamped");
    s[film::SplitTone]=1; const auto split1=response_development({.15f,.15f,.15f},response::prepare(s.data()));
    s[film::SplitTone]=3; const auto split3=response_development({.15f,.15f,.15f},response::prepare(s.data()));
    near(split3.b-.15f,(split1.b-.15f)*3,1e-7f,"Split endpoint still clamped");
    near(response_luma(split3),.15f,1e-7f,"Expanded split changes luminance");
    const auto pivot=response_development(neutralWork,response::prepare(s.data()));
    near(pivot.r,neutralWork.r,0,"Expanded split tints neutral pivot");
    near(pivot.g,neutralWork.g,0,"Expanded split tints neutral pivot");
    near(pivot.b,neutralWork.b,0,"Expanded split tints neutral pivot");
    s=settings(); s[7]=-1.2f; const auto densityLow=response_negative_stage(neutralWork,response::prepare(s.data()));
    s[7]=1.5f; const auto densityHigh=response_negative_stage(neutralWork,response::prepare(s.data()));
    near(densityLow.r,densityHigh.r,1e-7f,"Expanded density affects neutrals");
    require(response::prepare(s.data()).density==1.5f,"Density endpoint still clamped");
}

static void testSoftnessJoin()
{
    auto s=settings(); s[20]=1;
    const auto p=grain::prepare(s.data(),1080,37);
    s[20]=1.00001f; const auto nearJoin=grain::prepare(s.data(),1080,37);
    s[20]=2; const auto soft=grain::prepare(s.data(),1080,37);
    require(p.inverseSize==soft.inverseSize && p.seed==soft.seed && p.amount==soft.amount,"Softness changes lattice pitch, seed, or nominal amount");
    for (int y : {0,1,57,1079}) for (int x : {0,1,29,1919}) {
        const auto a=grain_delta(x,y,.45f,p), b=grain_delta(x,y,.45f,nearJoin);
        near(a.r,b.r,5e-7f,"Softness jumps at one");
        near(a.g,b.g,5e-7f,"Softness jumps at one");
        near(a.b,b.b,5e-7f,"Softness jumps at one");
    }
    for (float x : {-1.0f,-.50001f,-.5f,-.00001f,0.0f,.49999f,.5f,1.0f}) {
        const auto original=grain_field(x,x*.31f,p.seed), smoothZero=grain_soft_field(x,x*.31f,p.seed,0);
        near(original.r,smoothZero.r,3e-7f,"Soft lattice zero disagrees with original field");
        const auto a=grain_soft_field(x,.7f,p.seed,1), b=grain_soft_field(x+.00001f,.7f,p.seed,1);
        near(a.r,b.r,2e-5f,"Soft lattice has a cell boundary discontinuity");
    }
}

int main()
{
    try {
        testAnchors(); testExpandedColor(); testSoftnessJoin();
        std::puts("Slider tuning: v0.24 default/historical recipe anchors, expanded endpoints, skin/neutral isolation, finite color sweeps, and soft-grain continuity/pitch/seed pass.");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr,"FAILED: %s\n",error.what()); return 1;
    }
}
