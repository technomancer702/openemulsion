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
    if (!std::isfinite(a) || std::abs(a-b)>tolerance)
        std::fprintf(stderr,"%s: %.9g vs %.9g (tolerance %.9g)\n",message,a,b,tolerance);
    require(std::isfinite(a) && std::abs(a-b) <= tolerance,message);
}

static float luma(ColorRgb c) { return .2126f*c.r + .7152f*c.g + .0722f*c.b; }

static ColorRgb renderLinear(ColorRgb c)
{
    const auto p=color::prepare(color::LinearRec709,1,false,color::Automatic);
    return color_curve_rgb(color_render_work(color_curve_rgb(c,ColorSRGB,1),p),ColorSRGB,0);
}

static ColorRgb renderRetained(ColorRgb c, float amount)
{
    auto p=color::prepare(color::LinearRec709,1,false,color::Automatic);
    p.highlightRetention=amount;
    return color_curve_rgb(color_render_work(color_curve_rgb(c,ColorSRGB,1),p),ColorSRGB,0);
}

static void testEmitterShoulder()
{
    for (float ratio : {.001f,.0722f,.18f,.2126f,.5f,.6f,.7152f,.95f,1.0f}) {
        near(color_sdr_emitter_tone(ratio,1),color_sdr_tone(ratio),0,"Emitter join value");
        const float h=1e-4f;
        const float left=(color_sdr_emitter_tone(ratio,1)-color_sdr_emitter_tone(ratio*(1-h),1-h))/(ratio*h);
        const float right=(color_sdr_emitter_tone(ratio*(1+h),1+h)-color_sdr_emitter_tone(ratio,1))/(ratio*h);
        near(left,right,.003f,"Emitter join slope");
        near(right,color_sdr_slope(ratio),.003f,"Emitter join disagrees with analytical slope");
        float previous=-1;
        for (int i=0; i<2400; ++i) {
            const float peak=std::exp2(-14.0f+i*.012f), y=peak*ratio;
            const float mapped=color_sdr_emitter_tone(y,peak);
            require(mapped>=previous-1e-7f && mapped>=0 && mapped<=1,"Emitter shoulder exposure order/bounds");
            if (peak>1) require(mapped<=ratio+1e-7f,"Emitter shoulder forces peak RGB over one");
            previous=mapped;
        }
    }
    require(color_sdr_emitter_tone(0,32)==0,"Emitter shoulder raises black");
    require(color_sdr_emitter_tone(1e-9f,32)<=1e-9f,"Emitter shoulder adds a tiny-luminance pedestal");
    for (float join : {.15f,.75f}) for (float amount : {0.0f,.5f,1.0f}) {
        const float h=1e-5f;
        const auto a=renderRetained({4,4*(1-join+h),4*(1-join+h)},amount);
        const auto b=renderRetained({4,4*(1-join-h),4*(1-join-h)},amount);
        near(a.r,b.r,1e-4f,"Emitter chroma gate red discontinuity");
        near(a.g,b.g,1e-4f,"Emitter chroma gate green discontinuity");
        near(a.b,b.b,1e-4f,"Emitter chroma gate blue discontinuity");
    }
    const ColorRgb emitter {32,.1f,.3f};
    const float y=luma(emitter), mapped=color_sdr_tone(y), gain=mapped/y;
    const auto old=color_sdr_gamut({emitter.r*gain,emitter.g*gain,emitter.b*gain},mapped);
    const auto updated=renderRetained(emitter,0);
    require(updated.g<old.g*.75f && updated.b<old.b*.75f,"Default still forces red emitter near white");
    require(updated.r>updated.g && updated.r>updated.b,"Default emitter hue ordering");
    require(luma(updated)<luma(old) && luma(updated)>.25f*luma(old),"Default brightness tradeoff");
    for (const auto color : std::array<ColorRgb,4>{{{.38f,.2f,.12f},{1,.01f,.025f},{.02f,.03f,1},{8,7.8f,7.6f}}}) {
        const float luminance=luma(color), scale=color_sdr_tone(luminance)/luminance;
        const auto reference=color_sdr_gamut({color.r*scale,color.g*scale,color.b*scale},color_sdr_tone(luminance));
        const auto result=renderRetained(color,1);
        near(result.r,reference.r,2e-6f,"Low-intensity/pale color changed");
        near(result.g,reference.g,2e-6f,"Low-intensity/pale color changed");
        near(result.b,reference.b,2e-6f,"Low-intensity/pale color changed");
    }
    std::puts("SDR emitters: matching value/slope at peak one, exposure order, RGB bounds, default color retention and unchanged low-intensity/pale colors pass.");
}

