#pragma once

#include "vector.h"

void CoordinateSystem(const Float3& a, Float3& b, Float3& c);    

class Frame {
public:
    Frame() {}
    Frame(const Float3& n) :m_n(n) {
        CoordinateSystem(m_n, m_s, m_t);
    }

    Float3 ToLocal(const Float3& v) const {
        return Float3(Dot(v, m_s), Dot(v, m_t), Dot(v, m_n));
    }

    Float3 ToWorld(const Float3& v) const {
        return v.x * m_s + v.y * m_t + v.z * m_n;
    }

private:
    Float3 m_n, m_s, m_t;

public:
    static inline float CosTheta(const Float3& v) { return v.z; }
    static inline float AbsCosTheta(const Float3& v) { return std::fabs(v.z); }
    static inline float SphericalPhi(const Float3& v) { return std::atan2(v.y, v.x) + M_PI; }
    static inline float SphericalTheta(const Float3& v) { return std::acos(v.z); }
};

class Distribution1D {
public:
    Distribution1D(const float* ptr, int n);
    float Sample(const float& s, float& pdf) const;
    float GetSum() const { return m_sum; }

private:
    std::vector<float> m_func, m_cdf;
    float m_sum;
};

class Distribution2D {
public:
    Distribution2D(const float* ptr, int nu, int nv);
    Float2 Sample(const Float2& s) const;

private:
    std::vector<std::unique_ptr<Distribution1D>> m_conditional;
    std::unique_ptr<Distribution1D> m_marginal;
};

Float3 SampleCosineHemisphere(const Float2& s);
float PdfCosineHemisphere(const Float3& v);
Float2 SampleUniformTriangle(const Float2& s);
Float3 SampleUniformSphere(const Float2& s);
float PdfUniformSphere(const Float3& v);