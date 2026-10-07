// SPDX-License-Identifier: MPL-2.0

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>

#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "ofxsProcessing.h"
#include "ofxsSupportPrivate.h"
#include "HalationBlur.h"
#include "BloomBlur.h"
#include "HalationConfig.h"
#include "GrainConfig.h"
#include "ColorSpaceConfig.h"
#include "FilmResponseConfig.h"
#include "ModuleControlState.h"
#include "LookPresetConfig.h"
#include "UserPresetIO.h"

#define kPluginName "OpenEmulsion"
#define kPluginGrouping "OpenEmulsion"
#define kPluginDescription "Original film-emulation plugin with adjustable tone, print, grain, halation, aura, linear-light bloom, and selective color, with OpenCL acceleration."
#define kPluginIdentifier "org.openemulsion.film"
#define kPluginVersionMajor 0
#define kPluginVersionMinor 41

extern bool RunOpenEmulsionOpenCL(void* cmdQueue, int width, int height, double time, const float* settings, const float* input, float* output);

namespace {

using Rgb = ColorRgb;

struct Settings {
    int mode = 0;
    int system = 0;
    int printStyle = 1;
    int grainStyle = 1;
    int grainResponse = 0;
    int sourceSpace = color::Rec709Gamma24;
    int outputSpace = 0;
    int outputRendering = color::Automatic;
    double hdrPeak = 1000, hdrWhite = 203;
    double sdrContrast = 0, sdrRolloff = 0, sdrGamut = 0;
    bool enableNegative = true;
    bool enableDevelopment = true;
    bool enablePrint = true;
    bool enableHalation = true;
    bool enableAura = true;
    bool enableBloom = true;
    bool enableGrain = true;
    bool enableSelectiveColor = false;
    double selectiveAmount = 0, selectiveHue = 0, selectiveRange = 15, selectiveSoftness = 15, selectiveSaturation = .25;
    int selectiveView = 0;
    double exposure = 0.0;
    double temperature = 0.0;
    double tint = 0.0;
    double density = 0.18;
    double saturation = 0.95;
    double toe = 0.16;
    double contrast = 1.08;
    double printColor = printstyle::Presets[printstyle::Standard][printstyle::Color];
    double blackPoint = 0.45;
    double negativeShoulder = 0.50;
    double negativeCrosstalk = 0.35;
    double gamutCompression = 0.50;
    double highlightRetention = 0;
    double skinHue = 0.0;
    double printTone = 0.0;
    double printContrast = 1.0;
    double printRolloff = 0.55;
    double printNeutralize = 0.0;
    double printSaturation = 1.0;
    double printExposure = 0.0;
    double printRed = 0.0;
    double printGreen = 0.0;
    double printBlue = 0.0;
    double halation = 0.0;
    double halationRadius = 1.0;
    double aura = 0.0;
    double grain = 0.16;
    double grainSize = 0.45;
    double grainRoughness = 0.32;
    double grainSoftness = 0.25;
    double grainColor = 0.25;
    double grainShadows = 1.15;
    double grainMidtones = 0.80;
    double grainHighlights = 0.42;
    double negativeColorStrength = 1;
    double negativeToneStrength = 1;
    double printColorStrength = 1;
    double printToneStrength = 1;
    int filmGauge = 0;
    double halationThreshold = 0.48;
    double halationSoftness = 0.52;
    double halationColor = 0.5;
    double auraRadius = 1;
    double pushPull = 0;
    double colorRichness = 0;
    double splitTone = 0;
    double splitHue = 220;
    double splitPivot = 0.46135613;
    double splitWidth = 0.1;
    double splitShadows = 1;
    double splitHighlights = 1;
    double grainStretch = 1;
    double grainRed = 1;
    double grainGreen = 1;
    double grainBlue = 1;
    double bloom = 0;
    double bloomRadius = 1;
    double bloomThreshold = 0.65;
    double bloomSoftness = 0.35;
    double bloomColor = 1;
    double bloomProtection = 0.8;
    int grainSeed = 0;
    float gainR = 1.0f;
    float gainG = 1.0f;
    float gainB = 1.0f;
};

int moduleMask(const Settings& s)
{
    return (s.enableNegative ? film::Negative : 0) | (s.enablePrint ? film::Print : 0) |
           (s.enableDevelopment && (s.pushPull != 0 || s.colorRichness != 0 || s.splitTone != 0) ? film::Development : 0) |
           (s.enableHalation ? film::Halation : 0) | (s.enableAura ? film::Aura : 0) | (s.enableGrain ? film::Grain : 0) | (s.enableBloom ? film::Bloom : 0) |
           (s.enableSelectiveColor && (s.selectiveAmount > 0 || s.selectiveView != 0) ? film::SelectiveColor : 0);
}

bool colorEnabled(const Settings& s)
{
    return (film::modulesForMode(s.mode, moduleMask(s)) & film::Negative) != 0;
}

bool printEnabled(const Settings& s)
{
    return (film::modulesForMode(s.mode, moduleMask(s)) & film::Print) != 0;
}

float halationAmount(const Settings& s)
{
    return (film::modulesForMode(s.mode, moduleMask(s)) & film::Halation) ? static_cast<float>(s.halation) : 0.0f;
}

float auraAmount(const Settings& s)
{
    return (film::modulesForMode(s.mode, moduleMask(s)) & film::Aura) ? static_cast<float>(s.aura) : 0.0f;
}

bool spatialEnabled(const Settings& s)
{
    return halationAmount(s) > 0.0f || auraAmount(s) > 0.0f;
}

bool grainEnabled(const Settings& s)
{
    return (film::modulesForMode(s.mode, moduleMask(s)) & film::Grain) && s.grain > 0.0;
}

bool isIdentitySettings(const Settings& s)
{
    return film::isIdentity(s.mode, moduleMask(s), static_cast<float>(s.halation), static_cast<float>(s.aura), static_cast<float>(s.grain), static_cast<float>(s.bloom), static_cast<float>(s.selectiveAmount), s.selectiveView);
}

float clampf(float v, float lo, float hi)
{
    return std::max(lo, std::min(v, hi));
}

float luma(const Rgb& c)
{
    return c.r * 0.2126f + c.g * 0.7152f + c.b * 0.0722f;
}

Rgb readPixel(OFX::Image* img, int x, int y)
{
    const OfxRectI& b = img->getBounds();
    x = std::max(b.x1, std::min(x, b.x2 - 1));
    y = std::max(b.y1, std::min(y, b.y2 - 1));
    const float* p = static_cast<const float*>(img->getPixelAddress(x, y));
    if (!p) return {0.0f, 0.0f, 0.0f};
    return {p[0], p[1], p[2]};
}

Rgb cameraStage(Rgb c, const Settings& s)
{
    return color_balance(c, {s.gainR, s.gainG, s.gainB});
}

float highlightKey(Rgb c, HalationParameters p)
{
    return halation_key(c.r, c.g, c.b, p);
}

template<class Blur, class Reader>
class SpatialProcessor : public OFX::MultiThread::Processor {
public:
    SpatialProcessor(OFX::ImageEffect& effect, Blur& blur, Reader reader)
        : effect_(effect), blur_(blur), reader_(reader) {}

    void run()
    {
        for (phase_ = 0; phase_ < 3 && !effect_.abort(); ++phase_)
            multiThread(std::max(1u, std::min(OFX::MultiThread::getNumCPUs(), static_cast<unsigned>(blur_.height))));
    }

    void multiThreadFunction(unsigned thread, unsigned threads) override
    {
        const int begin = static_cast<int>(thread * blur_.height / threads);
        const int end = static_cast<int>((thread + 1) * blur_.height / threads);
        for (int y = begin; y < end && !effect_.abort(); ++y) {
            if (phase_ == 0) blur_.extractRows(y, y + 1, reader_);
            else blur_.blurRows(y, y + 1, phase_ == 1);
        }
    }

private:
    OFX::ImageEffect& effect_;
    Blur& blur_;
    Reader reader_;
    int phase_ = 0;
};

class FilmProcessor : public OFX::ImageProcessor {
public:
    explicit FilmProcessor(OFX::ImageEffect& instance) : OFX::ImageProcessor(instance) {}

    void setSrcImg(OFX::Image* src) { src_ = src; }
    void setSettings(const Settings& settings) { settings_ = settings; }
    void setTime(double time) { time_ = time; }

    void preProcess() override
    {
        grain::PackedSettings packed {};
        packSettings(settings_, packed.data());
        const OfxRectI& bounds = src_->getBounds();
        grainParameters_ = grain::prepare(packed.data(), bounds.y2 - bounds.y1, time_);
        colorParameters_ = color::prepare(packed.data());
        responseParameters_ = response::prepare(packed.data());
        haloConfig_ = halation::prepare(packed.data(), bounds.y2 - bounds.y1);
        bloomConfig_ = bloom::prepare(packed.data(), bounds.y2 - bounds.y1);
        if (_isEnabledOpenCLRender) return;
        if (spatialEnabled(settings_)) {
            const halation::Filter filter(halationAmount(settings_), static_cast<float>(settings_.halationRadius), auraAmount(settings_),
                                          haloConfig_.auraRadius, haloConfig_.radiusScale);
            blur_ = std::make_unique<halation::Blur>(bounds.x2 - bounds.x1, bounds.y2 - bounds.y1, filter, haloConfig_.downsample);
            SpatialProcessor(_effect, *blur_, [&](int x, int y) {
                return highlightKey(color_to_work(readPixel(src_, x + bounds.x1, y + bounds.y1), colorParameters_), haloConfig_.key);
            }).run();
        }
        if (bloomConfig_.parameters.amount > 0) {
            bloomBlur_ = std::make_unique<bloom::Blur>(bounds.x2 - bounds.x1, bounds.y2 - bounds.y1, bloomConfig_);
            SpatialProcessor(_effect, *bloomBlur_, [&](int x, int y) {
                return bloom_extract(readPixel(src_, x + bounds.x1, y + bounds.y1), colorParameters_, bloomConfig_.parameters);
            }).run();
        }
    }