static void testEmitterDetail()
{
    for (float gate : {0.0f,1e-8f,.01f,.5f,1.0f}) {
        near(color_sdr_detail_peak(1,gate),1,0,"Detail shoulder join value");
        const float h=1e-4f;
        near((color_sdr_detail_peak(1+h,gate)-1)/h,1,.001f,"Detail shoulder join slope");
        float previous=0;
        for (int i=0; i<2400; ++i) {
            const float peak=std::exp2(-14.0f+i*.012f);
            const float result=color_sdr_detail_peak(peak,gate);
            require(result>=previous && result<=peak+1e-5f,"Detail shoulder order/range");
            if (gate==0 || peak<=1) require(result==peak,"Detail shoulder changes neutral/sub-threshold signal");
            previous=result;
        }
    }
    // Equal chromaticity, different intensity: actual tonal contrast, not whitening.
    // At extreme intensities the v0.38 luminance shoulder passed ordering but
    // left these source variations effectively invisible.
    for (const auto ray : std::array<ColorRgb,4>{{{1,.003125f,.009375f},{1,-.015f,.02f},
                                                 {.01f,.03f,1},{1,1,.01f}}}) {
        for (float peak : {8.0f,16.0f,32.0f,64.0f}) for (float amount : {0.0f,1.0f}) {
            const auto a=renderRetained({ray.r*peak,ray.g*peak,ray.b*peak},amount);
            const auto b=renderRetained({ray.r*peak*1.25f,ray.g*peak*1.25f,ray.b*peak*1.25f},amount);
            const float contrast=(luma(b)-luma(a))/(.5f*(luma(a)+luma(b)));
            require(contrast>(ray.g==1 ? .0025f : .01f),"Bright emitter intensity detail flattened");
        }
    }
    std::puts("SDR emitter detail: smooth root input shoulder, neutral/low-intensity identity and bright source-intensity contrast pass.");
}

static void testHighlightRetention()
{
    for (float y : {0.0f,.0001f,.01f,.18f,.6f,1.0f,4.0f,64.0f,10000.0f}) {
        const auto baseline=renderRetained({y,y,y},0);
        for (float amount : {.01f,.5f,1.0f}) {
            const auto c=renderRetained({y,y,y},amount);
            require(c.r==baseline.r && c.g==baseline.g && c.b==baseline.b,"Retention changes neutral gray");
        }
    }
    for (const auto c : std::array<ColorRgb,4>{{{.38f,.2f,.12f},{.6f,.1f,.2f},{.1f,.2f,.6f},{.005f,.004f,.002f}}}) {
        const auto a=renderRetained(c,0), b=renderRetained(c,1);
        require(a.r==b.r && a.g==b.g && a.b==b.b,"Retention changes below-threshold color");
    }
    const auto baseline=renderRetained({32,.1f,.3f},0), retained=renderRetained({32,.1f,.3f},1);
    require(retained.r>retained.g && retained.r>retained.b,"Retention reverses red emitter ordering");
    require((retained.r-retained.g)/retained.r > (baseline.r-baseline.g)/baseline.r+.05f,"Retention has no visible highlight effect");
    require(luma(retained)<luma(baseline) && luma(retained)>.6f*luma(baseline),"Retention brightness tradeoff exceeds restrained range");
    for (const auto chip : std::array<ColorRgb,8>{{{1,.01f,.025f},{.01f,1,.02f},{.01f,.03f,1},{1,1,.01f},
            {1,.01f,1},{.01f,1,1},{1,-.05f,.25f},{.38f,.2f,.12f}}}) {
        for (float amount : {0.0f,.01f,.25f,.5f,1.0f}) {
            float previous=-1;
            for (int i=0; i<2400; ++i) {
                const float exposure=std::exp2(-14.0f+i*.012f);
                const auto c=renderRetained({chip.r*exposure,chip.g*exposure,chip.b*exposure},amount);
                for (float value : {c.r,c.g,c.b}) require(std::isfinite(value) && value>=0 && value<=1.000001f,"Retained RGB boundary");
                require(luma(c)+2e-6f>=previous,"Retention reverses exposure brightness");
                previous=luma(c);
            }
        }
    }
    for (float peak : {1.0f}) {
        const auto a=renderRetained({peak-1e-5f,.02f,.03f},1), b=renderRetained({peak+1e-5f,.02f,.03f},1);
        near(a.r,b.r,1e-4f,"Retention gate has a discontinuity");
        near(a.g,b.g,1e-4f,"Retention gate has a discontinuity");
    }
    const auto a=renderRetained({4,3.99999f,.1f},1), b=renderRetained({3.99999f,4,.1f},1);
    near(a.r,b.r,1e-4f,"Peak-channel crossing is discontinuous");
    near(a.g,b.g,1e-4f,"Peak-channel crossing is discontinuous");
    // Every amount is an affine blend in display linear at a fixed source pixel.
    for (int i=0; i<=100; ++i) {
        const float amount=i/100.0f;
        const auto c=renderRetained({32,.1f,.3f},amount);
        near(c.r,baseline.r+(retained.r-baseline.r)*amount,2e-6f,"Retention slider is not smooth/linear");
        near(c.g,baseline.g+(retained.g-baseline.g)*amount,2e-6f,"Retention slider is not smooth/linear");
    }
}

