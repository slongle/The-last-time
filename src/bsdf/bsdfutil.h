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