#pragma once

#include "core/vector.h"

inline float FresnelDieletric(const float& cosThetaI, float& cosThetaT, const float& eta) {
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

inline Spectrum FresnelConductor(const float& cosThetaI, const Spectrum& eta, const Spectrum& k) {
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