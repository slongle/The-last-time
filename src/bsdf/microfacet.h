#pragma once

#include "core/primitive.h"

class Microfacet {
public:
    Microfacet(const Float2& alpha) :m_alpha(alpha) {}

    Float3 SampleVisible(const Float3& wi, Float2& s) const;
    float PdfVisible(const Float3& wi, const Float3& m) const;
    float D(const Float3& m) const;
    float G(const Float3& wi, const Float3& wo, const Float3& m) const;
    float G1(const Float3& w, const Float3& m) const;
    float GetAlpha(const Float3& w) const;

    Float2 m_alpha;
};