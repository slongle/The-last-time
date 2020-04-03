#pragma once

#include <algorithm>

inline float Clamp(const float& v, const float& l, const float& r){
    return std::min(std::max(v, l), r);
}

struct sRGB {
    sRGB(float v = 0) :r(v), g(v), b(v) {}
    sRGB(float _r, float _g, float _b) :r(_r), g(_g), b(_b) {}
    float r, g, b;
};