    void processImagesOpenCL() override
    {
        const OfxRectI& bounds = src_->getBounds();
        const int width = bounds.x2 - bounds.x1;
        const int height = bounds.y2 - bounds.y1;
        float packed[film::SettingsCount];
        packSettings(settings_, packed);
        const bool ok = RunOpenEmulsionOpenCL(
            _pOpenCLCmdQ,
            width,
            height,
            time_,
            packed,
            static_cast<const float*>(src_->getPixelData()),
            static_cast<float*>(_dstImg->getPixelData()));
        if (!ok) {
            OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
        }
    }

    void multiThreadProcessImages(OfxRectI window) override
    {
        const bool identity = isIdentitySettings(settings_);
        const int modules = film::modulesForMode(settings_.mode, moduleMask(settings_));
        const bool clampOutput = response_clamps_negative(modules, responseParameters_);
        for (int y = window.y1; y < window.y2; ++y) {
            if (_effect.abort()) break;
            float* dstPix = static_cast<float*>(_dstImg->getPixelAddress(window.x1, y));
            for (int x = window.x1; x < window.x2; ++x) {
                const float* srcPix = static_cast<const float*>(src_ ? src_->getPixelAddress(x, y) : nullptr);
                if (srcPix && dstPix) {
                    const Rgb original { srcPix[0], srcPix[1], srcPix[2] };
                    if (identity) {
                        std::copy_n(srcPix, 4, dstPix);
                        dstPix += 4;
                        continue;
                    }
                    const Rgb work = color_to_work(original, colorParameters_);
                    Rgb c = work;
                    Rgb selectionSource = work;
                    if (colorEnabled(settings_)) {
                        c = cameraStage(c, settings_);
                        selectionSource = c;
                    }
                    c = color_render_work(c, colorParameters_);
                    if (colorEnabled(settings_)) c = response_negative_stage(c, responseParameters_);
                    if (modules & film::Development) c = response_development(c, responseParameters_);
                    if (grainParameters_.prePrint && grainEnabled(settings_)) {
                        const OfxRectI& b = src_->getBounds();
                        const auto delta = grain_delta(x - b.x1, y - b.y1, luma(c), grainParameters_);
                        c.r += delta.r; c.g += delta.g; c.b += delta.b;
                    }
                    if (printEnabled(settings_)) {
                        c = response_print(c, responseParameters_);
                    }
                    if (spatialEnabled(settings_)) {
                        const OfxRectI& b = src_->getBounds();
                        const float signal = halation::signal(blur_->sample(x - b.x1, y - b.y1), highlightKey(work, haloConfig_.key),
                                                              halationAmount(settings_), auraAmount(settings_));
                        if (settings_.mode == 5) {
                            const float h = clampf(signal * 2.0f, 0.0f, 1.0f);
                            c = {h, h * 0.55f, h * 0.10f};
                        } else {
                            c.r += signal * haloConfig_.key.red;
                            c.g += signal * haloConfig_.key.green;
                            c.b += signal * haloConfig_.key.blue;
                        }
                    } else if (settings_.mode == 5) {
                        c = {0.0f, 0.0f, 0.0f};
                    }
                    if (bloomConfig_.parameters.amount > 0 || settings_.mode == 6) {
                        const OfxRectI& b = src_->getBounds();
                        const Rgb glow = bloomBlur_ ? bloomBlur_->sample(x - b.x1, y - b.y1) : Rgb{0,0,0};
                        c = bloom_composite(c, glow, bloomConfig_.parameters, settings_.mode == 6);
                    }
                    if (grainEnabled(settings_) && !grainParameters_.prePrint) {
                        const OfxRectI& b = src_->getBounds();
                        const GrainVector delta = grain_delta(x - b.x1, y - b.y1, luma(c), grainParameters_);
                        c.r += delta.r;
                        c.g += delta.g;
                        c.b += delta.b;
                    }
                    c = response_finish(c, modules, responseParameters_);
                    if (modules & film::SelectiveColor) c = response_selective(c, selectionSource, responseParameters_);
                    if (clampOutput) c = {std::max(c.r, 0.0f), std::max(c.g, 0.0f), std::max(c.b, 0.0f)};
                    if (settings_.mode != 5 && settings_.mode != 6 && !((modules & film::SelectiveColor) && settings_.selectiveView == 1)) {
                        const bool unchanged = !(modules & (film::Negative | film::Development | film::Print | film::SelectiveColor)) && c.r == work.r && c.g == work.g && c.b == work.b;
                        c = unchanged ? original : color_from_work(c, colorParameters_);
                    }
                    dstPix[0] = c.r;
                    dstPix[1] = c.g;
                    dstPix[2] = c.b;
                    dstPix[3] = srcPix[3];
                } else if (dstPix) {
                    dstPix[0] = dstPix[1] = dstPix[2] = dstPix[3] = 0.0f;
                }
                dstPix += 4;
            }
        }
    }

private:
    static void packSettings(const Settings& s, float* out)
    {
        out[0] = static_cast<float>(s.mode);
        out[1] = static_cast<float>(s.system);
        out[2] = static_cast<float>(s.printStyle);
        out[3] = static_cast<float>(s.grainStyle);
        out[4] = s.gainR;
        out[5] = s.gainG;
        out[6] = s.gainB;
        out[7] = static_cast<float>(s.density);
        out[8] = static_cast<float>(s.saturation);
        out[9] = static_cast<float>(s.toe);
        out[10] = static_cast<float>(s.contrast);
        out[11] = static_cast<float>(s.printColor);
        out[12] = static_cast<float>(s.blackPoint);
        out[13] = static_cast<float>(s.halation);
        out[14] = static_cast<float>(s.halationRadius);
        out[15] = static_cast<float>(s.aura);
        out[16] = static_cast<float>(s.grain);
        out[17] = static_cast<float>(s.grainSize);
        out[18] = static_cast<float>(s.grainRoughness);
        out[19] = static_cast<float>(moduleMask(s));
        out[20] = static_cast<float>(s.grainSoftness);
        out[21] = static_cast<float>(s.grainColor);
        out[22] = static_cast<float>(s.grainShadows);
        out[23] = static_cast<float>(s.grainMidtones);
        out[24] = static_cast<float>(s.grainHighlights);
        out[25] = static_cast<float>(s.grainSeed);
        out[26] = static_cast<float>(s.sourceSpace);
        out[27] = static_cast<float>(s.outputSpace);
        out[film::OutputRendering] = static_cast<float>(s.outputRendering);
        out[film::HDRPeak] = static_cast<float>(s.hdrPeak);
        out[film::HDRWhite] = static_cast<float>(s.hdrWhite);
        out[film::SDRContrast] = static_cast<float>(s.sdrContrast);
        out[film::SDRRolloff] = static_cast<float>(s.sdrRolloff);
        out[film::SDRGamut] = static_cast<float>(s.sdrGamut);
        out[film::HighlightRetention] = static_cast<float>(s.highlightRetention);
        out[28] = static_cast<float>(s.negativeShoulder);
        out[29] = static_cast<float>(s.negativeCrosstalk);
        out[30] = static_cast<float>(s.gamutCompression);
        out[31] = static_cast<float>(s.skinHue);
        out[32] = static_cast<float>(s.printTone);
        out[33] = static_cast<float>(s.printContrast);
        out[34] = static_cast<float>(s.printRolloff);
        out[35] = static_cast<float>(s.printNeutralize);
        out[36] = static_cast<float>(s.printSaturation);
        out[37] = static_cast<float>(s.printExposure);
        out[38] = static_cast<float>(s.printRed);
        out[39] = static_cast<float>(s.printGreen);
        out[40] = static_cast<float>(s.printBlue);
        out[film::NegativeColorStrength] = static_cast<float>(s.negativeColorStrength);
        out[film::NegativeToneStrength] = static_cast<float>(s.negativeToneStrength);
        out[film::PrintColorStrength] = static_cast<float>(s.printColorStrength);
        out[film::PrintToneStrength] = static_cast<float>(s.printToneStrength);
        out[film::FilmGauge] = static_cast<float>(s.filmGauge);
        out[film::HalationThreshold] = static_cast<float>(s.halationThreshold);
        out[film::HalationSoftness] = static_cast<float>(s.halationSoftness);
        out[film::HalationColor] = static_cast<float>(s.halationColor);
        out[film::AuraRadius] = static_cast<float>(s.auraRadius);
        out[film::PushPull] = static_cast<float>(s.pushPull);
        out[film::ColorRichness] = static_cast<float>(s.colorRichness);
        out[film::SplitTone] = static_cast<float>(s.splitTone);
        out[film::SplitHue] = static_cast<float>(s.splitHue);
        out[film::SplitPivot] = static_cast<float>(s.splitPivot);
        out[film::SplitWidth] = static_cast<float>(s.splitWidth);
        out[film::SplitShadows] = static_cast<float>(s.splitShadows);
        out[film::SplitHighlights] = static_cast<float>(s.splitHighlights);
        out[film::GrainStretch] = static_cast<float>(s.grainStretch);
        out[film::GrainRed] = static_cast<float>(s.grainRed);
        out[film::GrainGreen] = static_cast<float>(s.grainGreen);
        out[film::GrainBlue] = static_cast<float>(s.grainBlue);
        out[film::BloomAmount] = static_cast<float>(s.bloom);
        out[film::BloomRadius] = static_cast<float>(s.bloomRadius);
        out[film::BloomThreshold] = static_cast<float>(s.bloomThreshold);
        out[film::BloomSoftness] = static_cast<float>(s.bloomSoftness);
        out[film::BloomColor] = static_cast<float>(s.bloomColor);
        out[film::BloomProtection] = static_cast<float>(s.bloomProtection);
        out[film::GrainResponse] = static_cast<float>(s.grainResponse);
        out[film::SelectiveAmount] = static_cast<float>(s.selectiveAmount);
        out[film::SelectiveHue] = static_cast<float>(s.selectiveHue);
        out[film::SelectiveRange] = static_cast<float>(s.selectiveRange);
        out[film::SelectiveSoftness] = static_cast<float>(s.selectiveSoftness);
        out[film::SelectiveSaturation] = static_cast<float>(s.selectiveSaturation);
        out[film::SelectiveView] = static_cast<float>(s.selectiveView);
    }

