// SPDX-License-Identifier: MPL-2.0

#ifndef OPENEMULSION_FILM_RESPONSE_MATH_H
#define OPENEMULSION_FILM_RESPONSE_MATH_H

#ifdef __cplusplus
#include "ColorMath.h"
#define RESPONSE_MAX std::fmax
#define RESPONSE_MIN std::fmin
#else
#define RESPONSE_MAX fmax
#define RESPONSE_MIN fmin
#endif

typedef struct ResponseTone { float contrast, toe, knee, ceiling; } ResponseTone;
typedef struct FilmResponseParameters {
    int system;
    float density;
    ResponseTone negativeTone, printTone;
    float negativeMatrix[9], printMatrix[9];
    float negativeSat, negativeCompression, skinHue, printSat, printCast, printLift;
    ColorRgb printGain;
    float printGamutKnee;
} FilmResponseParameters;

static inline float response_clamp(float x, float lo, float hi)
{
    return RESPONSE_MAX(lo, RESPONSE_MIN(x, hi));
}

static inline float response_luma(ColorRgb c)
{
    return c.r * 0.2126f + c.g * 0.7152f + c.b * 0.0722f;
}

static inline ColorRgb response_saturation(ColorRgb c, float amount)
{
    const float y = response_luma(c);
    ColorRgb o = {y + (c.r - y) * amount, y + (c.g - y) * amount, y + (c.b - y) * amount};
    return o;
}

#ifndef __cplusplus
#pragma OPENCL FP_CONTRACT OFF
#endif

// Monotonic, C1 toe/shoulder joins and a fixed middle-gray pivot; no per-pixel logs.
static inline float response_tone(float x, ResponseTone p)
{
    const float pivot = 0.46135613f;
    const float v = RESPONSE_MAX(x, 0.0f);
    float y = v < pivot ? pivot * v / (v + p.contrast * (pivot - v)) : pivot + p.contrast * (v - pivot);
    if (y < 0.23f) {
        const float d = 1.0f - y / 0.23f;
        y *= 1.0f - p.toe * d * d;
    }
    if (y > p.knee) {
        const float headroom = p.ceiling - p.knee, high = y - p.knee;
        y = p.knee + headroom * (high / (headroom + high));
    }
    return y;
}

static inline ColorRgb response_palette(ColorRgb c, const float* matrix)
{
    const float y = response_luma(c);
    ColorRgb o = color_matrix(c, matrix);
    const float correction = y - response_luma(o);
    o.r += correction; o.g += correction; o.b += correction;
    return o;
}

static inline ColorRgb response_luminance_curve(ColorRgb c, ResponseTone p)
{
    const float y = response_luma(c);
    if (y <= 0.0f) { ColorRgb black = {0,0,0}; return black; }
    const float scale = response_tone(y, p) / y;
    c.r *= scale; c.g *= scale; c.b *= scale;
    return c;
}

// Radial compression approaches the RGB boundary smoothly without changing hue/luminance.
static inline ColorRgb response_gamut(ColorRgb c, float ceiling, float knee)
{
    const float y = response_luma(c);
    const float hi = RESPONSE_MAX(c.r, RESPONSE_MAX(c.g, c.b)) - y;
    const float lo = y - RESPONSE_MIN(c.r, RESPONSE_MIN(c.g, c.b));
    const float distance = RESPONSE_MAX(hi / RESPONSE_MAX(ceiling - y, 1e-6f), lo / RESPONSE_MAX(y, 1e-6f));
    if (distance <= knee) return c;
    const float excess = distance - knee, width = 1.0f - knee;
    const float compressed = knee + width * (excess / (width + excess));
    const float scale = compressed / distance;
    ColorRgb o = {y + (c.r - y) * scale, y + (c.g - y) * scale, y + (c.b - y) * scale};
    return o;
}

static inline float response_smooth(float lo, float hi, float x)
{
    const float t = response_clamp((x - lo) / (hi - lo), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static inline ColorRgb response_skin(ColorRgb c, float hue)
{
    if (hue == 0.0f) return c;
    const float y = response_luma(c);
    const float chroma = RESPONSE_MAX(c.r, RESPONSE_MAX(c.g, c.b)) - RESPONSE_MIN(c.r, RESPONSE_MIN(c.g, c.b));
    const float mask = response_smooth(0.005f, 0.055f, c.r - c.g) * response_smooth(0.005f, 0.065f, c.g - c.b) *
        (1.0f - response_smooth(0.65f, 1.10f, chroma / RESPONSE_MAX(y, 0.05f))) *
        response_smooth(0.03f, 0.15f, y) * (1.0f - response_smooth(0.80f, 1.1f, y));
    const float delta = hue * mask * 0.035f;
    ColorRgb offset = {0.70f, -0.40f, 0.60f};
    const float correction = response_luma(offset);
    c.r += (offset.r - correction) * delta;
    c.g += (offset.g - correction) * delta;
    c.b += (offset.b - correction) * delta;
    return c;
}

static inline ColorRgb response_negative(ColorRgb c, FilmResponseParameters p)
{
    if (p.system == 4) {
        const float mono = c.r * 0.30f + c.g * 0.59f + c.b * 0.11f;
        c.r = c.g = c.b = mono;
    } else {
        c = response_palette(c, p.negativeMatrix);
        c = response_skin(c, p.skinHue);
    }
    c = response_luminance_curve(c, p.negativeTone);
    c = response_saturation(c, p.negativeSat);
    const float y = response_luma(c);
    const float chroma = RESPONSE_MAX(c.r, RESPONSE_MAX(c.g, c.b)) - RESPONSE_MIN(c.r, RESPONSE_MIN(c.g, c.b));
    const float relative = chroma / RESPONSE_MAX(y, 0.08f);
    const float density = 1.0f - p.density * 0.18f * (relative / (1.0f + relative));
    c.r *= density; c.g *= density; c.b *= density;
    if (p.negativeCompression > 0.0f) {
        const float ceiling = RESPONSE_MAX(p.negativeTone.ceiling, response_luma(c) + 0.05f);
        c = response_gamut(c, ceiling, 0.98f - p.negativeCompression * 0.53f);
    }
    return c;
}

static inline ColorRgb response_print(ColorRgb c, FilmResponseParameters p)
{
    c = color_balance(c, p.printGain);
    c = response_palette(c, p.printMatrix);
    c = response_luminance_curve(c, p.printTone);
    c = response_saturation(c, p.printSat);
    float y = response_luma(c);
    // A neutral lift raises black without tinting it; casts disappear at both endpoints.
    const float lifted = y * (1.0f - p.printLift) + p.printLift;
    c.r += lifted - y; c.g += lifted - y; c.b += lifted - y;
    y = response_clamp(y, 0.0f, 1.0f);
    const float shadow = 4.0f * y * (1.0f - y) * (1.0f - y);
    const float high = 4.0f * y * y * (1.0f - y);
    ColorRgb bias = {p.printCast * (high * 0.024f - shadow * 0.014f),
                     p.printCast * (high * 0.012f - shadow * 0.004f),
                     p.printCast * (shadow * 0.024f - high * 0.028f)};
    const float correction = response_luma(bias);
    c.r += bias.r - correction; c.g += bias.g - correction; c.b += bias.b - correction;
    return response_gamut(c, 1.0f, p.printGamutKnee);
}

#ifndef __cplusplus
#pragma OPENCL FP_CONTRACT DEFAULT
#endif
#endif
