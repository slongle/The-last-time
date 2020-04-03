#include "sampling.h"

void CoordinateSystem(const Float3& a, Float3& b, Float3& c)
{
    if (std::abs(a.x) > std::abs(a.y)) {
        float invLen = 1.0f / std::sqrt(a.x * a.x + a.z * a.z);
        c = Float3(a.z * invLen, 0.0f, -a.x * invLen);
    }
    else {
        float invLen = 1.0f / std::sqrt(a.y * a.y + a.z * a.z);
        c = Float3(0.0f, a.z * invLen, -a.y * invLen);
    }
    b = Cross(c, a);
}

Float3 SampleCosineHemisphere(const Float2& s)
{
    float sinTheta = std::sqrt(s.x), cosTheta = std::sqrt(1 - s.x);
    float sinPhi = std::sin(M_PI * 2 * s.y), cosPhi = std::cos(M_PI * 2 * s.y);
    return Float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

float PdfCosineHemisphere(const Float3& v)
{
    return v.z * INV_PI;
}

Float2 SampleUniformTriangle(const Float2& s)
{
    float su = std::sqrt(s.x);
    return Float2(1 - su, s.y * su);
}