    OFX::Image* src_ = nullptr;
    Settings settings_;
    double time_ = 0.0;
    std::unique_ptr<halation::Blur> blur_;
    GrainParameters grainParameters_ {};
    ColorParameters colorParameters_ {};
    FilmResponseParameters responseParameters_ {};
    halation::Configuration haloConfig_ {};
    bloom::Configuration bloomConfig_ {};
    std::unique_ptr<bloom::Blur> bloomBlur_;
};

class OpenEmulsionPlugin : public OFX::ImageEffect {
public:
    explicit OpenEmulsionPlugin(OfxImageEffectHandle handle)
        : ImageEffect(handle)
    {
        dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
        srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
        mode_ = fetchChoiceParam("mode");
        sourceSpace_ = fetchChoiceParam("sourceSpace");
        outputSpace_ = fetchChoiceParam("outputSpace");
        outputRendering_ = fetchChoiceParam("outputRendering");
        hdrPeak_ = fetchDoubleParam("hdrPeak");
        hdrWhite_ = fetchDoubleParam("hdrWhite");
        sdrContrast_ = fetchDoubleParam("sdrContrast");
        sdrRolloff_ = fetchDoubleParam("sdrRolloff");
        sdrGamut_ = fetchDoubleParam("sdrGamut");
        system_ = fetchChoiceParam("system");
        printStyle_ = fetchChoiceParam("printStyle");
        grainStyle_ = fetchChoiceParam("grainStyle");
        grainResponse_ = fetchChoiceParam("grainResponse");
        enableNegative_ = fetchBooleanParam("enableNegative");
        enableDevelopment_ = fetchBooleanParam("enableDevelopment");
        enablePrint_ = fetchBooleanParam("enablePrint");
        enableHalation_ = fetchBooleanParam("enableHalation");
        enableAura_ = fetchBooleanParam("enableAura");
        enableBloom_ = fetchBooleanParam("enableBloom");
        enableGrain_ = fetchBooleanParam("enableGrain");
        exposure_ = fetchDoubleParam("exposure");
        temperature_ = fetchDoubleParam("temperature");
        tint_ = fetchDoubleParam("tint");
        density_ = fetchDoubleParam("density");
        saturation_ = fetchDoubleParam("saturation");
        toe_ = fetchDoubleParam("toe");
        contrast_ = fetchDoubleParam("contrast");
        printColor_ = fetchDoubleParam("printColor");
        blackPoint_ = fetchDoubleParam("blackPoint");
        negativeShoulder_ = fetchDoubleParam("negativeShoulder");
        negativeCrosstalk_ = fetchDoubleParam("negativeCrosstalk");
        gamutCompression_ = fetchDoubleParam("gamutCompression");
        highlightRetention_ = fetchDoubleParam("highlightRetention");
        skinHue_ = fetchDoubleParam("skinHue");
        printTone_ = fetchDoubleParam("printTone");
        printContrast_ = fetchDoubleParam("printContrast");
        printRolloff_ = fetchDoubleParam("printRolloff");
        printNeutralize_ = fetchDoubleParam("printNeutralize");
        printSaturation_ = fetchDoubleParam("printSaturation");
        printExposure_ = fetchDoubleParam("printExposure");
        printRed_ = fetchDoubleParam("printRed");
        printGreen_ = fetchDoubleParam("printGreen");
        printBlue_ = fetchDoubleParam("printBlue");
        halation_ = fetchDoubleParam("halation");
        halationRadius_ = fetchDoubleParam("halationRadius");
        aura_ = fetchDoubleParam("aura");
        grain_ = fetchDoubleParam("grain");
        grainSize_ = fetchDoubleParam("grainSize");
        grainRoughness_ = fetchDoubleParam("grainRoughness");
        grainSoftness_ = fetchDoubleParam("grainSoftness");
        grainColor_ = fetchDoubleParam("grainColor");
        grainShadows_ = fetchDoubleParam("grainShadows");
        grainMidtones_ = fetchDoubleParam("grainMidtones");
        grainHighlights_ = fetchDoubleParam("grainHighlights");
        grainSeed_ = fetchIntParam("grainSeed");
        negativeColorStrength_ = fetchDoubleParam("negativeColorStrength");
        negativeToneStrength_ = fetchDoubleParam("negativeToneStrength");
        printColorStrength_ = fetchDoubleParam("printColorStrength");
        printToneStrength_ = fetchDoubleParam("printToneStrength");
        filmGauge_ = fetchChoiceParam("filmGauge");
        lookPreset_ = fetchChoiceParam("lookPreset");
        presetCategory_ = fetchChoiceParam("presetCategory");
        presetBrowser_ = fetchChoiceParam("presetBrowser");
        for (size_t i = 0; i < std::size(look::Controls); ++i) {
            const auto& control = look::Controls[i];
            if (control.kind == look::Choice) lookControls_[i].choice = fetchChoiceParam(control.name);
            else lookControls_[i].number = fetchDoubleParam(control.name);
        }
        enableSelectiveColor_ = fetchBooleanParam("enableSelectiveColor");
        selectiveAmount_ = fetchDoubleParam("selectiveAmount");
        selectiveHue_ = fetchDoubleParam("selectiveHue");
        selectiveRange_ = fetchDoubleParam("selectiveRange");
        selectiveSoftness_ = fetchDoubleParam("selectiveSoftness");
        selectiveSaturation_ = fetchDoubleParam("selectiveSaturation");
        selectiveView_ = fetchChoiceParam("selectiveView");
        for (size_t i = 0; i < std::size(userpreset::ContextControls); ++i) {
            const auto& control = userpreset::ContextControls[i];
            if (control.kind == userpreset::Choice) contextControls_[i].choice = fetchChoiceParam(control.name);
            else if (control.kind == userpreset::Integer) contextControls_[i].integer = fetchIntParam(control.name);
            else contextControls_[i].number = fetchDoubleParam(control.name);
        }
        preserveSpaces_ = fetchBooleanParam("presetPreserveSpaces");
        preserveCamera_ = fetchBooleanParam("presetPreserveCamera");
        preserveSeed_ = fetchBooleanParam("presetPreserveSeed");
        halationThreshold_ = fetchDoubleParam("halationThreshold");
        halationSoftness_ = fetchDoubleParam("halationSoftness");
        halationColor_ = fetchDoubleParam("halationColor");
        auraRadius_ = fetchDoubleParam("auraRadius");
        pushPull_ = fetchDoubleParam("pushPull");
        colorRichness_ = fetchDoubleParam("colorRichness");
        splitTone_ = fetchDoubleParam("splitTone");
        splitHue_ = fetchDoubleParam("splitHue");
        splitPivot_ = fetchDoubleParam("splitPivot");
        splitWidth_ = fetchDoubleParam("splitWidth");
        splitShadows_ = fetchDoubleParam("splitShadows");
        splitHighlights_ = fetchDoubleParam("splitHighlights");
        grainStretch_ = fetchDoubleParam("grainStretch");
        grainRed_ = fetchDoubleParam("grainRed");
        grainGreen_ = fetchDoubleParam("grainGreen");
        grainBlue_ = fetchDoubleParam("grainBlue");
        bloom_ = fetchDoubleParam("bloom");
        bloomRadius_ = fetchDoubleParam("bloomRadius");
        bloomThreshold_ = fetchDoubleParam("bloomThreshold");
        bloomSoftness_ = fetchDoubleParam("bloomSoftness");
        bloomColor_ = fetchDoubleParam("bloomColor");
        bloomProtection_ = fetchDoubleParam("bloomProtection");
        for (int control = 0; control < printstyle::ControlCount; ++control)
            printRecipeControls_[control] = fetchDoubleParam(printstyle::Parameters[control].name);
        moduleToggles_ = {{enableNegative_, enableDevelopment_, enablePrint_, enableHalation_,
                          enableAura_, enableBloom_, enableGrain_, enableSelectiveColor_}};
        for (size_t i = 0; i < std::size(moduleui::Controls); ++i)
            moduleControls_[i] = getParam(moduleui::Controls[i].name);
        printStyle_->getValue(printStyleForUI_);
        updatePresetBrowser();
        updateControlState();
    }

    void changedParam(const OFX::InstanceChangedArgs& args, const std::string& name) override
    {
        if (applyingControls_) return;
        if (name == "presetCategory") {
            updatePresetBrowser();
            return;
        }
        if (name == "presetBrowser") {
            if (args.reason != OFX::eChangeUserEdit) {
                updatePresetBrowser();
                return;
            }
            int category = 0, option = 0;
            presetCategory_->getValue(category);
            presetBrowser_->getValue(option);
            applyLookPreset(look::presetAt(category,option));
            return;
        }
        if (name == "saveUserPreset" || name == "loadUserPreset") {
            if (args.reason != OFX::eChangeUserEdit) return;
            try {
                const bool save = name == "saveUserPreset";
                // Snapshot before the modal dialog can pump host notifications or change UI context.
                auto preset = save ? userpreset::captureCurrent(lookControls_,moduleToggles_,contextControls_) : userpreset::Snapshot {};
                const auto path = userpreset::chooseFile(save);
                if (!path) return;
                if (save) {
                    preset.name = path->stem().u8string();
                    userpreset::saveFile(*path,preset);
                } else {
                    // Parse and validate the complete file before touching any host parameters.
                    const auto preset = userpreset::loadFile(*path);
                    applyUserPreset(preset);
                }
            } catch (const std::exception& error) {
                sendMessage(OFX::Message::eMessageError,"userPreset",std::string("OpenEmulsion preset: ") + error.what());
            }
            return;
        }
        int selected = look::Custom;
        lookPreset_->getValue(selected);
        const auto action = look::editAction(name, args.reason == OFX::eChangeUserEdit, applyingControls_, selected);
        if (action == look::Apply) {
            applyLookPreset(selected);
            return;
        }
        if (action == look::MarkCustom) {
            // Only the label changes; the edited recipe remains in the ordinary OFX controls.
            lookPreset_->setValue(look::Custom);
            updatePresetBrowser();
        }
        if (name == "lookPreset") updatePresetBrowser();
        const bool affectsControls = name == "mode" || name == "printStyle" || name == "system" ||
            name == "negativeColorStrength" || name == "sourceSpace" || name == "outputSpace" ||
            name == "outputRendering" || name == "selectiveView" || name == "selectiveAmount" ||
            name == "pushPull" || name == "colorRichness" || name == "splitTone" || args.reason == OFX::eChangeTime ||
            std::any_of(moduleui::Toggles.begin(), moduleui::Toggles.end(), [&](const auto& toggle) { return name == toggle.name; });
        if (!affectsControls) return;
        if (name == "mode" && args.reason == OFX::eChangeUserEdit) {
            int mode = 0;
            mode_->getValueAtTime(args.time, mode);
            const bool selectiveEnabled = enableSelectiveColor_->getValueAtTime(args.time);
            beginEditBlock("Apply processing mode");
            applyingControls_ = true;
            try {
                moduleui::applyMode(mode, [&](size_t i, bool enabled) {
                    auto* toggle = moduleToggles_[i];
                    if (toggle->getValueAtTime(args.time) == enabled) return;
                    if (toggle->getNumKeys() != 0) toggle->setValueAtTime(args.time, enabled);
                    else toggle->setValue(enabled);
                }, selectiveEnabled);
            } catch (...) {
                applyingControls_ = false;
                endEditBlock();
                throw;
            }
            applyingControls_ = false;
            endEditBlock();
        }
        updateControlState();
        if (name != "printStyle") return;
        int style = printstyle::Standard;
        printStyle_->getValue(style);
        const int previousStyle = printStyleForUI_;
        printStyleForUI_ = style;
        // Undo/redo and time notifications must not rewrite restored parameters.
        if (args.reason != OFX::eChangeUserEdit) return;
        const int recipeStyle = printstyle::isCustom(style) ? previousStyle : style;
        if (printstyle::isCustom(recipeStyle)) return;
        beginEditBlock("Apply print style");
        applyingControls_ = true;
        try {
            printstyle::applyPreset(recipeStyle, [&](int control, double value) {
                // A fixed recipe replaces Custom tuning, including its animation.
                printRecipeControls_[control]->deleteAllKeys();
                printRecipeControls_[control]->setValue(value);
            });
        } catch (...) {
            applyingControls_ = false;
            endEditBlock();
            throw;
        }
        applyingControls_ = false;
        endEditBlock();
    }

