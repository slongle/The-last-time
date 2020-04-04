#pragma once

#include "core/primitive.h"

class Matte : public BSDF{
public:
    Matte(const std::shared_ptr<Texture<Spectrum>>& reflectance) :m_reflectance(reflectance) {}

    Spectrum Eval(MaterialRecord& matRec) const {
        return m_reflectance->Evaluate(matRec.m_st) * INV_PI * Frame::AbsCosTheta(matRec.m_wo);
    }

    Spectrum EvalPdf(MaterialRecord& matRec) const {
        matRec.m_pdf = PdfCosineHemisphere(matRec.m_wo);
        return m_reflectance->Evaluate(matRec.m_st) * INV_PI * Frame::AbsCosTheta(matRec.m_wo);
    }

    std::shared_ptr<Texture<Spectrum>> m_reflectance;
};