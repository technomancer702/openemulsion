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
    float colorStrength, toneStrength, printColorStrength, printToneStrength;
    float push, developmentContrast, richness, splitAmount;
    float splitPivot, splitWidth, splitShadows, splitHighlights;
    ColorRgb splitTint;
    float selectiveAmount, selectiveHue, selectiveRange, selectiveSoftness, selectiveSaturation;
    int selectiveView;
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

// Radial compression preserves weighted brightness/chroma direction in the working RGB domain.
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

static inline ColorRgb response_mix(ColorRgb a, ColorRgb b, float amount)
{
    if (amount <= 0.0f) return a;
    if (amount >= 1.0f) return b;
    ColorRgb c = {a.r + (b.r - a.r) * amount, a.g + (b.g - a.g) * amount, a.b + (b.b - a.b) * amount};
    return c;
}

static inline ColorRgb response_apply_tone(ColorRgb c, ResponseTone tone, float strength)
{
    return strength > 0.0f ? response_mix(c, response_luminance_curve(c, tone), strength) : c;
}

static inline int response_clamps_negative(int modules, FilmResponseParameters p)
{
    return ((modules & 1) && (p.colorStrength > 0.0f || p.toneStrength > 0.0f)) ||
           ((modules & 2) && (p.printColorStrength > 0.0f || p.printToneStrength > 0.0f));
}

// Print and texture stages run after the negative; do not let them recolor Mono.
static inline ColorRgb response_finish(ColorRgb c, int modules, FilmResponseParameters p)
{
    if (!(modules & 1) || p.system != 4 || p.colorStrength <= 0.0f) return c;
    const float y = response_luma(c);
    ColorRgb neutral = {y, y, y};
    return response_mix(c, neutral, p.colorStrength);
}

static inline ColorRgb response_negative(ColorRgb c, FilmResponseParameters p)
{
    const ColorRgb original = c;
    if (p.colorStrength > 0.0f) {
        if (p.system == 4) {
            const float mono = c.r * 0.30f + c.g * 0.59f + c.b * 0.11f;
            c.r = c.g = c.b = mono;
        } else {
            c = response_palette(c, p.negativeMatrix);
            c = response_skin(c, p.skinHue);
        }
        c = response_mix(original, c, p.colorStrength);
    }
    c = response_apply_tone(c, p.negativeTone, p.toneStrength);
    if (p.colorStrength <= 0.0f) return c;
    const ColorRgb beforeColor = c;
    c = response_saturation(c, p.negativeSat);
    const float y = response_luma(c);
    const float chroma = RESPONSE_MAX(c.r, RESPONSE_MAX(c.g, c.b)) - RESPONSE_MIN(c.r, RESPONSE_MIN(c.g, c.b));
    const float relative = chroma / RESPONSE_MAX(y, 0.08f);
    const float density = 1.0f - p.density * 0.18f * (relative / (1.0f + relative));
    c.r *= density; c.g *= density; c.b *= density;
    if (p.negativeCompression > 0.0f) {
        const float ceiling = RESPONSE_MAX(p.toneStrength > 0.0f ? p.negativeTone.ceiling : 1.0f, response_luma(c) + 0.05f);
        // A fixed target makes Amount continuous at zero and uniform across the slider.
        c = response_mix(c, response_gamut(c, ceiling, 0.45f), p.negativeCompression);
    }
    return response_mix(beforeColor, c, p.colorStrength);
}

static inline ColorRgb response_negative_stage(ColorRgb c, FilmResponseParameters p)
{
    c = response_negative(c, p);
    // Match the negative-only node's floor before handing partial compression to later stages.
    if (response_clamps_negative(1, p)) {
        c.r = RESPONSE_MAX(c.r, 0.0f); c.g = RESPONSE_MAX(c.g, 0.0f); c.b = RESPONSE_MAX(c.b, 0.0f);
    }
    return c;
}

// Creative development, not calibrated chemistry. Gray exposure stays anchored.
static inline ColorRgb response_development(ColorRgb c, FilmResponseParameters p)
{
    if (p.push == 0.0f && p.richness == 0.0f && p.splitAmount == 0.0f) return c;
    float y = response_luma(c);
    if (p.push != 0.0f && y >= 0.0f) {
        const float pivot = 0.46135613f;
        float developed = y < pivot ? pivot * y / (y + p.developmentContrast * (pivot - y)) :
            pivot + p.developmentContrast * (y - pivot);
        const float shadow = 1.0f - response_clamp(y / pivot, 0.0f, 1.0f);
        if (p.push > 0.0f) developed += p.push * 0.012f * shadow * shadow;
        else developed *= 1.0f + p.push * 0.10f * shadow * shadow;
        // Scale chroma with brightness except near black, where fog is neutral.
        const float scale = 1.0f + (developed - y) / RESPONSE_MAX(y, 0.05f);
        c.r = developed + (c.r - y) * scale;
        c.g = developed + (c.g - y) * scale;
        c.b = developed + (c.b - y) * scale;
        y = developed;
    }
    if (p.richness != 0.0f) {
        const float chroma = RESPONSE_MAX(c.r, RESPONSE_MAX(c.g, c.b)) - RESPONSE_MIN(c.r, RESPONSE_MIN(c.g, c.b));
        const float relative = chroma / RESPONSE_MAX(y, 0.08f);
        if (chroma > 0.0f) c = response_saturation(c, 1.0f + p.richness * 0.65f / (1.0f + 4.0f * relative * relative));
    }
    if (p.splitAmount != 0.0f) {
        const float low = p.splitPivot - p.splitWidth * 0.5f;
        const float high = p.splitPivot + p.splitWidth * 0.5f;
        const float shadows = 1.0f - response_smooth(0.0f, low, y);
        const float highlights = response_smooth(high, 1.25f, y);
        const float envelope = RESPONSE_MAX(y, 0.0f) / (RESPONSE_MAX(y, 0.0f) + 0.05f);
        const float weight = p.splitAmount * 0.08f * envelope *
            (shadows * p.splitShadows - highlights * p.splitHighlights);
        c.r += p.splitTint.r * weight;
        c.g += p.splitTint.g * weight;
        c.b += p.splitTint.b * weight;
    }
    return c;
}