    void render(const OFX::RenderArguments& args) override
    {
        if (dstClip_->getPixelDepth() != OFX::eBitDepthFloat || dstClip_->getPixelComponents() != OFX::ePixelComponentRGBA) {
            OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
        }
        std::unique_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));
        std::unique_ptr<OFX::Image> src(srcClip_->fetchImage(args.time));
        if (!dst || !src) {
            OFX::throwSuiteStatusException(kOfxStatFailed);
        }
        if (src->getPixelDepth() != dst->getPixelDepth() || src->getPixelComponents() != dst->getPixelComponents()) {
            OFX::throwSuiteStatusException(kOfxStatErrValue);
        }

        const Settings settings = settingsAt(args.time);
        FilmProcessor processor(*this);
        processor.setDstImg(dst.get());
        processor.setSrcImg(src.get());
        processor.setGPURenderArgs(args);
        processor.setRenderWindow(args.renderWindow);
        processor.setSettings(settings);
        processor.setTime(args.time);
        processor.process();
    }

    bool isIdentity(const OFX::IsIdentityArguments& args, OFX::Clip*& identityClip, double& identityTime) override
    {
        const Settings s = settingsAt(args.time);
        if (isIdentitySettings(s)) {
            identityClip = srcClip_;
            identityTime = args.time;
            return true;
        }
        return false;
    }

