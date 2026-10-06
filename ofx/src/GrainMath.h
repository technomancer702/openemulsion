// SPDX-License-Identifier: MPL-2.0

#ifndef OPENEMULSION_GRAIN_MATH_H
#define OPENEMULSION_GRAIN_MATH_H

// This file is compiled by both C++ and OpenCL so rendering uses identical math.
#ifdef __cplusplus
#include <cmath>
#include <cstdint>
typedef uint32_t GrainUInt;
#define GRAIN_FLOOR std::floor
#define GRAIN_SQRT std::sqrt
#define GRAIN_ABS std::abs
#else
// Match CPU coordinate rounding so a GPU FMA cannot move a grain lattice sample.
#pragma OPENCL FP_CONTRACT OFF
typedef uint GrainUInt;
#define GRAIN_FLOOR floor
#define GRAIN_SQRT sqrt
#define GRAIN_ABS fabs
#endif

typedef struct GrainVector { float r, g, b; } GrainVector;
typedef struct GrainParameters {
    float inverseSize;
    float detailMix;
    float roughness;
    float color;
    float amount;
    float shadows;
    float midtones;
    float highlights;
    GrainUInt seed;
    int debug;
    float inverseStretch;
    float red, green, blue;
} GrainParameters;

static inline GrainUInt grain_hash(GrainUInt value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

static inline float grain_random(GrainUInt value)
{
    return (float)(grain_hash(value) & 0x00ffffffu) / 16777215.0f - 0.5f;
}

static inline GrainVector grain_corner(int x, int y, GrainUInt seed)
{
    GrainUInt base = (GrainUInt)x * 0x9e3779b9u ^ (GrainUInt)y * 0x85ebca6bu ^ seed;
    GrainVector result = {grain_random(base), grain_random(base + 0x68bc21ebu), grain_random(base + 0x02e5be93u)};
    return result;
}

static inline float grain_interpolate(float a, float b, float c, float d, float tx, float ty, float normalization)
{
    float top = a + (b - a) * tx;
    float bottom = c + (d - c) * tx;
    return (top + (bottom - top) * ty) * normalization;
}

static inline GrainVector grain_field(float x, float y, GrainUInt seed)
{
    int ix = (int)GRAIN_FLOOR(x), iy = (int)GRAIN_FLOOR(y);
    float tx = x - (float)ix, ty = y - (float)iy;
    tx = tx * tx * (3.0f - 2.0f * tx);
    ty = ty * ty * (3.0f - 2.0f * ty);
    float normalization = 1.0f / GRAIN_SQRT(((1.0f - tx) * (1.0f - tx) + tx * tx) *
                                          ((1.0f - ty) * (1.0f - ty) + ty * ty));
    GrainVector a = grain_corner(ix, iy, seed), b = grain_corner(ix + 1, iy, seed);
    GrainVector c = grain_corner(ix, iy + 1, seed), d = grain_corner(ix + 1, iy + 1, seed);
    GrainVector result = {
        grain_interpolate(a.r, b.r, c.r, d.r, tx, ty, normalization),
        grain_interpolate(a.g, b.g, c.g, d.g, tx, ty, normalization),
        grain_interpolate(a.b, b.b, c.b, d.b, tx, ty, normalization)
    };
    return result;
}

static inline float grain_smooth(float low, float high, float value)
{
    float t = (value - low) / (high - low);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

static inline float grain_shape(float value, float roughness)
{
    return value * (1.0f + roughness * (GRAIN_ABS(value) * 1.5f - 0.25f));
}

static inline GrainVector grain_delta(int x, int y, float luminance, GrainParameters p)
{
    float px = ((float)x + 0.5f) * p.inverseSize * p.inverseStretch;
    float py = ((float)y + 0.5f) * p.inverseSize;
    // Rotated, independently seeded layers avoid axis-aligned grain cells.
    float u = px * 0.8f + py * 0.6f + grain_random(p.seed + 11u) * 8.0f;
    float v = py * 0.8f - px * 0.6f + grain_random(p.seed + 29u) * 8.0f;
    GrainVector primary = grain_field(u, v, p.seed);
    GrainVector detail = grain_field(px * 2.1f - py * 0.7f, px * 0.7f + py * 2.1f, p.seed ^ 0xa511e9b3u);
    float normalization = 1.0f / GRAIN_SQRT(1.0f + p.detailMix * p.detailMix);
    GrainVector n = {(primary.r + detail.r * p.detailMix) * normalization,
                     (primary.g + detail.g * p.detailMix) * normalization,
                     (primary.b + detail.b * p.detailMix) * normalization};
    float mono = (n.r + n.g + n.b) * 0.5773502692f;
    float shadow = 1.0f - grain_smooth(0.08f, 0.45f, luminance);
    float high = grain_smooth(0.45f, 0.90f, luminance);
    float tonal = p.shadows * shadow + p.midtones * (1.0f - shadow - high) + p.highlights * high;
    float amplitude = p.amount * (p.debug ? 0.70f : 0.12f * tonal);
    GrainVector result = {
        grain_shape(mono + (n.r - mono) * p.color, p.roughness) * amplitude * p.red,
        grain_shape(mono + (n.g - mono) * p.color, p.roughness) * amplitude * p.green,
        grain_shape(mono + (n.b - mono) * p.color, p.roughness) * amplitude * p.blue
    };
    return result;
}

#undef GRAIN_FLOOR
#undef GRAIN_SQRT
#undef GRAIN_ABS
#ifndef __cplusplus
#pragma OPENCL FP_CONTRACT DEFAULT
#endif
#endif
