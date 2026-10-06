// SPDX-License-Identifier: MPL-2.0

#ifndef OPENEMULSION_BLOOM_MATH_H
#define OPENEMULSION_BLOOM_MATH_H

#ifdef __cplusplus
#include "FilmResponseMath.h"
#define BLOOM_MAX std::fmax
#else
#define BLOOM_MAX fmax
#endif

typedef struct BloomParameters {
    float threshold, inverseTransition, amount, color, protection;
} BloomParameters;

static inline ColorRgb bloom_extract(ColorRgb source, ColorParameters color, BloomParameters p)
{
    ColorRgb c = color_matrix(color_curve_rgb(source, color.sourceCurve, 0), color.to709);
    c.r = BLOOM_MAX(c.r, 0.0f); c.g = BLOOM_MAX(c.g, 0.0f); c.b = BLOOM_MAX(c.b, 0.0f);
    const float peak = BLOOM_MAX(c.r, BLOOM_MAX(c.g, c.b));
    float key = response_clamp((peak - p.threshold) * p.inverseTransition, 0.0f, 1.0f);
    key = key * key * (3.0f - 2.0f * key);
    // Bound extreme scene highlights while preserving their chromatic direction.
    const float scale = key / (1.0f + peak);
    const float y = response_luma(c);
    ColorRgb o = {(y + (c.r - y) * p.color) * scale,
                  (y + (c.g - y) * p.color) * scale,
                  (y + (c.b - y) * p.color) * scale};
    return o;
}

static inline ColorRgb bloom_composite(ColorRgb c, ColorRgb glow, BloomParameters p, int matte)
{
    if (matte) {
        ColorRgb o = {glow.r * p.amount * 0.35f, glow.g * p.amount * 0.35f, glow.b * p.amount * 0.35f};
        return color_curve_rgb(o, ColorSRGB, 1);
    }
    if (p.amount <= 0.0f || (glow.r == 0.0f && glow.g == 0.0f && glow.b == 0.0f)) return c;
    ColorRgb linear = color_curve_rgb(c, ColorSRGB, 0);
    const float peak = BLOOM_MAX(linear.r, BLOOM_MAX(linear.g, linear.b));
    const float scale = p.amount * 0.35f * (1.0f - p.protection * response_smooth(0.25f, 1.0f, peak));
    if (scale == 0.0f) return c;
    linear.r += glow.r * scale; linear.g += glow.g * scale; linear.b += glow.b * scale;
    return color_curve_rgb(linear, ColorSRGB, 1);
}

#undef BLOOM_MAX
#endif