private:
    void updatePresetBrowser()
    {
        int category = 0, selected = look::Custom;
        presetCategory_->getValue(category);
        lookPreset_->getValue(selected);
        category = std::clamp(category,0,look::CategoryCount-1);
        const bool previousGuard = applyingControls_;
        applyingControls_ = true;
        try {
            if (browserCategory_ != category) {
                presetBrowser_->resetOptions();
                for (int option = 0; option < look::optionCount(category); ++option) {
                    const int preset = look::presetAt(category,option);
                    presetBrowser_->appendOption(preset == look::Custom ? "Custom / Current Settings" : look::Labels[preset]);
                }
                browserCategory_ = category;
            }
            presetBrowser_->setValue(look::optionFor(category,selected));
        } catch (...) {
            applyingControls_ = previousGuard;
            throw;
        }
        applyingControls_ = previousGuard;
    }

    void applyLookPreset(int selected)
    {
        beginEditBlock("Apply look preset");
        applyingControls_ = true;
        try {
            lookPreset_->setValue(selected);
            look::applyPreset(selected, [&](size_t i, double value) {
                setControl(lookControls_[i],value);
            }, [&](size_t i, bool enabled) {
                moduleToggles_[i]->deleteAllKeys();
                moduleToggles_[i]->setValue(enabled);
            });
            printStyle_->getValue(printStyleForUI_);
        } catch (...) {
            applyingControls_ = false;
            endEditBlock();
            throw;
        }
        applyingControls_ = false;
        endEditBlock();
        updatePresetBrowser();
        updateControlState();
    }

    void applyUserPreset(const userpreset::Snapshot& preset)
    {
        const userpreset::ImportOptions options {preserveSpaces_->getValue(),preserveCamera_->getValue(),preserveSeed_->getValue()};
        beginEditBlock("Load user preset");
        applyingControls_ = true;
        try {
            userpreset::apply(preset,options,[&](size_t i, double value) { setControl(lookControls_[i],value); },
                [&](size_t i, bool enabled) {
                    moduleToggles_[i]->deleteAllKeys();
                    moduleToggles_[i]->setValue(enabled);
                }, [&](size_t i, double value) { setControl(contextControls_[i],value); });
            lookPreset_->setValue(look::Custom);
            printStyle_->getValue(printStyleForUI_);
        } catch (...) {
            applyingControls_ = false;
            endEditBlock();
            throw;
        }
        applyingControls_ = false;
        endEditBlock();
        updatePresetBrowser();
        updateControlState();
    }

    void updateControlState()
    {
        int mode = 0, style = printstyle::Standard, enabled = 0, system = 0;
        double colorStrength = 1;
        mode_->getValue(mode);
        printStyle_->getValue(style);
        system_->getValue(system);
        negativeColorStrength_->getValue(colorStrength);
        for (size_t i = 0; i < moduleToggles_.size(); ++i) {
            if (moduleToggles_[i]->getValue()) enabled |= moduleui::Toggles[i].module;
            moduleToggles_[i]->setEnabled(moduleui::controlEnabled(mode, film::All, moduleui::Toggles[i].module));
        }
        for (size_t i = 0; i < moduleControls_.size(); ++i)
            moduleControls_[i]->setEnabled(moduleui::controlEnabled(mode, enabled, moduleui::Controls[i].modules) &&
                moduleui::semanticControlEnabled(moduleui::Controls[i].name, mode, enabled, system, colorStrength));
        for (auto* parameter : printRecipeControls_)
            parameter->setEnabled(moduleui::printRecipeEnabled(mode, enabled, style));
        Settings s;
        sourceSpace_->getValue(s.sourceSpace); outputSpace_->getValue(s.outputSpace);
        outputRendering_->getValue(s.outputRendering); selectiveView_->getValue(s.selectiveView);
        pushPull_->getValue(s.pushPull); colorRichness_->getValue(s.colorRichness);
        splitTone_->getValue(s.splitTone); selectiveAmount_->getValue(s.selectiveAmount);
        int active = film::modulesForMode(mode,enabled);
        if (s.pushPull == 0 && s.colorRichness == 0 && s.splitTone == 0) active &= ~film::Development;
        if (s.selectiveAmount == 0 && s.selectiveView == 0) active &= ~film::SelectiveColor;
        const bool textureOnly = !(active & (film::Negative|film::Print|film::Development|film::SelectiveColor));
        const auto cp = color::prepare(s.sourceSpace,s.outputSpace,textureOnly,s.outputRendering,
                                       static_cast<float>(s.hdrPeak),static_cast<float>(s.hdrWhite));
        highlightRetention_->setEnabled(color::highlightRetentionEnabled(cp,active,system,
                                                                         static_cast<float>(colorStrength),s.selectiveView));
        const bool whiteEnabled = color::hdrWhiteEnabled(cp,active,s.selectiveView);
        hdrWhite_->setEnabled(whiteEnabled);
        hdrPeak_->setEnabled(whiteEnabled && cp.renderHDR);
        const bool sdrEnabled = color::sdrControlsEnabled(cp,active,s.selectiveView);
        sdrContrast_->setEnabled(sdrEnabled);
        sdrRolloff_->setEnabled(sdrEnabled);
        sdrGamut_->setEnabled(sdrEnabled);
    }

    Settings settingsAt(double time) const
    {
        Settings s;
        mode_->getValueAtTime(time, s.mode);
        sourceSpace_->getValueAtTime(time, s.sourceSpace);
        outputSpace_->getValueAtTime(time, s.outputSpace);
        outputRendering_->getValueAtTime(time, s.outputRendering);
        s.hdrPeak = hdrPeak_->getValueAtTime(time);
        s.hdrWhite = hdrWhite_->getValueAtTime(time);
        s.sdrContrast = sdrContrast_->getValueAtTime(time);
        s.sdrRolloff = sdrRolloff_->getValueAtTime(time);
        s.sdrGamut = sdrGamut_->getValueAtTime(time);
        system_->getValueAtTime(time, s.system);
        printStyle_->getValueAtTime(time, s.printStyle);
        grainStyle_->getValueAtTime(time, s.grainStyle);
        grainResponse_->getValueAtTime(time, s.grainResponse);
        s.enableNegative = enableNegative_->getValueAtTime(time);
        s.enableDevelopment = enableDevelopment_->getValueAtTime(time);
        s.enablePrint = enablePrint_->getValueAtTime(time);
        s.enableHalation = enableHalation_->getValueAtTime(time);
        s.enableAura = enableAura_->getValueAtTime(time);
        s.enableBloom = enableBloom_->getValueAtTime(time);
        s.enableGrain = enableGrain_->getValueAtTime(time);
        s.enableSelectiveColor = enableSelectiveColor_->getValueAtTime(time);
        s.selectiveAmount = selectiveAmount_->getValueAtTime(time);
        s.selectiveHue = selectiveHue_->getValueAtTime(time);
        s.selectiveRange = selectiveRange_->getValueAtTime(time);
        s.selectiveSoftness = selectiveSoftness_->getValueAtTime(time);
        s.selectiveSaturation = selectiveSaturation_->getValueAtTime(time);
        selectiveView_->getValueAtTime(time,s.selectiveView);
        s.exposure = exposure_->getValueAtTime(time);
        s.temperature = temperature_->getValueAtTime(time);
        s.tint = tint_->getValueAtTime(time);
        s.density = density_->getValueAtTime(time);
        s.saturation = saturation_->getValueAtTime(time);
        s.toe = toe_->getValueAtTime(time);
        s.contrast = contrast_->getValueAtTime(time);
        s.printColor = printColor_->getValueAtTime(time);
        s.blackPoint = blackPoint_->getValueAtTime(time);
        s.negativeShoulder = negativeShoulder_->getValueAtTime(time);
        s.negativeCrosstalk = negativeCrosstalk_->getValueAtTime(time);
        s.gamutCompression = gamutCompression_->getValueAtTime(time);
        s.highlightRetention = highlightRetention_->getValueAtTime(time);
        s.skinHue = skinHue_->getValueAtTime(time);
        s.printTone = printTone_->getValueAtTime(time);
        s.printContrast = printContrast_->getValueAtTime(time);
        s.printRolloff = printRolloff_->getValueAtTime(time);
        s.printNeutralize = printNeutralize_->getValueAtTime(time);
        s.printSaturation = printSaturation_->getValueAtTime(time);
        s.printExposure = printExposure_->getValueAtTime(time);
        s.printRed = printRed_->getValueAtTime(time);
        s.printGreen = printGreen_->getValueAtTime(time);
        s.printBlue = printBlue_->getValueAtTime(time);
        s.halation = halation_->getValueAtTime(time);
        s.halationRadius = halationRadius_->getValueAtTime(time);
        s.aura = aura_->getValueAtTime(time);
        s.grain = grain_->getValueAtTime(time);
        s.grainSize = grainSize_->getValueAtTime(time);
        s.grainRoughness = grainRoughness_->getValueAtTime(time);
        s.grainSoftness = grainSoftness_->getValueAtTime(time);
        s.grainColor = grainColor_->getValueAtTime(time);
        s.grainShadows = grainShadows_->getValueAtTime(time);
        s.grainMidtones = grainMidtones_->getValueAtTime(time);
        s.grainHighlights = grainHighlights_->getValueAtTime(time);
        s.grainSeed = grainSeed_->getValueAtTime(time);
        s.negativeColorStrength = negativeColorStrength_->getValueAtTime(time);
        s.negativeToneStrength = negativeToneStrength_->getValueAtTime(time);
        s.printColorStrength = printColorStrength_->getValueAtTime(time);
        s.printToneStrength = printToneStrength_->getValueAtTime(time);
        filmGauge_->getValueAtTime(time, s.filmGauge);
        s.halationThreshold = halationThreshold_->getValueAtTime(time);
        s.halationSoftness = halationSoftness_->getValueAtTime(time);
        s.halationColor = halationColor_->getValueAtTime(time);
        s.auraRadius = auraRadius_->getValueAtTime(time);
        s.pushPull = pushPull_->getValueAtTime(time);
        s.colorRichness = colorRichness_->getValueAtTime(time);
        s.splitTone = splitTone_->getValueAtTime(time);
        s.splitHue = splitHue_->getValueAtTime(time);
        s.splitPivot = splitPivot_->getValueAtTime(time);
        s.splitWidth = splitWidth_->getValueAtTime(time);
        s.splitShadows = splitShadows_->getValueAtTime(time);
        s.splitHighlights = splitHighlights_->getValueAtTime(time);
        s.grainStretch = grainStretch_->getValueAtTime(time);
        s.grainRed = grainRed_->getValueAtTime(time);
        s.grainGreen = grainGreen_->getValueAtTime(time);
        s.grainBlue = grainBlue_->getValueAtTime(time);
        s.bloom = bloom_->getValueAtTime(time);
        s.bloomRadius = bloomRadius_->getValueAtTime(time);
        s.bloomThreshold = bloomThreshold_->getValueAtTime(time);
        s.bloomSoftness = bloomSoftness_->getValueAtTime(time);
        s.bloomColor = bloomColor_->getValueAtTime(time);
        s.bloomProtection = bloomProtection_->getValueAtTime(time);
        const auto balance = color::cameraBalance(s.exposure, s.temperature, s.tint);
        s.gainR = balance.r; s.gainG = balance.g; s.gainB = balance.b;
        return s;
    }

    OFX::Clip* dstClip_ = nullptr;
    OFX::Clip* srcClip_ = nullptr;
    OFX::ChoiceParam* mode_ = nullptr;
    OFX::ChoiceParam* sourceSpace_ = nullptr;
    OFX::ChoiceParam* outputSpace_ = nullptr;
    OFX::ChoiceParam* outputRendering_ = nullptr;
    OFX::DoubleParam* hdrPeak_ = nullptr;
    OFX::DoubleParam* sdrContrast_ = nullptr;
    OFX::DoubleParam* sdrRolloff_ = nullptr;
    OFX::DoubleParam* sdrGamut_ = nullptr;
    OFX::DoubleParam* hdrWhite_ = nullptr;
    OFX::ChoiceParam* system_ = nullptr;
    OFX::ChoiceParam* printStyle_ = nullptr;
    std::array<OFX::DoubleParam*, printstyle::ControlCount> printRecipeControls_ {};
    int printStyleForUI_ = printstyle::Standard;
    std::array<OFX::BooleanParam*, moduleui::Toggles.size()> moduleToggles_ {};
    std::array<OFX::Param*, std::size(moduleui::Controls)> moduleControls_ {};
    struct LookControl {
        OFX::ChoiceParam* choice = nullptr;
        OFX::DoubleParam* number = nullptr;
        OFX::IntParam* integer = nullptr;
    };
    static void setControl(const LookControl& control, double value)
    {
        if (control.choice) { control.choice->deleteAllKeys(); control.choice->setValue(static_cast<int>(value)); }
        else if (control.integer) { control.integer->deleteAllKeys(); control.integer->setValue(static_cast<int>(value)); }
        else { control.number->deleteAllKeys(); control.number->setValue(value); }
    }
    OFX::ChoiceParam* lookPreset_ = nullptr;
    OFX::ChoiceParam* presetCategory_ = nullptr;
    OFX::ChoiceParam* presetBrowser_ = nullptr;
    int browserCategory_ = -1;
    std::array<LookControl, std::size(look::Controls)> lookControls_ {};
    std::array<LookControl, std::size(userpreset::ContextControls)> contextControls_ {};
    OFX::BooleanParam* preserveSpaces_ = nullptr;
    OFX::BooleanParam* preserveCamera_ = nullptr;
    OFX::BooleanParam* preserveSeed_ = nullptr;
    bool applyingControls_ = false;
    OFX::ChoiceParam* grainStyle_ = nullptr;
    OFX::ChoiceParam* grainResponse_ = nullptr;
    OFX::BooleanParam* enableNegative_ = nullptr;
    OFX::BooleanParam* enableDevelopment_ = nullptr;
    OFX::BooleanParam* enablePrint_ = nullptr;
    OFX::BooleanParam* enableHalation_ = nullptr;
    OFX::BooleanParam* enableAura_ = nullptr;
    OFX::BooleanParam* enableBloom_ = nullptr;
    OFX::BooleanParam* enableGrain_ = nullptr;
    OFX::BooleanParam* enableSelectiveColor_ = nullptr;
    OFX::DoubleParam* selectiveAmount_ = nullptr;
    OFX::DoubleParam* selectiveHue_ = nullptr;
    OFX::DoubleParam* selectiveRange_ = nullptr;
    OFX::DoubleParam* selectiveSoftness_ = nullptr;
    OFX::DoubleParam* selectiveSaturation_ = nullptr;
    OFX::ChoiceParam* selectiveView_ = nullptr;
    OFX::DoubleParam* exposure_ = nullptr;
    OFX::DoubleParam* temperature_ = nullptr;
    OFX::DoubleParam* tint_ = nullptr;
    OFX::DoubleParam* density_ = nullptr;
    OFX::DoubleParam* saturation_ = nullptr;
    OFX::DoubleParam* toe_ = nullptr;
    OFX::DoubleParam* contrast_ = nullptr;
    OFX::DoubleParam* printColor_ = nullptr;
    OFX::DoubleParam* blackPoint_ = nullptr;
    OFX::DoubleParam* negativeShoulder_ = nullptr;
    OFX::DoubleParam* negativeCrosstalk_ = nullptr;
    OFX::DoubleParam* gamutCompression_ = nullptr;
    OFX::DoubleParam* highlightRetention_ = nullptr;
    OFX::DoubleParam* skinHue_ = nullptr;
    OFX::DoubleParam* printTone_ = nullptr;
    OFX::DoubleParam* printContrast_ = nullptr;
    OFX::DoubleParam* printRolloff_ = nullptr;
    OFX::DoubleParam* printNeutralize_ = nullptr;
    OFX::DoubleParam* printSaturation_ = nullptr;
    OFX::DoubleParam* printExposure_ = nullptr;
    OFX::DoubleParam* printRed_ = nullptr;
    OFX::DoubleParam* printGreen_ = nullptr;
    OFX::DoubleParam* printBlue_ = nullptr;
    OFX::DoubleParam* halation_ = nullptr;
    OFX::DoubleParam* halationRadius_ = nullptr;
    OFX::DoubleParam* aura_ = nullptr;
    OFX::DoubleParam* grain_ = nullptr;
    OFX::DoubleParam* grainSize_ = nullptr;
    OFX::DoubleParam* grainRoughness_ = nullptr;
    OFX::DoubleParam* grainSoftness_ = nullptr;
    OFX::DoubleParam* grainColor_ = nullptr;
    OFX::DoubleParam* grainShadows_ = nullptr;
    OFX::DoubleParam* grainMidtones_ = nullptr;
    OFX::DoubleParam* grainHighlights_ = nullptr;
    OFX::IntParam* grainSeed_ = nullptr;
    OFX::DoubleParam* negativeColorStrength_ = nullptr;
    OFX::DoubleParam* negativeToneStrength_ = nullptr;
    OFX::DoubleParam* printColorStrength_ = nullptr;
    OFX::DoubleParam* printToneStrength_ = nullptr;
    OFX::ChoiceParam* filmGauge_ = nullptr;
    OFX::DoubleParam* halationThreshold_ = nullptr;
    OFX::DoubleParam* halationSoftness_ = nullptr;
    OFX::DoubleParam* halationColor_ = nullptr;
    OFX::DoubleParam* auraRadius_ = nullptr;
    OFX::DoubleParam* pushPull_ = nullptr;
    OFX::DoubleParam* colorRichness_ = nullptr;
    OFX::DoubleParam* splitTone_ = nullptr;
    OFX::DoubleParam* splitHue_ = nullptr;
    OFX::DoubleParam* splitPivot_ = nullptr;
    OFX::DoubleParam* splitWidth_ = nullptr;
    OFX::DoubleParam* splitShadows_ = nullptr;
    OFX::DoubleParam* splitHighlights_ = nullptr;
    OFX::DoubleParam* grainStretch_ = nullptr;
    OFX::DoubleParam* grainRed_ = nullptr;
    OFX::DoubleParam* grainGreen_ = nullptr;
    OFX::DoubleParam* grainBlue_ = nullptr;
    OFX::DoubleParam* bloom_ = nullptr;
    OFX::DoubleParam* bloomRadius_ = nullptr;
    OFX::DoubleParam* bloomThreshold_ = nullptr;
    OFX::DoubleParam* bloomSoftness_ = nullptr;
    OFX::DoubleParam* bloomColor_ = nullptr;
    OFX::DoubleParam* bloomProtection_ = nullptr;
};

