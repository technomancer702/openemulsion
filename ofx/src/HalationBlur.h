// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace halation {

struct Pair {
    float local = 0.0f;
    float aura = 0.0f;
};

inline float highlight(float r, float g, float b)
{
    const float hot = std::max({r, g, b});
    const float luma = r * 0.2126f + g * 0.7152f + b * 0.0722f;
    return std::clamp((std::max(hot, luma * 1.20f) - 0.48f) / 0.52f, 0.0f, 1.0f);
}

struct Filter {
    int radius;
    std::vector<Pair> weights;

    Filter(float amount, float size, float aura)
    {
        size = std::clamp(size, 0.0f, 2.0f);
        const float localSigma = 1.0f + size * 2.0f;
        const float auraSigma = 2.5f + size * 4.0f;
        const int localRadius = amount > 0.0f ? static_cast<int>(std::ceil(3.0f * localSigma)) : 0;
        const int auraRadius = aura > 0.0f ? static_cast<int>(std::ceil(3.0f * auraSigma)) : 0;
        radius = std::max(localRadius, auraRadius);
        weights.resize(2 * radius + 1);
        Pair sum;
        for (int i = -radius; i <= radius; ++i) {
            Pair& w = weights[i + radius];
            if (amount > 0.0f && std::abs(i) <= localRadius)
                w.local = std::exp(-0.5f * i * i / (localSigma * localSigma));
            if (aura > 0.0f && std::abs(i) <= auraRadius)
                w.aura = std::exp(-0.5f * i * i / (auraSigma * auraSigma));
            sum.local += w.local;
            sum.aura += w.aura;
        }
        for (Pair& w : weights) {
            if (sum.local > 0.0f) w.local /= sum.local;
            if (sum.aura > 0.0f) w.aura /= sum.aura;
        }
    }
};

// Half-resolution highlight extraction preserves small lights; dense separable
// filtering and bilinear reconstruction prevent displaced copies of them.
class Blur {
public:
    Blur(int sourceWidth, int sourceHeight, const Filter& filter)
        : width((sourceWidth + 1) / 2), height((sourceHeight + 1) / 2), filter_(filter),
          sourceWidth_(sourceWidth), sourceHeight_(sourceHeight),
          image_(static_cast<size_t>(width) * height), temporary_(image_.size()) {}

    template<class Reader>
    void extractRows(int begin, int end, Reader readHighlight)
    {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < width; ++x) {
                float sum = 0.0f;
                int count = 0;
                for (int dy = 0; dy < 2 && 2 * y + dy < sourceHeight_; ++dy) {
                    for (int dx = 0; dx < 2 && 2 * x + dx < sourceWidth_; ++dx) {
                        sum += readHighlight(2 * x + dx, 2 * y + dy);
                        ++count;
                    }
                }
                image_[static_cast<size_t>(y) * width + x] = {sum / count, sum / count};
            }
        }
    }

    void blurRows(int begin, int end, bool horizontal)
    {
        const auto& input = horizontal ? image_ : temporary_;
        auto& output = horizontal ? temporary_ : image_;
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < width; ++x) {
                Pair sum;
                for (int i = -filter_.radius; i <= filter_.radius; ++i) {
                    const int sx = horizontal ? std::clamp(x + i, 0, width - 1) : x;
                    const int sy = horizontal ? y : std::clamp(y + i, 0, height - 1);
                    const Pair& p = input[static_cast<size_t>(sy) * width + sx];
                    const Pair& w = filter_.weights[i + filter_.radius];
                    sum.local += p.local * w.local;
                    sum.aura += p.aura * w.aura;
                }
                output[static_cast<size_t>(y) * width + x] = sum;
            }
        }
    }

    Pair sample(int x, int y) const
    {
        const float fx = std::clamp((x - 0.5f) * 0.5f, 0.0f, static_cast<float>(width - 1));
        const float fy = std::clamp((y - 0.5f) * 0.5f, 0.0f, static_cast<float>(height - 1));
        const int x0 = static_cast<int>(fx), y0 = static_cast<int>(fy);
        const int x1 = std::min(x0 + 1, width - 1), y1 = std::min(y0 + 1, height - 1);
        const Pair& a = image_[static_cast<size_t>(y0) * width + x0];
        const Pair& b = image_[static_cast<size_t>(y0) * width + x1];
        const Pair& c = image_[static_cast<size_t>(y1) * width + x0];
        const Pair& d = image_[static_cast<size_t>(y1) * width + x1];
        const float tx = fx - x0, ty = fy - y0;
        const float topLocal = a.local + (b.local - a.local) * tx;
        const float bottomLocal = c.local + (d.local - c.local) * tx;
        const float topAura = a.aura + (b.aura - a.aura) * tx;
        const float bottomAura = c.aura + (d.aura - c.aura) * tx;
        return {topLocal + (bottomLocal - topLocal) * ty, topAura + (bottomAura - topAura) * ty};
    }

    int width, height;

private:
    Filter filter_;
    int sourceWidth_, sourceHeight_;
    std::vector<Pair> image_, temporary_;
};

inline float signal(Pair blurred, float center, float amount, float aura)
{
    return std::max(blurred.local - center * 0.20f, 0.0f) * amount * 1.45f +
           std::max(blurred.aura - center * 0.08f, 0.0f) * aura * 0.85f;
}

} // namespace halation
