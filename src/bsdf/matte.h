#pragma once

#include "core/primitive.h"

class Matte : public BSDF {
public:
    Matte(
        const std::shared_ptr<Texture<Spectrum>>& reflectance,
        const std::shared_ptr<Texture<float>>& alpha)
        :BSDF(alpha), m_reflectance(reflectance) {}

    Spectrum Sample(MaterialRecord& matRec, Float2 s) const
    {
        // Alpha texture
        bool opaque = !(m_alpha->Evaluate(matRec.m_st) >= 0.99f);
        if (opaque) {
            matRec.m_wo = -matRec.m_wi;
            matRec.m_pdf = 1.f;
            return Spectrum(1.f);
        }

        matRec.m_wo = SampleCosineHemisphere(s);
        if (Frame::CosTheta(matRec.m_wi) < 0) {
            matRec.m_wo.z *= -1;
        }

        matRec.m_eta = 1.f;
        matRec.m_pdf = PdfCosineHemisphere(matRec.m_wo);
        return m_reflectance->Evaluate(matRec.m_st);
    }

    Spectrum Eval(MaterialRecord& matRec) const {
        if (Frame::CosTheta(matRec.m_wi) <= 0 ||
            Frame::CosTheta(matRec.m_wo) <= 0) {
            return Spectrum(0.f);
        }

        return m_reflectance->Evaluate(matRec.m_st) * INV_PI * Frame::AbsCosTheta(matRec.m_wo);
    }

    Spectrum EvalPdf(MaterialRecord& matRec) const {
        if (!Frame::SameHemisphere(matRec.m_wo, matRec.m_wi)) {
            return Spectrum(0.f);
        }

        matRec.m_pdf = PdfCosineHemisphere(matRec.m_wo);
        return m_reflectance->Evaluate(matRec.m_st) * INV_PI * Frame::AbsCosTheta(matRec.m_wo);
    }

    bool IsDelta(const Float2& st) const {
        return BSDF::IsDelta(st) ? true : false;
    }

private:
    std::shared_ptr<Texture<Spectrum>> m_reflectance;
};