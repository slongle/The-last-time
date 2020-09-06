#include "bsdfutil.h"

Float3 Reflect(const Float3& v, const Float3& n)
{
    return 2 * Dot(v, n) * n - v;
}

Float3 Reflect(const Float3& v)
{
    return Float3(-v.x, -v.y, v.z);
}

float FresnelDieletric(const float& cosThetaI, float& cosThetaT, const float& eta)
{
    float factor = cosThetaI > 0.f ? 1.f / eta : eta;
    float cosThetaTSqr = 1 - (factor * factor) * (1 - cosThetaI * cosThetaI);
    // Total internal reflect
    if (cosThetaTSqr <= 0.f) {
        cosThetaT = 0.f;
        return 1.f;
    }
    float cosThetaT_ = std::sqrt(cosThetaTSqr);
    float cosThetaI_ = std::fabs(cosThetaI);
    float Rs = (eta * cosThetaI_ - cosThetaT_) / (eta * cosThetaI_ + cosThetaT_);
    float Rp = (cosThetaI_ - eta * cosThetaT_) / (cosThetaI_ + eta * cosThetaT_);
    cosThetaT = cosThetaI > 0.f ? -cosThetaT_ : cosThetaT_;
    return 0.5f * (Rs * Rs + Rp * Rp);
}

Spectrum FresnelConductor(const float& cosThetaI, const Spectrum& eta, const Spectrum& k)
{
    float cosThetaI2 = cosThetaI * cosThetaI,
        sinThetaI2 = 1 - cosThetaI2,
        sinThetaI4 = sinThetaI2 * sinThetaI2;

    Spectrum temp1 = eta * eta - k * k - Spectrum(sinThetaI2),
        a2pb2 = Sqrt(temp1 * temp1 + k * k * eta * eta * 4),
        a = Sqrt((a2pb2 + temp1) * 0.5f);

    Spectrum term1 = a2pb2 + Spectrum(cosThetaI2),
        term2 = a * (2 * cosThetaI);

    Spectrum Rs2 = (term1 - term2) / (term1 + term2);

    Spectrum term3 = a2pb2 * cosThetaI2 + Spectrum(sinThetaI4),
        term4 = term2 * sinThetaI2;

    Spectrum Rp2 = Rs2 * (term3 - term4) / (term3 + term4);

    return (Rp2 + Rs2) * 0.5f;
}

void FresnelDieletricExt(
    float cosThetaI, 
    float etaI, float etaT, 
    Float2& R, Float2& phi)
{
    float st1 = (1 - math::Sqr(cosThetaI));
    float nr = etaI / etaT;

    if (math::Sqr(nr) * st1 > 1) { 
        // Total reflection
        R = Float2(1);
        float a = -math::Sqr(nr) * math::SafeSqrt(st1 - 1.0 / math::Sqr(nr)) / cosThetaI;
        float b = -math::SafeSqrt(st1 - 1.0 / math::Sqr(nr)) / cosThetaI;       
        phi = Float2(2.0 * std::atan2(a,b));
    }
    else {   
        // Transmission & Reflection
        float ct2 = math::SafeSqrt(1 - math::Sqr(nr) * st1);
        Float2 r((etaT * cosThetaI - etaI * ct2) / (etaT * cosThetaI + etaI * ct2),
            (etaI * cosThetaI - etaT * ct2) / (etaI * cosThetaI + etaT * ct2));
        phi.x = (r.x < 0.0) ? M_PI : 0.0;
        phi.y = (r.y < 0.0) ? M_PI : 0.0;
        R = Sqr(r);
    }
}

void FresnelConductorExt(
    float cosThetaI, 
    float etaI, float etaT, float kT, 
    Float2& R, Float2& phi)
{
    float k = kT;
    float ct1 = cosThetaI;
    float n1 = etaI, n2 = etaT;
    if (k == 0) { // use dielectric formula to avoid numerical issues
        FresnelDieletricExt(ct1, n1, n2, R, phi);
        return;
    }

    float A = math::Sqr(n2) * (1 - math::Sqr(k)) - math::Sqr(n1) * (1 - math::Sqr(ct1));
    float B = sqrt(math::Sqr(A) + math::Sqr(2 * math::Sqr(n2) * k));
    float U = sqrt((A + B) / 2.0);
    float V = sqrt((B - A) / 2.0);

    R.y = (math::Sqr(n1 * ct1 - U) + math::Sqr(V)) / (math::Sqr(n1 * ct1 + U) + math::Sqr(V));
    phi.y = std::atan2(2 * n1 * V * ct1, math::Sqr(U) + math::Sqr(V) - math::Sqr(n1 * ct1)) + M_PI;

    R.x = (math::Sqr(math::Sqr(n2) * (1 - math::Sqr(k)) * ct1 - n1 * U) + math::Sqr(2 * math::Sqr(n2) * k * ct1 - n1 * V))
        / (math::Sqr(math::Sqr(n2) * (1 - math::Sqr(k)) * ct1 + n1 * U) + math::Sqr(2 * math::Sqr(n2) * k * ct1 + n1 * V));
    phi.x = std::atan2(2 * n1 * math::Sqr(n2) * ct1 * (2 * k * U - (1 - math::Sqr(k)) * V), math::Sqr(math::Sqr(n2) * (1 + math::Sqr(k)) * ct1) - math::Sqr(n1) * (math::Sqr(U) + math::Sqr(V)));
}
