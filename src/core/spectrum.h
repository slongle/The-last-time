#pragma once

#include <assert.h>
#include <cmath>
#include <iostream>

#include "color.h"
#include "vector.h"

class RGBSpectrum {
public:
    RGBSpectrum(float v = 0) :r(v), g(v), b(v) {}
    RGBSpectrum(float _r, float _g, float _b) :r(_r), g(_g), b(_b) {}
    RGBSpectrum(const Float3& v) :r(std::fabs(v.x)), g(std::fabs(v.y)), b(std::fabs(v.z)) {}

    RGBSpectrum operator * (float v) const { return RGBSpectrum(r * v, g * v, b * v); }
    RGBSpectrum operator / (float v) const { assert(v != 0.f); return RGBSpectrum(r / v, g / v, b / v); }
    RGBSpectrum& operator /= (float v) { assert(v != 0.f); r /= v; g /= v; b /= v; return *this; }

    RGBSpectrum operator + (const RGBSpectrum& s) const { return RGBSpectrum(r + s.r, g + s.g, b + s.b); }
    RGBSpectrum& operator += (const RGBSpectrum& s) { r += s.r; g += s.g; b += s.b; return *this; }
    RGBSpectrum operator * (const RGBSpectrum& v) const { return RGBSpectrum(r * v.r, g * v.g, b * v.b); }
    RGBSpectrum& operator *= (const RGBSpectrum& v) { r *= v.r; g *= v.g; b *= v.b; return *this; }

    float y() const { return (r * 0.299) + (g * 0.587) + (b * 0.114); }
    bool IsBlack() const { return r == 0 && g == 0 && b == 0; }

    sRGB TosRGB() const {
        return sRGB(GammaCorrect(r), GammaCorrect(g), GammaCorrect(b));
    }

    float r, g, b;

    friend std::ostream& operator <<(std::ostream& os, const RGBSpectrum& s) {
        os << s.r << ' ' << s.g << ' ' << s.b;
        return os;
    }
};

inline float MaxComponent(const RGBSpectrum& s) { return std::max(s.r, std::max(s.g, s.b)); }

typedef RGBSpectrum Spectrum;