// SPDX-License-Identifier: MPL-2.0

#ifndef OPENEMULSION_COLOR_MATH_H
#define OPENEMULSION_COLOR_MATH_H

// Shared C++/OpenCL transfer math. Coefficients and conventions: docs/COLOR_SPACES.md.
#ifdef __cplusplus
#include <cmath>
#define COLOR_POW std::pow
#define COLOR_LOG std::log
#define COLOR_LOG2 std::log2
#define COLOR_EXP std::exp
#define COLOR_EXP2 std::exp2
#define COLOR_ABS std::abs
#define COLOR_SIGN std::copysign
#else
#pragma OPENCL FP_CONTRACT OFF
#define COLOR_POW pow
#define COLOR_LOG log
#define COLOR_LOG2 log2
#define COLOR_EXP exp
#define COLOR_EXP2 exp2
#define COLOR_ABS fabs
#define COLOR_SIGN copysign
#endif

enum ColorCurve {
    ColorGamma24, ColorSRGB, ColorLinear, ColorLogC3, ColorLogC4,
    ColorSLog3, ColorIntermediate, ColorFilmGen5, ColorLog3G10,
    ColorCanonLog2, ColorCanonLog3, ColorACEScct, ColorVLog
};
typedef struct ColorRgb { float r, g, b; } ColorRgb;
typedef struct ColorParameters {
    int sourceCurve, outputCurve, sourceIsWork, outputIsWork;
    float to709[9];
    float from709[9];
} ColorParameters;

static inline float color_signed_power(float x, float exponent)
{
    return COLOR_SIGN(COLOR_POW(COLOR_ABS(x), exponent), x);
}

static inline float color_pow10(float x)
{
    return COLOR_EXP2(x * 3.321928094887362f);
}

static inline float color_log10(float x)
{
    return COLOR_LOG2(x) * 0.3010299956639812f;
}

static inline float color_decode(float v, int curve)
{
    switch (curve) {
    case ColorGamma24: return color_signed_power(v, 2.4f);
    case ColorSRGB:
        return COLOR_ABS(v) <= 0.04045f ? v / 12.92f :
            COLOR_SIGN(COLOR_POW((COLOR_ABS(v) + 0.055f) / 1.055f, 2.4f), v);
    case ColorLogC3:
        return v > 5.367655f * 0.010591f + 0.092809f ?
            (color_pow10((v - 0.385537f) / 0.247190f) - 0.052272f) / 5.555556f :
            (v - 0.092809f) / 5.367655f;
    case ColorLogC4: {
        const float a = (262144.0f - 16.0f) / 117.45f, b = 928.0f / 1023.0f, c = 95.0f / 1023.0f;
        const float s = 7.0f * 0.69314718056f * COLOR_EXP2(7.0f - 14.0f * c / b) / (a * b);
        const float t = (COLOR_EXP2(6.0f - 14.0f * c / b) - 64.0f) / a;
        return v < 0.0f ? v * s + t : (COLOR_EXP2(14.0f * (v - c) / b + 6.0f) - 64.0f) / a;
    }
    case ColorSLog3:
        return v >= 171.2102946929f / 1023.0f ?
            color_pow10((v * 1023.0f - 420.0f) / 261.5f) * 0.19f - 0.01f :
            (v * 1023.0f - 95.0f) * 0.01125f / (171.2102946929f - 95.0f);
    case ColorIntermediate:
        return v > 0.02740668f ? COLOR_EXP2(v / 0.07329248f - 7.0f) - 0.0075f : v / 10.44426855f;
    case ColorFilmGen5:
        return v >= 8.2836059324f * 0.005f + 0.0924657534f ?
            COLOR_EXP((v - 0.5300133392f) / 0.08692876065f) - 0.005494072432f :
            (v - 0.0924657534f) / 8.2836059324f;
    case ColorLog3G10:
        return v < 0.0f ? v / 15.1927f - 0.01f :
            (color_pow10(v / 0.224282f) - 1.0f) / 155.975327f - 0.01f;
    case ColorCanonLog2:
        return COLOR_SIGN((color_pow10(COLOR_ABS(v - 0.092864125f) / 0.24136077f) - 1.0f) /
                          87.09937546f * 0.9f, v - 0.092864125f);
    case ColorCanonLog3:
        if (v < 0.097465473f) return -(color_pow10((0.12783901f - v) / 0.36726845f) - 1.0f) / 14.98325f * 0.9f;
        if (v <= 0.15277891f) return (v - 0.12512219f) / 1.9754798f * 0.9f;
        return (color_pow10((v - 0.12240537f) / 0.36726845f) - 1.0f) / 14.98325f * 0.9f;
    case ColorACEScct:
        return v <= 0.155251141552511f ? (v - 0.0729055341958355f) / 10.5402377416545f :
            COLOR_EXP2((v < 1.467996312f ? v : 1.467996312f) * 17.52f - 9.72f);
    case ColorVLog:
        return v < 0.181f ? (v - 0.125f) / 5.6f : color_pow10((v - 0.598206f) / 0.241514f) - 0.00873f;
    default: return v;
    }
}

