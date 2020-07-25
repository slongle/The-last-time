#pragma once

#include <assert.h>
#include <cmath>
#include <iostream>

#include "color.h"
#include "vector.h"

inline float Sqrt(const float& a) { return std::sqrt(std::max(0.f, a)); }

static const int nCIESamples = 471;
extern const float CIE_X[nCIESamples];
extern const float CIE_Y[nCIESamples];
extern const float CIE_Z[nCIESamples];
extern const float CIE_lambda[nCIESamples];
static const float CIE_Y_integral = 106.856895;

class RGBSpectrum {
public:
    RGBSpectrum(float v = 0) :r(v), g(v), b(v) {}
    RGBSpectrum(float _r, float _g, float _b) :r(_r), g(_g), b(_b) {}
    RGBSpectrum(const float* v) : r(v[0]), g(v[1]), b(v[2]) {}
    RGBSpectrum(const std::string& filename);
    RGBSpectrum(const Float3& v) :r(std::fabs(v.x)), g(std::fabs(v.y)), b(std::fabs(v.z)) {}

    float operator [] (const uint32_t& i) const {
        if (i == 0) return r;
        else if (i == 1) return g;
        else return b;
    }
    bool IsNaN() const { return std::isnan(r) | std::isnan(g) | std::isnan(b); }

    RGBSpectrum operator * (float v) const { return RGBSpectrum(r * v, g * v, b * v); }
    RGBSpectrum operator / (float v) const { assert(v != 0.f); return RGBSpectrum(r / v, g / v, b / v); }
    RGBSpectrum& operator /= (float v) { assert(v != 0.f); r /= v; g /= v; b /= v; return *this; }

    RGBSpectrum operator + (const RGBSpectrum& s) const { return RGBSpectrum(r + s.r, g + s.g, b + s.b); }
    RGBSpectrum& operator += (const RGBSpectrum& s) { r += s.r; g += s.g; b += s.b; return *this; }
    RGBSpectrum operator - (const RGBSpectrum& s) const { return RGBSpectrum(r - s.r, g - s.g, b - s.b); }
    RGBSpectrum operator * (const RGBSpectrum& v) const { return RGBSpectrum(r * v.r, g * v.g, b * v.b); }
    RGBSpectrum& operator *= (const RGBSpectrum& v) { r *= v.r; g *= v.g; b *= v.b; return *this; }
    RGBSpectrum operator / (const RGBSpectrum& v) const { return RGBSpectrum(r / v.r, g / v.g, b / v.b); }

    float Average() const { return double(r + g + b) / 3; }
    float y() const { return (r * 0.299) + (g * 0.587) + (b * 0.114); }
    bool IsBlack() const { return r == 0 && g == 0 && b == 0; }

    sRGB TosRGB() const { return sRGB(GammaCorrect(r), GammaCorrect(g), GammaCorrect(b)); }

    static RGBSpectrum FromSPD(const float* lambda, const float* val, const int& n);

    float r, g, b;

    friend RGBSpectrum Exp(const RGBSpectrum& s) { return RGBSpectrum(std::exp(s.r), std::exp(s.g), std::exp(s.b)); }
    friend RGBSpectrum Sqrt(const RGBSpectrum& s) { return RGBSpectrum(Sqrt(s.r), Sqrt(s.g), Sqrt(s.b)); }
    friend float MaxComponent(const RGBSpectrum& s) { return std::max(s.r, std::max(s.g, s.b)); }
    friend std::ostream& operator <<(std::ostream& os, const RGBSpectrum& s) {
        os << s.r << ' ' << s.g << ' ' << s.b;
        return os;
    }
};

typedef RGBSpectrum Spectrum;