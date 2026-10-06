// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <algorithm>
#include <array>
#include "ColorMath.h"
#include "FilmModules.h"

namespace color {

inline ColorRgb cameraBalance(double exposure, double temperature, double tint)
{
    const float gain = std::pow(2.0f, static_cast<float>(exposure));
    const float warm = static_cast<float>(temperature) * 0.085f;
    const float green = static_cast<float>(tint) * 0.065f;
    return {gain * (1.0f + warm) * (1.0f - green * 0.30f), gain * (1.0f + green),
            gain * (1.0f - warm) * (1.0f - green * 0.30f)};
}

enum SpaceId {
    AlexaLogC3, ArriLogC4, SonyCine, SonyGamut3, DaVinciIntermediate,
    Rec709Gamma24, SRGB, BlackmagicGen5, RedLog3G10, CanonLog2, CanonLog3,
    PanasonicVLog, ACEScct, ACEScg, LinearRec709, SpaceCount
};

struct Space {
    const char* label;
    int curve;
    std::array<double, 6> primaries;
    double wx = 0.3127, wy = 0.3290;
};

inline const std::array<Space, SpaceCount>& spaces()
{
    static const std::array<double, 6> rec709 {0.64, 0.33, 0.30, 0.60, 0.15, 0.06};
    static const std::array<double, 6> ap1 {0.713, 0.293, 0.165, 0.830, 0.128, 0.044};
    static const std::array<Space, SpaceCount> list {{
        {"ARRI Alexa LogC3 / Wide Gamut 3 (EI 800)", ColorLogC3, {0.684, 0.313, 0.221, 0.848, 0.0861, -0.1020}},
        {"ARRI LogC4 / Wide Gamut 4", ColorLogC4, {0.7347, 0.2653, 0.1424, 0.8576, 0.0991, -0.0308}},
        {"Sony S-Log3 / S-Gamut3.Cine", ColorSLog3, {0.766, 0.275, 0.225, 0.800, 0.089, -0.087}},
        {"Sony S-Log3 / S-Gamut3", ColorSLog3, {0.730, 0.280, 0.140, 0.855, 0.100, -0.050}},
        {"DaVinci Wide Gamut / Intermediate", ColorIntermediate, {0.8000, 0.3130, 0.1682, 0.9877, 0.0790, -0.1155}},
        {"Rec.709 / Gamma 2.4", ColorGamma24, rec709},
        {"sRGB", ColorSRGB, rec709},
        {"Blackmagic Wide Gamut / Film Gen 5", ColorFilmGen5, {0.7177215, 0.3171181, 0.2280410, 0.8615690, 0.1005841, -0.0820452}, 0.3127170, 0.3290312},
        {"REDWideGamutRGB / Log3G10", ColorLog3G10, {0.780308, 0.304253, 0.121595, 1.493994, 0.095612, -0.084589}},
        {"Canon Cinema Gamut / Canon Log 2", ColorCanonLog2, {0.740, 0.270, 0.170, 1.140, 0.080, -0.100}},
        {"Canon Cinema Gamut / Canon Log 3", ColorCanonLog3, {0.740, 0.270, 0.170, 1.140, 0.080, -0.100}},
        {"Panasonic V-Gamut / V-Log", ColorVLog, {0.730, 0.280, 0.165, 0.840, 0.100, -0.030}},
        {"ACEScct / AP1", ColorACEScct, ap1, 0.32168, 0.33767},
        {"ACEScg / AP1 Linear", ColorLinear, ap1, 0.32168, 0.33767},
        {"Linear / Rec.709", ColorLinear, rec709}
    }};
    return list;
}

inline constexpr std::array<int, 6> OutputSpaces {Rec709Gamma24, Rec709Gamma24, DaVinciIntermediate, ACEScct, SRGB, LinearRec709};
inline constexpr std::array<const char*, 6> OutputLabels {
    "Same as Input", "Rec.709 / Gamma 2.4", "DaVinci Wide Gamut / Intermediate", "ACEScct / AP1", "sRGB", "Linear / Rec.709"
};

using Matrix = std::array<double, 9>;

inline Matrix multiply(const Matrix& a, const Matrix& b)
{
    Matrix o {};
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            for (int k = 0; k < 3; ++k) o[r * 3 + c] += a[r * 3 + k] * b[k * 3 + c];
    return o;
}

inline Matrix inverse(const Matrix& m)
{
    Matrix o {m[4]*m[8]-m[5]*m[7], m[2]*m[7]-m[1]*m[8], m[1]*m[5]-m[2]*m[4],
              m[5]*m[6]-m[3]*m[8], m[0]*m[8]-m[2]*m[6], m[2]*m[3]-m[0]*m[5],
              m[3]*m[7]-m[4]*m[6], m[1]*m[6]-m[0]*m[7], m[0]*m[4]-m[1]*m[3]};
    const double det = m[0]*o[0] + m[1]*o[3] + m[2]*o[6];
    for (double& v : o) v /= det;
    return o;
}

inline Matrix rgbToXYZ(const Space& s)
{
    Matrix m {};
    for (int c = 0; c < 3; ++c) {
        const double x = s.primaries[c * 2], y = s.primaries[c * 2 + 1];
        m[c] = x / y; m[c + 3] = 1.0; m[c + 6] = (1.0 - x - y) / y;
    }
    const Matrix inv = inverse(m);
    const std::array<double, 3> white {s.wx / s.wy, 1.0, (1.0 - s.wx - s.wy) / s.wy};
    for (int c = 0; c < 3; ++c) {
        const double scale = inv[c * 3] * white[0] + inv[c * 3 + 1] * white[1] + inv[c * 3 + 2] * white[2];
        for (int r = 0; r < 3; ++r) m[r * 3 + c] *= scale;
    }
    return m;
}

inline Matrix adaptToD65(const Space& s)
{
    if (s.wx == 0.3127 && s.wy == 0.3290) return {1,0,0, 0,1,0, 0,0,1};
    const Matrix bradford {0.8951,0.2664,-0.1614, -0.7502,1.7135,0.0367, 0.0389,-0.0685,1.0296};
    const std::array<double, 3> src {s.wx / s.wy, 1, (1 - s.wx - s.wy) / s.wy};
    const std::array<double, 3> dst {0.3127 / 0.3290, 1, (1 - 0.3127 - 0.3290) / 0.3290};
    Matrix diagonal {};
    for (int r = 0; r < 3; ++r) {
        double a = 0, b = 0;
        for (int c = 0; c < 3; ++c) { a += bradford[r*3+c]*src[c]; b += bradford[r*3+c]*dst[c]; }
        diagonal[r*3+r] = b / a;
    }
    return multiply(inverse(bradford), multiply(diagonal, bradford));
}

inline const std::array<Matrix, SpaceCount>& to709Matrices()
{
    static const auto matrices = [] {
        std::array<Matrix, SpaceCount> result {};
        const Matrix xyzTo709 = inverse(rgbToXYZ(spaces()[Rec709Gamma24]));
        for (size_t i = 0; i < result.size(); ++i)
            result[i] = multiply(xyzTo709, multiply(adaptToD65(spaces()[i]), rgbToXYZ(spaces()[i])));
        return result;
    }();
    return matrices;
}

inline ColorParameters prepare(int source, int output, bool textureOnly)
{
    source = std::clamp(source, 0, static_cast<int>(spaces().size()) - 1);
    output = std::clamp(output, 0, static_cast<int>(OutputSpaces.size()) - 1);
    const int destination = textureOnly || output == 0 ? source : OutputSpaces[output];
    ColorParameters p {};
    p.sourceCurve = spaces()[source].curve;
    p.outputCurve = spaces()[destination].curve;
    p.sourceIsWork = source == SRGB;
    p.outputIsWork = destination == SRGB;
    const Matrix& a = to709Matrices()[source];
    const Matrix b = inverse(to709Matrices()[destination]);
    for (int i = 0; i < 9; ++i) { p.to709[i] = static_cast<float>(a[i]); p.from709[i] = static_cast<float>(b[i]); }
    return p;
}

inline ColorParameters prepare(const float* settings)
{
    const int modules = film::modulesForSettings(settings);
    return prepare(static_cast<int>(settings[26]), static_cast<int>(settings[27]), !(modules & (film::Negative | film::Development | film::Print)));
}

} // namespace color
