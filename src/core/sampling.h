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

    Float3 m_n, m_s, m_t;

    static inline float CosTheta(const Float3& v) { return v.z; }
    static inline float AbsCosTheta(const Float3& v) { return std::fabs(v.z); }
};

Float3 SampleCosineHemisphere(const Float2& s);
float PdfCosineHemisphere(const Float3& v);
Float2 SampleUniformTriangle(const Float2& s);