static inline ColorRgb response_print(ColorRgb c, FilmResponseParameters p)
{
    c = color_balance(c, p.printGain);
    if (p.printColorStrength > 0.0f) c = response_mix(c, response_palette(c, p.printMatrix), p.printColorStrength);
    c = response_apply_tone(c, p.printTone, p.printToneStrength);
    if (p.printColorStrength > 0.0f) c = response_mix(c, response_saturation(c, p.printSat), p.printColorStrength);
    float y = response_luma(c);
    // A neutral lift raises black without tinting it; casts disappear at both endpoints.
    const float lift = p.printLift * p.printToneStrength;
    const float lifted = y * (1.0f - lift) + lift;
    c.r += lifted - y; c.g += lifted - y; c.b += lifted - y;
    if (p.printColorStrength <= 0.0f) return c;
    const ColorRgb beforeCast = c;
    y = response_clamp(y, 0.0f, 1.0f);
    const float shadow = 4.0f * y * (1.0f - y) * (1.0f - y);
    const float high = 4.0f * y * y * (1.0f - y);
    ColorRgb bias = {p.printCast * (high * 0.024f - shadow * 0.014f),
                     p.printCast * (high * 0.012f - shadow * 0.004f),
                     p.printCast * (shadow * 0.024f - high * 0.028f)};
    const float correction = response_luma(bias);
    c.r += bias.r - correction; c.g += bias.g - correction; c.b += bias.b - correction;
    const float ceiling = p.printToneStrength >= 1.0f ? 1.0f : RESPONSE_MAX(1.0f, response_luma(c) + 0.05f);
    return response_mix(beforeCast, response_gamut(c, ceiling, p.printGamutKnee), p.printColorStrength);
}

// Source-keyed hue selection avoids grain and creative print shifts changing the matte.
static inline float response_selective_mask(ColorRgb source, FilmResponseParameters p)
{
    source.r = RESPONSE_MAX(source.r, 0.0f);
    source.g = RESPONSE_MAX(source.g, 0.0f);
    source.b = RESPONSE_MAX(source.b, 0.0f);
    const float high = RESPONSE_MAX(source.r, RESPONSE_MAX(source.g, source.b));
    const float low = RESPONSE_MIN(source.r, RESPONSE_MIN(source.g, source.b));
    const float chroma = high - low;
    if (high <= 1e-7f || chroma <= 1e-7f) return 0.0f;
    const float saturation = chroma / high;
    const float saturationMask = p.selectiveSaturation <= 0.0f ? 1.0f :
        response_smooth(RESPONSE_MAX(0.0f, p.selectiveSaturation - 0.05f), p.selectiveSaturation, saturation);
    float hue = source.r == high ? (source.g - source.b) / chroma :
        source.g == high ? 2.0f + (source.b - source.r) / chroma : 4.0f + (source.r - source.g) / chroma;
    hue *= 60.0f;
    if (hue < 0.0f) hue += 360.0f;
    const float difference = COLOR_ABS(hue - p.selectiveHue);
    const float distance = RESPONSE_MIN(difference, 360.0f - difference);
    if (p.selectiveRange >= 180.0f) return saturationMask;
    const float edge = RESPONSE_MIN(180.0f, p.selectiveRange + p.selectiveSoftness);
    const float hueMask = edge <= p.selectiveRange ? (distance <= p.selectiveRange ? 1.0f : 0.0f) :
        1.0f - response_smooth(p.selectiveRange, edge, distance);
    return hueMask * saturationMask;
}

static inline ColorRgb response_selective(ColorRgb c, ColorRgb source, FilmResponseParameters p)
{
    if (p.selectiveAmount <= 0.0f && p.selectiveView == 0) return c;
    const float mask = response_selective_mask(source, p);
    if (p.selectiveView != 0) { ColorRgb matte = {mask,mask,mask}; return matte; }
    if (mask >= 1.0f) return c;
    return response_saturation(c, 1.0f - p.selectiveAmount * (1.0f - mask));
}

#ifndef __cplusplus
#pragma OPENCL FP_CONTRACT DEFAULT
#endif
#endif
