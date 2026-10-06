// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <vector>
#include "BloomConfig.h"

namespace bloom {

struct Pixel { float r = 0, g = 0, b = 0, padding = 0; };

struct Filter {
    int radius;
    std::vector<float> weights;
    explicit Filter(float sigma) : radius(static_cast<int>(std::ceil(3.0f * sigma))), weights(2 * radius + 1)
    {
        float sum = 0;
        for (int i = -radius; i <= radius; ++i) {
            weights[i + radius] = std::exp(-0.5f * i * i / (sigma * sigma));
            sum += weights[i + radius];
        }
        for (auto& w : weights) w /= sum;
    }
};

// Three-channel linear light, with the same dense/area-averaged sampling as halation.
class Blur {
public:
    Blur(int sourceWidth, int sourceHeight, const Configuration& config)
        : width((sourceWidth + config.downsample - 1) / config.downsample),
          height((sourceHeight + config.downsample - 1) / config.downsample),
          filter_(config.sigma), step_(config.downsample), sourceWidth_(sourceWidth), sourceHeight_(sourceHeight),
          image_(static_cast<size_t>(width) * height), temporary_(image_.size()) {}

    template<class Reader> void extractRows(int begin, int end, Reader read)
    {
        for (int y = begin; y < end; ++y) for (int x = 0; x < width; ++x) {
            Pixel sum; int count = 0;
            for (int dy = 0; dy < step_ && y * step_ + dy < sourceHeight_; ++dy)
                for (int dx = 0; dx < step_ && x * step_ + dx < sourceWidth_; ++dx) {
                    const auto c = read(x * step_ + dx, y * step_ + dy);
                    sum.r += c.r; sum.g += c.g; sum.b += c.b; ++count;
                }
            image_[static_cast<size_t>(y) * width + x] = {sum.r/count, sum.g/count, sum.b/count, 0};
        }
    }

    void blurRows(int begin, int end, bool horizontal)
    {
        const auto& input = horizontal ? image_ : temporary_;
        auto& output = horizontal ? temporary_ : image_;
        for (int y = begin; y < end; ++y) for (int x = 0; x < width; ++x) {
            Pixel sum;
            for (int i = -filter_.radius; i <= filter_.radius; ++i) {
                const int sx = horizontal ? std::clamp(x+i,0,width-1) : x;
                const int sy = horizontal ? y : std::clamp(y+i,0,height-1);
                const auto& p = input[static_cast<size_t>(sy)*width+sx];
                const float w = filter_.weights[i+filter_.radius];
                sum.r += p.r*w; sum.g += p.g*w; sum.b += p.b*w;
            }
            output[static_cast<size_t>(y)*width+x] = sum;
        }
    }

    ColorRgb sample(int x, int y) const
    {
        const float fx = std::clamp((x-(step_-1)*0.5f)/step_,0.0f,static_cast<float>(width-1));
        const float fy = std::clamp((y-(step_-1)*0.5f)/step_,0.0f,static_cast<float>(height-1));
        const int x0 = static_cast<int>(fx), y0 = static_cast<int>(fy);
        const int x1 = std::min(x0+1,width-1), y1 = std::min(y0+1,height-1);
        auto rgb = [](Pixel p) -> ColorRgb { return {p.r,p.g,p.b}; };
        const auto top = response_mix(rgb(image_[static_cast<size_t>(y0)*width+x0]),rgb(image_[static_cast<size_t>(y0)*width+x1]),fx-x0);
        const auto bottom = response_mix(rgb(image_[static_cast<size_t>(y1)*width+x0]),rgb(image_[static_cast<size_t>(y1)*width+x1]),fx-x0);
        return response_mix(top,bottom,fy-y0);
    }

    int width, height;
private:
    Filter filter_;
    int step_, sourceWidth_, sourceHeight_;
    std::vector<Pixel> image_, temporary_;
};

} // namespace bloom