class OpenEmulsionFactory : public OFX::PluginFactoryHelper<OpenEmulsionFactory> {
public:
    OpenEmulsionFactory()
        : OFX::PluginFactoryHelper<OpenEmulsionFactory>(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor)
    {}

    void load() override {}
    void unload() override {}

    void describe(OFX::ImageEffectDescriptor& desc) override
    {
        desc.setLabels(kPluginName, kPluginName, kPluginName);
        desc.setPluginGrouping(kPluginGrouping);
        desc.setPluginDescription(kPluginDescription);
        desc.addSupportedContext(OFX::eContextFilter);
        desc.addSupportedContext(OFX::eContextGeneral);
        desc.addSupportedBitDepth(OFX::eBitDepthFloat);
        desc.setSingleInstance(false);
        desc.setHostFrameThreading(false);
        desc.setSupportsMultiResolution(false);
        desc.setSupportsTiles(false);
        desc.setTemporalClipAccess(false);
        desc.setRenderTwiceAlways(false);
        desc.setSupportsMultipleClipPARs(false);
        desc.setNoSpatialAwareness(false);
        desc.setSupportsOpenCLRender(true);
    }

    void describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum) override
    {
        using namespace OFX;
        ClipDescriptor* srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
        srcClip->addSupportedComponent(ePixelComponentRGBA);
        srcClip->setTemporalClipAccess(false);
        srcClip->setSupportsTiles(false);
        srcClip->setIsMask(false);

        ClipDescriptor* dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
        dstClip->addSupportedComponent(ePixelComponentRGBA);
        dstClip->setSupportsTiles(false);

        PageParamDescriptor* page = desc.definePageParam("Controls");

        ChoiceParamDescriptor* choice = desc.defineChoiceParam("mode");
        choice->setLabels("Mode", "Mode", "Mode");
        choice->appendOption("Full");
        choice->appendOption("Color Only");
        choice->appendOption("Halation, Bloom & Grain Only");
        choice->appendOption("Grain Only");
        choice->appendOption("Bypass");
        choice->appendOption("Halation Matte");
        choice->appendOption("Bloom Matte");
        choice->setDefault(0);
        page->addChild(*choice);

        choice = desc.defineChoiceParam("sourceSpace");
        choice->setLabels("Input Color Space", "Input Color Space", "Input Color Space");
        choice->setHint("Choose the RGB space entering this node, not necessarily the camera's recording space. LogC3 uses EI 800.");
        for (const auto& space : color::spaces()) choice->appendOption(space.label);
        choice->setDefault(color::Rec709Gamma24);
        page->addChild(*choice);

        choice = desc.defineChoiceParam("outputSpace");
        choice->setLabels("Output Color Space", "Output Color Space", "Output Color Space");
        choice->setHint("Applies when Film Color, Film Development, or Print is active. Texture-only processing always returns the input space. Bypass ignores this setting.");
        for (const auto* label : color::OutputLabels) choice->appendOption(label);
        choice->setDefault(0);
        page->addChild(*choice);

        choice = desc.defineChoiceParam("outputRendering");
        choice->setLabels("Output Rendering", "Output Rendering", "Output Rendering");
        for (const auto* label : color::RenderingLabels) choice->appendOption(label);
        choice->setDefault(color::Automatic);
        choice->setHint("Auto renders scene-log/linear input for the selected display output: SDR for Rec.709/sRGB, HDR for Rec.2100 PQ. SDR runs before creative film; HDR runs after creative and texture stages. Conversion Only leaves rendering to another stage (PQ still encodes absolute luminance using HDR Reference White). Standard SDR and Standard HDR explicitly render only their matching output targets. Managed log/linear output, texture-only, bypass and mattes do not apply display rendering.");
        page->addChild(*choice);

        addDouble(desc,page,"hdrPeak","HDR Peak Luminance (nits)",1000,400,10000,1,nullptr,
                  "PQ rendering ceiling. Default 1000 nits; match the intended mastering target. Only active with PQ output and HDR rendering. Does not configure Resolve monitoring, export tags or HDR metadata.");
        addDouble(desc,page,"hdrWhite","HDR Reference White (nits)",203,80,300,1,nullptr,
                  "Linear white 1 maps to this luminance in PQ; default 203 nits. Active for PQ output, including Conversion Only. Peak Luminance remains higher across the allowed ranges. Creative tone/print can compress highlights before output; lower their tone strengths for more headroom.");

        choice = desc.defineChoiceParam("filmGauge");
        choice->setLabels("Film Gauge", "Film Gauge", "Film Gauge");
        for (const auto& profile : gauge::Profiles) choice->appendOption(profile.label);
        choice->setDefault(0);
        choice->setHint("Creative format presets scale grain size/strength and halation/aura spread together. 70 mm represents a 15-perf large-frame look. No crop or image resize; sliders remain independent and zero strength stays zero.");
        page->addChild(*choice);

        choice = desc.defineChoiceParam("lookPreset");
        choice->setLabels("Stored Preset", "Stored Preset", "Stored Preset");
        for (const auto* label : look::Labels) choice->appendOption(label);
        choice->setDefault(look::Custom);
        choice->setAnimates(false);
        choice->setIsSecret(true);
        choice->setEvaluateOnChange(false);

        choice = desc.defineChoiceParam("presetCategory");
        choice->setLabels("Preset Category", "Preset Category", "Preset Category");
        for (const auto* label : look::CategoryLabels) choice->appendOption(label);
        choice->setDefault(look::AllLooks);
        choice->setAnimates(false);
        choice->setEvaluateOnChange(false);
        choice->setHint("Filters the preset menu only; browsing categories never changes the image. A look outside this category appears as Custom / Current Settings.");
        page->addChild(*choice);

        choice = desc.defineChoiceParam("presetBrowser");
        choice->setLabels("Preset", "Preset", "Preset");
        for (int option = 0; option < look::optionCount(look::AllLooks); ++option) {
            const int preset = look::presetAt(look::AllLooks,option);
            choice->appendOption(preset == look::Custom ? "Custom / Current Settings" : look::Labels[preset]);
        }
        choice->setDefault(0);
        choice->setAnimates(false);
        choice->setIsPersistant(false);
        choice->setEvaluateOnChange(false);
        choice->setHint("Original stock and creative interpretations, not measured stock profiles or exact movie grades. Loads Full mode, gauge, module switches and editable settings; replaces creative keyframes. Preserves color spaces/output rendering, camera balance and grain seed. Neutral removes creative film, development, glow and grain, but retains conversion/balance and the selected output rendering. Custom changes nothing. Film/Print strengths are zero after Neutral; raise them to add their response.");
        page->addChild(*choice);

        GroupParamDescriptor* presets = addGroup(desc, page, "userPresetControls", "User Presets", false);
        for (const auto& command : std::array<std::pair<const char*,const char*>,2> {{{"saveUserPreset","Save Preset..."},{"loadUserPreset","Load Preset..."}}}) {
            auto* button = desc.definePushButtonParam(command.first);
            button->setLabels(command.second,command.second,command.second);
            button->setHint(command.first == std::string("saveUserPreset") ?
                "Save all current control values when clicked, before opening the file dialog. Animation curves are not exported." :
                "Restore all saved settings in one undoable edit. Replaces their keyframes; enable a Preserve option only to keep that context group instead.");
            button->setParent(*presets);
            page->addChild(*button);
        }
        for (const auto& option : std::array<std::pair<const char*,const char*>,3> {{{"presetPreserveSpaces","Preserve Color Spaces"},{"presetPreserveCamera","Preserve Camera Balance"},{"presetPreserveSeed","Preserve Grain Seed"}}}) {
            auto* toggle = desc.defineBooleanParam(option.first);
            toggle->setLabels(option.second,option.second,option.second);
            toggle->setDefault(false);
            toggle->setAnimates(false);
            toggle->setEvaluateOnChange(false);
            toggle->setHint("When loading a user preset, keep this node's current settings and animation in this group. Does not affect saving or built-in presets.");
            toggle->setParent(*presets);
            page->addChild(*toggle);
        }

        GroupParamDescriptor* negative = addGroup(desc, page, "negativeControls", "Film Color", false);
        addToggle(desc, page, negative, "enableNegative", "Enable");
        GroupParamDescriptor* development = addGroup(desc, page, "developmentControls", "Film Development", false);
        addToggle(desc, page, development, "enableDevelopment", "Enable");
        GroupParamDescriptor* print = addGroup(desc, page, "printControls", "Print", false);
        addToggle(desc, page, print, "enablePrint", "Enable");
        GroupParamDescriptor* halation = addGroup(desc, page, "halationControls", "Halation", false);
        addToggle(desc, page, halation, "enableHalation", "Enable");
        GroupParamDescriptor* aura = addGroup(desc, page, "auraControls", "Aura", false);
        addToggle(desc, page, aura, "enableAura", "Enable");
        GroupParamDescriptor* bloom = addGroup(desc, page, "bloomControls", "Bloom", false);
        addToggle(desc, page, bloom, "enableBloom", "Enable");
        GroupParamDescriptor* grain = addGroup(desc, page, "grainControls", "Grain", true);
        addToggle(desc, page, grain, "enableGrain", "Enable");
        GroupParamDescriptor* selective = addGroup(desc, page, "selectiveColorControls", "Selective Color", false);
        addToggle(desc, page, selective, "enableSelectiveColor", "Enable", false);

        choice = desc.defineChoiceParam("system");
        choice->setLabels("Film System", "Film System", "Film System");
        choice->appendOption("Cinema Negative");
        choice->appendOption("Vintage Negative");
        choice->appendOption("Bleach Retained");
        choice->appendOption("Still C41");
        choice->appendOption("Mono Negative");
        choice->appendOption("Reversal");
        choice->setDefault(0);
        choice->setParent(*negative);
        page->addChild(*choice);

        choice = desc.defineChoiceParam("printStyle");
        choice->setLabels("Print Style", "Print Style", "Print Style");
        for (const auto* label : printstyle::Labels) choice->appendOption(label);
        choice->setDefault(printstyle::Standard);
        choice->setAnimates(false);
        choice->setHint("Full, Standard, and Extended load fixed print recipes. Custom unlocks the last recipe without changing its look. Selecting a named recipe replaces Custom tuning and its keyframes; strength and exposure/balance remain independent.");
        choice->setParent(*print);
        page->addChild(*choice);

        choice = desc.defineChoiceParam("grainStyle");
        choice->setLabels("Grain Style", "Grain Style", "Grain Style");
        choice->appendOption("Fine");
        choice->appendOption("Classic");
        choice->appendOption("Rough");
        choice->appendOption("Debug");
        choice->setDefault(1);
        choice->setParent(*grain);
        page->addChild(*choice);

        choice = desc.defineChoiceParam("grainResponse");
        choice->setLabels("Grain Response", "Grain Response", "Grain Response");
        choice->appendOption("Post Print");
        choice->appendOption("Negative & Print");
        choice->setDefault(0);
        choice->setParent(*grain);
        choice->setHint("Post Print retains the original texture. Negative & Print keys grain from the developed negative and adds it before Print, allowing print tone/color to shape it. Texture-only modes retain Post Print behavior. Creative approximation, not measured emulsion density.");
        page->addChild(*choice);

        addDouble(desc, page, "negativeColorStrength", "Film Color Strength", 1.0, 0.0, 1.0, 0.01, negative, "Scales the film palette, monochrome conversion, skin hue, saturation, density, and gamut compression. Zero removes these; camera balance and Film Tone remain independent.");
        addDouble(desc, page, "negativeToneStrength", "Film Tone Strength", 1.0, 0.0, 1.0, 0.01, negative, "Scales film contrast, toe, and shoulder. Zero removes tone shaping; Film Color and camera balance remain independent.");
        addDouble(desc, page, "printColorStrength", "Print Color Strength", 1.0, 0.0, 1.0, 0.01, print, "Scales print palette, saturation, cast, and gamut compression. Zero removes these; print exposure, balance, and tone remain independent.");
        addDouble(desc, page, "printToneStrength", "Print Tone Strength", 1.0, 0.0, 1.0, 0.01, print, "Scales print contrast, toe, rolloff, and black lift. Zero removes these; print color, exposure, and balance remain independent.");
        addDouble(desc, page, "exposure", "Exposure", 0.0, -4.0, 4.0, 0.01, negative);
        addDouble(desc, page, "temperature", "Temperature", 0.0, -3.0, 3.0, 0.01, negative, "Broader linear red/blue balance. Zero is neutral; existing values retain their effect. Creative units, not kelvin.");
        addDouble(desc, page, "tint", "Tint", 0.0, -3.0, 3.0, 0.01, negative, "Broader linear green/magenta balance. Zero is neutral; existing values retain their effect.");
        addDouble(desc, page, "density", "Color Density", 0.18, -1.2, 1.5, 0.01, negative, "Positive values darken saturated colors; negative values brighten them. Neutral grays and chromatic direction are unchanged. Creative color density, not calibrated photographic optical density or an extreme-saturation-only adjustment.");
        addDouble(desc, page, "saturation", "Saturation", 0.95, 0.0, 2.0, 0.01, negative);
        addDouble(desc, page, "toe", "Toe", 0.16, 0.0, 1.0, 0.01, negative);
        addDouble(desc, page, "contrast", "Contrast", 1.08, 0.5, 2.0, 0.01, negative);
        addDouble(desc, page, "negativeShoulder", "Negative Shoulder", 0.50, 0.0, 1.0, 0.01, negative);
        addDouble(desc, page, "negativeCrosstalk", "Color Crosstalk", 0.35, 0.0, 3.0, 0.01, negative, "Zero is no palette mixing, one is the original family matrix, and values above one intensify that palette. Not a global film strength control.");
        addDouble(desc, page, "gamutCompression", "Gamut Compression", 0.50, 0.0, 1.0, 0.01, negative, "Continuously blends negative gamut compression: 0 is off, 0.5 is half strength, and 1 is full strength. Preserves working-space brightness and chroma direction. Partial strength can retain out-of-range values; Print and downstream color management determine the final display range.");
        addDouble(desc, page, "highlightRetention", "Highlight Color Retention", 0.0, 0.0, 1.0, 0.01, negative, "Adds retention to the SDR foundation's automatic colored-highlight shoulder, trading some brightness for chroma and channel gradation. Zero uses the updated default, not the pre-v0.38 response. Requires Film Color and active SDR rendering. Neutral/pale colors and peak scene-linear RGB at or below one are unchanged; full-strength Mono Negative and selective matte ignore this extra adjustment. Independent of Film Color/Tone Strength; built-in presets reset it to zero. Cannot restore clipped source detail.");
        addDouble(desc, page, "skinHue", "Skin Hue", 0.0, -3.0, 3.0, 0.01, negative, "Selective warm-color adjustment toward magenta or green, with extra endpoint range. Not face detection; neutral and cool colors are excluded.");
        addDouble(desc, page, "printTone", "Print Tone Curve", 0.0, -1.0, 1.0, 0.01, print, "Minus one gives a stronger print-like tone curve; plus one gives a gentler telecine-like response. Print Tone Strength separately blends the complete tonal response.");
        addDouble(desc, page, "printContrast", "Print Contrast", 1.0, 0.5, 2.0, 0.01, print);
        addDouble(desc, page, "printRolloff", "Highlight Rolloff", 0.55, 0.0, 1.0, 0.01, print);
        addDouble(desc, page, "printColor", "Print Color", printstyle::Presets[printstyle::Standard][printstyle::Color], 0.0, 1.0, 0.01, print, "Zero gives the strongest print-like palette, cold shadows, and warm highlights; one gives a more neutral, less constrained telecine-like color response. Print Color Strength separately blends the complete chromatic response, including saturation and gamut compression.");
        addDouble(desc, page, "printNeutralize", "Neutralize Balance", 0.0, 0.0, 1.0, 0.01, print, "Removes only the print's cold-shadow/warm-highlight color bias. Does not bypass its palette, saturation, gamut compression, or tone curve. Has less effect when Print Color is high.");
        addDouble(desc, page, "printSaturation", "Print Saturation", 1.0, 0.0, 2.0, 0.01, print);
        addDouble(desc, page, "blackPoint", "Black Point", 0.45, 0.0, 1.0, 0.01, print);
        addDouble(desc, page, "printExposure", "Print Exposure", 0.0, -2.0, 2.0, 0.01, print);
        addDouble(desc, page, "printRed", "Print Red", 0.0, -2.0, 2.0, 0.01, print);
        addDouble(desc, page, "printGreen", "Print Green", 0.0, -2.0, 2.0, 0.01, print);
        addDouble(desc, page, "printBlue", "Print Blue", 0.0, -2.0, 2.0, 0.01, print);
        addDouble(desc, page, "halation", "Halation", 0.0, 0.0, 2.0, 0.01, halation);
        addDouble(desc, page, "halationRadius", "Halation Radius", 1.0, 0.0, 2.0, 0.01, halation, "Scales the tight halo independently of Aura Radius. Uses a 1080-line reference and the selected Film Gauge.");
        addDouble(desc, page, "halationThreshold", "Highlight Threshold", 0.48, 0.0, 2.0, 0.01, halation, "Source-highlight cutoff in the managed perceptual working space, not camera log code values or stops. Lower values include darker sources. Also affects Aura.");
        addDouble(desc, page, "halationSoftness", "Highlight Transition", 0.52, 0.01, 2.0, 0.01, halation, "Width of the smooth transition from no highlight contribution to full contribution. Smaller values isolate a narrower brightness range. Also affects Aura.");
        addDouble(desc, page, "halationColor", "Red / Amber", 0.50, 0.0, 1.0, 0.01, halation, "Zero is redder; one is more amber. Maintains halo luminance as the tint changes. Also affects Aura; does not recolor the diagnostic matte.");
        addDouble(desc, page, "auraRadius", "Aura Radius", 1.0, 0.0, 2.0, 0.01, aura, "Scales the broad aura independently of Halation Radius. Uses a 1080-line reference and the selected Film Gauge.");
        addDouble(desc, page, "aura", "Aura", 0.0, 0.0, 1.0, 0.01, aura);
        addDouble(desc, page, "grain", "Grain", 0.16, 0.0, 2.0, 0.01, grain);
        addDouble(desc, page, "grainSize", "Grain Size", 0.45, 0.0, 1.0, 0.01, grain);
        addDouble(desc, page, "grainSoftness", "Grain Softness", 0.25, 0.0, 2.0, 0.01, grain, "Zero to one suppresses fine detail; one to two also smooths the primary grain field at the same lattice pitch, with normalized variance. Existing values retain their texture.");
        addDouble(desc, page, "grainRoughness", "Grain Roughness", 0.32, 0.0, 1.0, 0.01, grain);
        addDouble(desc, page, "grainColor", "Grain Color", 0.25, 0.0, 1.0, 0.01, grain, "Zero gives monochrome grain. Active full-strength Mono Negative automatically uses monochrome grain; texture-only modes retain this setting.");
        addDouble(desc, page, "grainShadows", "Shadow Grain", 1.15, 0.0, 2.0, 0.01, grain);
        addDouble(desc, page, "grainMidtones", "Midtone Grain", 0.80, 0.0, 2.0, 0.01, grain);
        addDouble(desc, page, "grainHighlights", "Highlight Grain", 0.42, 0.0, 2.0, 0.01, grain);
        addDouble(desc, page, "pushPull", "Push / Pull", 0, -3, 3, 0.01, development, "Creative development amount: changes contrast and shadow fog around fixed middle gray; also scales enabled grain strength in Full mode without changing grain size or position. Not calibrated camera exposure stops.");
        addDouble(desc, page, "colorRichness", "Richness", 0, -1, 1, 0.01, development, "Positive values enrich muted colors; negative values desaturate them. Affects already saturated colors less, preserving neutral gray and working-space luminance. Separate from Saturation and Color Density.");
        addDouble(desc, page, "splitTone", "Split Tone", 0, 0, 3, 0.01, development, "Shifts shadows toward the selected hue and highlights toward its chromatic opposite. Middle gray remains neutral by default. Values above one allow stronger creative toning.");
        addDouble(desc, page, "splitHue", "Shadow Hue", 220, 0, 360, 1, development, "Shadow hue in degrees: red 0, green 120, blue 240. Highlights use the opposite direction.");
        addDouble(desc, page, "splitPivot", "Split Pivot", 0.46135613, 0.2, 0.8, 0.01, development, "Neutral split point in the managed perceptual working space. Default is scene-linear 18% gray.");
        addDouble(desc, page, "splitWidth", "Dead Zone Width", 0.1, 0, 0.3, 0.01, development, "Width of the neutral tonal range around Split Pivot that Split Tone leaves unaffected. Not a shadow lift or black-level adjustment.");
        addDouble(desc, page, "splitShadows", "Shadow Intensity", 1, 0, 2, 0.01, development, "Scales the shadow side of Split Tone independently.");
        addDouble(desc, page, "splitHighlights", "Highlight Intensity", 1, 0, 2, 0.01, development, "Scales the highlight side of Split Tone independently.");
        addDouble(desc, page, "grainStretch", "Horizontal Stretch", 1, 0.5, 2, 0.01, grain, "Horizontal grain desqueeze ratio. One is round grain; two doubles its horizontal scale without changing frame dimensions.");
        addDouble(desc, page, "grainRed", "Red Grain", 1, 0, 2, 0.01, grain, "Scales red-channel grain intensity; zero removes it. Full-strength Mono Negative still finishes the composite monochrome.");
        addDouble(desc, page, "grainGreen", "Green Grain", 1, 0, 2, 0.01, grain, "Scales green-channel grain intensity independently.");
        addDouble(desc, page, "grainBlue", "Blue Grain", 1, 0, 2, 0.01, grain, "Scales blue-channel grain intensity independently.");
        addDouble(desc, page, "bloom", "Bloom", 0, 0, 2, 0.01, bloom, "Strength of independent linear-light highlight diffusion. Zero skips all bloom work.");
        addDouble(desc, page, "bloomRadius", "Bloom Radius", 1, 0, 2, 0.01, bloom, "Spread uses a 1080-line reference; independent of Film Gauge, Halation Radius, and Aura Radius.");
        addDouble(desc, page, "bloomThreshold", "Highlight Threshold", 0.65, 0, 2, 0.01, bloom, "Source highlight cutoff, expressed in the perceptual Rec.709 working domain and converted to linear light for extraction.");
        addDouble(desc, page, "bloomSoftness", "Highlight Transition", 0.35, 0.01, 2, 0.01, bloom, "Width above the threshold over which source contribution rises smoothly.");
        addDouble(desc, page, "bloomColor", "Source Color", 1, 0, 1, 0.01, bloom, "One retains source-highlight color. Zero makes diffusion neutral at the same linear luminance. Mono Negative still finishes monochrome.");
        addDouble(desc, page, "bloomProtection", "Protect Highlights", 0.8, 0, 1, 0.01, bloom, "Reduces bloom added to already bright destination pixels; does not reduce its spread into nearby shadows. One fully protects linear peaks at or above one.");
        addDouble(desc, page, "selectiveAmount", "Amount", 0, 0, 1, .01, selective, "Zero leaves color unchanged; one makes unselected colors monochrome. Runs after print, glow and grain. Cannot restore color removed by Mono Negative or prior nodes.");
        addDouble(desc, page, "selectiveHue", "Keep Hue", 0, 0, 360, 1, selective, "Source hue in managed Rec.709/sRGB: red 0/360, yellow 60, green 120, cyan 180, blue 240, magenta 300. Keyed before film/development/print and texture, after enabled camera balance. Selects colors, not objects.");
        addDouble(desc, page, "selectiveRange", "Hue Range", 15, 0, 180, 1, selective, "Fully retained hue range on either side of Keep Hue, in degrees. 180 retains all hues meeting Minimum Saturation. Red selection wraps around zero.");
        addDouble(desc, page, "selectiveSoftness", "Hue Feather", 15, 0, 90, 1, selective, "Smooth transition outside the retained range, in degrees. Zero makes a hard hue cutoff and can reveal noise or edge artifacts.");
        addDouble(desc, page, "selectiveSaturation", "Minimum Saturation", .25, 0, 1, .01, selective, "Rejects pale/near-neutral source colors with a smooth 0.05 transition below the cutoff. Increase to reduce skin contamination when keeping red. Not face detection.");
        choice = desc.defineChoiceParam("selectiveView");
        choice->setLabels("View", "View", "View");
        choice->appendOption("Image"); choice->appendOption("Selection Matte");
        choice->setDefault(0); choice->setParent(*selective);
        choice->setHint("White retains color, black becomes monochrome, gray is a partial selection. Shows the raw selection independently of Amount and Output Color Space; alpha is unchanged. Disabled and texture-only modes ignore this preview.");
        page->addChild(*choice);
        IntParamDescriptor* seed = desc.defineIntParam("grainSeed");
        seed->setLabels("Grain Seed", "Grain Seed", "Grain Seed");
        seed->setDefault(0);
        seed->setRange(0, 1000000);
        seed->setDisplayRange(0, 1000);
        seed->setParent(*grain);
        page->addChild(*seed);

        auto* sdr = addGroup(desc,page,"sdrViewing","SDR Viewing",false);
        addDouble(desc,page,"sdrContrast","Viewing Contrast",0,-1,1,.01,sdr,
                  "Adjusts the SDR viewing curve around fixed middle gray, before creative negative/print response. Negative softens contrast; positive deepens shadows and increases midtone separation. Zero preserves the v0.39 curve. Does not replace camera Exposure or recover clipped detail. Built-in looks preserve this output setting.");
        addDouble(desc,page,"sdrRolloff","Highlight Rolloff",0,-1,1,.01,sdr,
                  "Moves the SDR shoulder while preserving middle gray and its slope. Positive starts rolloff earlier for darker, softer highlights; negative delays rolloff for brighter highlights. Zero preserves v0.39. Negative and Print tone curves can add further compression. Inactive outside SDR rendering.");
        addDouble(desc,page,"sdrGamut","Gamut Compression",0,-1,1,.01,sdr,
                  "Adjusts the SDR display-gamut shoulder independently of Film Color Gamut Compression. Negative reduces in-gamut softening; positive starts compression earlier. Zero preserves v0.39. Minus one still limits out-of-gamut RGB to the display boundary; it does not disable gamut safety. Inactive outside SDR rendering.");
    }

    OFX::ImageEffect* createInstance(OfxImageEffectHandle handle, OFX::ContextEnum) override
    {
        return new OpenEmulsionPlugin(handle);
    }

