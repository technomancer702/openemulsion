// SPDX-License-Identifier: MPL-2.0

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>

#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "ofxsProcessing.h"
#include "ofxsSupportPrivate.h"
#include "HalationBlur.h"
#include "GrainConfig.h"
#include "ColorSpaceConfig.h"
#include "FilmResponseConfig.h"

#define kPluginName "OpenEmulsion"
#define kPluginGrouping "OpenEmulsion"
#define kPluginDescription "Original film-emulation plugin with adjustable tone, print, grain, and smooth halation, with OpenCL acceleration."
#define kPluginIdentifier "org.openemulsion.film"
#define kPluginVersionMajor 0
#define kPluginVersionMinor 12

extern bool RunOpenEmulsionOpenCL(void* cmdQueue, int width, int height, double time, const float* settings, const float* input, float* output);

namespace {

using Rgb = ColorRgb;

struct Settings {
    int mode = 0;
    int system = 0;
    int printStyle = 1;
    int grainStyle = 1;
    int sourceSpace = color::Rec709Gamma24;
    int outputSpace = 0;
    bool enableNegative = true;
    bool enablePrint = true;
    bool enableHalation = true;
    bool enableAura = true;
    bool enableGrain = true;
    double exposure = 0.0;
    double temperature = 0.0;
    double tint = 0.0;
    double density = 0.18;
    double saturation = 0.95;
    double toe = 0.16;
    double contrast = 1.08;
    double printColor = 0.25;
    double blackPoint = 0.45;
    double negativeShoulder = 0.50;
    double negativeCrosstalk = 0.35;
    double gamutCompression = 0.50;
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
    int grainSeed = 0;
    float gainR = 1.0f;
    float gainG = 1.0f;
    float gainB = 1.0f;
};

int moduleMask(const Settings& s)
{
    return (s.enableNegative ? film::Negative : 0) | (s.enablePrint ? film::Print : 0) |
           (s.enableHalation ? film::Halation : 0) | (s.enableAura ? film::Aura : 0) | (s.enableGrain ? film::Grain : 0);
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
    return film::isIdentity(s.mode, moduleMask(s), static_cast<float>(s.halation), static_cast<float>(s.aura), static_cast<float>(s.grain));
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

float highlightKey(Rgb c)
{
    return halation::highlight(c.r, c.g, c.b);
}

class HalationProcessor : public OFX::MultiThread::Processor {
public:
    HalationProcessor(OFX::ImageEffect& effect, OFX::Image* src, halation::Blur& blur, ColorParameters color)
        : effect_(effect), src_(src), blur_(blur), color_(color) {}

    void run()
    {
        for (phase_ = 0; phase_ < 3 && !effect_.abort(); ++phase_)
            multiThread(std::max(1u, std::min(OFX::MultiThread::getNumCPUs(), static_cast<unsigned>(blur_.height))));
    }

    void multiThreadFunction(unsigned thread, unsigned threads) override
    {
        const int begin = static_cast<int>(thread * blur_.height / threads);
        const int end = static_cast<int>((thread + 1) * blur_.height / threads);
        const OfxRectI& bounds = src_->getBounds();
        for (int y = begin; y < end && !effect_.abort(); ++y) {
            if (phase_ == 0) {
                blur_.extractRows(y, y + 1, [&](int x, int sy) {
                    return highlightKey(color_to_work(readPixel(src_, x + bounds.x1, sy + bounds.y1), color_));
                });
            } else {
                blur_.blurRows(y, y + 1, phase_ == 1);
            }
        }
    }

private:
    OFX::ImageEffect& effect_;
    OFX::Image* src_;
    halation::Blur& blur_;
    ColorParameters color_;
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
        if (_isEnabledOpenCLRender || !spatialEnabled(settings_)) return;
        const OfxRectI& b = src_->getBounds();
        const halation::Filter filter(halationAmount(settings_), static_cast<float>(settings_.halationRadius), auraAmount(settings_));
        blur_ = std::make_unique<halation::Blur>(b.x2 - b.x1, b.y2 - b.y1, filter);
        HalationProcessor(_effect, src_, *blur_, colorParameters_).run();
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
        const bool clampOutput = colorEnabled(settings_) || printEnabled(settings_);
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
                    if (colorEnabled(settings_)) {
                        c = cameraStage(c, settings_);
                        c = response_negative(c, responseParameters_);
                    }
                    if (printEnabled(settings_)) {
                        c = response_print(c, responseParameters_);
                    }
                    if (spatialEnabled(settings_)) {
                        const OfxRectI& b = src_->getBounds();
                        const float signal = halation::signal(blur_->sample(x - b.x1, y - b.y1), highlightKey(work),
                                                              halationAmount(settings_), auraAmount(settings_));
                        if (settings_.mode == 5) {
                            const float h = clampf(signal * 2.0f, 0.0f, 1.0f);
                            c = {h, h * 0.55f, h * 0.10f};
                        } else {
                            c.r += signal * 0.55f;
                            c.g += signal * 0.24f;
                            c.b += signal * 0.045f;
                        }
                    } else if (settings_.mode == 5) {
                        c = {0.0f, 0.0f, 0.0f};
                    }
                    if (grainEnabled(settings_)) {
                        const OfxRectI& b = src_->getBounds();
                        const GrainVector delta = grain_delta(x - b.x1, y - b.y1, luma(c), grainParameters_);
                        c.r += delta.r;
                        c.g += delta.g;
                        c.b += delta.b;
                    }
                    if (clampOutput) c = {std::max(c.r, 0.0f), std::max(c.g, 0.0f), std::max(c.b, 0.0f)};
                    if (settings_.mode != 5) {
                        const bool unchanged = !clampOutput && c.r == work.r && c.g == work.g && c.b == work.b;
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
    }

    OFX::Image* src_ = nullptr;
    Settings settings_;
    double time_ = 0.0;
    std::unique_ptr<halation::Blur> blur_;
    GrainParameters grainParameters_ {};
    ColorParameters colorParameters_ {};
    FilmResponseParameters responseParameters_ {};
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
        system_ = fetchChoiceParam("system");
        printStyle_ = fetchChoiceParam("printStyle");
        grainStyle_ = fetchChoiceParam("grainStyle");
        enableNegative_ = fetchBooleanParam("enableNegative");
        enablePrint_ = fetchBooleanParam("enablePrint");
        enableHalation_ = fetchBooleanParam("enableHalation");
        enableAura_ = fetchBooleanParam("enableAura");
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
    Settings settingsAt(double time) const
    {
        Settings s;
        mode_->getValueAtTime(time, s.mode);
        sourceSpace_->getValueAtTime(time, s.sourceSpace);
        outputSpace_->getValueAtTime(time, s.outputSpace);
        system_->getValueAtTime(time, s.system);
        printStyle_->getValueAtTime(time, s.printStyle);
        grainStyle_->getValueAtTime(time, s.grainStyle);
        s.enableNegative = enableNegative_->getValueAtTime(time);
        s.enablePrint = enablePrint_->getValueAtTime(time);
        s.enableHalation = enableHalation_->getValueAtTime(time);
        s.enableAura = enableAura_->getValueAtTime(time);
        s.enableGrain = enableGrain_->getValueAtTime(time);
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
        const float gain = std::pow(2.0f, static_cast<float>(s.exposure));
        const float warm = static_cast<float>(s.temperature) * 0.085f;
        const float green = static_cast<float>(s.tint) * 0.065f;
        s.gainR = gain * (1.0f + warm) * (1.0f - green * 0.30f);
        s.gainG = gain * (1.0f + green);
        s.gainB = gain * (1.0f - warm) * (1.0f - green * 0.30f);
        return s;
    }

    OFX::Clip* dstClip_ = nullptr;
    OFX::Clip* srcClip_ = nullptr;
    OFX::ChoiceParam* mode_ = nullptr;
    OFX::ChoiceParam* sourceSpace_ = nullptr;
    OFX::ChoiceParam* outputSpace_ = nullptr;
    OFX::ChoiceParam* system_ = nullptr;
    OFX::ChoiceParam* printStyle_ = nullptr;
    OFX::ChoiceParam* grainStyle_ = nullptr;
    OFX::BooleanParam* enableNegative_ = nullptr;
    OFX::BooleanParam* enablePrint_ = nullptr;
    OFX::BooleanParam* enableHalation_ = nullptr;
    OFX::BooleanParam* enableAura_ = nullptr;
    OFX::BooleanParam* enableGrain_ = nullptr;
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
        choice->appendOption("Halation & Grain Only");
        choice->appendOption("Grain Only");
        choice->appendOption("Bypass");
        choice->appendOption("Halation Matte");
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
        choice->setHint("Applies when Film Color or Print is active. Texture-only processing always returns the input space. Bypass ignores this setting.");
        for (const auto* label : color::OutputLabels) choice->appendOption(label);
        choice->setDefault(0);
        page->addChild(*choice);

        GroupParamDescriptor* modules = addGroup(desc, page, "modules", "Modules", true);
        addToggle(desc, page, modules, "enableNegative", "Film Color");
        addToggle(desc, page, modules, "enablePrint", "Print");
        addToggle(desc, page, modules, "enableHalation", "Halation");
        addToggle(desc, page, modules, "enableAura", "Aura");
        addToggle(desc, page, modules, "enableGrain", "Grain");
        GroupParamDescriptor* negative = addGroup(desc, page, "negativeControls", "Film Color", false);
        GroupParamDescriptor* print = addGroup(desc, page, "printControls", "Print", false);
        GroupParamDescriptor* halation = addGroup(desc, page, "halationControls", "Halation", false);
        GroupParamDescriptor* aura = addGroup(desc, page, "auraControls", "Aura", false);
        GroupParamDescriptor* grain = addGroup(desc, page, "grainControls", "Grain", true);

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
        choice->appendOption("Contact");
        choice->appendOption("Standard");
        choice->appendOption("Telecine");
        choice->appendOption("Custom");
        choice->setDefault(1);
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

        addDouble(desc, page, "exposure", "Exposure", 0.0, -4.0, 4.0, 0.01, negative);
        addDouble(desc, page, "temperature", "Temperature", 0.0, -1.0, 1.0, 0.01, negative);
        addDouble(desc, page, "tint", "Tint", 0.0, -1.0, 1.0, 0.01, negative);
        addDouble(desc, page, "density", "Negative Density", 0.18, -0.6, 0.9, 0.01, negative);
        addDouble(desc, page, "saturation", "Saturation", 0.95, 0.0, 2.0, 0.01, negative);
        addDouble(desc, page, "toe", "Toe", 0.16, 0.0, 1.0, 0.01, negative);
        addDouble(desc, page, "contrast", "Contrast", 1.08, 0.5, 2.0, 0.01, negative);
        addDouble(desc, page, "negativeShoulder", "Negative Shoulder", 0.50, 0.0, 1.0, 0.01, negative);
        addDouble(desc, page, "negativeCrosstalk", "Color Crosstalk", 0.35, 0.0, 1.0, 0.01, negative);
        addDouble(desc, page, "gamutCompression", "Gamut Compression", 0.50, 0.0, 1.0, 0.01, negative);
        addDouble(desc, page, "skinHue", "Skin Hue", 0.0, -1.0, 1.0, 0.01, negative);
        addDouble(desc, page, "printTone", "Print Tone", 0.0, -1.0, 1.0, 0.01, print);
        addDouble(desc, page, "printContrast", "Print Contrast", 1.0, 0.5, 2.0, 0.01, print);
        addDouble(desc, page, "printRolloff", "Highlight Rolloff", 0.55, 0.0, 1.0, 0.01, print);
        addDouble(desc, page, "printColor", "Print Color", 0.25, 0.0, 1.0, 0.01, print);
        addDouble(desc, page, "printNeutralize", "Neutralize Print", 0.0, 0.0, 1.0, 0.01, print);
        addDouble(desc, page, "printSaturation", "Print Saturation", 1.0, 0.0, 2.0, 0.01, print);
        addDouble(desc, page, "blackPoint", "Black Point", 0.45, 0.0, 1.0, 0.01, print);
        addDouble(desc, page, "printExposure", "Print Exposure", 0.0, -2.0, 2.0, 0.01, print);
        addDouble(desc, page, "printRed", "Print Red", 0.0, -2.0, 2.0, 0.01, print);
        addDouble(desc, page, "printGreen", "Print Green", 0.0, -2.0, 2.0, 0.01, print);
        addDouble(desc, page, "printBlue", "Print Blue", 0.0, -2.0, 2.0, 0.01, print);
        addDouble(desc, page, "halation", "Halation", 0.0, 0.0, 2.0, 0.01, halation);
        addDouble(desc, page, "halationRadius", "Halation Radius", 1.0, 0.0, 2.0, 0.01, halation);
        addDouble(desc, page, "aura", "Aura", 0.0, 0.0, 1.0, 0.01, aura);
        addDouble(desc, page, "grain", "Grain", 0.16, 0.0, 2.0, 0.01, grain);
        addDouble(desc, page, "grainSize", "Grain Size", 0.45, 0.0, 1.0, 0.01, grain);
        addDouble(desc, page, "grainSoftness", "Grain Softness", 0.25, 0.0, 1.0, 0.01, grain);
        addDouble(desc, page, "grainRoughness", "Grain Roughness", 0.32, 0.0, 1.0, 0.01, grain);
        addDouble(desc, page, "grainColor", "Grain Color", 0.25, 0.0, 1.0, 0.01, grain);
        addDouble(desc, page, "grainShadows", "Shadow Grain", 1.15, 0.0, 2.0, 0.01, grain);
        addDouble(desc, page, "grainMidtones", "Midtone Grain", 0.80, 0.0, 2.0, 0.01, grain);
        addDouble(desc, page, "grainHighlights", "Highlight Grain", 0.42, 0.0, 2.0, 0.01, grain);
        IntParamDescriptor* seed = desc.defineIntParam("grainSeed");
        seed->setLabels("Grain Seed", "Grain Seed", "Grain Seed");
        seed->setDefault(0);
        seed->setRange(0, 1000000);
        seed->setDisplayRange(0, 1000);
        seed->setParent(*grain);
        page->addChild(*seed);
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
                          const std::string& name, const std::string& label)
    {
        auto* param = desc.defineBooleanParam(name);
        param->setLabels(label, label, label);
        param->setDefault(true);
        param->setParent(*group);
        page->addChild(*param);
    }

    static void addDouble(OFX::ImageEffectDescriptor& desc, OFX::PageParamDescriptor* page, const std::string& name,
                          const std::string& label, double def, double min, double max, double increment, OFX::GroupParamDescriptor* parent)
    {
        OFX::DoubleParamDescriptor* param = desc.defineDoubleParam(name);
        param->setLabels(label, label, label);
        param->setScriptName(name);
        param->setDefault(def);
        param->setRange(min, max);
        param->setDisplayRange(min, max);
        param->setIncrement(increment);
        param->setDoubleType(OFX::eDoubleTypePlain);
        param->setParent(*parent);
        page->addChild(*param);
    }
};

} // namespace

void OFX::Plugin::getPluginIDs(PluginFactoryArray& factoryArray)
{
    static OpenEmulsionFactory factory;
    factoryArray.push_back(&factory);
}