static void testHDR()
{
    // Independent ST 2084 numerical anchors (normalized float code values).
    for (const auto anchor : std::array<std::pair<float,float>,6>{{
        {0,.000000730956f},{100,.508078422f},{203,.580688881f},
        {1000,.751827096f},{4000,.902572393f},{10000,1}}}) {
        near(color_encode(anchor.first/10000,ColorPQ),anchor.second,8e-6f,"PQ inverse EOTF anchor");
        near(color_decode(anchor.second,ColorPQ)*10000,anchor.first,
             std::max(.003f,anchor.first*.0001f),"PQ EOTF anchor");
    }
    const auto matrix=color::inverse(color::to709Matrices()[color::Rec2100PQ]);
    const std::array<double,9> reference {.62740390,.32928304,.04331307,.06909729,.91954040,.01136232,.01639144,.08801331,.89559525};
    for (size_t i=0; i<matrix.size(); ++i) near(static_cast<float>(matrix[i]),static_cast<float>(reference[i]),2e-6f,"Rec.709 to Rec.2020 matrix");
    for (float peak : {400.0f,1000.0f,4000.0f,10000.0f}) for (float white : {80.0f,203.0f,300.0f}) {
        const auto p=color::prepare(color::LinearRec709,color::HDRPQOutput,false,color::StandardHDR,peak,white);
        require(p.renderHDR && !p.renderSDR,"HDR accidentally renders SDR first");
        near(color_hdr_tone(.18f,peak/white),.12f,1e-7f,"HDR gray anchor");
        near(color_hdr_tone(1,peak/white),1,1e-7f,"HDR reference white anchor");
        for (float join : {.18f,1.0f}) {
            const float h=1e-4f;
            near((color_hdr_tone(join,peak/white)-color_hdr_tone(join-h,peak/white))/h,
                 (color_hdr_tone(join+h,peak/white)-color_hdr_tone(join,peak/white))/h,.005f,"HDR slope continuity");
        }
        float previous=-1;
        for (int i=0; i<2000; ++i) {
            const float y=std::exp2(-20.0f+i*.019f);
            const auto pq=color_from_work(color_curve_rgb({y,y,y},ColorSRGB,1),p);
            const auto nits=color_curve_rgb(pq,ColorPQ,0);
            for (float v : {nits.r,nits.g,nits.b})
                require(std::isfinite(v) && v>=0 && v*10000<=peak+.2f,"HDR luminance ceiling");
            near(nits.r,nits.g,2e-5f,"HDR neutral axis");
            if (nits.r+2e-5f<previous) std::fprintf(stderr,"HDR ramp: peak %.0f white %.0f source %.9g previous %.9g current %.9g code %.9g\n",peak,white,y,previous,nits.r,pq.r);
            require(nits.r+2e-5f>=previous,"HDR neutral exposure order");
            previous=nits.r;
        }
        for (float y : {0.0f,.18f,1.0f,4.0f,64.0f}) {
            const auto pq=color_from_work(color_curve_rgb({y,y,y},ColorSRGB,1),p);
            near(color_decode(pq.r,ColorPQ)*10000,color_hdr_tone(y,peak/white)*white,
                 std::max(.01f,peak*.00015f),"HDR rendered neutral luminance");
        }
        for (const auto chip : std::array<ColorRgb,6>{{{32,.1f,.3f},{-.03f,.5f,.1f},{.01f,.03f,32},
            {4,4,.1f},{.38f,.2f,.12f},{1,-.1f,.3f}}}) {
            const auto c=color_from_work(color_curve_rgb(chip,ColorSRGB,1),p);
            for (float v : {c.r,c.g,c.b}) require(std::isfinite(v) && v>=0 && color_decode(v,ColorPQ)*10000<=peak+.2f,"HDR colored gamut bound");
        }
        auto conversion=p; conversion.renderHDR=0;
        const auto c=color_from_work(color_curve_rgb({1,1,1},ColorSRGB,1),conversion);
        near(color_decode(c.r,ColorPQ)*10000,white,.05f,"Conversion-only PQ scale");
    }
    for (int source=0; source<color::SpaceCount; ++source) for (int output=0; output<static_cast<int>(color::OutputSpaces.size()); ++output)
        for (int rendering=0; rendering<color::RenderingCount; ++rendering) for (bool texture : {false,true}) {
            const auto p=color::prepare(source,output,texture,rendering);
            const bool expected=!texture && output==color::HDRPQOutput && (rendering==color::StandardHDR ||
                (rendering==color::Automatic && source!=color::Rec709Gamma24 && source!=color::SRGB));
            require(bool(p.renderHDR)==expected,"HDR automatic/explicit policy");
            require(!(p.renderHDR && p.renderSDR),"SDR/HDR stack");
            if (p.renderHDR) {
                const ColorRgb work {-.02f,.3f,4};
                const auto c=color_render_work(work,p);
                require(c.r==work.r && c.g==work.g && c.b==work.b,"HDR is rendered before creative stages");
                require(!color::highlightRetentionEnabled(p,film::Negative,0,1,0),"SDR retention leaks into HDR");
            }
        }
    auto recipe=look::recipe(look::Neutral);
    std::array<float,film::SettingsCount> s {};
    for (size_t i=0; i<s.size(); ++i) s[i]=static_cast<float>(recipe[i]);
    s[26]=color::AlexaLogC3; s[27]=color::HDRPQOutput;
    for (int mode=0; mode<7; ++mode) for (int mask=0; mask<=film::All; ++mask) {
        s[0]=static_cast<float>(mode); s[19]=static_cast<float>(mask);
        const int active=film::modulesForSettings(s.data());
        const auto p=color::prepare(s.data());
        require(bool(p.renderHDR)==bool(active & (film::Negative|film::Print|film::Development|film::SelectiveColor)),"HDR module isolation");
        require(color::hdrWhiteEnabled(p,active,0)==bool(p.renderHDR),"HDR UI policy");
    }
    s[0]=0; s[19]=film::Negative|film::SelectiveColor; s[film::SelectiveView]=1;
    const auto p=color::prepare(s.data());
    require(!color::hdrWhiteEnabled(p,film::Negative|film::SelectiveColor,1),"HDR controls enabled on Selection Matte");
    for (int preset=1; preset<look::Count; ++preset) {
        const auto look=look::recipe(preset);
        for (size_t i=0; i<s.size(); ++i) s[i]=static_cast<float>(look[i]);
        s[26]=color::LinearRec709; s[27]=color::HDRPQOutput; s[film::OutputRendering]=color::StandardHDR;
        const auto cp=color::prepare(s.data()); const auto hdr=response::prepare(s.data());
        s[27]=1; const auto sdr=response::prepare(s.data());
        require(hdr.printTone.ceiling>sdr.printTone.ceiling && hdr.negativeTone.ceiling>sdr.negativeTone.ceiling,
                "HDR creative shoulders have no headroom");
        near(hdr.printTone.knee,sdr.printTone.knee,0,"HDR changes print knee");
        const auto modules=film::modulesForSettings(s.data());
        auto c=color_curve_rgb({32,32,32},ColorSRGB,1);
        if (modules & film::Negative) c=response_negative_stage(c,hdr);
        if (modules & film::Development) c=response_development(c,hdr);
        if (modules & film::Print) c=response_print(c,hdr);
        c=response_finish(c,modules,hdr);
        const auto out=color_from_work(c,cp);
        for (float code : {out.r,out.g,out.b}) require(std::isfinite(code) && code>=0 && code<1,"HDR recipe range");
        if (preset==look::Daylight50)
            require(color_decode(out.r,ColorPQ)*10000>300,"Print preset loses HDR highlight headroom");
    }
    std::puts("HDR PQ: independent transfer/matrix anchors, black/gray/white, C1 joins, exposure order, gamut/peak bounds, scale, output policy and module/matte isolation pass.");
}

