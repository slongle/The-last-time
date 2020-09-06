#pragma once

#include "core/vector.h"
#include "core/spectrum.h"

Float3 Reflect(const Float3& v, const Float3& n);
Float3 Reflect(const Float3& v);

float FresnelDieletric(const float& cosThetaI, float& cosThetaT, const float& eta);
Spectrum FresnelConductor(const float& cosThetaI, const Spectrum& eta, const Spectrum& k);

/**
 * @brief
 * @param cosThetaI
 * @param etaI
 * @param etaT
 * @param R R.x is perpendicular(Rs), R.y is parallel(Rp)
 * @param phi phi.x is perpendicular, phi.y is parallel
*/
void FresnelDieletricExt(
    float cosThetaI,
    float etaI, float etaT,
    Float2& R, Float2& phi);

/**
 * @brief
 * @param cosThetaI
 * @param etaI
 * @param etaT
 * @param kT
 * @param R R.x is perpendicular(Rs), R.y is parallel(Rp)
 * @param phi phi.x is perpendicular, phi.y is parallel
*/
void FresnelConductorExt(
    float cosThetaI,
    float etaI, float etaT, float kT,
    Float2& R, Float2& phi);