static inline float color_encode(float x, int curve)
{
    switch (curve) {
    case ColorGamma24: return color_signed_power(x, 1.0f / 2.4f);
    case ColorSRGB:
        return COLOR_ABS(x) <= 0.0031308f ? 12.92f * x :
            COLOR_SIGN(1.055f * COLOR_POW(COLOR_ABS(x), 1.0f / 2.4f) - 0.055f, x);
    case ColorLogC3:
        return x > 0.010591f ? 0.247190f * color_log10(5.555556f * x + 0.052272f) + 0.385537f :
            5.367655f * x + 0.092809f;
    case ColorLogC4: {
        const float a = (262144.0f - 16.0f) / 117.45f, b = 928.0f / 1023.0f, c = 95.0f / 1023.0f;
        const float s = 7.0f * 0.69314718056f * COLOR_EXP2(7.0f - 14.0f * c / b) / (a * b);
        const float t = (COLOR_EXP2(6.0f - 14.0f * c / b) - 64.0f) / a;
        return x < t ? (x - t) / s : (COLOR_LOG2(a * x + 64.0f) - 6.0f) / 14.0f * b + c;
    }
    case ColorSLog3:
        return x >= 0.01125f ? (420.0f + color_log10((x + 0.01f) / 0.19f) * 261.5f) / 1023.0f :
            (x * (171.2102946929f - 95.0f) / 0.01125f + 95.0f) / 1023.0f;
    case ColorIntermediate:
        return x > 0.00262409f ? (COLOR_LOG2(x + 0.0075f) + 7.0f) * 0.07329248f : x * 10.44426855f;
    case ColorFilmGen5:
        return x >= 0.005f ? 0.08692876065f * COLOR_LOG(x + 0.005494072432f) + 0.5300133392f :
            8.2836059324f * x + 0.0924657534f;
    case ColorLog3G10:
        return x < -0.01f ? (x + 0.01f) * 15.1927f : 0.224282f * color_log10((x + 0.01f) * 155.975327f + 1.0f);
    case ColorCanonLog2:
        return COLOR_SIGN(0.24136077f * color_log10(COLOR_ABS(x) / 0.9f * 87.09937546f + 1.0f), x) + 0.092864125f;
    case ColorCanonLog3: {
        const float v = x / 0.9f;
        if (v < -0.014f) return -0.36726845f * color_log10(1.0f - 14.98325f * v) + 0.12783901f;
        if (v <= 0.014f) return 1.9754798f * v + 0.12512219f;
        return 0.36726845f * color_log10(14.98325f * v + 1.0f) + 0.12240537f;
    }
    case ColorACEScct:
        return x <= 0.0078125f ? 10.5402377416545f * x + 0.0729055341958355f : (COLOR_LOG2(x) + 9.72f) / 17.52f;
    case ColorVLog:
        return x < 0.01f ? 5.6f * x + 0.125f : 0.241514f * color_log10(x + 0.00873f) + 0.598206f;
    default: return x;
    }
}

static inline ColorRgb color_matrix(ColorRgb v, const float* m)
{
    ColorRgb o = {m[0] * v.r + m[1] * v.g + m[2] * v.b,
                  m[3] * v.r + m[4] * v.g + m[5] * v.b,
                  m[6] * v.r + m[7] * v.g + m[8] * v.b};
    return o;
}

static inline ColorRgb color_curve_rgb(ColorRgb v, int curve, int encode)
{
    ColorRgb o = {encode ? color_encode(v.r, curve) : color_decode(v.r, curve),
                  encode ? color_encode(v.g, curve) : color_decode(v.g, curve),
                  encode ? color_encode(v.b, curve) : color_decode(v.b, curve)};
    return o;
}

static inline ColorRgb color_balance(ColorRgb v, ColorRgb gain)
{
    const int linear = (gain.r != 1.0f || gain.g != 1.0f || gain.b != 1.0f);
    if (linear) v = color_curve_rgb(v, ColorSRGB, 0);
    v.r *= gain.r; v.g *= gain.g; v.b *= gain.b;
    return linear ? color_curve_rgb(v, ColorSRGB, 1) : v;
}

// A perceptual Rec.709-primary sRGB domain keeps grain stable and has a finite slope at black.
static inline ColorRgb color_to_work(ColorRgb v, ColorParameters p)
{
    if (p.sourceIsWork) return v;
    return color_curve_rgb(color_matrix(color_curve_rgb(v, p.sourceCurve, 0), p.to709), ColorSRGB, 1);
}

static inline ColorRgb color_from_work(ColorRgb v, ColorParameters p)
{
    if (p.outputIsWork) return v;
    return color_curve_rgb(color_matrix(color_curve_rgb(v, ColorSRGB, 0), p.from709), p.outputCurve, 1);
}

#ifndef __cplusplus
#pragma OPENCL FP_CONTRACT DEFAULT
#endif
#endif
