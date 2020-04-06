#pragma once

#include <algorithm>

inline float Clamp(const float& v, const float& l, const float& r){
    return std::min(std::max(v, l), r);
}

inline float GammaCorrect(const float& v) {
    return v < 0.0031308f ? 12.92f * v : 1.055f * std::pow(v, 1.0f / 2.4f) - 0.055f;
}

inline float InverseGammaCorrect(const float& v) {
    return v <= 0.04045f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f);
}


struct sRGB {
    sRGB(float v = 0) :r(v), g(v), b(v) {}
    sRGB(float _r, float _g, float _b) :r(_r), g(_g), b(_b) {}
    float r, g, b;
};