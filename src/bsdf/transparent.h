#pragma once

#include "core/primitive.h"

class Transparent :public BSDF {
public:
    Transparent(
        const std::shared_ptr<Texture<Spectrum>>& reflectance,
        const std::shared_ptr<Texture<float>>& alpha)
        :BSDF(alpha), m_reflectance(reflectance) {}

    Spectrum Sample(MaterialRecord& matRec, Float2 s) const {
        // Alpha texture
        bool opaque = !(m_alpha->Evaluate(matRec.m_st) >= 0.99f);
        if (opaque) {
            matRec.m_wo = -matRec.m_wi;
            matRec.m_pdf = 1.f;
            return Spectrum(1.f);
        }

        matRec.m_wo = -matRec.m_wi;
        matRec.m_pdf = 1.f;
        return m_reflectance->Evaluate(matRec.m_st);
    }
    Spectrum Eval(MaterialRecord& matRec) const {
        if (!Frame::SameDirection(matRec.m_wo, -matRec.m_wi)) {
            return Spectrum(0.f);
        }
        return m_reflectance->Evaluate(matRec.m_st);
    }
    float Pdf(MaterialRecord& matRec) const {
        if (!Frame::SameDirection(matRec.m_wo, -matRec.m_wi)) {
            return 0.f;
        }
        return matRec.m_pdf = 1.f;
    }
    Spectrum EvalPdf(MaterialRecord& matRec) const {
        if (!Frame::SameDirection(matRec.m_wo, -matRec.m_wi)) {
            matRec.m_pdf = 0.f;
            return Spectrum(0.f);
        }
        matRec.m_pdf = 1.f;
        return m_reflectance->Evaluate(matRec.m_st);
    }
    bool IsDelta(const Float2& st) const { return true; }
    bool IsTransparent() const { return true; }

private:
    std::shared_ptr<Texture<Spectrum>> m_reflectance;
};