int main()
{
    try {
        testHDR();
        testEmitterShoulder();
        testEmitterDetail();
        testHighlightRetention();
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
                const float sourcePeak=std::max({source.r,source.g,source.b});
                const float low=std::min({source.r,source.g,source.b});
                const float t=std::clamp(((sourcePeak-low)/sourcePeak-.15f)/.6f,0.0f,1.0f);
                const float peak=color_sdr_detail_peak(sourcePeak,t*t*(3-2*t));
                const float detailY=luma(source)*(peak/sourcePeak);
                const float baseline=color_sdr_tone(detailY);
                const float retained=color_sdr_emitter_tone(detailY,peak);
                require(luma(c)<=baseline+3e-6f && luma(c)>=retained-3e-6f,"SDR luminance outside shoulder endpoints");
                const float weight=peak>1 ? .5f*t*t*(3-2*t) : 0;
                near(luma(c),baseline+(retained-baseline)*weight,3e-6f,"SDR blend target luminance");
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
        std::puts("SDR rendering: gray/white/black anchors, smooth monotonic HDR, preserved shadow gradation, gamut/luminance/chroma, skin/LEDs, all-space policies, all-recipe module isolation, exact disabled behavior and restrained highlight retention pass.");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr,"FAILED: %s\n",error.what());
        return 1;
    }
}