private:
    static OFX::GroupParamDescriptor* addGroup(OFX::ImageEffectDescriptor& desc, OFX::PageParamDescriptor* page,
                                              const std::string& name, const std::string& label, bool open)
    {
        auto* group = desc.defineGroupParam(name);
        group->setLabels(label, label, label);
        group->setOpen(open);
        page->addChild(*group);
        return group;
    }

    static void addToggle(OFX::ImageEffectDescriptor& desc, OFX::PageParamDescriptor* page, OFX::GroupParamDescriptor* group,
                          const std::string& name, const std::string& label, bool initial = true)
    {
        auto* param = desc.defineBooleanParam(name);
        param->setLabels(label, label, label);
        param->setDefault(initial);
        param->setParent(*group);
        page->addChild(*param);
    }

    static void addDouble(OFX::ImageEffectDescriptor& desc, OFX::PageParamDescriptor* page, const std::string& name,
                          const std::string& label, double def, double min, double max, double increment, OFX::GroupParamDescriptor* parent,
                          const char* hint = nullptr)
    {
        OFX::DoubleParamDescriptor* param = desc.defineDoubleParam(name);
        param->setLabels(label, label, label);
        param->setScriptName(name);
        param->setDefault(def);
        param->setRange(min, max);
        param->setDisplayRange(min, max);
        param->setIncrement(increment);
        param->setDoubleType(OFX::eDoubleTypePlain);
        if (hint) param->setHint(hint);
        for (int control = 0; control < printstyle::ControlCount; ++control) {
            if (name == printstyle::Parameters[control].name) {
                param->setDefault(printstyle::Presets[printstyle::Standard][control]);
                param->setEnabled(false);
            }
        }
        if (parent) param->setParent(*parent);
        page->addChild(*param);
    }
};

} // namespace

void OFX::Plugin::getPluginIDs(PluginFactoryArray& factoryArray)
{
    static OpenEmulsionFactory factory;
    factoryArray.push_back(&factory);
}
