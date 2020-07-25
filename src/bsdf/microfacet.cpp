#include "microfacet.h"

Float3 Microfacet::SampleVisible(const Float3& wi, Float2& u) const
{
    // Section 3.2: transforming the view direction to the hemisphere configuration
    Float3 Vh = Normalize(Float3(m_alpha.x * wi.x, m_alpha.y * wi.y, wi.z));
    // Section 4.1: orthonormal basis (with special case if cross product is zero)
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    float invSqrtLensq = 1.0f / std::sqrt(lensq);
    Float3 T1 = lensq > 0 ? Float3(-Vh.y, Vh.x, 0) * invSqrtLensq : Float3(1, 0, 0);
    Float3 T2 = Cross(Vh, T1);
    // Section 4.2: parameterization of the projected area
    float r = std::sqrt(u.x);
    float phi = 2.0 * M_PI * u.y;
    float t1 = r * std::cos(phi);
    float t2 = r * std::sin(phi);
    float s = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - s) * std::sqrt(1.0 - t1 * t1) + s * t2;
    // Section 4.3: reprojection onto hemisphere
    Float3 Nh = t1 * T1 + t2 * T2 + std::sqrt(std::max(0.0f, 1.0f - t1 * t1 - t2 * t2)) * Vh;
    // Section 3.4: transforming the normal back to the ellipsoid configuration
    Float3 Ne = Normalize(Float3(m_alpha.x * Nh.x, m_alpha.y * Nh.y, std::max(0.0f, Nh.z)));    
    return Ne;    
}

float Microfacet::PdfVisible(const Float3& wi, const Float3& m) const
{
    float cosTheta = Frame::AbsCosTheta(wi);
    LOG_IF(FATAL, cosTheta == 0) << "Divide zero.";
    float result = D(m) * G1(wi, m) * std::abs(Dot(wi, m)) / cosTheta;
    return result;
}

float Microfacet::D(const Float3& m) const
{
    if (Frame::CosTheta(m) <= 0) return 0.f;

    float cos2Theta = Frame::Cos2Theta(m);

    float root = cos2Theta + (std::sqr(m.x) / std::sqr(m_alpha.x) + std::sqr(m.y) / std::sqr(m_alpha.y));
    float D = 1.f / (M_PI * m_alpha.x * m_alpha.y * root * root);
    return D;
}

float Microfacet::G(const Float3& wi, const Float3& wo, const Float3& m) const
{
    return G1(wi, m) * G1(wo, m);
}

float Microfacet::G1(const Float3& w, const Float3& m) const
{
    float tan2Theta = Frame::Tan2Theta(w);
    float alpha = GetAlpha(w);
    float root = alpha * alpha * tan2Theta;
    float result = 2.0f / (1 + std::sqrt(1 + root));
    return result;
}

float Microfacet::GetAlpha(const Float3& w) const
{
    float sin2Theta = Frame::Sin2Theta(w);
    float cos2Phi, sin2Phi;
    if (sin2Theta < 1e-3) {
        cos2Phi = 1.f;
        sin2Phi = 0.f;
    }
    else {
        float invSin2Theta = 1.f / sin2Theta;
        cos2Phi = w.x * w.x * invSin2Theta;
        sin2Phi = w.y * w.y * invSin2Theta;
    }
    float alpha = std::sqrt(cos2Phi * std::sqr(m_alpha.x) + sin2Phi * std::sqr(m_alpha.y));
    return alpha;
}
