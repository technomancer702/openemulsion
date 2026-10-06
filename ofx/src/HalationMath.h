// SPDX-License-Identifier: MPL-2.0

#ifndef OPENEMULSION_HALATION_MATH_H
#define OPENEMULSION_HALATION_MATH_H

#ifdef __cplusplus
#include <cmath>
#define HALATION_MAX std::fmax
#define HALATION_MIN std::fmin
#else
#define HALATION_MAX fmax
#define HALATION_MIN fmin
#pragma OPENCL FP_CONTRACT OFF
#endif

typedef struct HalationParameters {
    float threshold, inverseTransition, red, green, blue;
} HalationParameters;

static inline float halation_key(float r, float g, float b, HalationParameters p)
{
    const float hot = HALATION_MAX(r, HALATION_MAX(g, b));
    const float luma = r * 0.2126f + g * 0.7152f + b * 0.0722f;
    const float t = HALATION_MAX(0.0f, HALATION_MIN(1.0f,
        (HALATION_MAX(hot, luma * 1.20f) - p.threshold) * p.inverseTransition));
    return t * t * (3.0f - 2.0f * t);
}

static inline float halation_signal(float tight, float broad, float center, float amount, float aura)
{
    return HALATION_MAX(tight - center * 0.20f, 0.0f) * amount * 1.45f +
           HALATION_MAX(broad - center * 0.08f, 0.0f) * aura * 0.85f;
}

#ifndef __cplusplus
#pragma OPENCL FP_CONTRACT DEFAULT
#endif
#endif
